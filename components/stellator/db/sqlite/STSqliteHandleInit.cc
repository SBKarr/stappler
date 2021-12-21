/**
 Copyright (c) 2021 Roman Katuntsev <sbkarr@stappler.org>

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

#include "STSqliteHandle.h"

NS_DB_SQLITE_BEGIN

constexpr static uint32_t getDefaultFunctionVersion() { return 10; }

constexpr static const char * DATABASE_DEFAULTS = R"Sql(
CREATE TABLE IF NOT EXISTS "__removed" (
	__oid BIGINT NOT NULL PRIMARY KEY
) WITHOUT ROWID;

CREATE TABLE IF NOT EXISTS "__sessions" (
	name BLOB NOT NULL PRIMARY KEY,
	mtime BIGINT NOT NULL,
	maxage BIGINT NOT NULL,
	data BLOB
) WITHOUT ROWID;

CREATE TABLE IF NOT EXISTS "__broadcasts" (
	id INTEGER PRIMARY KEY AUTOINCREMENT,
	date BIGINT NOT NULL,
	msg BLOB
);

CREATE TABLE IF NOT EXISTS "__login" (
	id INTEGER PRIMARY KEY AUTOINCREMENT,
	"user" BIGINT NOT NULL,
	name TEXT NOT NULL,
	password BLOB NOT NULL,
	date BIGINT NOT NULL,
	success BOOLEAN NOT NULL,
	addr TEXT,
	host TEXT,
	path TEXT
);

CREATE INDEX IF NOT EXISTS "__broadcasts_idx_date" ON "__broadcasts" ("date");
CREATE INDEX IF NOT EXISTS "__login_idx_user" ON "__login" ("user");
CREATE INDEX IF NOT EXISTS "__login_idx_date" ON "__login" ("date");
)Sql";

struct ColRec {
	using Type = Interface::StorageType;

	enum Flags {
		None,
		IsNotNull,
		PrimaryKey
	};

	Type type = Type::Unknown;
	mem::String custom;
	Flags flags = Flags::None;

	bool isNotNull() const;

	ColRec(Type t, Flags flags = Flags::None) : type(t), flags(flags) { }
	ColRec(const mem::StringView &t, Flags flags = Flags::None) : custom(t.str<mem::Interface>()), flags(flags) { }
};

SP_DEFINE_ENUM_AS_MASK(ColRec::Flags)

struct IndexRec {
	mem::Vector<mem::String> fields;
	bool unique = false;

	IndexRec(mem::StringView str, bool unique = false) : fields(mem::Vector<mem::String>({str.str<mem::Interface>()})), unique(unique) { }
	IndexRec(mem::Vector<mem::String> &&fields, bool unique = false) : fields(std::move(fields)), unique(unique) { }
};

struct TriggerRec {
	enum Type {
		Delete,
		Update,
		Insert
	};

	enum Bind {
		Before,
		After
	};

	Type type = Type::Delete;
	Bind bind = Bind::Before;
	const Field *rootField = nullptr;
	const Scheme *rootScheme = nullptr;
	mem::String sourceTable;
	mem::String sourceField;
	mem::String targetTable;
	mem::String targetField;
	mem::String tagField;
	RemovePolicy onRemove = RemovePolicy::Null;

	TriggerRec(mem::StringView def) {
		uint32_t valueIdx = 0;
		while (!def.empty()) {
			auto value = def.readUntil<mem::StringView::Chars<':'>>();
			switch (valueIdx) {
			case 0:
				if (value == "BEFORE") {
					bind = Before;
				} else if (value == "AFTER") {
					bind = After;
				} else {
					return;
				}
				break;
			case 1:
				if (value == "DELETE") {
					type = Delete;
				} else if (value == "UPDATE") {
					type = Update;
				} else if (value == "INSERT") {
					type = Insert;
				} else {
					return;
				}
				break;
			case 2: {
				auto table = value.readUntil<mem::StringView::Chars<'@'>>();
				if (value.is('@')) {
					++ value;
					sourceTable = table.str<mem::Interface>();
					sourceField = value.str<mem::Interface>();
				} else {
					return;
				}
				break;
			}
			case 3: {
				auto table = value.readUntil<mem::StringView::Chars<'@'>>();
				if (value.is('@')) {
					++ value;
					targetTable = table.str<mem::Interface>();
					targetField = value.str<mem::Interface>();
				} else {
					return;
				}
				break;
			}
			case 4:
				if (value == "CASCADE") {
					onRemove = RemovePolicy::Cascade;
				} else if (value == "RESTRICT") {
					onRemove = RemovePolicy::Restrict;
				} else if (value == "REF") {
					onRemove = RemovePolicy::Reference;
				} else if (value == "SREF") {
					onRemove = RemovePolicy::StrongReference;
				} else {
					return;
				}
				break;
			default:
				break;
			}
			if (def.is(':')) {
				++ def;
				++ valueIdx;
			}
		}
	}

	TriggerRec(Type t, Bind b, mem::StringView sourceTable, mem::StringView sourceField,
			mem::StringView targetTable, mem::StringView targetField, const Field *f = nullptr)
	: type(t), bind(b), rootField(f)
	, sourceTable(sourceTable.str<mem::Interface>())
	, sourceField(sourceField.str<mem::Interface>())
	, targetTable(targetTable.str<mem::Interface>())
	, targetField(targetField.str<mem::Interface>()) { }

	mem::String makeName() const {
		mem::StringStream stream;
		switch (bind) {
		case Bind::Before: stream << "ST_TRIGGER:BEFORE:"; break;
		case Bind::After: stream << "ST_TRIGGER:AFTER:"; break;
		}
		switch (type) {
		case Type::Delete: stream << "DELETE:"; break;
		case Type::Update: stream << "UPDATE:"; break;
		case Type::Insert: stream << "INSERT:"; break;
		}
		stream << sourceTable << "@" << sourceField << ":";
		stream << targetTable << "@" << targetField;
		switch (onRemove) {
		case RemovePolicy::Null: break;
		case RemovePolicy::Cascade: stream << ":CASCADE"; break;
		case RemovePolicy::Restrict: stream << ":RESTRICT"; break;
		case RemovePolicy::Reference: stream << ":REF"; break;
		case RemovePolicy::StrongReference: stream << ":SREF"; break;
		}
		return stream.str();
	}
};

struct TableRec {
	using Scheme = db::Scheme;

	static mem::Map<mem::StringView, TableRec> parse(const db::Interface::Config &cfg,
			const mem::Map<mem::StringView, const Scheme *> &s);
	static mem::Map<mem::StringView, TableRec> get(Handle &h, mem::StringStream &stream);

	static void writeCompareResult(Handle &h, mem::StringStream &stream,
			mem::Map<mem::StringView, TableRec> &required, mem::Map<mem::StringView, TableRec> &existed,
			const mem::Map<mem::StringView, const db::Scheme *> &s);

	TableRec();
	TableRec(const db::Interface::Config &cfg, const db::Scheme *scheme);

	mem::Map<mem::String, ColRec> cols;
	mem::Map<mem::String, IndexRec> indexes;
	mem::Map<mem::String, TriggerRec> triggers;
	bool exists = false;
	bool valid = false;
	bool withOids = false;
	bool detached = false;

	const db::Scheme *viewScheme = nullptr;
	const db::FieldView *viewField = nullptr;
};

static Interface::StorageType getStorageType(mem::StringView type) {
	if (type == "BIGINT" || type == "INTEGER" || type == "INT") {
		return Interface::StorageType::Int8;
	} else if (type == "NUMERIC") {
		return Interface::StorageType::Numeric;
	} else if (type == "BOOLEAN") {
		return Interface::StorageType::Bool;
	} else if (type == "BLOB") {
		return Interface::StorageType::Bytes;
	} else if (type == "TEXT") {
		return Interface::StorageType::Text;
	} else if (type == "REAL" || type == "DOUBLE") {
		return Interface::StorageType::Float8;
	}
	return Interface::StorageType::Unknown;
}

static mem::StringView getStorageTypeName(Interface::StorageType type, mem::StringView custom) {
	switch (type) {
	case ColRec::Type::Unknown: return custom; break;
	case ColRec::Type::Bool:	return "BOOLEAN"; break;
	case ColRec::Type::Float4:	return "DOUBLE"; break;
	case ColRec::Type::Float8:	return "DOUBLE"; break;
	case ColRec::Type::Int2:	return "BIGINT"; break;
	case ColRec::Type::Int4:	return "BIGINT"; break;
	case ColRec::Type::Int8:	return "BIGINT"; break;
	case ColRec::Type::Text: 	return "TEXT"; break;
	case ColRec::Type::VarChar: return "TEXT"; break;
	case ColRec::Type::Numeric: return "NUMERIC"; break;
	case ColRec::Type::Bytes:	return "BLOB"; break;
	default: break;
	}
	return mem::StringView();
}

bool ColRec::isNotNull() const { return (flags & Flags::IsNotNull) != Flags::None; }

void TableRec::writeCompareResult(Handle &h, mem::StringStream &outstream,
		mem::Map<mem::StringView, TableRec> &required, mem::Map<mem::StringView, TableRec> &existed,
		const mem::Map<mem::StringView, const db::Scheme *> &s) {

	auto writeTriggerHeader = [&] (mem::StringView name, TriggerRec &t, mem::StringView updateField) {
		outstream << "CREATE TRIGGER IF NOT EXISTS \"" << name << "\"";
		switch (t.bind) {
		case TriggerRec::Before: outstream << " BEFORE"; break;
		case TriggerRec::After: outstream << " AFTER"; break;
		}
		switch (t.type) {
		case TriggerRec::Delete: outstream << " DELETE"; break;
		case TriggerRec::Update: outstream << " UPDATE";
			if (!updateField.empty()) {
				outstream << " OF \"" << updateField << "\"";
			}
			break;
		case TriggerRec::Insert: outstream << " INSERT"; break;
		}
		outstream << " ON \"" << t.sourceTable << "\" FOR EACH ROW";
	};

	auto writeTrigger = [&] (mem::StringView name, TriggerRec &t) {
		if (t.rootField) {
			switch (t.rootField->getType()) {
			case Type::Array:
				writeTriggerHeader(name, t,mem::StringView());
				outstream << " BEGIN\n\tDELETE FROM \"" << t.targetTable << "\""
						" WHERE \"" << t.targetTable << "\".\"" << t.targetField << "\"=OLD.__oid;\nEND;\n";
				break;
			case Type::File:
			case Type::Image:
				writeTriggerHeader(name, t,mem::StringView());
				switch (t.type) {
				case TriggerRec::Delete:
					outstream << " WHEN OLD.\"" << t.sourceField << "\" IS NOT NULL BEGIN"
							"\n\tINSERT INTO __removed (__oid) VALUES (OLD.\"" << t.sourceField << "\");\nEND;\n";
					break;
				case TriggerRec::Update:
					outstream << " WHEN OLD.\"" << t.sourceField << "\" IS NOT NULL BEGIN"
							"\n\tINSERT INTO __removed (__oid) VALUES (OLD.\"" << t.sourceField << "\");\nEND;\n";
					break;
				default:
					break;
				}
				break;
			case Type::Set:
				switch (static_cast<const db::FieldObject *>(t.rootField->getSlot())->onRemove) {
				case RemovePolicy::Reference:
				case RemovePolicy::StrongReference:
					writeTriggerHeader(name, t,mem::StringView());
					outstream << " BEGIN\n\tDELETE FROM \"" << t.targetTable << "\" WHERE \""
							<< t.targetTable << "\".\"" << t.targetField << "\"=OLD.\"" << t.sourceField << "\";\nEND;\n";
					break;
				default: break;
				}
				break;
			case Type::View:
				writeTriggerHeader(name, t,mem::StringView());
				outstream << " BEGIN\n\tDELETE FROM \"" << t.targetTable << "\" WHERE \""
						<< t.targetTable << "\".\"" << t.targetField << "\"=OLD.\"" << t.sourceField << "\";\nEND;\n";
				break;
			case Type::Object:
				switch (t.onRemove) {
				case RemovePolicy::Cascade:
					writeTriggerHeader(name, t,mem::StringView());
					outstream << " BEGIN\n\tDELETE FROM \"" << t.targetTable << "\" WHERE \""
							<< t.targetTable << "\".\"" << t.targetField << "\"=OLD.\"" << t.sourceField << "\";\nEND;\n";
					break;
				case RemovePolicy::Restrict:
					writeTriggerHeader(name, t,mem::StringView());
					outstream << " BEGIN\n\tSELECT RAISE(ABORT, 'Restrict constraint failed on "
							<< t.targetTable << "." << t.targetField << "' FROM \""<< t.targetTable << "\" WHERE \""
							<< t.targetTable << "\".\"" << t.targetField << "\"=OLD.\"" << t.sourceField << "\";\nEND;\n";
					break;
				case RemovePolicy::Null:
				case RemovePolicy::Reference:
					writeTriggerHeader(name, t,mem::StringView());
					outstream << " BEGIN\n\tUPDATE \"" << t.targetTable << "\" SET \"" << t.targetField << "\"=NULL WHERE \""
							<< t.targetTable << "\".\"" << t.targetField << "\"=OLD.\"" << t.sourceField << "\";\nEND;\n";
					break;
				case RemovePolicy::StrongReference:
					// Reverse trigger
					switch (t.type) {
					case TriggerRec::Delete:
						writeTriggerHeader(name, t,mem::StringView());
						outstream << " BEGIN\n\tDELETE FROM \"" << t.targetTable << "\" WHERE \""
								<< t.targetTable << "\".\"" << t.targetField << "\"=OLD.\"" << t.sourceField << "\";\nEND;\n";
						break;
					case TriggerRec::Update:
						writeTriggerHeader(name, t, t.sourceField);
						outstream << " WHEN OLD.\"" << t.sourceField << "\" IS NOT NULL BEGIN"
								"\n\tDELETE FROM \"" << t.targetTable << "\" WHERE \""
								<< t.targetTable << "\".\"" << t.targetField << "\"=OLD.\"" << t.sourceField << "\";\nEND;\n";
						break;
					default:
						break;
					}
					break;
				}
				break;
			default:
				break;
			}
		} else if (t.rootScheme && t.rootScheme->hasDelta()) {
			switch (t.type) {
			case TriggerRec::Delete:
				writeTriggerHeader(name, t,mem::StringView());
				outstream << " BEGIN"
						"\n\tINSERT INTO " << t.targetTable << "(\"object\",\"action\",\"time\",\"user\") VALUES"
						"(OLD.__oid," << stappler::toInt(DeltaAction::Delete) << ",stellator_now(),stellator_user());"
						"\nEND;\n";
				break;
			case TriggerRec::Update:
				writeTriggerHeader(name, t,mem::StringView());
				outstream << " BEGIN"
						"\n\tINSERT INTO " << t.targetTable << "(\"object\",\"action\",\"time\",\"user\") VALUES"
						"(NEW.__oid," << stappler::toInt(DeltaAction::Update) << ",stellator_now(),stellator_user());"
						"\nEND;\n";
				break;
			case TriggerRec::Insert:
				writeTriggerHeader(name, t,mem::StringView());
				outstream << " BEGIN"
						"\n\tINSERT INTO " << t.targetTable << "(\"object\",\"action\",\"time\",\"user\") VALUES"
						"(OLD.__oid," << stappler::toInt(DeltaAction::Create) << ",stellator_now(),stellator_user());"
						"\nEND;\n";
				break;
			}
		} else if (t.sourceField == "__delta") {
			switch (t.type) {
			case TriggerRec::Delete:
				writeTriggerHeader(name, t,mem::StringView());
				outstream << " BEGIN"
						"\n\tINSERT INTO " << t.targetTable << "(\"tag\",\"object\",\"time\",\"user\") VALUES"
						"(OLD.\"" << t.tagField << "\",OLD.\"" << t.targetField  << "\",stellator_now(),stellator_user());"
						"\nEND;\n";
				break;
			case TriggerRec::Update:
				writeTriggerHeader(name, t,mem::StringView());
				outstream << " BEGIN"
						"\n\tINSERT INTO " << t.targetTable << "(\"tag\",\"object\",\"time\",\"user\") VALUES"
						"(NEW.\"" << t.tagField << "\",NEW.\"" << t.targetField  << "\",stellator_now(),stellator_user());"
						"\nEND;\n";
				break;
			case TriggerRec::Insert:
				writeTriggerHeader(name, t,mem::StringView());
				outstream << " BEGIN"
						"\n\tINSERT INTO " << t.targetTable << "(\"tag\",\"object\",\"time\",\"user\") VALUES"
						"(NEW.\"" << t.tagField << "\",NEW.\"" << t.targetField  << "\",stellator_now(),stellator_user());"
						"\nEND;\n";
				break;
			}
		}
	};

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
					outstream << "DROP INDEX IF EXISTS \"" << ex_idx_it.first << "\";\n";
				} else {
					req_t.indexes.erase(req_idx_it);
				}
			}

			for (auto &ex_col_it : ex_t.cols) {
				if (ex_col_it.first == "__oid") {
					continue;
				}

				auto req_col_it = req_t.cols.find(ex_col_it.first);
				if (req_col_it == req_t.cols.end()) {
					outstream << "ALTER TABLE \"" << ex_it.first << "\" DROP COLUMN \"" << ex_col_it.first << "\";\n";
				} else {
					auto &req_col = req_col_it->second;
					auto &ex_col = ex_col_it.second;

					auto req_type = req_col.type;

					if (req_type != ex_col.type) {
						outstream << "ALTER TABLE \"" << ex_it.first << "\" DROP COLUMN IF EXISTS \"" << ex_col_it.first << "\";\n";
					} else if (ex_col.type == ColRec::Type::Unknown && req_type == ColRec::Type::Unknown && ex_col.custom != req_col.custom) {
						outstream << "ALTER TABLE \"" << ex_it.first << "\" DROP COLUMN IF EXISTS \"" << ex_col_it.first << "\";\n";
					} else {
						req_t.cols.erase(req_col_it);
					}
				}
			}

			for (auto &ex_tgr_it : ex_t.triggers) {
				auto req_tgr_it = req_t.triggers.find(ex_tgr_it.first);
				if (req_tgr_it == req_t.triggers.end()) {
					outstream << "DROP TRIGGER IF EXISTS \"" << ex_tgr_it.first << "\";\n";
				} else {
					req_t.triggers.erase(ex_tgr_it.first);
				}
			}
		}
	}

	// write table structs
	for (auto &it : required) {
		auto &t = it.second;
		if (!it.second.exists) {
			outstream << "CREATE TABLE IF NOT EXISTS \"" << it.first << "\" (\n";

			bool first = true;
			if (it.second.withOids) {
				first = false;
				if (it.second.detached) {
					outstream << "\t\"__oid\" INTEGER PRIMARY KEY AUTOINCREMENT";
				} else {
					outstream << "\t\"__oid\" INTEGER DEFAULT (stellator_next_oid())";
				}
			}

			mem::StringView pkey;

			for (auto cit = t.cols.begin(); cit != t.cols.end(); cit ++) {
				if (first) { first = false; } else { outstream << ",\n"; }
				outstream << "\t\"" << cit->first << "\" "
						<< getStorageTypeName(cit->second.type, cit->second.custom);

				if ((cit->second.flags & ColRec::IsNotNull) != ColRec::None) {
					outstream << " NOT NULL";
				}

				if (!it.second.withOids && (cit->second.flags & ColRec::PrimaryKey) != ColRec::Flags::None) {
					outstream << " PRIMARY KEY";
				}
			}

			outstream << "\n);\n";
		} else {
			for (auto cit : t.cols) {
				if (cit.first != "__oid") {
					outstream << "ALTER TABLE \"" << it.first << "\" ADD COLUMN \"" << cit.first << "\" "
							<< getStorageTypeName(cit.second.type, cit.second.custom);
					if ((cit.second.flags & ColRec::Flags::IsNotNull) != ColRec::Flags::None) {
						outstream << " NOT NULL";
					}
					outstream << ";\n";
				}
			}
		}
	}

	// indexes
	for (auto &it : required) {
		for (auto & cit : it.second.indexes) {
			outstream << "CREATE";
			if (cit.second.unique) {
				outstream << " UNIQUE";
			}
			outstream << " INDEX IF NOT EXISTS \"" << cit.first << "\" ON \"" << it.first << "\"";
			if (cit.second.fields.size() == 0 && cit.second.fields.front().back() == ')') {
				outstream <<  " " << cit.second.fields.front() << ";\n";
			} else {
				outstream << " (";
				bool first = true;
				for (auto &field : cit.second.fields) {
					if (first) { first = false; } else { outstream << ","; }
					outstream << "\"" << field << "\"";
				}
				outstream << ");\n";
			}
		}

		if (!it.second.triggers.empty()) {
			for (auto & tit : it.second.triggers) {
				writeTrigger(tit.first, tit.second);
			}

			/*auto scheme_it = s.find(it.first);
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
			}*/
		}
	}
}

