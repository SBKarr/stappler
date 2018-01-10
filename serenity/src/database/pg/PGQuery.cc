// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2017-2018 Roman Katuntsev <sbkarr@stappler.org>

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
#include "StorageField.h"
#include "StorageScheme.h"
#include "PGHandleTypes.h"

NS_SA_EXT_BEGIN(pg)

size_t Binder::push(String &&val) {
	params.emplace_back(Bytes());
	params.back().assign_strong((uint8_t *)val.data(), val.size() + 1);
	binary.emplace_back(false);
	return params.size();
}

size_t Binder::push(const StringView &val) {
	params.emplace_back(Bytes());
	params.back().assign_strong((uint8_t *)val.data(), val.size() + 1);
	binary.emplace_back(false);
	return params.size();
}

size_t Binder::push(Bytes &&val) {
	params.emplace_back(std::move(val));
	binary.emplace_back(true);
	return params.size();
}
size_t Binder::push(StringStream &query, const data::Value &val, bool force) {
	if (!force) {
		switch (val.getType()) {
		case data::Value::Type::EMPTY:
			query << "NULL";
			break;
		case data::Value::Type::BOOLEAN:
			if (val.asBool()) {
				query << "TRUE";
			} else {
				query << "FALSE";
			}
			break;
		case data::Value::Type::INTEGER:
			query << val.asInteger();
			break;
		case data::Value::Type::DOUBLE:
			if (isnan(val.asDouble())) {
				query << "NaN";
			} else if (val.asDouble() == std::numeric_limits<double>::infinity()) {
				query << "-Infinity";
			} else if (-val.asDouble() == std::numeric_limits<double>::infinity()) {
				query << "Infinity";
			} else {
				query << val.asDouble();
			}
			break;
		case data::Value::Type::CHARSTRING:
			params.emplace_back(Bytes());
			params.back().assign_strong((uint8_t *)val.getString().data(), val.size() + 1);
			binary.emplace_back(false);
			query << "$" << params.size() << "::text";
			break;
		case data::Value::Type::BYTESTRING:
			params.emplace_back(val.asBytes());
			binary.emplace_back(true);
			query << "$" << params.size() << "::bytea";
			break;
		case data::Value::Type::ARRAY:
		case data::Value::Type::DICTIONARY:
			params.emplace_back(data::write(val, data::EncodeFormat::Cbor));
			binary.emplace_back(true);
			query << "$" << params.size() << "::bytea";
			break;
		default: break;
		}
	} else {
		params.emplace_back(data::write(val, data::EncodeFormat::Cbor));
		binary.emplace_back(true);
		query << "$" << params.size() << "::bytea";
	}
	return params.size();
}

void Binder::writeBind(StringStream &query, int64_t val) {
	query << val;
}
void Binder::writeBind(StringStream &query, uint64_t val) {
	query << val;
}

void Binder::writeBind(StringStream &query, const String &val) {
	if (auto num = push(String(val))) {
		query << "$" << num << "::text";
	}
}
void Binder::writeBind(StringStream &query, String &&val) {
	if (auto num = push(move(val))) {
		query << "$" << num << "::text";
	}
}
void Binder::writeBind(StringStream &query, const Bytes &val) {
	if (auto num = push(Bytes(val))) {
		query << "$" << num << "::bytea";
	}
}
void Binder::writeBind(StringStream &query, Bytes &&val) {
	if (auto num = push(move(val))) {
		query << "$" << num << "::bytea";
	}
}

void Binder::writeBind(StringStream &query, const data::Value &val) {
	push(query, val, false);
}
void Binder::writeBind(StringStream &query, const DataField &f) {
	push(query, f.data, f.force);
}
void Binder::writeBind(StringStream &query, const TypeString &type) {
	if (auto num = push(type.str)) {
		query << "$" << num << "::" << type.type;
	}
}

void Binder::clear() {
	params.clear();
	binary.clear();
}

const Vector<Bytes> &Binder::getParams() const {
	return params;
}
const Vector<bool> &Binder::getBinaryVec() const {
	return binary;
}

void ExecQuery::clear() {
	stream.clear();
	binder.clear();
}

