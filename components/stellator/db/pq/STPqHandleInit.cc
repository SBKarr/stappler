// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2016-2019 Roman Katuntsev <sbkarr@stappler.org>

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

#include "STPqHandle.h"

NS_DB_PQ_BEGIN

using RemovePolicy = db::RemovePolicy;

struct ConstraintRec {
	enum Type {
		Unique,
		Reference,
	};

	Type type;
	mem::Vector<mem::String> fields;
	mem::String reference;
	db::RemovePolicy remove = db::RemovePolicy::Null;

	ConstraintRec(Type t) : type(t) { }
	ConstraintRec(Type t, std::initializer_list<mem::String> il) : type(t), fields(il) { }
	ConstraintRec(Type t, const mem::String &col, const mem::String &ref = "", db::RemovePolicy r = db::RemovePolicy::Null)
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
		TsVector,
		Custom
	};

	Type type = Type::Custom;
	mem::String custom;
	bool notNull = false;

	ColRec(Type t, bool notNull = false) : type(t), notNull(notNull) { }
	ColRec(const mem::StringView &t, bool notNull = false) : custom(t.str<mem::Interface>()), notNull(notNull) { }
};

struct TableRec {
	using Scheme = db::Scheme;

	static mem::String getNameForDelta(const Scheme &);

	static mem::Map<mem::String, TableRec> parse(const db::Interface::Config &cfg, const mem::Map<mem::String, const Scheme *> &s);
	static mem::Map<mem::String, TableRec> get(Handle &h, mem::StringStream &stream);

	static void writeCompareResult(mem::StringStream &stream,
			mem::Map<mem::String, TableRec> &required, mem::Map<mem::String, TableRec> &existed,
			const mem::Map<mem::String, const db::Scheme *> &s);

	TableRec();
	TableRec(const db::Interface::Config &cfg, const db::Scheme *scheme);

	mem::Map<mem::String, ColRec> cols;
	mem::Map<mem::String, ConstraintRec> constraints;
	mem::Map<mem::String, mem::String> indexes;
	mem::Vector<mem::String> pkey;
	mem::Set<mem::String> triggers;
	bool objects = true;

	bool exists = false;
	bool valid = false;

	const db::Scheme *viewScheme = nullptr;
	const db::FieldView *viewField = nullptr;
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
CREATE INDEX IF NOT EXISTS __login_date ON __login (date);

CREATE EXTENSION IF NOT EXISTS intarray;)Sql";

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

static void writeFileUpdateTrigger(mem::StringStream &stream, const db::Scheme *s, const db::Field &obj) {
	stream << "\t\tIF (NEW.\"" << obj.getName() << "\" IS NULL OR OLD.\"" << obj.getName() << "\" <> NEW.\"" << obj.getName() << "\") THEN\n"
		<< "\t\t\tIF (OLD.\"" << obj.getName() << "\" IS NOT NULL) THEN\n"
		<< "\t\t\t\tINSERT INTO __removed (__oid) VALUES (OLD.\"" << obj.getName() << "\");\n"
		<< "\t\t\tEND IF;\n\t\tEND IF;\n";
}

static void writeFileRemoveTrigger(mem::StringStream &stream, const db::Scheme *s, const db::Field &obj) {
	stream << "\t\tIF (OLD.\"" << obj.getName() << "\" IS NOT NULL) THEN\n"
		<< "\t\t\tINSERT INTO __removed (__oid) VALUES (OLD.\"" << obj.getName() << "\");\n"
		<< "\t\tEND IF;\n";
}

static void writeObjectSetRemoveTrigger(mem::StringStream &stream, const db::Scheme *s, const db::FieldObject *obj) {
	auto source = s->getName();
	auto target = obj->scheme->getName();

	stream << "\t\tDELETE FROM " << target << " WHERE __oid IN (SELECT " << target << "_id FROM "
			<< s->getName() << "_f_" << obj->name << " WHERE "<< source << "_id=OLD.__oid);\n";
}