mem::Map<mem::StringView, TableRec> TableRec::parse(const db::Interface::Config &cfg,
		const mem::Map<mem::StringView, const db::Scheme *> &s) {
	mem::Map<mem::StringView, TableRec> tables;
	for (auto &it : s) {
		auto scheme = it.second;
		tables.emplace(scheme->getName(), TableRec(cfg, scheme)).first->second;
	}

	for (auto &it : s) {
		auto scheme = it.second;
		auto tableIt = tables.find(scheme->getName());
		if (tableIt == tables.end()) {
			continue;
		}
		auto schemeTable = &tableIt->second;

		// check for extra tables
		for (auto &fit : scheme->getFields()) {
			auto &f = fit.second;
			auto type = fit.second.getType();

			if (type == db::Type::Set) {
				auto ref = static_cast<const db::FieldObject *>(f.getSlot());
				if (ref->onRemove == db::RemovePolicy::Reference || ref->onRemove == db::RemovePolicy::StrongReference) {
					// create many-to-many table link
					mem::String name = mem::toString(it.first, "_f_", fit.first);
					auto & source = it.first;
					auto target = ref->scheme->getName();
					TableRec table;
					table.cols.emplace(mem::toString(source, "_id"), ColRec(ColRec::Type::Int8, ColRec::IsNotNull));
					table.cols.emplace(mem::toString(target, "_id"), ColRec(ColRec::Type::Int8, ColRec::IsNotNull));

					/*table.constraints.emplace(mem::toString(name, "_ref_", source), ConstraintRec(
							ConstraintRec::Reference, mem::toString(source, "_id"), source, db::RemovePolicy::Cascade));
					table.constraints.emplace(name + "_ref_" + ref->getName(), ConstraintRec(
							ConstraintRec::Reference, mem::toString(target, "_id"), target.str<mem::Interface>(), db::RemovePolicy::Cascade));*/

					table.indexes.emplace(mem::toString(name, "_idx_", source), mem::toString(source, "_id"));
					table.indexes.emplace(mem::toString(name, "_idx_", target), mem::toString(target, "_id"));

					auto extraTable = &tables.emplace(mem::StringView(name).pdup(), std::move(table)).first->second;

					do {
						TriggerRec trigger(TriggerRec::Delete, TriggerRec::Before, source, "__oid",
								name, mem::toString(source, "_id"));
						trigger.rootField = &fit.second;
						auto triggerName = trigger.makeName();
						schemeTable->triggers.emplace(std::move(triggerName), std::move(trigger));
					} while (0);

					do {
						auto targetIt = tables.find(target);
						if (targetIt != tables.end()) {
							TriggerRec trigger(TriggerRec::Delete, TriggerRec::After, target, "__oid",
									name, mem::toString(target, "_id"));
							trigger.rootField = &fit.second;
							trigger.rootScheme = scheme;
							auto triggerName = trigger.makeName();
							targetIt->second.triggers.emplace(std::move(triggerName), std::move(trigger));
						}

						if (ref->onRemove == db::RemovePolicy::StrongReference && targetIt != tables.end()) {
							TriggerRec trigger(TriggerRec::Delete, TriggerRec::Before, name, mem::toString(target, "_id"),
									target, "__oid");
							trigger.rootField = &fit.second;
							trigger.rootScheme = scheme;
							auto triggerName = trigger.makeName();
							extraTable->triggers.emplace(std::move(triggerName), std::move(trigger));
						}
					} while (0);
				}
			} else if (type == db::Type::Object) {
				auto ref = static_cast<const db::FieldObject *>(f.getSlot());
				auto targetIt = tables.find(ref->scheme->getName());
				if (targetIt != tables.end()) {
					TriggerRec trigger(TriggerRec::Delete, TriggerRec::Before, ref->scheme->getName(), "__oid",
							scheme->getName(), fit.second.getName());
					trigger.rootField = &fit.second;
					trigger.rootScheme = scheme;
					trigger.onRemove = ref->onRemove;
					if (ref->onRemove == RemovePolicy::StrongReference) {
						trigger.onRemove = RemovePolicy::Reference; // make trigger to remove just reference
					}

					auto triggerName = trigger.makeName();
					targetIt->second.triggers.emplace(std::move(triggerName), std::move(trigger));

					if (ref->onRemove == RemovePolicy::StrongReference) {
						// make reverse-trigger to remove object with strong reference
						do {
							TriggerRec trigger(TriggerRec::Delete, TriggerRec::Before, scheme->getName(), fit.second.getName(),
									ref->scheme->getName(), "__oid");
							trigger.rootField = &fit.second;
							trigger.rootScheme = scheme;
							trigger.onRemove = ref->onRemove;

							auto triggerName = trigger.makeName();
							schemeTable->triggers.emplace(std::move(triggerName), std::move(trigger));
						} while (0);

						do {
							TriggerRec trigger(TriggerRec::Update, TriggerRec::Before, scheme->getName(), fit.second.getName(),
									ref->scheme->getName(), "__oid");
							trigger.rootField = &fit.second;
							trigger.rootScheme = scheme;
							trigger.onRemove = ref->onRemove;

							auto triggerName = trigger.makeName();
							schemeTable->triggers.emplace(std::move(triggerName), std::move(trigger));
						} while (0);
					}
				}
			} else if (type == db::Type::Array) {
				auto slot = static_cast<const db::FieldArray *>(f.getSlot());
				if (slot->tfield && slot->tfield.isSimpleLayout()) {

					mem::String name = mem::toString(it.first, "_f_", fit.first);
					auto & source = it.first;

					auto sourceFieldName = mem::toString(source, "_id");

					TableRec table;
					table.cols.emplace(sourceFieldName, ColRec(ColRec::Type::Int8));

					auto type = slot->tfield.getType();
					switch (type) {
					case db::Type::Float: table.cols.emplace("data", ColRec(ColRec::Type::Float8)); break;
					case db::Type::Boolean: table.cols.emplace("data", ColRec(ColRec::Type::Bool)); break;
					case db::Type::Text: table.cols.emplace("data", ColRec(ColRec::Type::Text)); break;
					case db::Type::Integer: table.cols.emplace("data", ColRec(ColRec::Type::Int8)); break;
					case db::Type::Data:
					case db::Type::Bytes:
					case db::Type::Extra:
						table.cols.emplace("data", ColRec(ColRec::Type::Bytes));
						break;
					default:
						break;
					}

					table.indexes.emplace(mem::toString(name, "_idx_", source), mem::toString(source, "_id"));
					if (f.hasFlag(db::Flags::Unique)) {
						table.indexes.emplace(mem::toString(name, "_uidx_data"), IndexRec(mem::toString("data"), true));
					}

					tables.emplace(mem::StringView(name).pdup(), std::move(table));

					TriggerRec trigger(TriggerRec::Delete, TriggerRec::Before, scheme->getName(), f.getName(), name, sourceFieldName);
					trigger.rootField = &f;
					auto triggerName = trigger.makeName();

					schemeTable->triggers.emplace(std::move(triggerName), std::move(trigger));
				}
			} else if (type == db::Type::View) {
				auto slot = static_cast<const db::FieldView *>(f.getSlot());

				mem::String name = mem::toString(it.first, "_f_", fit.first, "_view");
				auto & source = it.first;
				auto target = slot->scheme->getName();

				TableRec table;
				table.viewScheme = it.second;
				table.viewField = slot;
				table.cols.emplace(mem::toString(source, "_id"), ColRec(ColRec::Type::Int8, ColRec::IsNotNull));
				table.cols.emplace(mem::toString(target, "_id"), ColRec(ColRec::Type::Int8, ColRec::IsNotNull));

				do {
					TriggerRec trigger(TriggerRec::Delete, TriggerRec::Before, source, "__oid",
							name, mem::toString(source, "_id"));
					trigger.rootField = &fit.second;
					auto triggerName = trigger.makeName();
					schemeTable->triggers.emplace(std::move(triggerName), std::move(trigger));
				} while (0);

				do {
					auto targetIt = tables.find(target);
					if (targetIt != tables.end()) {
						TriggerRec trigger(TriggerRec::Delete, TriggerRec::After, target, "__oid",
								name, mem::toString(target, "_id"));
						trigger.rootField = &fit.second;
						trigger.rootScheme = scheme;
						auto triggerName = trigger.makeName();
						targetIt->second.triggers.emplace(std::move(triggerName), std::move(trigger));
					}
				} while (0);

				table.indexes.emplace(mem::toString(name, "_idx_", source), mem::toString(source, "_id"));
				table.indexes.emplace(mem::toString(name, "_idx_", target), mem::toString(target, "_id"));

				auto tblIt = tables.emplace(mem::StringView(name).pdup(), std::move(table)).first;

				if (slot->delta) {
					mem::String deltaName = mem::toString(it.first, "_f_", fit.first, "_delta");
					table.cols.emplace("tag", ColRec(ColRec::Type::Int8, ColRec::IsNotNull));
					table.cols.emplace("object", ColRec(ColRec::Type::Int8, ColRec::IsNotNull));
					table.cols.emplace("time", ColRec(ColRec::Type::Int8, ColRec::IsNotNull));
					table.cols.emplace("user", ColRec(ColRec::Type::Int8));

					table.indexes.emplace(deltaName + "_idx_tag", "tag");
					table.indexes.emplace(deltaName + "_idx_object", "object");
					table.indexes.emplace(deltaName + "_idx_time", "time");

					do {
						TriggerRec trigger(TriggerRec::Insert, TriggerRec::After, name, "__delta", deltaName, mem::toString(target, "_id"));
						trigger.tagField = mem::toString(source, "_id");
						auto triggerName = trigger.makeName();
						tblIt->second.triggers.emplace(std::move(triggerName), std::move(trigger));
					} while (0);

					do {
						TriggerRec trigger(TriggerRec::Update, TriggerRec::After, name, "__delta", deltaName, mem::toString(target, "_id"));
						trigger.tagField = mem::toString(source, "_id");
						auto triggerName = trigger.makeName();
						tblIt->second.triggers.emplace(std::move(triggerName), std::move(trigger));
					} while (0);

					do {
						TriggerRec trigger(TriggerRec::Delete, TriggerRec::After, name, "__delta", deltaName, mem::toString(target, "_id"));
						trigger.tagField = mem::toString(source, "_id");
						auto triggerName = trigger.makeName();
						tblIt->second.triggers.emplace(std::move(triggerName), std::move(trigger));
					} while (0);

					tables.emplace(mem::StringView(name).pdup(), std::move(table));
				}
			}

			if (scheme->hasDelta()) {
				auto name = Handle::getNameForDelta(*scheme);
				TableRec table;
				table.cols.emplace("object", ColRec(ColRec::Type::Int8, ColRec::IsNotNull));
				table.cols.emplace("time", ColRec(ColRec::Type::Int8, ColRec::IsNotNull));
				table.cols.emplace("action", ColRec(ColRec::Type::Int8, ColRec::IsNotNull));
				table.cols.emplace("user", ColRec(ColRec::Type::Int8));

				table.indexes.emplace(name + "_idx_object", "object");
				table.indexes.emplace(name + "_idx_time", "time");
				tables.emplace(mem::StringView(name).pdup(), std::move(table));
			}
		}
	}
	return tables;
}

