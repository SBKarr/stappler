// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2016 Roman Katuntsev <sbkarr@stappler.org>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
**/

#include "Define.h"
#include "DatabaseUtils.h"
#include "DatabaseHandle.h"
#include "StorageScheme.h"
#include "SPAprStringStream.h"

NS_SA_EXT_BEGIN(database)

static constexpr uint32_t getDefaultFunctionVersion() { return 5; }

void writeQueryBinaryData(apr::ostringstream &query, const Bytes &bytes) {
	query << "E'\\\\x";
	base16::encode(query, bytes);
	query << '\'';
}

void writeQueryDataValue(apr::ostringstream &query, const data::Value &val, ap_dbd_t *handle, bool forceEncode) {
	if (!forceEncode) {
		switch (val.getType()) {
		case data::Value::Type::EMPTY:
			query << "NULL";
			break;
		case data::Value::Type::BOOLEAN:
			query << (val.asBool() ? "TRUE" : "FALSE");
			break;
		case data::Value::Type::INTEGER:
			query << val.asInteger();
			break;
		case data::Value::Type::DOUBLE: {
			auto d = val.asDouble();
			if (isnan(d)) {
				query << "'NaN'";
			} else if (d == std::numeric_limits<double>::infinity()) {
				query << "'-Infinity'";
			} else if (-d == std::numeric_limits<double>::infinity()) {
				query << "'Infinity'";
			} else {
				query << val.asDouble();
			}
		}
			break;
		case data::Value::Type::CHARSTRING:
			query << '\'' << apr_dbd_escape(handle->driver, query.get_allocator(), val.asString().c_str(), handle->handle) << '\'';
			break;
		case data::Value::Type::BYTESTRING:
			writeQueryBinaryData(query, val.asBytes());
			break;
		case data::Value::Type::ARRAY:
			writeQueryBinaryData(query, data::write(val, data::EncodeFormat::Cbor));
			break;
		case data::Value::Type::DICTIONARY:
			writeQueryBinaryData(query, data::write(val, data::EncodeFormat::Cbor));
			break;
		default: break;
		}
	} else {
		writeQueryBinaryData(query, data::write(val, data::EncodeFormat::Cbor));
	}
}

data::Value parseQueryResultValue(const storage::Field &f, String &str) {
	switch(f.getType()) {
	case storage::Type::Integer:
		return data::Value((int64_t)strtoll(str.c_str(), nullptr, 10));
		break;
	case storage::Type::Object:
	case storage::Type::Set:
	case storage::Type::Array:
	case storage::Type::File:
	case storage::Type::Image:
		if (!str.empty()) {
			return data::Value((int64_t)strtoll(str.c_str(), nullptr, 10));
		}
		break;
	case storage::Type::Float:
		return data::Value(strtod(str.c_str(), nullptr));
		break;
	case storage::Type::Boolean:
		return data::Value((str.front() == 'T' || str.front() == 't')?true:false);
		break;
	case storage::Type::Text:
		if (!str.empty()) {
			return data::Value(std::move(str));
		}
		break;
	case storage::Type::Bytes:
		if (str.compare(0, 2, "\\x") == 0) {
			return data::Value(base16::decode(CoderSource(str.c_str() + 2, str.size() - 2)));
		}
		break;
	case storage::Type::Data:
	case storage::Type::Extra:
		if (str.compare(0, 2, "\\x") == 0) {
			return data::Value(data::read(base16::decode(CoderSource(str.c_str() + 2, str.size() - 2))));
		}
		break;
	default:
		break;
	}

	return data::Value();
}

void parseQueryResultString(data::Value &val, const storage::Field &f, String && name, String &str) {
	auto d = parseQueryResultValue(f, str);
	if (d) {
		val.setValue(std::move(d), std::move(name));
	}
}