static void writeObjectUpdateTrigger(mem::StringStream &stream, const db::Scheme *s, const db::FieldObject *obj) {
	auto target = obj->scheme->getName();

	stream << "\t\tIF (NEW.\"" << obj->getName() << "\" IS NULL OR OLD.\"" << obj->getName() << "\" <> NEW.\"" << obj->getName() << "\") THEN\n"
		<< "\t\t\tIF (OLD.\"" << obj->getName() << "\" IS NOT NULL) THEN\n"
		<< "\t\t\t\tDELETE FROM " << target << " WHERE __oid=OLD." << obj->getName() << ";\n";
	stream << "\t\t\tEND IF;\n\t\tEND IF;\n";
}

static void writeObjectRemoveTrigger(mem::StringStream &stream, const db::Scheme *s, const db::FieldObject *obj) {
	auto target = obj->scheme->getName();

	stream << "\t\tIF (OLD.\"" << obj->getName() << "\" IS NOT NULL) THEN\n"
		<< "\t\t\tDELETE FROM " << target << " WHERE __oid=OLD." << obj->getName() << ";\n";
	stream << "\t\tEND IF;\n";
}

static void writeAfterTrigger(mem::StringStream &stream, const db::Scheme *s, const mem::String &triggerName) {
	auto &fields = s->getFields();

	auto writeInsertDelta = [&] (Handle::DeltaAction a) {
		if (a == Handle::DeltaAction::Create || a == Handle::DeltaAction::Update) {
			stream << "\t\tINSERT INTO " << TableRec::getNameForDelta(*s) << "(\"object\",\"action\",\"time\",\"user\")"
				"VALUES(NEW.__oid,1,current_setting('serenity.now')::bigint,current_setting('serenity.user')::bigint);\n";
		} else {
			stream << "\t\tINSERT INTO " << TableRec::getNameForDelta(*s) << "(\"object\",\"action\",\"time\",\"user\")"
				"VALUES(OLD.__oid," << stappler::toInt(a) << ",current_setting('serenity.now')::bigint,current_setting('serenity.user')::bigint);\n";
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
		} else if (it.second.getType() == db::Type::Object) {
			const db::FieldObject *objSlot = static_cast<const db::FieldObject *>(it.second.getSlot());
			if (objSlot->onRemove == db::RemovePolicy::StrongReference) {
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
		} else if (it.second.getType() == db::Type::Object) {
			const db::FieldObject *objSlot = static_cast<const db::FieldObject *>(it.second.getSlot());
			if (objSlot->onRemove == db::RemovePolicy::StrongReference) {
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

static void writeBeforeTrigger(mem::StringStream &stream, const db::Scheme *s, const mem::String &triggerName) {
	auto &fields = s->getFields();

	stream << "CREATE OR REPLACE FUNCTION " << triggerName << "_func() RETURNS TRIGGER AS $" << triggerName
			<< "$ BEGIN\n\tIF (TG_OP = 'DELETE') THEN\n";

	for (auto &it : fields) {
		if (it.second.getType() == db::Type::Set) {
			const db::FieldObject *objSlot = static_cast<const db::FieldObject *>(it.second.getSlot());
			if (objSlot->onRemove == db::RemovePolicy::StrongReference) {
				writeObjectSetRemoveTrigger(stream, s, objSlot);
			}
		}
	}

	stream << "\tEND IF;\n\tRETURN OLD;\n";
	stream << "\nEND; $" << triggerName << "$ LANGUAGE plpgsql;\n";

	stream << "CREATE TRIGGER " << triggerName << " BEFORE DELETE ON \"" << s->getName()
			<< "\" FOR EACH ROW EXECUTE PROCEDURE " << triggerName << "_func();\n";
}

static void writeDeltaTrigger(mem::StringStream &stream, const mem::StringView &name, const TableRec &s, const mem::StringView &triggerName) {
	mem::String deltaName = mem::toString(name.sub(0, name.size() - 5), "_delta");
	mem::String tagField = mem::toString(s.viewScheme->getName(), "_id");
	mem::String objField = mem::toString(s.viewField->scheme->getName(), "_id");

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

void TableRec::writeCompareResult(mem::StringStream &stream,
		mem::Map<mem::String, TableRec> &required, mem::Map<mem::String, TableRec> &existed,
		const mem::Map<mem::String, const db::Scheme *> &s) {
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
					} else if (ex_col.type == ColRec::Type::Custom && req_type == ColRec::Type::Custom && ex_col.custom != req_col.custom) {
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
				case ColRec::Type::Custom:  stream << cit->second.custom; break;
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
				if (cit.first != "__oid") {
					stream << "ALTER TABLE " << it.first << " ADD COLUMN \"" << cit.first << "\" ";
					switch(cit.second.type) {
					case ColRec::Type::Binary:	stream << "bytea"; break;
					case ColRec::Type::Integer:	stream << "bigint"; break;
					case ColRec::Type::Serial:	stream << "bigserial"; break;
					case ColRec::Type::Float:	stream << "double precision"; break;
					case ColRec::Type::Boolean:	stream << "boolean"; break;
					case ColRec::Type::Text: 	stream << "text"; break;
					case ColRec::Type::TsVector:stream << "tsvector"; break;
					case ColRec::Type::Custom:  stream << cit.second.custom; break;
					default: break;
					}
					if (cit.second.notNull) {
						stream << " NOT NULL";
					}
					stream << ";\n";
				}
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
				case db::RemovePolicy::Cascade:
					stream << " ON DELETE CASCADE";
					break;
				case db::RemovePolicy::Restrict:
					stream << " ON DELETE RESTRICT";
					break;
				case db::RemovePolicy::Null:
				case db::RemovePolicy::Reference:
				case db::RemovePolicy::StrongReference:
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
			if (cit.second.back() != ')') {
				stream << "CREATE INDEX IF NOT EXISTS \"" << cit.first
						<< "\" ON " << it.first << " ( \"" << cit.second << "\" );\n";
			} else {
				stream << "CREATE INDEX IF NOT EXISTS \"" << cit.first
						<< "\" ON " << it.first << " " << cit.second << ";\n";
			}
		}

		if (!it.second.triggers.empty()) {
			auto scheme_it = s.find(it.first);
			if (scheme_it != s.end()) {
				for (auto & tit : it.second.triggers) {
					if (mem::StringView(tit).starts_with("_tr_a_")) {
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

mem::String TableRec::getNameForDelta(const Scheme &scheme) {
	return mem::toString("__delta_", scheme.getName());
}

mem::Map<mem::String, TableRec> TableRec::parse(const db::Interface::Config &cfg, const mem::Map<mem::String, const db::Scheme *> &s) {
	mem::Map<mem::String, TableRec> tables;
	for (auto &it : s) {
		auto scheme = it.second;
		tables.emplace(it.first, TableRec(cfg, scheme));

		// check for extra tables
		for (auto &fit : scheme->getFields()) {
			auto &f = fit.second;
			auto type = fit.second.getType();

			if (type == db::Type::Set) {
				auto ref = static_cast<const db::FieldObject *>(f.getSlot());
				if (ref->onRemove == db::RemovePolicy::Reference || ref->onRemove == db::RemovePolicy::StrongReference) {
					mem::String name = it.first + "_f_" + fit.first;
					auto & source = it.first;
					auto target = ref->scheme->getName();
					TableRec table;
					table.cols.emplace(source + "_id", ColRec(ColRec::Type::Integer, true));
					table.cols.emplace(mem::toString(target, "_id"), ColRec(ColRec::Type::Integer, true));

					table.constraints.emplace(name + "_ref_" + source, ConstraintRec(
							ConstraintRec::Reference, source + "_id", source, db::RemovePolicy::Cascade));
					table.constraints.emplace(name + "_ref_" + ref->getName(), ConstraintRec(
							ConstraintRec::Reference, mem::toString(target, "_id"), target.str<mem::Interface>(), db::RemovePolicy::Cascade));

					table.indexes.emplace(name + "_idx_" + source, source + "_id");
					table.indexes.emplace(mem::toString(name, "_idx_", target), mem::toString(target, "_id"));

					table.pkey.emplace_back(source + "_id");
					table.pkey.emplace_back(mem::toString(target, "_id"));
					tables.emplace(std::move(stappler::string::tolower(name)), std::move(table));
				}
			} else if (type == db::Type::Array) {
				auto slot = static_cast<const db::FieldArray *>(f.getSlot());
				if (slot->tfield && slot->tfield.isSimpleLayout()) {

					mem::String name = it.first + "_f_" + fit.first;
					auto & source = it.first;

					TableRec table;
					table.cols.emplace("id", ColRec(ColRec::Type::Serial, true));
					table.cols.emplace(source + "_id", ColRec(ColRec::Type::Integer));

					auto type = slot->tfield.getType();
					switch (type) {
					case db::Type::Float:
						table.cols.emplace("data", ColRec(ColRec::Type::Float));
						break;
					case db::Type::Boolean:
						table.cols.emplace("data", ColRec(ColRec::Type::Boolean));
						break;
					case db::Type::Text:
						table.cols.emplace("data", ColRec(ColRec::Type::Text));
						break;
					case db::Type::Data:
					case db::Type::Bytes:
					case db::Type::Extra:
						table.cols.emplace("data", ColRec(ColRec::Type::Binary));
						break;
					case db::Type::Integer:
						table.cols.emplace("data", ColRec(ColRec::Type::Integer));
						break;
					default:
						break;
					}

					table.constraints.emplace(name + "_ref_" + source, ConstraintRec (
							ConstraintRec::Reference, source + "_id", source, db::RemovePolicy::Cascade));
					table.pkey.emplace_back("id");

					if (f.hasFlag(db::Flags::Unique)) {
						table.constraints.emplace(name + "_unique", ConstraintRec(ConstraintRec::Unique, {source + "_id", "data"}));
					}

					table.indexes.emplace(name + "_idx_" + source, source + "_id");
					tables.emplace(std::move(name), std::move(table));
				}
			} else if (type == db::Type::View) {
				auto slot = static_cast<const db::FieldView *>(f.getSlot());

				mem::String name = mem::toString(it.first, "_f_", fit.first, "_view");
				auto & source = it.first;
				auto target = slot->scheme->getName();

				TableRec table;
				table.viewScheme = it.second;
				table.viewField = slot;
				table.cols.emplace("__vid", ColRec(ColRec::Type::Serial, true));
				table.cols.emplace(source + "_id", ColRec(ColRec::Type::Integer, true));
				table.cols.emplace(mem::toString(target, "_id"), ColRec(ColRec::Type::Integer, true));

				table.constraints.emplace(name + "_ref_" + source, ConstraintRec(
						ConstraintRec::Reference, source + "_id", source, db::RemovePolicy::Cascade));
				table.constraints.emplace(name + "_ref_" + slot->getName(), ConstraintRec(
						ConstraintRec::Reference, mem::toString(target, "_id"), target.str<mem::Interface>(), db::RemovePolicy::Cascade));

				table.indexes.emplace(name + "_idx_" + source, source + "_id");
				table.indexes.emplace(mem::toString(name, "_idx_", target), mem::toString(target, "_id"));

				table.pkey.emplace_back("__vid");
				auto tblIt = tables.emplace(std::move(name), std::move(table)).first;

				if (slot->delta) {
					mem::StringStream hashStream;
					hashStream << getDefaultFunctionVersion() << tblIt->first << "_delta";

					size_t id = std::hash<mem::String>{}(hashStream.weak());
					hashStream.clear();
					hashStream << "_trig_" << tblIt->first << "_" << id;
					tblIt->second.triggers.emplace(mem::StringView(hashStream.weak()).sub(0, 56).str<mem::Interface>());

					mem::String name = mem::toString(it.first, "_f_", fit.first, "_delta");
					table.cols.emplace("id", ColRec(ColRec::Type::Serial, true));
					table.cols.emplace("tag", ColRec(ColRec::Type::Integer, true));
					table.cols.emplace("object", ColRec(ColRec::Type::Integer, true));
					table.cols.emplace("time", ColRec(ColRec::Type::Integer, true));
					table.cols.emplace("user", ColRec(ColRec::Type::Integer));

					table.pkey.emplace_back("id");
					table.indexes.emplace(name + "_idx_tag", "tag");
					table.indexes.emplace(name + "_idx_object", "object");
					table.indexes.emplace(name + "_idx_time", "time");
					tables.emplace(std::move(name), std::move(table));
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
				tables.emplace(std::move(name), std::move(table));
			}
		}
	}
	return tables;
}

mem::Map<mem::String, TableRec> TableRec::get(Handle &h, mem::StringStream &stream) {
	mem::Map<mem::String, TableRec> ret;

	h.performSimpleSelect("SELECT table_name FROM information_schema.tables "
			"WHERE table_schema='public' AND table_type='BASE TABLE';",
			[&] (db::sql::Result &tables) {
		for (auto it : tables) {
			ret.emplace(it.at(0).str<mem::Interface>(), TableRec());
			stream << "TABLE " << it.at(0) << "\n";
		}
		tables.clear();
	});

	h.performSimpleSelect("SELECT table_name, column_name, is_nullable, data_type, udt_name::regtype FROM information_schema.columns "
			"WHERE table_schema='public';"_weak,
			[&] (db::sql::Result &columns) {
		for (auto it : columns) {
			auto tname = it.at(0).str<mem::Interface>();
			auto f = ret.find(tname);
			if (f != ret.end()) {
				auto &table = f->second;
				bool isNullable = (it.at(2) == "YES");
				auto type = it.at(3);
				if (it.at(1) != "__oid") {
					if (type == "bigint") {
						table.cols.emplace(it.at(1).str<mem::Interface>(), ColRec(ColRec::Type::Integer, !isNullable));
					} else if (type == "boolean") {
						table.cols.emplace(it.at(1).str<mem::Interface>(), ColRec(ColRec::Type::Boolean, !isNullable));
					} else if (type == "double precision") {
						table.cols.emplace(it.at(1).str<mem::Interface>(), ColRec(ColRec::Type::Float, !isNullable));
					} else if (type == "text") {
						table.cols.emplace(it.at(1).str<mem::Interface>(), ColRec(ColRec::Type::Text, !isNullable));
					} else if (type == "bytea") {
						table.cols.emplace(it.at(1).str<mem::Interface>(), ColRec(ColRec::Type::Binary, !isNullable));
					} else if (type == "tsvector") {
						table.cols.emplace(it.at(1).str<mem::Interface>(), ColRec(ColRec::Type::TsVector, !isNullable));
					} else {
						if (type == "ARRAY") {
							table.cols.emplace(it.at(1).str<mem::Interface>(), ColRec(it.at(4), !isNullable));
						} else {
							table.cols.emplace(it.at(1).str<mem::Interface>(), ColRec(type, !isNullable));
						}
					}
				}
				stream << "COLUMNS " << it.at(0) << " " << it.at(1) << " " << it.at(2) << " " << it.at(3) << " (" <<  it.at(4) << ")\n";
			}
		}
		columns.clear();
	});

	h.performSimpleSelect("SELECT table_name, constraint_name, constraint_type FROM information_schema.table_constraints "
			"WHERE table_schema='public' AND constraint_schema='public';"_weak,
			[&] (db::sql::Result &constraints) {
		for (auto it : constraints) {
			auto tname = it.at(0).str<mem::Interface>();
			auto f = ret.find(tname);
			if (f != ret.end()) {
				auto &table = f->second;
				if (it.at(2) == "UNIQUE") {
					table.constraints.emplace(it.at(1).str<mem::Interface>(), ConstraintRec(ConstraintRec::Unique));
					stream << "CONSTRAINT " << it.at(0) << " " << it.at(1) << " " << it.at(2) << "\n";
				} else if (it.at(2) == "FOREIGN KEY") {
					table.constraints.emplace(it.at(1).str<mem::Interface>(), ConstraintRec(ConstraintRec::Reference));
					stream << "CONSTRAINT " << it.at(0) << " " << it.at(1) << " " << it.at(2) << "\n";
				}
			}
		}
		constraints.clear();
	});

	h.performSimpleSelect(mem::String::make_weak(INDEX_QUERY),
			[&] (db::sql::Result &indexes) {
		for (auto it : indexes) {
			auto tname = it.at(0).str<mem::Interface>();
			auto f = ret.find(tname);
			if (f != ret.end()) {
				auto &table = f->second;
				auto name = it.at(1);
				name.readUntilString("_idx_");
				if (name.is("_idx_")) {
					table.indexes.emplace(it.at(1).str<mem::Interface>(), it.at(2).str<mem::Interface>());
					stream << "INDEX " << it.at(0) << " " << it.at(1) << " " << it.at(2) << "\n";
				}
			}
		}
		indexes.clear();
	});

	h.performSimpleSelect("SELECT event_object_table, trigger_name FROM information_schema.triggers "
			"WHERE trigger_schema='public';"_weak,
			[&] (db::sql::Result &triggers) {
		for (auto it : triggers) {
			auto tname = it.at(0).str<mem::Interface>();
			auto f = ret.find(tname);
			if (f != ret.end()) {
				auto &table = f->second;
				table.triggers.emplace(it.at(1).str<mem::Interface>());
				stream << "TRIGGER " << it.at(0) << " " << it.at(1) << "\n";
			}
		}
		triggers.clear();
	});
	return ret;
}

TableRec::TableRec() : objects(false) { }
TableRec::TableRec(const db::Interface::Config &cfg, const db::Scheme *scheme) {
	mem::StringStream hashStreamAfter; hashStreamAfter << getDefaultFunctionVersion();
	mem::StringStream hashStreamBefore; hashStreamBefore << getDefaultFunctionVersion();

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

		if (type == db::Type::File || type == db::Type::Image) {
			hasAfterTrigger = true;
			hashStreamAfter << it.first << stappler::toInt(type);
		}

		switch (type) {
		case db::Type::None:
		case db::Type::Array:
		case db::Type::View:
			break;

		case db::Type::Float:
			cols.emplace(it.first, ColRec(ColRec::Type::Float, f.hasFlag(db::Flags::Required)));
			emplaced = true;
			break;

		case db::Type::Boolean:
			cols.emplace(it.first, ColRec(ColRec::Type::Boolean, f.hasFlag(db::Flags::Required)));
			emplaced = true;
			break;

		case db::Type::Text:
			cols.emplace(it.first, ColRec(ColRec::Type::Text, f.hasFlag(db::Flags::Required)));
			emplaced = true;
			break;

		case db::Type::Data:
		case db::Type::Bytes:
		case db::Type::Extra:
			cols.emplace(it.first, ColRec(ColRec::Type::Binary, f.hasFlag(db::Flags::Required)));
			emplaced = true;
			break;

		case db::Type::Integer:
		case db::Type::File:
		case db::Type::Image:
			cols.emplace(it.first, ColRec(ColRec::Type::Integer, f.hasFlag(db::Flags::Required)));
			emplaced = true;
			break;

		case db::Type::FullTextView:
			cols.emplace(it.first, ColRec(ColRec::Type::TsVector, f.hasFlag(db::Flags::Required)));
			emplaced = true;
			break;

		case db::Type::Object:
			cols.emplace(it.first, ColRec(ColRec::Type::Integer, f.hasFlag(db::Flags::Required)));
			if (f.isReference()) {
				auto objSlot = static_cast<const db::FieldObject *>(f.getSlot());
				if (objSlot->onRemove == db::RemovePolicy::StrongReference) {
					hasAfterTrigger = true;
					hashStreamAfter << it.first << stappler::toInt(type);
				}
			}
			emplaced = true;
			break;

		case db::Type::Set:
			if (f.isReference()) {
				auto objSlot = static_cast<const db::FieldObject *>(f.getSlot());
				if (objSlot->onRemove == db::RemovePolicy::StrongReference) {
					hasBeforeTrigger = true;
					hashStreamBefore << it.first << stappler::toInt(type);
				}
			}
			break;

		case db::Type::Custom:
			auto objSlot = f.getSlot<db::FieldCustom>();
			cols.emplace(it.first, ColRec(objSlot->getTypeName(), f.hasFlag(db::Flags::Required)));
			emplaced = true;
			break;
		}

		if (emplaced) {
			if (type == db::Type::Object) {
				auto ref = static_cast<const db::FieldObject *>(f.getSlot());
				auto target = ref->scheme->getName();
				auto cname = mem::toString(name, "_ref_", it.first, "_", target);
				constraints.emplace(cname, ConstraintRec(ConstraintRec::Reference, it.first, target.str<mem::Interface>(), ref->onRemove));
				indexes.emplace(mem::toString(name, "_idx_", it.first), mem::toString("( \"", it.first, "\" )"));
			} else if (type == db::Type::File || type == db::Type::Image) {
				auto ref = cfg.fileScheme;
				auto cname = mem::toString(name, "_ref_", it.first);
				auto target = ref->getName();
				constraints.emplace(cname, ConstraintRec(ConstraintRec::Reference, it.first, target.str<mem::Interface>(), db::RemovePolicy::Null));
			}

			if ((type == db::Type::Text && f.getTransform() == db::Transform::Alias) ||
					f.hasFlag(db::Flags::Unique)) {
				constraints.emplace(mem::toString(name, "_unique_", it.first), ConstraintRec(ConstraintRec::Unique, it.first));
			}

			if ((type == db::Type::Text && f.getTransform() == db::Transform::Alias)
					|| (f.hasFlag(db::Flags::Indexed) && !f.hasFlag(db::Flags::Unique))) {
				if (type != db::Type::Custom) {
					indexes.emplace(mem::toString(name, "_idx_", it.first), mem::toString("( \"", it.first, "\" )"));
				} else {
					auto c = f.getSlot<db::FieldCustom>();
					indexes.emplace(mem::toString(name, "_idx_", c->getIndexName()), c->getIndexField());
				}
			}
		}
	}

	if (scheme->isDetouched()) {
		cols.emplace("__oid", ColRec(ColRec::Type::Serial, true));
		objects = false;
	}

	if (hasAfterTrigger) {
		size_t id = std::hash<mem::String>{}(hashStreamAfter.weak());

		hashStreamAfter.clear();
		hashStreamAfter << "_tr_a_" << scheme->getName() << "_" << id;
		triggers.emplace(mem::StringView(hashStreamAfter.weak()).sub(0, 56).str<mem::Interface>());
	}

	if (hasBeforeTrigger) {
		size_t id = std::hash<mem::String>{}(hashStreamBefore.weak());

		hashStreamBefore.clear();
		hashStreamBefore << "_tr_b_" << scheme->getName() << "_" << id;
		triggers.emplace(mem::StringView(hashStreamBefore.weak()).sub(0, 56).str<mem::Interface>());
	}
}

bool Handle::init(const Interface::Config &cfg, const mem::Map<mem::String, const Scheme *> &s) {
	if (!performSimpleQuery(mem::String::make_weak(DATABASE_DEFAULTS))) {
		return false;
	}

	if (!performSimpleQuery("START TRANSACTION; LOCK TABLE __objects;"_weak)) {
		return false;
	}

	mem::StringStream tables;
	tables << "Server: " << cfg.name << "\n";

	auto requiredTables = TableRec::parse(cfg, s);
	auto existedTables = TableRec::get(*this, tables);

	mem::StringStream stream;
	TableRec::writeCompareResult(stream, requiredTables, existedTables, s);

	auto name = mem::toString(".serenity/update.", stappler::Time::now().toMilliseconds(), ".sql");
	if (stream.size() > 3) {
		if (performSimpleQuery(stream.weak())) {
			performSimpleQuery("COMMIT;"_weak);
		} else {
			stappler::log::format("Database", "Fail to perform update %s", name.c_str());
			stream << "\nError: " << driver->getStatusMessage(lastError) << "\n";
			performSimpleQuery("ROLLBACK;"_weak);
		}

		tables << "\n" << stream;
		stappler::filesystem::write(name, (const uint8_t *)tables.data(), tables.size());
	} else {
		performSimpleQuery("COMMIT;"_weak);
	}

	beginTransaction_pg(TransactionLevel::ReadCommited);
	mem::StringStream query;
	query << "DELETE FROM __login WHERE \"date\" < " << (stappler::Time::now() - config::getInternalsStorageTime()).toSeconds() << ";";
	performSimpleQuery(query.weak());
	query.clear();

	query << "DELETE FROM __error WHERE \"time\" < " << (stappler::Time::now() - config::getInternalsStorageTime()).toMicros() << ";";
	performSimpleQuery(query.weak());
	query.clear();
	endTransaction_pg();

	return true;
}

NS_DB_PQ_END