mem::Map<mem::StringView, TableRec> TableRec::get(Handle &h, mem::StringStream &stream) {
	mem::Map<mem::StringView, TableRec> ret;

	h.performSimpleSelect("SELECT name FROM sqlite_schema WHERE type='table';",
			[&] (db::sql::Result &tables) {
		for (auto it : tables) {
			ret.emplace(it.at(0).pdup(), TableRec());
			stream << "TABLE " << it.at(0) << "\n";
			// std::cout << "TABLE " << it.at(0) << "\n";
		}
	});

	for (auto &it : ret) {
		auto &table = it.second;
		h.performSimpleSelect(toString("PRAGMA table_info('", it.first, "');"),
					[&] (db::sql::Result &columns) {
			for (auto col : columns) {
				auto name = col.at(1);
				auto t = getStorageType(col.at(2));

				ColRec::Flags flags = ColRec::Flags::None;
				if (col.toBool(3)) {
					flags |= ColRec::Flags::IsNotNull;
				}
				if (col.toBool(5)) {
					flags |= ColRec::Flags::PrimaryKey;
				}

				if (t == Interface::StorageType::Unknown) {
					table.cols.emplace(name.str<mem::Interface>(), ColRec(col.at(2).str<mem::Interface>(), flags));
				} else {
					table.cols.emplace(name.str<mem::Interface>(), ColRec(t, flags));
				}
			}
		});
	}

	h.performSimpleSelect("SELECT tbl_name, name, sql FROM sqlite_schema WHERE type='index';",
			[&] (db::sql::Result &indexes) {
		for (auto it : indexes) {
			auto tname = it.at(0).str<mem::Interface>();
			auto f = ret.find(tname);
			if (f != ret.end()) {
				auto &table = f->second;
				auto name = it.at(1);
				auto sql = it.at(2);
				if (!name.starts_with("sqlite_autoindex_")) {
					bool unique = false;
					if (sql.starts_with("CREATE UNIQUE")) {
						unique = true;
					}

					auto stream = mem::toString("\"", it.at(1), "\" ON \"", tname, "\" ");
					sql.readUntilString(stream);
					sql += stream.size();
					sql.skipChars<mem::StringView::WhiteSpace>();
					if (sql.is("(")) {
						++ sql;
						mem::Vector<mem::String> fields;
						while (!sql.empty() && !sql.is(')')) {
							sql.skipUntil<mem::StringView::Chars<'"'>>();
							if (sql.is('"')) {
								++ sql;
								auto field = sql.readUntil<mem::StringView::Chars<'"'>>();
								if (sql.is('"')) {
									fields.emplace_back(field.str<mem::Interface>());
									++ sql;
								}
							}
						}
						table.indexes.emplace(it.at(1).str<mem::Interface>(), IndexRec(std::move(fields), unique));
					}
				}
			}
		}
	});

	h.performSimpleSelect("SELECT tbl_name, name, sql FROM sqlite_schema WHERE type='trigger';",
			[&] (db::sql::Result &triggers) {
		for (auto it : triggers) {
			auto tableName = it.at(0);
			auto f = ret.find(tableName);
			if (f != ret.end()) {
				auto triggerName = it.at(1);
				if (!triggerName.starts_with("ST_TRIGGER:")) {
					continue;
				}

				triggerName += "ST_TRIGGER:"_len;

				TriggerRec trigger(triggerName);
				f->second.triggers.emplace(it.at(1).str<mem::Interface>(), std::move(trigger));
			}
		}
	});

	return ret;
}