/*static void writeObjectSetInsertTrigger(apr::ostringstream &stream, storage::Scheme *s,
		const storage::FieldObject *obj, const storage::Field *link) {
	stream << "\t\t\tIF (NEW.\"" << obj->name << "\" IS NOT NULL) THEN\n"
		<< "\t\t\t\tINSERT INTO " << obj->scheme->getName() << "_f_" << link->getName()
		<< "(\"" << obj->scheme->getName() << "_id\", \"" << s->getName() << "_id\") "
		<< "VALUES (NEW.\"" << obj->name << "\", NEW.__oid);\n"
		<< "\t\t\tEND IF;\n";
}

static void writeObjectSetRemoveTrigger(apr::ostringstream &stream, storage::Scheme *s,
		const storage::FieldObject *obj, const storage::Field *link) {
	stream << "\t\t\tIF (OLD.\"" << obj->name << "\" IS NOT NULL) THEN\n"
		<< "\t\t\t\tDELETE FROM " << obj->scheme->getName() << "_f_" << link->getName() << " WHERE "
		<< "\"" << obj->scheme->getName() << "_id\"=OLD.\"" << obj->name << "\" AND \"" << s->getName() << "_id\"=OLD.__oid;"
		<< "\t\t\tEND IF;\n";
}*/

static void writeFileUpdateTrigger(apr::ostringstream &stream, storage::Scheme *s, const storage::Field &obj) {
	stream << "\t\tIF (OLD.\"" << obj.getName() << "\" <> NEW.\"" << obj.getName() << "\") THEN\n"
		<< "\t\t\tIF (OLD.\"" << obj.getName() << "\" IS NOT NULL) THEN\n"
		<< "\t\t\t\tINSERT INTO __removed (__oid) VALUES (OLD.\"" << obj.getName() << "\");\n"
		<< "\t\t\tEND IF;\n\t\tEND IF;\n";
}

static void writeFileRemoveTrigger(apr::ostringstream &stream, storage::Scheme *s, const storage::Field &obj) {
	stream << "\t\tIF (OLD.\"" << obj.getName() << "\" IS NOT NULL) THEN\n"
		<< "\t\t\tINSERT INTO __removed (__oid) VALUES (OLD.\"" << obj.getName() << "\");\n"
		<< "\t\tEND IF;\n";
}

static void writeTrigger(apr::ostringstream &stream, storage::Scheme *s, const String &triggerName) {
	auto &fields = s->getFields();

	stream << "CREATE OR REPLACE FUNCTION " << triggerName << "_func() RETURNS TRIGGER AS $" << triggerName
			<< "$ BEGIN\n\tIF (TG_OP = 'INSERT') THEN\n";
	//for (auto &it : fields) {
		// do nothing for now
	//}
	stream << "\tELSIF (TG_OP = 'UPDATE') THEN\n";
	for (auto &it : fields) {
		if (it.second.isFile()) {
			writeFileUpdateTrigger(stream, s, it.second);
		}
	}
	stream << "\tELSIF (TG_OP = 'DELETE') THEN\n";
	for (auto &it : fields) {
		if (it.second.isFile()) {
			writeFileRemoveTrigger(stream, s, it.second);
		}
	}
	stream << "\tEND IF;\n\tRETURN NULL;\n";
	stream << "\nEND; $" << triggerName << "$ LANGUAGE plpgsql;\n";

	stream << "CREATE TRIGGER " << triggerName << " AFTER INSERT OR UPDATE OR DELETE ON \"" << s->getName()
			<< "\" FOR EACH ROW EXECUTE PROCEDURE " << triggerName << "_func();\n";
}

