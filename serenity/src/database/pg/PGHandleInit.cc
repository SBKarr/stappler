// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2016-2018 Roman Katuntsev <sbkarr@stappler.org>

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
#include "PGHandle.h"
#include "StorageScheme.h"

NS_SA_EXT_BEGIN(pg)

using RemovePolicy = storage::RemovePolicy;

struct ConstraintRec {
	enum Type {
		Unique,
		Reference,
	};

	Type type;
	Vector<String> fields;
	String reference;
	RemovePolicy remove = RemovePolicy::Null;

	ConstraintRec(Type t) : type(t) { }
	ConstraintRec(Type t, std::initializer_list<String> il) : type(t), fields(il) { }
	ConstraintRec(Type t, const String &col, const String &ref = "", RemovePolicy r = RemovePolicy::Null)
	: type(t), fields{col}, reference(ref), remove(r) { }
};

struct ColRec {
	enum class Type {
		None,
		Binary,
		Integer,
		Serial,
		Float,
		Boolean,
		Text,
		TsVector
	};

	Type type = Type::None;
	bool notNull = false;

	ColRec(Type t, bool notNull = false) : type(t), notNull(notNull) { }
};

struct TableRec {
	using Scheme = storage::Scheme;

	static String getNameForDelta(const Scheme &);

	static Map<String, TableRec> parse(Server &serv, const Map<String, const Scheme *> &s);
	static Map<String, TableRec> get(Handle &h, StringStream &stream);

	static void writeCompareResult(StringStream &stream,
			Map<String, TableRec> &required, Map<String, TableRec> &existed,
			const Map<String, const storage::Scheme *> &s);

	TableRec();
	TableRec(Server &serv, const storage::Scheme *scheme);

	Map<String, ColRec> cols;
	Map<String, ConstraintRec> constraints;
	Map<String, String> indexes;
	Vector<String> pkey;
	Set<String> triggers;
	bool objects = true;

	bool exists = false;
	bool valid = false;

	const storage::Scheme *viewScheme = nullptr;
	const storage::FieldView *viewField = nullptr;
};


constexpr static uint32_t getDefaultFunctionVersion() { return 10; }

constexpr static const char * DATABASE_DEFAULTS = R"Sql(
CREATE TABLE IF NOT EXISTS __objects (
	__oid bigserial NOT NULL,
	CONSTRAINT __objects_pkey PRIMARY KEY (__oid)
) WITH ( OIDS=FALSE );

CREATE TABLE IF NOT EXISTS __removed (
	__oid bigint NOT NULL,
	CONSTRAINT __removed_pkey PRIMARY KEY (__oid)
) WITH ( OIDS=FALSE );

CREATE TABLE IF NOT EXISTS __sessions (
	name bytea NOT NULL,
	mtime bigint NOT NULL,
	maxage bigint NOT NULL,
	data bytea,
	CONSTRAINT __sessions_pkey PRIMARY KEY (name)
) WITH ( OIDS=FALSE );

CREATE TABLE IF NOT EXISTS __broadcasts (
	id bigserial NOT NULL,
	date bigint NOT NULL,
	msg bytea,
	CONSTRAINT __broadcasts_pkey PRIMARY KEY (id)
) WITH ( OIDS=FALSE );
CREATE INDEX IF NOT EXISTS __broadcasts_date ON __broadcasts ("date" DESC);

CREATE TABLE IF NOT EXISTS __login (
	id bigserial NOT NULL,
	"user" bigint NOT NULL,
	name text NOT NULL,
	password bytea NOT NULL,
	date bigint NOT NULL,
	success boolean NOT NULL,
	addr inet,
	host text,
	path text,
	CONSTRAINT __login_pkey PRIMARY KEY (id)
) WITH ( OIDS=FALSE );
CREATE INDEX IF NOT EXISTS __login_user ON __login ("user");
CREATE INDEX IF NOT EXISTS __login_date ON __login (date);)Sql";