TableRec::TableRec() { }
TableRec::TableRec(const db::Interface::Config &cfg, const db::Scheme *scheme) {
	withOids = true;
	if (scheme->isDetouched()) {
		detached = true;
	}

	auto name = scheme->getName();

	for (auto &it : scheme->getFields()) {
		bool emplaced = false;
		auto &f = it.second;
		auto type = it.second.getType();

		ColRec::Flags flags = ColRec::None;
		if (f.hasFlag(db::Flags::Required)) {
			flags |= ColRec::IsNotNull;
		}

		switch (type) {
		case db::Type::None:
		case db::Type::Array:
		case db::Type::View:
		case db::Type::Virtual:
			break;

		case db::Type::Float:
			cols.emplace(it.first, ColRec(ColRec::Type::Float8, flags));
			emplaced = true;
			break;

		case db::Type::Boolean:
			cols.emplace(it.first, ColRec(ColRec::Type::Bool, flags));
			emplaced = true;
			break;

		case db::Type::Text:
			cols.emplace(it.first, ColRec(ColRec::Type::Text, flags));
			emplaced = true;
			break;

		case db::Type::Data:
		case db::Type::Bytes:
		case db::Type::Extra:
			cols.emplace(it.first, ColRec(ColRec::Type::Bytes, flags));
			emplaced = true;
			break;

		case db::Type::Integer:
		case db::Type::File:
		case db::Type::Image:
			cols.emplace(it.first, ColRec(ColRec::Type::Int8, flags));
			emplaced = true;
			break;

		case db::Type::FullTextView:
			cols.emplace(it.first, ColRec(ColRec::Type::TsVector, flags));
			emplaced = true;
			break;

		case db::Type::Object:
			cols.emplace(it.first, ColRec(ColRec::Type::Int8, flags));
			emplaced = true;
			break;

		case db::Type::Set:
			if (f.isReference()) {
				// set is filled with references
			} else {
				//
			}
			break;

		case db::Type::Custom:
			/*if (auto objSlot = f.getSlot<db::FieldCustom>()) {
				auto name = objSlot->getTypeName();
				int64_t oid = 0;
				for (auto &it : customs) {
					if (it.first == name) {
						oid = it.second;
						break;
					}
				}
				if (oid) {
					cols.emplace(it.first, ColRec(objSlot->getTypeName(), oid, f.hasFlag(db::Flags::Required)));
				} else {
					cols.emplace(it.first, ColRec(objSlot->getTypeName(), f.hasFlag(db::Flags::Required)));
				}
				emplaced = true;
			}*/
			break;
		}

		if (emplaced) {
			bool unique = f.hasFlag(db::Flags::Unique) || f.getTransform() == db::Transform::Alias;
			if (type == db::Type::Object) {
				auto ref = static_cast<const db::FieldObject *>(f.getSlot());
				auto target = ref->scheme->getName();
				mem::StringStream cname; cname << name << "_ref_" << it.first << "_" << target;

				switch (ref->onRemove) {
				case RemovePolicy::Cascade: cname << "_csc"; break;
				case RemovePolicy::Restrict: cname << "_rst"; break;
				case RemovePolicy::Reference: cname << "_ref"; break;
				case RemovePolicy::StrongReference: cname << "_sref"; break;
				case RemovePolicy::Null: break;
				}

				indexes.emplace(mem::toString(name, (unique ? "_uidx_" : "_idx_"), it.first), IndexRec(it.first, unique));
			} else if (type == db::Type::File || type == db::Type::Image) {
				TriggerRec updateTrigger(TriggerRec::Update, TriggerRec::After, name, it.second.getName(),
						cfg.fileScheme->getName(), "__oid", &it.second);
				auto updateTriggerName = updateTrigger.makeName();
				triggers.emplace(std::move(updateTriggerName), std::move(updateTrigger));

				TriggerRec removeTrigger(TriggerRec::Delete, TriggerRec::After, name, it.second.getName(),
						cfg.fileScheme->getName(), "__oid", &it.second);
				auto removeTriggerName = removeTrigger.makeName();
				triggers.emplace(std::move(removeTriggerName), std::move(removeTrigger));
			}

			if ((type == db::Type::Text && f.getTransform() == db::Transform::Alias) || (f.hasFlag(db::Flags::Indexed))) {
				indexes.emplace(mem::toString(name, (unique ? "_uidx_" : "_idx_"), it.first), IndexRec(it.first, unique));
			}

			/*if (type == db::Type::Text) {
				if (f.hasFlag(db::Flags::PatternIndexed)) {
					indexes.emplace(mem::toString(name, "_idx_", it.first, "_pattern"), mem::toString("USING btree ( \"", it.first, "\" text_pattern_ops)"));
				}
				if (f.hasFlag(db::Flags::TrigramIndexed)) {
					indexes.emplace(mem::toString(name, "_idx_", it.first, "_trgm"), mem::toString("USING GIN ( \"", it.first, "\" gin_trgm_ops)"));
				}
			}*/
		}
	}

	for (auto &it : scheme->getUnique()) {
		mem::StringStream nameStream;
		nameStream << name << "_uidx";
		mem::Vector<mem::String> values;
		for (auto &f : it.fields) {
			values.emplace_back(f->getName().str<mem::Interface>());
			nameStream << "_" << f->getName();
		}
		indexes.emplace(nameStream.str(), IndexRec(std::move(values), true));
	}

	if (scheme->hasDelta()) {
		do {
			TriggerRec trigger(TriggerRec::Insert, TriggerRec::After, name, "__delta",
									Handle::getNameForDelta(*scheme), "object");
			trigger.rootScheme = scheme;
			auto triggerName = trigger.makeName();
			triggers.emplace(std::move(triggerName), std::move(trigger));
		} while (0);

		do {
			TriggerRec trigger(TriggerRec::Update, TriggerRec::After, name, "__delta",
									Handle::getNameForDelta(*scheme), "object");
			trigger.rootScheme = scheme;
			auto triggerName = trigger.makeName();
			triggers.emplace(std::move(triggerName), std::move(trigger));
		} while (0);

		do {
			TriggerRec trigger(TriggerRec::Delete, TriggerRec::After, name, "__delta",
									Handle::getNameForDelta(*scheme), "object");
			trigger.rootScheme = scheme;
			auto triggerName = trigger.makeName();
			triggers.emplace(std::move(triggerName), std::move(trigger));
		} while (0);
	}

	if (withOids && !detached) {
		indexes.emplace(mem::toString(name, "_idx___oid"), IndexRec("__oid"));
	}
}