void ExecQuery::writeAliasRequest(ExecQuery::SelectWhere &w, Operator op, const Scheme &s, const String &a) {
	w.parenthesis(op, [&] (ExecQuery::WhereBegin &wh) {
		auto whi = wh.where();
		for (auto &it : s.getFields()) {
			if (it.second.getType() == storage::Type::Text && it.second.getTransform() == storage::Transform::Alias) {
				whi.where(Operator::Or, it.first, Comparation::Equal, String(a));
			}
		}
	});
}
void ExecQuery::writeQueryRequest(ExecQuery::SelectWhere &w, Operator op, const Scheme &s, const Vector<pg::Query::Select> &where) {
	w.parenthesis(op, [&] (ExecQuery::WhereBegin &wh) {
		auto whi = wh.where();
		auto &fields = s.getFields();
		for (auto &it : where) {
			auto f_it = fields.find(it.field);
			if ((f_it != fields.end() && f_it->second.isIndexed() && storage::checkIfComparationIsValid(f_it->second.getType(), it.compare))
					|| (it.field == "__oid" && storage::checkIfComparationIsValid(storage::Type::Integer, it.compare))) {
				whi.where(Operator::And, ExecQuery::Field(s.getName(), it.field), it.compare, it.value1, it.value2);
			}
		}
	});
}

void ExecQuery::writeQueryReqest(ExecQuery::SelectFrom &s, const QueryList::Item &item) {
	auto &q = item.query;
	if (!item.all && !item.query.empty()) {
		auto w = s.where();
		if (q.getSelectOid()) {
			w.where(Operator::And, ExecQuery::Field(item.scheme->getName(), "__oid"), Comparation::Equal, q.getSelectOid());
		} else if (!q.getSelectAlias().empty()) {
			writeAliasRequest(w, Operator::And, *item.scheme, q.getSelectAlias());
		} else if (!q.getSelectList().empty()) {
			writeQueryRequest(w, Operator::And, *item.scheme, q.getSelectList());
		}
	}

	if (q.hasOrder() || q.hasLimit() || q.hasOffset()) {
		String orderField;
		if (q.hasOrder()) {
			orderField = q.getOrderField();
		} else if (q.getSelectList().size() == 1) {
			orderField = q.getSelectList().back().field;
		} else {
			orderField = "__oid";
		}

		SelectOrder o = s.order(q.getOrdering(), ExecQuery::Field(item.scheme->getName(), orderField),
				q.getOrdering() == Ordering::Descending?sql::Nulls::Last:sql::Nulls::None);

		if (q.hasLimit() && q.hasOffset()) {
			o.limit(q.getLimitValue(), q.getOffsetValue());
		} else if (q.hasLimit()) {
			o.limit(q.getLimitValue());
		} else if (q.hasOffset()) {
			o.offset(q.getOffsetValue());
		}
	}
}

static void ExecQuery_writeJoin(ExecQuery::SelectFrom &s, const String &sqName, const String &schemeName, const QueryList::Item &item) {
	s.innerJoinOn(sqName, [&] (ExecQuery::WhereBegin &w) {
		String fieldName = item.ref
				? ( item.ref->getType() == Type::Set ? String("__oid") : item.ref->getName() )
				: String("__oid");
		w.where(ExecQuery::Field(schemeName, fieldName), Comparation::Equal, ExecQuery::Field(sqName, "id"));
	});
}

ExecQuery::Select &ExecQuery::writeSelectFields(const Scheme &scheme, ExecQuery::Select &sel, const Set<const storage::Field *> &fields, const StringView &source) {
	if (!fields.empty()) {
		if (!source.empty()) { sel.field(ExecQuery::Field(source, "__oid")); } else { sel.field(ExecQuery::Field("__oid")); }
		for (auto &it : fields) {
			if (it != nullptr) {
				auto type = it->getType();
				if (type != storage::Type::Set && type != storage::Type::Array && type != storage::Type::View) {
					if (!source.empty()) { sel.field(ExecQuery::Field(source, it->getName())); } else { sel.field(ExecQuery::Field(it->getName())); }
				}
			}
		}
		for (auto &it : scheme.getFields()) {
			if (it.second.hasFlag(Flags::ForceInclude) && fields.find(&it.second) == fields.end()) {
				if (!source.empty()) { sel.field(ExecQuery::Field(source, it.first)); } else { sel.field(ExecQuery::Field(it.first)); }
			}
		}
		for (auto &it : scheme.getForceInclude()) {
			if (fields.find(it) == fields.end()) {
				if (!source.empty()) { sel.field(ExecQuery::Field(source, it->getName())); } else { sel.field(ExecQuery::Field(it->getName())); }
			}
		}
	} else {
		if (!source.empty()) { sel.field(ExecQuery::Field::all(source)); } else { sel.field(ExecQuery::Field::all()); }
	}
	return sel;
}