void TableRec::writeCompareResult(apr::ostringstream &stream,
		Map<String, TableRec> &required, Map<String, TableRec> &existed,
		const Map<String, storage::Scheme *> &s) {
	for (auto &ex_it : existed) {
		auto req_it = required.find(ex_it.first);
		if (req_it != required.end()) {
			auto &req_t = req_it->second;
			auto &ex_t = ex_it.second;
			req_t.exists = true;

			for (auto &ex_idx_it : ex_t.indexes) {
				auto req_idx_it = req_t.indexes.find(ex_idx_it.first);
				if (req_idx_it == req_t.indexes.end()) {
					// index is not required any more, drop it
					stream << "DROP INDEX IF EXISTS \"" << ex_idx_it.first << "\";\n";
				} else {
					req_t.indexes.erase(req_idx_it);
				}
			}

			for (auto &ex_cst_it : ex_t.constraints) {
				auto req_cst_it = req_t.constraints.find(ex_cst_it.first);
				if (req_cst_it == req_t.constraints.end()) {
					// constraint is not required any more, drop it
					stream << "ALTER TABLE " << ex_it.first << " DROP CONSTRAINT IF EXISTS \"" << ex_cst_it.first << "\";\n";
				} else {
					req_t.constraints.erase(req_cst_it);
				}
			}

			for (auto &ex_col_it : ex_t.cols) {
				auto req_col_it = req_t.cols.find(ex_col_it.first);
				if (req_col_it == req_t.cols.end()) {
					stream << "ALTER TABLE " << ex_it.first << " DROP COLUMN IF EXISTS \"" << ex_col_it.first << "\";\n";
				} else {
					auto &req_col = req_col_it->second;
					auto &ex_col = ex_col_it.second;

					auto req_type = req_col.type;
					if (req_type == ColRec::Type::Serial) { req_type = ColRec::Type::Integer; }

					if (ex_col.type == ColRec::Type::None || req_type != ex_col.type) {
						stream << "ALTER TABLE " << ex_it.first << " DROP COLUMN IF EXISTS \"" << ex_col_it.first << "\";\n";
					} else {
						req_t.cols.erase(req_col_it);
					}
				}
			}

			for (auto &ex_tgr_it : ex_t.triggers) {
				auto req_tgr_it = req_t.triggers.find(ex_tgr_it);
				if (req_tgr_it == req_t.triggers.end()) {
					stream << "DROP TRIGGER IF EXISTS \"" << ex_tgr_it << "\" ON \"" << ex_it.first << "\";\n";
					stream << "DROP FUNCTION IF EXISTS \"" << ex_tgr_it << "_func\"();\n";
				} else {
					req_t.triggers.erase(ex_tgr_it);
				}
			}
		}
	}

	// write table structs
	for (auto &it : required) {
		auto &t = it.second;
		if (!it.second.exists) {
			stream << "CREATE TABLE IF NOT EXISTS " << it.first << " (\n";

			bool first = true;
			for (auto cit = t.cols.begin(); cit != t.cols.end(); cit ++) {
				if (first) { first = false; } else { stream << ",\n"; }
				stream << "\t\"" << cit->first << "\" ";
				switch(cit->second.type) {
				case ColRec::Type::Binary:	stream << "bytea"; break;
				case ColRec::Type::Integer:	stream << "bigint"; break;
				case ColRec::Type::Serial:	stream << "bigserial"; break;
				case ColRec::Type::Float:	stream << "double precision"; break;
				case ColRec::Type::Boolean:	stream << "boolean"; break;
				case ColRec::Type::Text: 	stream << "text"; break;
				default: break;
				}

				if (cit->second.notNull) {
					stream << " NOT NULL";
				}
			}

			first = true;
			if (!t.pkey.empty()) {
				stream << ",\n\tPRIMARY KEY (";
				for (auto &key : t.pkey) {
					if (first) { first = false; } else { stream << ", "; }
					stream << "\"" << key << "\"";
				}
				stream << ")";
			}

			stream << "\n)";
			if (it.second.objects) {
				stream << " INHERITS (__objects)";
			}
			stream << " WITH ( OIDS=FALSE );\n\n";
		} else {
			for (auto cit : t.cols) {
				stream << "ALTER TABLE " << it.first << " ADD COLUMN \"" << cit.first << "\" ";
				switch(cit.second.type) {
				case ColRec::Type::Binary:	stream << "bytea"; break;
				case ColRec::Type::Integer:	stream << "bigint"; break;
				case ColRec::Type::Serial:	stream << "bigserial"; break;
				case ColRec::Type::Float:	stream << "double precision"; break;
				case ColRec::Type::Boolean:	stream << "boolean"; break;
				case ColRec::Type::Text: 	stream << "text"; break;
				default: break;
				}
				stream << ";\n";
			}
		}
	}

	// write constraints
	for (auto &it : required) {
		for (auto & cit : it.second.constraints) {
			stream << "ALTER TABLE " << it.first << " ADD CONSTRAINT \"" << cit.first << "\" ";

			bool first = true;
			switch (cit.second.type) {
			case ConstraintRec::Unique:
				stream << " UNIQUE ( ";
				for (auto &key : cit.second.fields) {
					if (first) { first = false; } else { stream << ", "; }
					stream << "\"" << key << "\"";
				}
				stream << " )";
				break;
			case ConstraintRec::Reference:
				stream << " FOREIGN KEY (";
				for (auto &key : cit.second.fields) {
					if (first) { first = false; } else { stream << ", "; }
					stream << "\"" << key << "\"";
				}
				stream << ") REFERENCES " << cit.second.reference << " ( \"__oid\" )";
				switch (cit.second.remove) {
				case ConstraintRec::RemovePolicy::Cascade:
					stream << " ON DELETE CASCADE";
					break;
				case ConstraintRec::RemovePolicy::Restrict:
					stream << " ON DELETE RESTRICT";
					break;
				case ConstraintRec::RemovePolicy::Null:
				case ConstraintRec::RemovePolicy::Reference:
					stream << " ON DELETE SET NULL";
					break;
				}
				break;
			}

			stream << ";\n";
		}
	}

	// indexes
	for (auto &it : required) {
		for (auto & cit : it.second.indexes) {
			stream << "CREATE INDEX IF NOT EXISTS \"" << cit.first
					<< "\" ON " << it.first << "( \"" << cit.second << "\" );\n" ;
		}

		if (!it.second.triggers.empty()) {
			auto scheme_it = s.find(it.first);
			if (scheme_it != s.end()) {
				for (auto & tit : it.second.triggers) {
					writeTrigger(stream, scheme_it->second, tit);
				}
			}
		}
	}
}