bool Handle::init(const Interface::Config &cfg, const mem::Map<mem::StringView, const Scheme *> &s) {
	level = TransactionLevel::Exclusive;
	beginTransaction();

	if (!performSimpleQuery(mem::StringView(DATABASE_DEFAULTS))) {
		endTransaction();
		return false;
	}

	mem::StringStream tables;
	tables << "Server: " << cfg.name << "\n";
	auto existedTables = TableRec::get(*this, tables);
	auto requiredTables = TableRec::parse(cfg, s);

	mem::StringStream stream;
	TableRec::writeCompareResult(*this, stream, requiredTables, existedTables, s);

	if (!stream.empty()) {
		std::cout << stream.weak() << "\n";
		if (!performSimpleQuery(stream.weak())) {
			endTransaction();
			return false;
		}
	}

	mem::StringStream query;
	query << "DELETE FROM __login WHERE \"date\" < " << stappler::Time::now().toSeconds() - config::getInternalsStorageTime().toSeconds() << ";";
	performSimpleQuery(query.weak());
	query.clear();

	auto iit = existedTables.find(mem::StringView("__error"));
	if (iit != existedTables.end()) {
		query << "DELETE FROM __error WHERE \"time\" < " << stappler::Time::now().toMicros() - config::getInternalsStorageTime().toMicros() << ";";
		performSimpleQuery(query.weak());
		query.clear();
	}

	endTransaction();
	return true;
}

NS_DB_SQLITE_END