constexpr static const char * INDEX_QUERY = R"Sql(
WITH tables AS (SELECT table_name AS name FROM information_schema.tables WHERE table_schema='public' AND table_type='BASE TABLE')
SELECT pg_class.relname as table_name, i.relname as index_name, array_to_string(array_agg(a.attname), ', ') as column_names
FROM pg_class INNER JOIN tables ON (tables.name = pg_class.relname), pg_class i, pg_index ix, pg_attribute a
WHERE pg_class.oid = ix.indrelid
	AND i.oid = ix.indexrelid
	AND a.attrelid = pg_class.oid
	AND a.attnum = ANY(ix.indkey)
	AND pg_class.relkind = 'r'
GROUP BY pg_class.relname, i.relname ORDER BY pg_class.relname, i.relname;)Sql";

static void writeFileUpdateTrigger(StringStream &stream, const storage::Scheme *s, const storage::Field &obj) {
	stream << "\t\tIF (NEW.\"" << obj.getName() << "\" IS NULL OR OLD.\"" << obj.getName() << "\" <> NEW.\"" << obj.getName() << "\") THEN\n"
		<< "\t\t\tIF (OLD.\"" << obj.getName() << "\" IS NOT NULL) THEN\n"
		<< "\t\t\t\tINSERT INTO __removed (__oid) VALUES (OLD.\"" << obj.getName() << "\");\n"
		<< "\t\t\tEND IF;\n\t\tEND IF;\n";
}

static void writeFileRemoveTrigger(StringStream &stream, const storage::Scheme *s, const storage::Field &obj) {
	stream << "\t\tIF (OLD.\"" << obj.getName() << "\" IS NOT NULL) THEN\n"
		<< "\t\t\tINSERT INTO __removed (__oid) VALUES (OLD.\"" << obj.getName() << "\");\n"
		<< "\t\tEND IF;\n";
}

static void writeObjectSetRemoveTrigger(StringStream &stream, const storage::Scheme *s, const storage::FieldObject *obj) {
	auto source = s->getName();
	auto target = obj->scheme->getName();

	stream << "\t\tDELETE FROM " << target << " WHERE __oid IN (SELECT " << target << "_id FROM "
			<< s->getName() << "_f_" << obj->name << " WHERE "<< source << "_id=OLD.__oid);\n";
}

static void writeObjectUpdateTrigger(StringStream &stream, const storage::Scheme *s, const storage::FieldObject *obj) {
	auto target = obj->scheme->getName();

	stream << "\t\tIF (NEW.\"" << obj->getName() << "\" IS NULL OR OLD.\"" << obj->getName() << "\" <> NEW.\"" << obj->getName() << "\") THEN\n"
		<< "\t\t\tIF (OLD.\"" << obj->getName() << "\" IS NOT NULL) THEN\n"
		<< "\t\t\t\tDELETE FROM " << target << " WHERE __oid=OLD." << obj->getName() << ";\n";
	stream << "\t\t\tEND IF;\n\t\tEND IF;\n";
}

static void writeObjectRemoveTrigger(StringStream &stream, const storage::Scheme *s, const storage::FieldObject *obj) {
	auto target = obj->scheme->getName();

	stream << "\t\tIF (OLD.\"" << obj->getName() << "\" IS NOT NULL) THEN\n"
		<< "\t\t\tDELETE FROM " << target << " WHERE __oid=OLD." << obj->getName() << ";\n";
	stream << "\t\tEND IF;\n";
}