auto ExecQuery::writeSelectFrom(GenericQuery &q, const QueryList::Item &item, bool idOnly, const StringView &schemeName, const StringView &fieldName) -> SelectFrom {
	if (idOnly) {
		return q.select(ExecQuery::Field(schemeName, fieldName).as("id")).from(schemeName);
	}

	auto sel = q.select();
	auto &fields = item.getQueryFields();

	auto s = writeSelectFields(*item.scheme, sel, fields, schemeName);
	return s.from(schemeName);
}

void ExecQuery::writeQueryListItem(GenericQuery &q, const QueryList &list, size_t idx, bool idOnly, const storage::Field *field) {
	auto &items = list.getItems();
	const QueryList::Item &item = items.at(idx);
	const storage::Field *sourceField = nullptr;
	const storage::FieldView *viewField = nullptr;
	String refQueryTag;
	if (idx > 0) {
		sourceField = items.at(idx - 1).field;
	}

	if (idx > 0 && !item.ref && sourceField && sourceField->getType() != storage::Type::Object) {
		String prevSq = toString("sq", idx - 1);
		const QueryList::Item &prevItem = items.at(idx - 1);

		if (sourceField->getType() == storage::Type::View) {
			viewField = static_cast<const FieldView *>(sourceField->getSlot());
		}
		String tname = viewField
				? toString(prevItem.scheme->getName(), "_f_", prevItem.field->getName(), "_view")
				: toString(prevItem.scheme->getName(), "_f_", prevItem.field->getName());

		String targetIdField = toString(item.scheme->getName(), "_id");
		String sourceIdField = toString(prevItem.scheme->getName(), "_id");

		if (idOnly && item.query.empty()) { // optimize id-only empty request
			q.select(ExecQuery::Field(targetIdField).as("id"))
					.from(tname)
					.innerJoinOn(prevSq, [&] (WhereBegin &w) {
				w.where(sourceIdField, Comparation::Equal, ExecQuery::Field(prevSq, "id"));
			});
			return;
		}

		refQueryTag = toString("sq", idx, "_ref");
		q.with(refQueryTag, [&] (GenericQuery &sq) {
			sq.select(ExecQuery::Field(targetIdField).as("id"))
					.from(tname).innerJoinOn(prevSq, [&] (WhereBegin &w) {
				w.where(ExecQuery::Field(sourceIdField), Comparation::Equal, ExecQuery::Field(prevSq, "id"));
			});
		});
	}

	const storage::Field * f = field?field:item.field;

	String schemeName(item.scheme->getName());
	String fieldName( (f && (f->getType() == Type::Object || f->isFile())) ? f->getName() : String("__oid") );

	auto s = writeSelectFrom(q, item, idOnly, schemeName, fieldName);
	if (idx > 0) {
		if (refQueryTag.empty()) {
			ExecQuery_writeJoin(s, toString("sq", idx - 1), item.scheme->getName(), item);
		} else {
			ExecQuery_writeJoin(s, refQueryTag, item.scheme->getName(), item);
		}
	}
	writeQueryReqest(s, item);
}

void ExecQuery::writeQueryList(const QueryList &list, bool idOnly, size_t count) {
	const QueryList::Item &item = list.getItems().back();
	if (item.query.hasDelta() && list.isDeltaApplicable()) {
		if (!list.isView()) {
			writeQueryDelta(*item.scheme, Time::microseconds(item.query.getDeltaToken()), item.fields.getResolves(), false);
		} else {
			writeQueryViewDelta(list, Time::microseconds(item.query.getDeltaToken()), item.fields.getResolves(), false);
		}
		return;
	} else if (item.query.hasDelta()) {
		messages::error("Query", "Delta is not applicable for this query");
	}

	auto &items = list.getItems();
	count = min(items.size(), count);

	GenericQuery q(this);
	size_t i = 0;
	for (; i < count - 1; ++ i) {
		q.with(toString("sq", i), [&] (GenericQuery &sq) {
			writeQueryListItem(sq, list, i, true);
		});
	}

	writeQueryListItem(q, list, i, idOnly);
}