Map<String, TableRec> TableRec::parse(Server &serv, const Map<String, storage::Scheme *> &s) {
	using namespace storage;
	Map<String, TableRec> tables;
	for (auto &it : s) {
		auto scheme = it.second;
		tables.emplace(it.first, TableRec(serv, scheme));

		// check for extra tables
		for (auto &fit : scheme->getFields()) {
			auto &f = fit.second;
			auto type = fit.second.getType();

			if (type == storage::Type::Set) {
				auto ref = static_cast<const storage::FieldObject *>(f.getSlot());
				if (ref->onRemove == storage::RemovePolicy::Reference) {
					String name = it.first + "_f_" + fit.first;
					auto & source = it.first;
					auto & target = ref->scheme->getName();
					TableRec table;
					table.cols.emplace(source + "_id", ColRec(ColRec::Type::Integer));
					table.cols.emplace(target + "_id", ColRec(ColRec::Type::Integer));

					table.constraints.emplace(name + "_ref_" + source, ConstraintRec(
							ConstraintRec::Reference, source + "_id", source, ConstraintRec::RemovePolicy::Cascade));
					table.constraints.emplace(name + "_ref_" + ref->getName(), ConstraintRec(
							ConstraintRec::Reference, target + "_id", target, ConstraintRec::RemovePolicy::Cascade));

					table.pkey.emplace_back(source + "_id");
					table.pkey.emplace_back(target + "_id");
					tables.emplace(std::move(name), std::move(table));
				}
			}

			if (type == storage::Type::Array) {
				auto slot = static_cast<const FieldArray *>(f.getSlot());
				if (slot->tfield && slot->tfield.isSimpleLayout()) {

					String name = it.first + "_f_" + fit.first;
					auto & source = it.first;

					TableRec table;
					table.cols.emplace("id", ColRec(ColRec::Type::Serial));
					table.cols.emplace(source + "_id", ColRec(ColRec::Type::Integer));

					auto type = slot->tfield.getType();
					switch (type) {
					case storage::Type::Float:
						table.cols.emplace("data", ColRec(ColRec::Type::Float));
						break;
					case storage::Type::Boolean:
						table.cols.emplace("data", ColRec(ColRec::Type::Boolean));
						break;
					case storage::Type::Text:
						table.cols.emplace("data", ColRec(ColRec::Type::Text));
						break;
					case storage::Type::Data:
					case storage::Type::Bytes:
					case storage::Type::Extra:
						table.cols.emplace("data", ColRec(ColRec::Type::Binary));
						break;
					case storage::Type::Integer:
						table.cols.emplace("data", ColRec(ColRec::Type::Integer));
						break;
					default:
						break;
					}

					table.constraints.emplace(name + "_ref_" + source, ConstraintRec (
							ConstraintRec::Reference, source + "_id", source, ConstraintRec::RemovePolicy::Cascade));
					table.pkey.emplace_back("id");

					if (f.hasFlag(storage::Flags::Unique)) {
						table.constraints.emplace(name + "_unique", ConstraintRec(ConstraintRec::Unique, {source + "_id", "data"}));
					}

					table.indexes.emplace(name + "_idx_" + source, source + "_id");
					tables.emplace(std::move(name), std::move(table));
				}
			}
		}
	}
	return tables;
}