static void writeAfterTrigger(StringStream &stream, const storage::Scheme *s, const String &triggerName) {
	auto &fields = s->getFields();

	auto writeInsertDelta = [&] (Handle::DeltaAction a) {
		if (a == Handle::DeltaAction::Create || a == Handle::DeltaAction::Update) {
			stream << "\t\tINSERT INTO " << TableRec::getNameForDelta(*s) << "(\"object\",\"action\",\"time\",\"user\")"
				"VALUES(NEW.__oid,1,current_setting('serenity.now')::bigint,current_setting('serenity.user')::bigint);\n";
		} else {
			stream << "\t\tINSERT INTO " << TableRec::getNameForDelta(*s) << "(\"object\",\"action\",\"time\",\"user\")"
				"VALUES(OLD.__oid," << toInt(a) << ",current_setting('serenity.now')::bigint,current_setting('serenity.user')::bigint);\n";
		}
	};

	stream << "CREATE OR REPLACE FUNCTION " << triggerName << "_func() RETURNS TRIGGER AS $" << triggerName
			<< "$ BEGIN\n\tIF (TG_OP = 'INSERT') THEN\n";
	if (s->hasDelta()) {
		writeInsertDelta(Handle::DeltaAction::Create);
	}
	stream << "\tELSIF (TG_OP = 'UPDATE') THEN\n";
	for (auto &it : fields) {
		if (it.second.isFile()) {
			writeFileUpdateTrigger(stream, s, it.second);
		} else if (it.second.getType() == storage::Type::Object) {
			const storage::FieldObject *objSlot = static_cast<const storage::FieldObject *>(it.second.getSlot());
			if (objSlot->onRemove == storage::RemovePolicy::StrongReference) {
				writeObjectUpdateTrigger(stream, s, objSlot);
			}
		}
	}
	if (s->hasDelta()) {
		writeInsertDelta(Handle::DeltaAction::Update);
	}
	stream << "\tELSIF (TG_OP = 'DELETE') THEN\n";
	for (auto &it : fields) {
		if (it.second.isFile()) {
			writeFileRemoveTrigger(stream, s, it.second);
		} else if (it.second.getType() == storage::Type::Object) {
			const storage::FieldObject *objSlot = static_cast<const storage::FieldObject *>(it.second.getSlot());
			if (objSlot->onRemove == storage::RemovePolicy::StrongReference) {
				writeObjectRemoveTrigger(stream, s, objSlot);
			}
		}
	}
	if (s->hasDelta()) {
		writeInsertDelta(Handle::DeltaAction::Delete);
	}
	stream << "\tEND IF;\n\tRETURN NULL;\n";
	stream << "\nEND; $" << triggerName << "$ LANGUAGE plpgsql;\n";

	stream << "CREATE TRIGGER " << triggerName << " AFTER INSERT OR UPDATE OR DELETE ON \"" << s->getName()
			<< "\" FOR EACH ROW EXECUTE PROCEDURE " << triggerName << "_func();\n";
}

static void writeBeforeTrigger(StringStream &stream, const storage::Scheme *s, const String &triggerName) {
	auto &fields = s->getFields();

	stream << "CREATE OR REPLACE FUNCTION " << triggerName << "_func() RETURNS TRIGGER AS $" << triggerName
			<< "$ BEGIN\n\tIF (TG_OP = 'DELETE') THEN\n";

	for (auto &it : fields) {
		if (it.second.getType() == storage::Type::Set) {
			const storage::FieldObject *objSlot = static_cast<const storage::FieldObject *>(it.second.getSlot());
			if (objSlot->onRemove == storage::RemovePolicy::StrongReference) {
				writeObjectSetRemoveTrigger(stream, s, objSlot);
			}
		}
	}

	stream << "\tEND IF;\n\tRETURN OLD;\n";
	stream << "\nEND; $" << triggerName << "$ LANGUAGE plpgsql;\n";

	stream << "CREATE TRIGGER " << triggerName << " BEFORE DELETE ON \"" << s->getName()
			<< "\" FOR EACH ROW EXECUTE PROCEDURE " << triggerName << "_func();\n";
}