void ExecQuery::writeQueryFile(const QueryList &list, const storage::Field *field) {
	auto &items = list.getItems();
	auto count = items.size();
	GenericQuery q(this);
	for (size_t i = 0; i < count - 1; ++ i) {
		q.with(toString("sq", i), [&] (GenericQuery &sq) {
			writeQueryListItem(sq, list, i, true);
		});
	}

	q.with(toString("sq", count - 1), [&] (GenericQuery &sq) {
		writeQueryListItem(sq, list, count - 1, true, field);
	});

	auto fileScheme = Server(apr::pool::server()).getFileScheme();
	q.select(ExecQuery::Field::all(fileScheme->getName()))
			.from(fileScheme->getName())
			.innerJoinOn(toString("sq", count - 1), [&] (ExecQuery::WhereBegin &w) {
		w.where(ExecQuery::Field(fileScheme->getName(), "__oid"), Comparation::Equal, ExecQuery::Field(toString("sq", count - 1), "id"));
	});
}

void ExecQuery::writeQueryArray(const QueryList &list, const storage::Field *field) {
	auto &items = list.getItems();
	auto count = items.size();
	GenericQuery q(this);
	for (size_t i = 0; i < count; ++ i) {
		q.with(toString("sq", i), [&] (GenericQuery &sq) {
			writeQueryListItem(sq, list, i, true);
		});
	}

	auto scheme = items.back().scheme;

	q.select(ExecQuery::Field("t", "data"))
			.from(ExecQuery::Field(toString(scheme->getName(), "_f_", field->getName())).as("t"))
			.innerJoinOn(toString("sq", count - 1), [&] (ExecQuery::WhereBegin &w) {
		w.where(ExecQuery::Field("t", toString(scheme->getName(), "_id")), Comparation::Equal, ExecQuery::Field(toString("sq", count - 1), "id"));
	});
}

void ExecQuery::writeQueryDelta(const Scheme &scheme, const Time &time, const Set<const storage::Field *> &fields, bool idOnly) {
	GenericQuery q(this);
	auto s = q.with("d", [&] (ExecQuery::GenericQuery &sq) {
		sq.select()
			.aggregate("max", Field("time").as("time"))
			.aggregate("max", Field("action").as("action"))
			.field("object")
			.from(TableRec::getNameForDelta(scheme))
			.where("time", Comparation::GreatherThen, time.toMicroseconds())
			.group("object")
			.order(Ordering::Descending, "time");
	}).select();
	if (!idOnly) {
		writeSelectFields(scheme, s, fields, "t");
	} else {
		s.field(Field("t", "__oid"));
	}
	s.fields(Field("d", "action").as("__d_action"), Field("d", "time").as("__d_time"), Field("d", "object").as("__d_object"))
		.from(Field(scheme.getName()).as("t"))
		.rightJoinOn("d", [&] (ExecQuery::WhereBegin &w) {
			w.where(Field("d", "object"), Comparation::Equal, Field("t", "__oid"));
	});
}