Map<String, TableRec> TableRec::get(Handle &h) {
	auto tables = h.select("SELECT table_name FROM information_schema.tables "
			"WHERE table_schema='public' AND table_type='BASE TABLE';");

	auto columns = h.select("SELECT table_name, column_name, is_nullable, data_type FROM information_schema.columns "
			"WHERE table_schema='public';");

	auto constraints = h.select("SELECT table_name, constraint_name, constraint_type FROM information_schema.table_constraints "
			"WHERE table_schema='public' AND constraint_schema='public';");

	auto indexes = h.select(
			"SELECT t.relname as table_name, i.relname as index_name, "
			"		array_to_string(array_agg(a.attname), ', ') as column_names"
			" FROM pg_class t, pg_class i, pg_index ix, pg_attribute a"
			" WHERE t.oid = ix.indrelid AND i.oid = ix.indexrelid AND a.attrelid = t.oid"
			"		AND a.attnum = ANY(ix.indkey) AND t.relkind = 'r'"
			" GROUP BY t.relname, i.relname ORDER BY t.relname, i.relname;");

	auto triggers = h.select("SELECT event_object_table, trigger_name FROM information_schema.triggers "
			"WHERE trigger_schema='public';");

	Map<String, TableRec> ret;

	for (auto &it : tables.data) {
		ret.emplace(it.at(0), TableRec());
	}

	for (auto &it : columns.data) {
		auto &tname = it.at(0);
		auto f = ret.find(tname);
		if (f != ret.end()) {
			auto &table = f->second;
			bool isNullable = (it.at(2) == "YES");
			auto &type = it.at(3);
			if (it.at(1) != "__oid") {
				if (type == "bigint") {
					table.cols.emplace(it.at(1), ColRec(ColRec::Type::Integer, !isNullable));
				} else if (type == "boolean") {
					table.cols.emplace(it.at(1), ColRec(ColRec::Type::Boolean, !isNullable));
				} else if (type == "double precision") {
					table.cols.emplace(it.at(1), ColRec(ColRec::Type::Float, !isNullable));
				} else if (type == "text") {
					table.cols.emplace(it.at(1), ColRec(ColRec::Type::Text, !isNullable));
				} else if (type == "bytea") {
					table.cols.emplace(it.at(1), ColRec(ColRec::Type::Binary, !isNullable));
				} else {
					table.cols.emplace(it.at(1), ColRec(ColRec::Type::None, !isNullable));
				}
			}
		}
	}

	for (auto &it : constraints.data) {
		auto &tname = it.at(0);
		auto f = ret.find(tname);
		if (f != ret.end()) {
			auto &table = f->second;
			if (it.at(2) == "UNIQUE") {
				table.constraints.emplace(it.at(1), ConstraintRec(ConstraintRec::Unique));
			} else if (it.at(2) == "FOREIGN KEY") {
				table.constraints.emplace(it.at(1), ConstraintRec(ConstraintRec::Reference));
			}
		}
	}

	for (auto &it : indexes.data) {
		auto &tname = it.at(0);
		auto f = ret.find(tname);
		if (f != ret.end()) {
			auto &table = f->second;
			auto &name = it.at(1);
			if (name.find("_idx_") != String::npos) {
				table.indexes.emplace(name, it.at(2));
			}
		}
	}

	for (auto &it : triggers.data) {
		auto &tname = it.at(0);
		auto f = ret.find(tname);
		if (f != ret.end()) {
			auto &table = f->second;
			table.triggers.emplace(it.at(1));
		}
	}

	return ret;
}