static void writeDeltaTrigger(StringStream &stream, const StringView &name, const TableRec &s, const StringView &triggerName) {
	String deltaName = toString(name.sub(0, name.size() - 5), "_delta");
	String tagField = toString(s.viewScheme->getName(), "_id");
	String objField = toString(s.viewField->scheme->getName(), "_id");

	stream << "CREATE OR REPLACE FUNCTION " << triggerName << "_func() RETURNS TRIGGER AS $" << triggerName
			<< "$ BEGIN\n"
			"\tIF (TG_OP = 'INSERT') THEN\n";


	stream << "\tINSERT INTO " << deltaName << " (\"tag\", \"object\", \"time\", \"user\") VALUES("
			"NEW.\"" << tagField << "\",NEW.\"" << objField << "\","
			"current_setting('serenity.now')::bigint,current_setting('serenity.user')::bigint);\n";


	stream << "\tELSIF (TG_OP = 'UPDATE') THEN\n";


	stream << "\tINSERT INTO " << deltaName << " (\"tag\", \"object\", \"time\", \"user\") VALUES("
			"OLD.\"" << tagField << "\",OLD.\"" << objField << "\","
			"current_setting('serenity.now')::bigint,current_setting('serenity.user')::bigint);\n";


	stream << "\tELSIF (TG_OP = 'DELETE') THEN\n";


	stream << "\tINSERT INTO " << deltaName << " (\"tag\", \"object\", \"time\", \"user\") VALUES("
			"OLD.\"" << tagField << "\",OLD.\"" << objField << "\","
			"current_setting('serenity.now')::bigint,current_setting('serenity.user')::bigint);\n";


	stream << "\tEND IF;\n"
			"\tRETURN NULL;\n"
			"END; $" << triggerName << "$ LANGUAGE plpgsql;\n";
	stream << "CREATE TRIGGER " << triggerName << " AFTER INSERT OR UPDATE OR DELETE ON \"" << name
			<< "\" FOR EACH ROW EXECUTE PROCEDURE " << triggerName << "_func();\n";
}

void TableRec::writeCompareResult(StringStream &stream,
		Map<String, TableRec> &required, Map<String, TableRec> &existed,
		const Map<String, const storage::Scheme *> &s) {
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
						if (ex_col.notNull != req_col.notNull) {
							if (ex_col.notNull) {
								stream << "ALTER TABLE " << ex_it.first << " ALTER COLUMN \"" << ex_col_it.first << "\" DROP NOT NULL;\n";
							} else {
								stream << "ALTER TABLE " << ex_it.first << " ALTER COLUMN \"" << ex_col_it.first << "\" SET NOT NULL;\n";
							}
						}
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
				case ColRec::Type::TsVector:stream << "tsvector"; break;
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
				case ColRec::Type::TsVector:stream << "tsvector"; break;
				default: break;
				}
				if (cit.second.notNull) {
					stream << " NOT NULL";
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
				case RemovePolicy::Cascade:
					stream << " ON DELETE CASCADE";
					break;
				case RemovePolicy::Restrict:
					stream << " ON DELETE RESTRICT";
					break;
				case RemovePolicy::Null:
				case RemovePolicy::Reference:
				case RemovePolicy::StrongReference:
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
					if (StringView(tit).starts_with("_tr_a_")) {
						writeAfterTrigger(stream, scheme_it->second, tit);
					} else {
						writeBeforeTrigger(stream, scheme_it->second, tit);
					}
				}
			} else if (it.second.viewField) {
				for (auto & tit : it.second.triggers) {
					writeDeltaTrigger(stream, it.first, it.second, tit);
				}
			}
		}
	}
}

String TableRec::getNameForDelta(const Scheme &scheme) {
	return toString("__delta_", scheme.getName());
}