void ExecQuery::writeQueryViewDelta(const QueryList &list, const Time &time, const Set<const storage::Field *> &fields, bool idOnly) {
	auto &items = list.getItems();
	const QueryList::Item &item = items.back();
	auto prevScheme = items.size() > 1 ? items.at(items.size() - 2).scheme : nullptr;
	auto viewField = items.size() > 1 ? items.at(items.size() - 2).field : items.back().field;
	auto view = static_cast<const FieldView *>(viewField->getSlot());

	GenericQuery q(this);
	const Scheme &scheme = *item.scheme;
	String deltaName = toString(prevScheme->getName(), "_f_", view->name, "_delta");
	String viewName = toString(prevScheme->getName(), "_f_", view->name, "_view");
	auto s = q.with("dv", [&] (ExecQuery::GenericQuery &sq) {
		uint64_t id = 0;
		String sqName;
		// optimize id-only
		if (items.size() != 2 || items.front().query.getSelectOid() == 0) {
			size_t i = 0;
			for (; i < items.size() - 1; ++ i) {
				sq.with(toString("sq", i), [&] (GenericQuery &sq) {
					writeQueryListItem(sq, list, i, true);
				});
			}
			sqName = toString("sq", i - 1);
		} else {
			id = items.front().query.getSelectOid();
		}

		sq.with("d", [&] (ExecQuery::GenericQuery &sq) {
			if (id) {
				sq.select()
					.aggregate("max", Field("time").as("time"))
					.field("object")
					.field("tag")
					.from(deltaName)
					.where(ExecQuery::Field("tag"), Comparation::Equal, id)
					.where(Operator::And, "time", Comparation::GreatherThen, time.toMicroseconds())
					.group("object").field("tag");
			} else {
				sq.select()
					.aggregate("max", Field("time").as("time"))
					.field("object")
					.field(Field(sqName, "id").as("tag"))
					.from(deltaName)
					.innerJoinOn(sqName, [&] (ExecQuery::WhereBegin &w) {
					if (id) {
					} else {
						w.where(ExecQuery::Field(deltaName, "tag"), Comparation::Equal, ExecQuery::Field(sqName, "id"));
					}
				})
					.where("time", Comparation::GreatherThen, time.toMicroseconds())
					.group("object").field(ExecQuery::Field(sqName, "id"));;
			}
		}).select().fields(Field("d", "time"), Field("d", "object"), Field("__vid"))
			.from(viewName).rightJoinOn("d", [&] (ExecQuery::WhereBegin &w) {
				w.where(ExecQuery::Field("d", "tag"), Comparation::Equal, ExecQuery::Field(viewName, toString(prevScheme->getName(), "_id")))
						.where(Operator::And, ExecQuery::Field("d", "object"), Comparation::Equal, ExecQuery::Field(viewName, toString(scheme.getName(), "_id")));
			});
	}).select();

	if (!idOnly) {
		writeSelectFields(scheme, s, fields, "t");
	} else {
		s.field(Field("t", "__oid"));
	}
	s.fields(Field("dv", "time").as("__d_time"), Field("dv", "object").as("__d_object"), Field("dv", "__vid"))
		.from(Field(view->scheme->getName()).as("t"))
		.innerJoinOn("dv", [&] (ExecQuery::WhereBegin &w) {
			w.where(Field("dv", "object"), Comparation::Equal, Field("t", "__oid"));
	});

	std::cout << stream.weak() << "\n";
}

const StringStream &ExecQuery::getQuery() const {
	return stream;
}

const Vector<Bytes> &ExecQuery::getParams() const {
	return binder.getParams();
}

const Vector<bool> &ExecQuery::getBinaryVec() const {
	return binder.getBinaryVec();
}

inline constexpr bool pgsql_is_success(ExecStatusType x) {
	return (x == PGRES_EMPTY_QUERY) || (x == PGRES_COMMAND_OK) || (x == PGRES_TUPLES_OK) || (x == PGRES_SINGLE_TUPLE);
}

ResultRow::ResultRow(const Result *res, size_t r) : result(res), row(r) { }

ResultRow::ResultRow(const ResultRow & other) noexcept : result(other.result), row(other.row) { }
ResultRow & ResultRow::operator=(const ResultRow &other) noexcept {
	result = other.result;
	row = other.row;
	return *this;
}