TableRec::TableRec() : objects(false) { }
TableRec::TableRec(Server &serv, storage::Scheme *scheme) {
	apr::ostringstream hashStream;
	hashStream << getDefaultFunctionVersion();

	bool hasTriggers = false;
	auto &name = scheme->getName();
	pkey.emplace_back("__oid");

	for (auto &it : scheme->getFields()) {
		bool emplaced = false;
		auto &f = it.second;
		auto type = it.second.getType();

		if (type == storage::Type::File || type == storage::Type::Image) {
			hasTriggers = true;
			hashStream << it.first << toInt(type);
		}

		switch (type) {
		case storage::Type::None:
		case storage::Type::Set:
		case storage::Type::Array:
			break;

		case storage::Type::Float:
			cols.emplace(it.first, ColRec(ColRec::Type::Float, f.hasFlag(storage::Flags::Required)));
			emplaced = true;
			break;

		case storage::Type::Boolean:
			cols.emplace(it.first, ColRec(ColRec::Type::Boolean, f.hasFlag(storage::Flags::Required)));
			emplaced = true;
			break;

		case storage::Type::Text:
			cols.emplace(it.first, ColRec(ColRec::Type::Text, f.hasFlag(storage::Flags::Required)));
			emplaced = true;
			break;

		case storage::Type::Data:
		case storage::Type::Bytes:
		case storage::Type::Extra:
			cols.emplace(it.first, ColRec(ColRec::Type::Binary, f.hasFlag(storage::Flags::Required)));
			emplaced = true;
			break;

		case storage::Type::Integer:
		case storage::Type::Object:
		case storage::Type::File:
		case storage::Type::Image:
			cols.emplace(it.first, ColRec(ColRec::Type::Integer, f.hasFlag(storage::Flags::Required)));
			emplaced = true;
			break;
		}

		if (emplaced) {
			if (type == storage::Type::Object) {
				auto ref = static_cast<const storage::FieldObject *>(f.getSlot());
				auto cname = name + "_ref_" + it.first;
				auto & target = ref->scheme->getName();
				constraints.emplace(cname, ConstraintRec(ConstraintRec::Reference, it.first, target, ref->onRemove));
				indexes.emplace(name + "_idx_" + it.first, it.first);
			} else if (type == storage::Type::File || type == storage::Type::Image) {
				auto ref = serv.getFileScheme();
				auto cname = name + "_ref_" + it.first;
				auto & target = ref->getName();
				constraints.emplace(cname, ConstraintRec(ConstraintRec::Reference, it.first, target, storage::RemovePolicy::Null));
			}

			if ((type == storage::Type::Text && f.getTransform() == storage::Transform::Alias) ||
					f.hasFlag(storage::Flags::Unique)) {
				constraints.emplace(name + "_unique_" + it.first, ConstraintRec(ConstraintRec::Unique, it.first));
			}

			if ((type == storage::Type::Text && f.getTransform() == storage::Transform::Alias)
					|| (f.hasFlag(storage::Flags::Indexed) && !f.hasFlag(storage::Flags::Unique))) {
				indexes.emplace(name + "_idx_" + it.first, it.first);
			}
		}
	}

	if (hasTriggers) {
		size_t id = std::hash<String>{}(hashStream.weak());

		hashStream.clear();
		hashStream << "_sa_auto_" << scheme->getName() << "_" << id << "_trigger";
		triggers.emplace(hashStream.str());
	}
}

NS_SA_EXT_END(database)