Map<String, TableRec> TableRec::parse(Server &serv, const Map<String, const storage::Scheme *> &s) {
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
				if (ref->onRemove == RemovePolicy::Reference || ref->onRemove == RemovePolicy::StrongReference) {
					String name = it.first + "_f_" + fit.first;
					auto & source = it.first;
					auto target = ref->scheme->getName();
					TableRec table;
					table.cols.emplace(source + "_id", ColRec(ColRec::Type::Integer, true));
					table.cols.emplace(toString(target, "_id"), ColRec(ColRec::Type::Integer, true));

					table.constraints.emplace(name + "_ref_" + source, ConstraintRec(
							ConstraintRec::Reference, source + "_id", source, RemovePolicy::Cascade));
					table.constraints.emplace(name + "_ref_" + ref->getName(), ConstraintRec(
							ConstraintRec::Reference, toString(target, "_id"), target.str(), RemovePolicy::Cascade));

					table.pkey.emplace_back(source + "_id");
					table.pkey.emplace_back(toString(target, "_id"));
					tables.emplace(std::move(string::tolower(name)), std::move(table));
				}
			}

			if (type == storage::Type::Array) {
				auto slot = static_cast<const storage::FieldArray *>(f.getSlot());
				if (slot->tfield && slot->tfield.isSimpleLayout()) {

					String name = it.first + "_f_" + fit.first;
					auto & source = it.first;

					TableRec table;
					table.cols.emplace("id", ColRec(ColRec::Type::Serial, true));
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
							ConstraintRec::Reference, source + "_id", source, RemovePolicy::Cascade));
					table.pkey.emplace_back("id");

					if (f.hasFlag(storage::Flags::Unique)) {
						table.constraints.emplace(name + "_unique", ConstraintRec(ConstraintRec::Unique, {source + "_id", "data"}));
					}

					table.indexes.emplace(name + "_idx_" + source, source + "_id");
					tables.emplace(std::move(name), std::move(table));
				}
			}

			if (type == storage::Type::View) {
				auto slot = static_cast<const storage::FieldView *>(f.getSlot());

				String name = toString(it.first, "_f_", fit.first, "_view");
				auto & source = it.first;
				auto target = slot->scheme->getName();

				TableRec table;
				table.viewScheme = it.second;
				table.viewField = slot;
				table.cols.emplace("__vid", ColRec(ColRec::Type::Serial, true));
				table.cols.emplace(source + "_id", ColRec(ColRec::Type::Integer, true));
				table.cols.emplace(toString(target, "_id"), ColRec(ColRec::Type::Integer, true));

				for (auto &it : slot->fields) {
					auto type = it.second.getType();
					switch (type) {
					case storage::Type::Float:
						table.cols.emplace(it.first, ColRec(ColRec::Type::Float));
						break;
					case storage::Type::Boolean:
						table.cols.emplace(it.first, ColRec(ColRec::Type::Boolean));
						break;
					case storage::Type::Text:
						table.cols.emplace(it.first, ColRec(ColRec::Type::Text));
						break;
					case storage::Type::Data:
					case storage::Type::Bytes:
					case storage::Type::Extra:
						table.cols.emplace(it.first, ColRec(ColRec::Type::Binary));
						break;
					case storage::Type::Integer:
						table.cols.emplace(it.first, ColRec(ColRec::Type::Integer));
						break;
					default:
						break;
					}

					if (it.second.isIndexed()) {
						table.indexes.emplace(name + "_idx_" + it.first, it.first);
					}
				}

				table.constraints.emplace(name + "_ref_" + source, ConstraintRec(
						ConstraintRec::Reference, source + "_id", source, RemovePolicy::Cascade));
				table.constraints.emplace(name + "_ref_" + slot->getName(), ConstraintRec(
						ConstraintRec::Reference, toString(target, "_id"), target.str(), RemovePolicy::Cascade));

				table.pkey.emplace_back("__vid");
				auto tblIt = tables.emplace(std::move(name), std::move(table)).first;

				if (slot->delta) {
					StringStream hashStream;
					hashStream << getDefaultFunctionVersion() << tblIt->first << "_delta";

					size_t id = std::hash<String>{}(hashStream.weak());
					hashStream.clear();
					hashStream << "_trig_" << tblIt->first << "_" << id;
					tblIt->second.triggers.emplace(StringView(hashStream.weak()).sub(0, 56).str());

					String name = toString(it.first, "_f_", fit.first, "_delta");
					table.cols.emplace("id", ColRec(ColRec::Type::Serial, true));
					table.cols.emplace("tag", ColRec(ColRec::Type::Integer, true));
					table.cols.emplace("object", ColRec(ColRec::Type::Integer, true));
					table.cols.emplace("time", ColRec(ColRec::Type::Integer, true));
					table.cols.emplace("user", ColRec(ColRec::Type::Integer));

					table.pkey.emplace_back("id");
					table.indexes.emplace(name + "_idx_tag", "tag");
					table.indexes.emplace(name + "_idx_object", "object");
					table.indexes.emplace(name + "_idx_time", "time");
					tables.emplace(move(name), std::move(table));
				}
			}

			if (scheme->hasDelta()) {
				auto name = getNameForDelta(*scheme);
				TableRec table;
				table.cols.emplace("id", ColRec(ColRec::Type::Serial, true));
				table.cols.emplace("object", ColRec(ColRec::Type::Integer, true));
				table.cols.emplace("time", ColRec(ColRec::Type::Integer, true));
				table.cols.emplace("action", ColRec(ColRec::Type::Integer, true));
				table.cols.emplace("user", ColRec(ColRec::Type::Integer));

				table.pkey.emplace_back("id");
				table.indexes.emplace(name + "_idx_object", "object");
				table.indexes.emplace(name + "_idx_time", "time");
				tables.emplace(move(name), std::move(table));
			}
		}
	}
	return tables;
}