size_t ResultRow::size() const {
	return result->_nfields;
}
data::Value ResultRow::toData(const Scheme &scheme, const Map<String, Field> &viewFields) {
	data::Value row;
	data::Value *deltaPtr = nullptr;
	for (size_t i = 0; i < result->_nfields; i++) {
		auto n = result->name(i);
		if (n == "__oid") {
			if (!isNull(i)) {
				row.setInteger(toInteger(i), n.str());
			}
		} else if (n == "__vid") {
			row.setInteger(isNull(i)?int64_t(0):toInteger(i), n.str());
		} else if (n == "__d_action") {
			if (!deltaPtr) {
				deltaPtr = &row.emplace("__delta");
			}
			switch (Handle::DeltaAction(toInteger(i))) {
			case Handle::DeltaAction::Create: deltaPtr->setString("create", "action"); break;
			case Handle::DeltaAction::Update: deltaPtr->setString("update", "action"); break;
			case Handle::DeltaAction::Delete: deltaPtr->setString("delete", "action"); break;
			case Handle::DeltaAction::Append: deltaPtr->setString("append", "action"); break;
			case Handle::DeltaAction::Erase: deltaPtr->setString("erase", "action");  break;
			default: break;
			}
		} else if (n == "__d_object") {
			row.setInteger(toInteger(i), "__oid");
		} else if (n == "__d_time") {
			if (!deltaPtr) {
				deltaPtr = &row.emplace("__delta");
			}
			deltaPtr->setInteger(toInteger(i), "time");
		} else if (!isNull(i)) {
			if (auto f_it = scheme.getField(n)) {
				row.setValue(toData(i, *f_it), n.str());
			} else {
				auto ef_it = viewFields.find(n);
				if (ef_it != viewFields.end()) {
					row.setValue(toData(i, ef_it->second), n.str());
				}
			}
		}
	}
	return row;
}

StringView ResultRow::front() const {
	return at(0);
}
StringView ResultRow::back() const {
	return at(result->_nfields - 1);
}

bool ResultRow::isNull(size_t n) const {
	return PQgetisnull(result->result, row, n);
}

StringView ResultRow::at(size_t n) const {
	return StringView(PQgetvalue(result->result, row, n), size_t(PQgetlength(result->result, row, n)));
}

String ResultRow::toStringWeak(size_t n) const {
	return String::make_weak(PQgetvalue(result->result, row, n), size_t(PQgetlength(result->result, row, n)));
}
Bytes ResultRow::toBytesWeak(size_t n) const {
	if (PQfformat(result->result, n) == 0) {
		return toBytes(n);
	} else {
		return Bytes::make_weak((uint8_t *)PQgetvalue(result->result, row, n), size_t(PQgetlength(result->result, row, n)));
	}
}

String ResultRow::toString(size_t n) const {
	return String(PQgetvalue(result->result, row, n), size_t(PQgetlength(result->result, row, n)));
}

Bytes ResultRow::toBytes(size_t n) const {
	auto val = PQgetvalue(result->result, row, n);
	auto len = PQgetlength(result->result, row, n);
	if (PQfformat(result->result, n) == 0) {
		if (len > 2 && memcmp(val, "\\x", 2) == 0) {
			return base16::decode(CoderSource(val + 2, len - 2));
		}
	}
	return Bytes((uint8_t *)val, (uint8_t *)(val + len));
}

int64_t ResultRow::toInteger(size_t n) const {
	if (PQfformat(result->result, n) == 0) {
		auto val = PQgetvalue(result->result, row, n);
		return StringToNumber<int64_t>(val, nullptr);
	} else {
		DataReader<ByteOrder::Network> r((const uint8_t *)PQgetvalue(result->result, row, n), size_t(PQgetlength(result->result, row, n)));
		switch (r.size()) {
		case 1: return r.readUnsigned(); break;
		case 2: return r.readUnsigned16(); break;
		case 4: return r.readUnsigned32(); break;
		case 8: return r.readUnsigned64(); break;
		default: break;
		}
		return 0;
	}
}

double ResultRow::toDouble(size_t n) const {
	if (PQfformat(result->result, n) == 0) {
		auto val = PQgetvalue(result->result, row, n);
		return StringToNumber<int64_t>(val, nullptr);
	} else {
		DataReader<ByteOrder::Network> r((const uint8_t *)PQgetvalue(result->result, row, n), size_t(PQgetlength(result->result, row, n)));
		switch (r.size()) {
		case 2: return r.readFloat16(); break;
		case 4: return r.readFloat32(); break;
		case 8: return r.readFloat64(); break;
		default: break;
		}
		return 0;
	}
}

bool ResultRow::toBool(size_t n) const {
	auto val = PQgetvalue(result->result, row, n);
	if (PQfformat(result->result, n) == 0) {
		if (val && (*val == 'T' || *val == 'y')) {
			return true;
		}
		return false;
	} else {
		return val && *val != 0;
	}
}

