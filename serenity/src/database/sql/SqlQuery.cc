/**
Copyright (c) 2018-2019 Roman Katuntsev <sbkarr@stappler.org>

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

#include "SqlQuery.h"
#include "SqlHandle.h"
#include "StorageScheme.h"

NS_SA_EXT_BEGIN(sql)

void Binder::setInterface(QueryInterface *iface) {
	_iface = iface;
}
QueryInterface * Binder::getInterface() const {
	return _iface;
}

void Binder::writeBind(StringStream &query, int64_t val) {
	_iface->bindInt(*this, query, val);
}
void Binder::writeBind(StringStream &query, uint64_t val) {
	_iface->bindUInt(*this, query, val);
}
void Binder::writeBind(StringStream &query, const String &val) {
	_iface->bindString(*this, query, val);
}
void Binder::writeBind(StringStream &query, String &&val) {
	_iface->bindMoveString(*this, query, move(val));
}
void Binder::writeBind(StringStream &query, const StringView &val) {
	_iface->bindStringView(*this, query, val);
}
void Binder::writeBind(StringStream &query, const Bytes &val) {
	_iface->bindBytes(*this, query, val);
}
void Binder::writeBind(StringStream &query, Bytes &&val) {
	_iface->bindMoveBytes(*this, query, move(val));
}
void Binder::writeBind(StringStream &query, const CoderSource &val) {
	_iface->bindCoderSource(*this, query, val);
}
void Binder::writeBind(StringStream &query, const data::Value &val) {
	_iface->bindValue(*this, query, val);
}
void Binder::writeBind(StringStream &query, const DataField &f) {
	_iface->bindDataField(*this, query, f);
}
void Binder::writeBind(StringStream &query, const TypeString &type) {
	_iface->bindTypeString(*this, query, type);
}
void Binder::writeBind(StringStream &query, const FullTextField &d) {
	_iface->bindFullText(*this, query, d);
}
void Binder::writeBind(StringStream &query, const FullTextRank &rank) {
	_iface->bindFullTextRank(*this, query, rank);
}
void Binder::writeBind(StringStream &query, const FullTextData &data) {
	_iface->bindFullTextData(*this, query, data);
}
void Binder::clear() {
	_iface->clear();
}

SqlQuery::SqlQuery(QueryInterface *iface) {
	binder.setInterface(iface);
}

void SqlQuery::clear() {
	stream.clear();
	binder.clear();
}

void SqlQuery::writeWhere(SqlQuery::SelectWhere &w, Operator op, const Scheme &scheme, const query::Query &q) {
	if (q.getSingleSelectId()) {
		w.where(op, "__oid", Comparation::Equal, q.getSingleSelectId());
	} else if (!q.getSelectIds().empty()) {
		w.parenthesis(op, [&] (SqlQuery::WhereBegin &wh) {
			auto whi = wh.where();
			for (auto &it : q.getSelectIds()) {
				whi.where(Operator::Or, SqlQuery::Field(scheme.getName(), "__oid"), Comparation::Equal, it);
			}
		});
	} else if (!q.getSelectAlias().empty()) {
		w.parenthesis(op, [&] (SqlQuery::WhereBegin &wh) {
			auto whi = wh.where();
			for (auto &it : scheme.getFields()) {
				if (it.second.getType() == storage::Type::Text && it.second.getTransform() == storage::Transform::Alias) {
					whi.where(Operator::Or, SqlQuery::Field(scheme.getName(), it.first), Comparation::Equal, q.getSelectAlias());
				}
			}
		});
	} else if (q.getSelectList().size() > 0) {
		w.parenthesis(op, [&] (SqlQuery::WhereBegin &wh) {
			auto whi = wh.where();
			auto &fields = scheme.getFields();
			for (auto &it : q.getSelectList()) {
				auto f_it = fields.find(it.field);
				if (f_it != fields.end()) {
					if (f_it->second.getType() == storage::Type::FullTextView) {
						whi.where(Operator::And, SqlQuery::Field(scheme.getName(), it.field), Comparation::Includes, RawString{toString("__ts_query_", it.field)});
					} else if ((f_it->second.isIndexed() && storage::checkIfComparationIsValid(f_it->second.getType(), it.compare))
							|| (it.field == "__oid" && storage::checkIfComparationIsValid(storage::Type::Integer, it.compare))) {
						whi.where(Operator::And, SqlQuery::Field(scheme.getName(), it.field), it.compare, it.value1, it.value2);
					}
				}

			}
		});
	}
}

void SqlQuery::writeOrdering(SqlQuery::SelectFrom &s, const Scheme &scheme, const query::Query &q) {
	if (q.hasOrder() || q.hasLimit() || q.hasOffset()) {
		auto ordering = q.getOrdering();
		String orderField;
		String schemeName = scheme.getName().str();
		if (q.hasOrder()) {
			if (auto f = scheme.getField(q.getOrderField())) {
				if (f->getType() == storage::Type::FullTextView) {
					orderField = toString("__ts_rank_", q.getOrderField());
					schemeName.clear();
				} else {
					orderField = q.getOrderField();
				}
			} else if (q.getOrderField() == "__oid") {
				orderField = q.getOrderField();
			} else {
				return;
			}
		} else if (q.getSelectList().size() == 1) {
			orderField = q.getSelectList().back().field;
			if (!scheme.getField(orderField)) {
				return;
			}
		} else {
			orderField = "__oid";
		}

		SelectOrder o = s.order(ordering, schemeName.empty() ? SqlQuery::Field(orderField) : SqlQuery::Field(scheme.getName(), orderField),
				ordering == Ordering::Descending?sql::Nulls::Last:sql::Nulls::None);

		if (q.hasLimit() && q.hasOffset()) {
			o.limit(q.getLimitValue(), q.getOffsetValue());
		} else if (q.hasLimit()) {
			o.limit(q.getLimitValue());
		} else if (q.hasOffset()) {
			o.offset(q.getOffsetValue());
		}
	}
}

void SqlQuery::writeQueryReqest(SqlQuery::SelectFrom &s, const QueryList::Item &item) {
	auto &q = item.query;
	if (!item.all && !item.query.empty()) {
		auto w = s.where();
		writeWhere(w, Operator::And, *item.scheme, q);
	}

	writeOrdering(s, *item.scheme, q);
}

static void SqlQuery_writeJoin(SqlQuery::SelectFrom &s, const StringView &sqName, const StringView &schemeName, const QueryList::Item &item) {
	s.innerJoinOn(sqName, [&] (SqlQuery::WhereBegin &w) {
		StringView fieldName = item.ref
				? ( item.ref->getType() == Type::Set ? StringView("__oid") : item.ref->getName() )
				: StringView("__oid");
		w.where(SqlQuery::Field(schemeName, fieldName), Comparation::Equal, SqlQuery::Field(sqName, "id"));
	});
}

void SqlQuery::writeFullTextRank(Select &sel, const Scheme &scheme, const query::Query &q) {
	for (auto &it : q.getSelectList()) {
		if (auto f = scheme.getField(it.field)) {
			if (f->getType() == Type::FullTextView) {
				sel.query->writeBind(Binder::FullTextRank{scheme.getName(), f});
				sel.query->getStream() << " AS __ts_rank_" << it.field << ", ";
			}
		}
	}
}

auto SqlQuery::writeFullTextFrom(Select &sel, const Scheme &scheme, const query::Query &q) -> SelectFrom {
	auto f = sel.from();
	for (auto &it : q.getSelectList()) {
		if (auto field = scheme.getField(it.field)) {
			if (field->getType() == Type::FullTextView) {
				if (!it.searchData.empty()) {
					StringStream queryFrom;
					for (auto &searchIt : it.searchData) {
						if (!queryFrom.empty()) {
							queryFrom  << " && ";
						}
						binder.writeBind(queryFrom, searchIt);
					}
					auto queryStr = queryFrom.weak();
					f = f.from(Query::Field(queryStr).as(toString("__ts_query_", it.field)));
				} else if (it.value1) {
					auto d = field->getSlot<FieldFullTextView>();
					auto q = d->parseQuery(it.value1);
					if (!q.empty()) {
						StringStream queryFrom;
						for (auto &searchIt : q) {
							if (!queryFrom.empty()) {
								queryFrom  << " && ";
							}
							binder.writeBind(queryFrom, searchIt);
						}
						auto queryStr = queryFrom.weak();
						f = f.from(Query::Field(queryStr).as(toString("__ts_query_", it.field)));
					}
				}
			}
		}
	}
	return f;
}

auto SqlQuery::writeSelectFrom(GenericQuery &q, const QueryList::Item &item, bool idOnly, const StringView &schemeName, const StringView &fieldName, bool isSimpleGet) -> SelectFrom {
	if (idOnly) {
		return q.select(SqlQuery::Field(schemeName, fieldName).as("id")).from(schemeName);
	}

	auto sel = q.select();
	writeFullTextRank(sel, *item.scheme, item.query);
	item.readFields([&] (const StringView &name, const storage::Field *) {
		sel = sel.field(SqlQuery::Field(schemeName, name));
	}, isSimpleGet);
	return writeFullTextFrom(sel, *item.scheme, item.query).from(schemeName);
}

auto SqlQuery::writeSelectFrom(Select &sel, Worker &worker, const query::Query &q) -> SelectFrom {
	writeFullTextRank(sel, worker.scheme(), q);
	worker.readFields(worker.scheme(), q, [&] (const StringView &name, const storage::Field *) {
		sel = sel.field(SqlQuery::Field(name));
	});
	return writeFullTextFrom(sel, worker.scheme(), q).from(worker.scheme().getName());
}

void SqlQuery::writeQueryListItem(GenericQuery &q, const QueryList &list, size_t idx, bool idOnly, const storage::Field *field, bool forSubquery) {
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
			q.select(SqlQuery::Field(targetIdField).as("id"))
					.from(tname)
					.innerJoinOn(prevSq, [&] (WhereBegin &w) {
				w.where(sourceIdField, stappler::sql::Comparation::Equal, SqlQuery::Field(prevSq, "id"));
			});
			return;
		}

		refQueryTag = toString("sq", idx, "_ref");
		q.with(refQueryTag, [&] (GenericQuery &sq) {
			sq.select(SqlQuery::Field(targetIdField).as("id"))
					.from(tname).innerJoinOn(prevSq, [&] (WhereBegin &w) {
				w.where(SqlQuery::Field(sourceIdField), stappler::sql::Comparation::Equal, SqlQuery::Field(prevSq, "id"));
			});
		});
	}

	const storage::Field * f = field?field:item.field;

	StringView schemeName(item.scheme->getName());
	StringView fieldName( (f && (
		(f->getType() == Type::Object && (forSubquery || !idOnly || idx + 1 == items.size()))
		|| f->isFile()))
			? f->getName()
			: StringView("__oid") );

	auto s = writeSelectFrom(q, item, idOnly, schemeName, fieldName, list.hasFlag(QueryList::SimpleGet));
	if (idx > 0) {
		if (refQueryTag.empty()) {
			SqlQuery_writeJoin(s, toString("sq", idx - 1), item.scheme->getName(), item);
		} else {
			SqlQuery_writeJoin(s, refQueryTag, item.scheme->getName(), item);
		}
	}
	writeQueryReqest(s, item);
}

void SqlQuery::writeQueryList(const QueryList &list, bool idOnly, size_t count) {
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
	if (count > 0) {
		for (; i < count - 1; ++ i) {
			q.with(toString("sq", i), [&] (GenericQuery &sq) {
				writeQueryListItem(sq, list, i, true, nullptr, true);
			});
		}
	}

	writeQueryListItem(q, list, i, idOnly, nullptr, false);
}

void SqlQuery::writeQueryFile(const QueryList &list, const storage::Field *field) {
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
	q.select(SqlQuery::Field::all(fileScheme->getName()))
			.from(fileScheme->getName())
			.innerJoinOn(toString("sq", count - 1), [&] (SqlQuery::WhereBegin &w) {
		w.where(SqlQuery::Field(fileScheme->getName(), "__oid"), Comparation::Equal, SqlQuery::Field(toString("sq", count - 1), "id"));
	});
}

void SqlQuery::writeQueryArray(const QueryList &list, const storage::Field *field) {
	auto &items = list.getItems();
	auto count = items.size();
	GenericQuery q(this);
	for (size_t i = 0; i < count; ++ i) {
		q.with(toString("sq", i), [&] (GenericQuery &sq) {
			writeQueryListItem(sq, list, i, true);
		});
	}

	auto scheme = items.back().scheme;

	q.select(SqlQuery::Field("t", "data"))
			.from(SqlQuery::Field(toString(scheme->getName(), "_f_", field->getName())).as("t"))
			.innerJoinOn(toString("sq", count - 1), [&] (SqlQuery::WhereBegin &w) {
		w.where(SqlQuery::Field("t", toString(scheme->getName(), "_id")), Comparation::Equal, SqlQuery::Field(toString("sq", count - 1), "id"));
	});
}

void SqlQuery::writeQueryDelta(const Scheme &scheme, const Time &time, const Set<const storage::Field *> &fields, bool idOnly) {
	GenericQuery q(this);
	auto s = q.with("d", [&] (SqlQuery::GenericQuery &sq) {
		sq.select()
			.aggregate("max", Field("time").as("time"))
			.aggregate("max", Field("action").as("action"))
			.field("object")
			.from(SqlHandle::getNameForDelta(scheme))
			.where("time", Comparation::GreatherThen, time.toMicroseconds())
			.group("object")
			.order(Ordering::Descending, "time");
	}).select();
	if (!idOnly) {
		QueryList::readFields(scheme, fields, [&] (const StringView &name, const storage::Field *field) {
			s.field(SqlQuery::Field("t", name));
		});
	} else {
		s.field(Field("t", "__oid"));
	}
	s.fields(Field("d", "action").as("__d_action"), Field("d", "time").as("__d_time"), Field("d", "object").as("__d_object"))
		.from(Field(scheme.getName()).as("t"))
		.rightJoinOn("d", [&] (SqlQuery::WhereBegin &w) {
			w.where(Field("d", "object"), Comparation::Equal, Field("t", "__oid"));
	});
}

void SqlQuery::writeQueryViewDelta(const QueryList &list, const Time &time, const Set<const storage::Field *> &fields, bool idOnly) {
	auto &items = list.getItems();
	const QueryList::Item &item = items.back();
	auto prevScheme = items.size() > 1 ? items.at(items.size() - 2).scheme : nullptr;
	auto viewField = items.size() > 1 ? items.at(items.size() - 2).field : items.back().field;
	auto view = static_cast<const FieldView *>(viewField->getSlot());

	GenericQuery q(this);
	const Scheme &scheme = *item.scheme;
	String deltaName = toString(prevScheme->getName(), "_f_", view->name, "_delta");
	String viewName = toString(prevScheme->getName(), "_f_", view->name, "_view");
	auto s = q.with("dv", [&] (SqlQuery::GenericQuery &sq) {
		uint64_t id = 0;
		String sqName;
		// optimize id-only
		if (items.size() != 2 || items.front().query.getSingleSelectId() == 0) {
			size_t i = 0;
			for (; i < items.size() - 1; ++ i) {
				sq.with(toString("sq", i), [&] (GenericQuery &sq) {
					writeQueryListItem(sq, list, i, true);
				});
			}
			sqName = toString("sq", i - 1);
		} else {
			id = items.front().query.getSingleSelectId();
		}

		sq.with("d", [&] (SqlQuery::GenericQuery &sq) {
			if (id) {
				sq.select()
					.aggregate("max", Field("time").as("time"))
					.field("object")
					.field("tag")
					.from(deltaName)
					.where(SqlQuery::Field("tag"), Comparation::Equal, id)
					.where(Operator::And, "time", Comparation::GreatherThen, time.toMicroseconds())
					.group("object").field("tag");
			} else {
				sq.select()
					.aggregate("max", Field("time").as("time"))
					.field("object")
					.field(Field(sqName, "id").as("tag"))
					.from(deltaName)
					.innerJoinOn(sqName, [&] (SqlQuery::WhereBegin &w) {
						w.where(SqlQuery::Field(deltaName, "tag"), Comparation::Equal, SqlQuery::Field(sqName, "id"));
				})
					.where("time", Comparation::GreatherThen, time.toMicroseconds())
					.group("object").field(SqlQuery::Field(sqName, "id"));;
			}
		}).select().fields(Field("d", "time"), Field("d", "object"), Field("__vid"))
			.from(viewName).rightJoinOn("d", [&] (SqlQuery::WhereBegin &w) {
				w.where(SqlQuery::Field("d", "tag"), Comparation::Equal, SqlQuery::Field(viewName, toString(prevScheme->getName(), "_id")))
						.where(Operator::And, SqlQuery::Field("d", "object"), Comparation::Equal, SqlQuery::Field(viewName, toString(scheme.getName(), "_id")));
			});
	}).select();

	if (!idOnly) {
		QueryList::readFields(scheme, fields, [&] (const StringView &name, const storage::Field *field) {
			s.field(SqlQuery::Field("t", name));
		});
	} else {
		s.field(Field("t", "__oid"));
	}
	s.fields(Field("dv", "time").as("__d_time"), Field("dv", "object").as("__d_object"), Field("dv", "__vid"))
		.from(Field(view->scheme->getName()).as("t"))
		.rightJoinOn("dv", [&] (SqlQuery::WhereBegin &w) {
			w.where(Field("dv", "object"), Comparation::Equal, Field("t", "__oid"));
	});
}

const StringStream &SqlQuery::getQuery() const {
	return stream;
}

QueryInterface * SqlQuery::getInterface() const {
	return binder.getInterface();
}

NS_SA_EXT_END(sql)