Map<String, TableRec> TableRec::get(Handle &h, StringStream &stream) {
	Map<String, TableRec> ret;

	h.performSimpleSelect("SELECT table_name FROM information_schema.tables "
			"WHERE table_schema='public' AND table_type='BASE TABLE';",
			[&] (sql::Result &tables) {
		for (auto it : tables) {
			ret.emplace(it.at(0).str(), TableRec());
			stream << "TABLE " << it.at(0) << "\n";
		}
		tables.clear();
	});

	h.performSimpleSelect("SELECT table_name, column_name, is_nullable, data_type FROM information_schema.columns "
			"WHERE table_schema='public';"_weak,
			[&] (sql::Result &columns) {
		for (auto it : columns) {
			auto tname = it.at(0).str();
			auto f = ret.find(tname);
			if (f != ret.end()) {
				auto &table = f->second;
				bool isNullable = (it.at(2) == "YES");
				auto type = it.at(3);
				if (it.at(1) != "__oid") {
					if (type == "bigint") {
						table.cols.emplace(it.at(1).str(), ColRec(ColRec::Type::Integer, !isNullable));
					} else if (type == "boolean") {
						table.cols.emplace(it.at(1).str(), ColRec(ColRec::Type::Boolean, !isNullable));
					} else if (type == "double precision") {
						table.cols.emplace(it.at(1).str(), ColRec(ColRec::Type::Float, !isNullable));
					} else if (type == "text") {
						table.cols.emplace(it.at(1).str(), ColRec(ColRec::Type::Text, !isNullable));
					} else if (type == "bytea") {
						table.cols.emplace(it.at(1).str(), ColRec(ColRec::Type::Binary, !isNullable));
					} else if (type == "tsvector") {
						table.cols.emplace(it.at(1).str(), ColRec(ColRec::Type::TsVector, !isNullable));
					} else {
						table.cols.emplace(it.at(1).str(), ColRec(ColRec::Type::None, !isNullable));
					}
				}
				stream << "COLUMNS " << it.at(0) << " " << it.at(1) << " " << it.at(2) << " " << it.at(3) << "\n";
			}
		}
		columns.clear();
	});

	h.performSimpleSelect("SELECT table_name, constraint_name, constraint_type FROM information_schema.table_constraints "
			"WHERE table_schema='public' AND constraint_schema='public';"_weak,
			[&] (sql::Result &constraints) {
		for (auto it : constraints) {
			auto tname = it.at(0).str();
			auto f = ret.find(tname);
			if (f != ret.end()) {
				auto &table = f->second;
				if (it.at(2) == "UNIQUE") {
					table.constraints.emplace(it.at(1).str(), ConstraintRec(ConstraintRec::Unique));
					stream << "CONSTRAINT " << it.at(0) << " " << it.at(1) << " " << it.at(2) << "\n";
				} else if (it.at(2) == "FOREIGN KEY") {
					table.constraints.emplace(it.at(1).str(), ConstraintRec(ConstraintRec::Reference));
					stream << "CONSTRAINT " << it.at(0) << " " << it.at(1) << " " << it.at(2) << "\n";
				}
			}
		}
		constraints.clear();
	});

	h.performSimpleSelect(String::make_weak(INDEX_QUERY),
			[&] (sql::Result &indexes) {
		for (auto it : indexes) {
			auto tname = it.at(0).str();
			auto f = ret.find(tname);
			if (f != ret.end()) {
				auto &table = f->second;
				auto name = it.at(1);
				name.readUntilString("_idx_");
				if (name.is("_idx_")) {
					table.indexes.emplace(it.at(1).str(), it.at(2).str());
					stream << "INDEX " << it.at(0) << " " << it.at(1) << " " << it.at(2) << "\n";
				}
			}
		}
		indexes.clear();
	});

	h.performSimpleSelect("SELECT event_object_table, trigger_name FROM information_schema.triggers "
			"WHERE trigger_schema='public';"_weak,
			[&] (sql::Result &triggers) {
		for (auto it : triggers) {
			auto tname = it.at(0).str();
			auto f = ret.find(tname);
			if (f != ret.end()) {
				auto &table = f->second;
				table.triggers.emplace(it.at(1).str());
				stream << "TRIGGER " << it.at(0) << " " << it.at(1) << "\n";
			}
		}
		triggers.clear();
	});
	return ret;
}