data::Value ResultRow::toData(size_t n, const Field &f) {
	switch(f.getType()) {
	case storage::Type::Integer:
	case storage::Type::Object:
	case storage::Type::Set:
	case storage::Type::Array:
	case storage::Type::File:
	case storage::Type::Image:
		return data::Value(toInteger(n));
		break;
	case storage::Type::Float:
		return data::Value(toDouble(n));
		break;
	case storage::Type::Boolean:
		return data::Value(toBool(n));
		break;
	case storage::Type::Text:
		return data::Value(toString(n));
		break;
	case storage::Type::Bytes:
		return data::Value(toBytes(n));
		break;
	case storage::Type::Data:
	case storage::Type::Extra:
		return data::read(toBytesWeak(n));
		break;
	default:
		break;
	}

	return data::Value();
}

Result::Result(PGresult *res) : result(res) {
	err = result ? PQresultStatus(result) : PGRES_FATAL_ERROR;
	if (success()) {
		_nrows = PQntuples(result);
		_nfields = PQnfields(result);
	}
}
Result::~Result() {
	clear();
}

Result::Result(Result &&res) : result(res.result), err(res.err), _nrows(res._nrows), _nfields(res._nfields) {
	res.result = nullptr;
}
Result & Result::operator=(Result &&res) {
	clear();
	result = res.result;
	err = res.err;
	_nrows = res._nrows;
	_nfields = res._nfields;

	res.result = nullptr;
	return *this;
}

Result::operator bool () const {
	return success();
}
bool Result::success() const {
	return result && pgsql_is_success(err);
}

ExecStatusType Result::getError() const {
	return err;
}

data::Value Result::info() const {
	return data::Value {
		pair("error", data::Value(err)),
		pair("status", data::Value(PQresStatus(err))),
		pair("desc", data::Value(result ? PQresultErrorMessage(result) : "Fatal database error")),
	};
}

bool Result::empty() const {
	return _nrows == 0;
}
size_t Result::nrows() const {
	return _nrows;
}
size_t Result::nfields() const {
	return _nfields;
}

int64_t Result::readId() const {
	if (PQfformat(result, 0) == 0) {
		auto val = PQgetvalue(result, 0, 0);
		return StringToNumber<int64_t>(val, nullptr);
	} else {
		DataReader<ByteOrder::Network> r((const uint8_t *)PQgetvalue(result, 0, 0), size_t(PQgetlength(result, 0, 0)));
		switch (r.size()) {
		case 1: return int64_t(r.readUnsigned()); break;
		case 2: return int64_t(r.readUnsigned16()); break;
		case 4: return int64_t(r.readUnsigned32()); break;
		case 8: return int64_t(r.readUnsigned64()); break;
		default: break;
		}
		return 0;
	}
}

size_t Result::getAffectedRows() const {
	return StringToNumber<size_t>(PQcmdTuples(result));
}

void Result::clear() {
	if (result) {
        PQclear(result);
        result = nullptr;
	}
}

Result::Iter Result::begin() const {
	return Result::Iter(this, 0);
}
Result::Iter Result::end() const {
	return Result::Iter(this, nrows());
}

ResultRow Result::front() const {
	return ResultRow(this, 0);
}
ResultRow Result::back() const {
	return ResultRow(this, _nrows - 1);
}
ResultRow Result::at(size_t n) const {
	return ResultRow(this, n);
}

StringView Result::name(size_t n) const {
	auto ptr = PQfname(result, n);
	if (ptr) {
		return StringView(ptr);
	}
	return StringView();
}

data::Value Result::decode(const Scheme &scheme) const {
	data::Value ret;
	for (auto it : *this) {
		ret.addValue(it.toData(scheme));
	}
	return ret;
}
data::Value Result::decode(const Field &field) const {
	data::Value ret;
	if (_nrows > 0) {
		if (field.getType() == Type::Array) {
			auto &arrF = static_cast<const FieldArray *>(field.getSlot())->tfield;
			for (auto it : *this) {
				ret.addValue(it.toData(0, arrF));
			}
		} else if (field.getType() == Type::View) {
			auto v = static_cast<const FieldView *>(field.getSlot());
			for (auto it : *this) {
				ret.addValue(it.toData(*v->scheme, v->fields));
			}
		} else {
			for (auto it : *this) {
				ret.addValue(it.toData(0, field));
			}
		}
	}
	return ret;
}

NS_SA_EXT_END(pg)