TableRec::TableRec() : objects(false) { }
TableRec::TableRec(Server &serv, const storage::Scheme *scheme) {
	StringStream hashStreamAfter; hashStreamAfter << getDefaultFunctionVersion();
	StringStream hashStreamBefore; hashStreamBefore << getDefaultFunctionVersion();

	bool hasAfterTrigger = false;
	bool hasBeforeTrigger = false;
	auto name = scheme->getName();
	pkey.emplace_back("__oid");

	if (scheme->hasDelta()) {
		hasAfterTrigger = true;
		hashStreamAfter << ":delta:";
	}

	for (auto &it : scheme->getFields()) {
		bool emplaced = false;
		auto &f = it.second;
		auto type = it.second.getType();

		if (type == storage::Type::File || type == storage::Type::Image) {
			hasAfterTrigger = true;
			hashStreamAfter << it.first << toInt(type);
		}

		switch (type) {
		case storage::Type::None:
		case storage::Type::Array:
		case storage::Type::View:
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
		case storage::Type::File:
		case storage::Type::Image:
			cols.emplace(it.first, ColRec(ColRec::Type::Integer, f.hasFlag(storage::Flags::Required)));
			emplaced = true;
			break;

		case storage::Type::FullTextView:
			cols.emplace(it.first, ColRec(ColRec::Type::TsVector, f.hasFlag(storage::Flags::Required)));
			emplaced = true;
			break;

		case storage::Type::Object:
			cols.emplace(it.first, ColRec(ColRec::Type::Integer, f.hasFlag(storage::Flags::Required)));
			if (f.isReference()) {
				auto objSlot = static_cast<const storage::FieldObject *>(f.getSlot());
				if (objSlot->onRemove == storage::RemovePolicy::StrongReference) {
					hasAfterTrigger = true;
					hashStreamAfter << it.first << toInt(type);
				}
			}
			emplaced = true;
			break;

		case storage::Type::Set:
			if (f.isReference()) {
				auto objSlot = static_cast<const storage::FieldObject *>(f.getSlot());
				if (objSlot->onRemove == storage::RemovePolicy::StrongReference) {
					hasBeforeTrigger = true;
					hashStreamBefore << it.first << toInt(type);
				}
			}
			break;
		}

		if (emplaced) {
			if (type == storage::Type::Object) {
				auto ref = static_cast<const storage::FieldObject *>(f.getSlot());
				auto target = ref->scheme->getName();
				auto cname = toString(name, "_ref_", it.first, "_", target);
				constraints.emplace(cname, ConstraintRec(ConstraintRec::Reference, it.first, target.str(), ref->onRemove));
				indexes.emplace(toString(name, "_idx_", it.first), it.first);
			} else if (type == storage::Type::File || type == storage::Type::Image) {
				auto ref = serv.getFileScheme();
				auto cname = toString(name, "_ref_", it.first);
				auto target = ref->getName();
				constraints.emplace(cname, ConstraintRec(ConstraintRec::Reference, it.first, target.str(), storage::RemovePolicy::Null));
			}

			if ((type == storage::Type::Text && f.getTransform() == storage::Transform::Alias) ||
					f.hasFlag(storage::Flags::Unique)) {
				constraints.emplace(toString(name, "_unique_", it.first), ConstraintRec(ConstraintRec::Unique, it.first));
			}

			if ((type == storage::Type::Text && f.getTransform() == storage::Transform::Alias)
					|| (f.hasFlag(storage::Flags::Indexed) && !f.hasFlag(storage::Flags::Unique))) {
				indexes.emplace(toString(name, "_idx_", it.first), it.first);
			}
		}
	}

	if (hasAfterTrigger) {
		size_t id = std::hash<String>{}(hashStreamAfter.weak());

		hashStreamAfter.clear();
		hashStreamAfter << "_tr_a_" << scheme->getName() << "_" << id;
		triggers.emplace(StringView(hashStreamAfter.weak()).sub(0, 56).str());
	}

	if (hasBeforeTrigger) {
		size_t id = std::hash<String>{}(hashStreamBefore.weak());

		hashStreamBefore.clear();
		hashStreamBefore << "_tr_b_" << scheme->getName() << "_" << id;
		triggers.emplace(StringView(hashStreamBefore.weak()).sub(0, 56).str());
	}
}

bool Handle::init(Server &serv, const Map<String, const Scheme *> &s) {
	if (!performSimpleQuery(String::make_weak(DATABASE_DEFAULTS))) {
		return false;
	}

	if (!performSimpleQuery("START TRANSACTION; LOCK TABLE __objects;"_weak)) {
		return false;
	}

	StringStream tables;
	tables << "Server: " << serv.getServerHostname() << "\n";

	auto requiredTables = TableRec::parse(serv, s);
	auto existedTables = TableRec::get(*this, tables);

	StringStream stream;
	TableRec::writeCompareResult(stream, requiredTables, existedTables, s);

	auto name = toString(".serenity/update.", Time::now().toMilliseconds(), ".sql");
	if (stream.size() > 3) {
		if (performSimpleQuery(stream.weak())) {
			performSimpleQuery("COMMIT;"_weak);
		} else {
			log::format("Database", "Fail to perform update %s", name.c_str());
			stream << "\nError: " << lastError << "\n" << apr_dbd_error(handle->driver, handle->handle, lastError) << "\n";

			performSimpleQuery("ROLLBACK;"_weak);
		}

		tables << "\n" << stream;
		filesystem::write(name, (const uint8_t *)tables.data(), tables.size());
	} else {
		performSimpleQuery("COMMIT;"_weak);
	}

	return true;
}

NS_SA_EXT_END(pg)
