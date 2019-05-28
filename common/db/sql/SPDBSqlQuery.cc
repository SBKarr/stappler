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

#include "SPDBSqlQuery.h"
#include "SPDBSqlHandle.h"
#include "SPDBStorageScheme.h"

NS_DB_SQL_BEGIN

SqlQuery::SqlQuery(db::QueryInterface *iface) {
	binder.setInterface(iface);
}

void SqlQuery::clear() {
	stream.clear();
	binder.clear();
}

void SqlQuery::writeWhere(SqlQuery::SelectWhere &w, db::Operator op, const db::Scheme &scheme, const db::Query &q) {
	if (q.getSingleSelectId()) {
		w.where(op, "__oid", db::Comparation::Equal, q.getSingleSelectId());
	} else if (!q.getSelectIds().empty()) {
		w.parenthesis(op, [&] (SqlQuery::WhereBegin &wh) {
			auto whi = wh.where();
			for (auto &it : q.getSelectIds()) {
				whi.where(db::Operator::Or, SqlQuery::Field(scheme.getName(), "__oid"), db::Comparation::Equal, it);
			}
		});
	} else if (!q.getSelectAlias().empty()) {
		w.parenthesis(op, [&] (SqlQuery::WhereBegin &wh) {
			auto whi = wh.where();
			for (auto &it : scheme.getFields()) {
				if (it.second.getType() == db::Type::Text && it.second.getTransform() == db::Transform::Alias) {
					whi.where(db::Operator::Or, SqlQuery::Field(scheme.getName(), it.first), db::Comparation::Equal, q.getSelectAlias());
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
					if (f_it->second.getType() == db::Type::FullTextView) {
						whi.where(db::Operator::And, SqlQuery::Field(scheme.getName(), it.field), db::Comparation::Includes, RawString{mem::toString("__ts_query_", it.field)});
					} else if ((f_it->second.isIndexed() && db::checkIfComparationIsValid(f_it->second.getType(), it.compare))) {
						whi.where(db::Operator::And, SqlQuery::Field(scheme.getName(), it.field), it.compare, it.value1, it.value2);
					}
				} else if (it.field == "__oid" && db::checkIfComparationIsValid(db::Type::Integer, it.compare)) {
					whi.where(db::Operator::And, SqlQuery::Field(scheme.getName(), it.field), it.compare, it.value1, it.value2);
				}
			}
		});
	}
}

void SqlQuery::writeOrdering(SqlQuery::SelectFrom &s, const db::Scheme &scheme, const db::Query &q) {
	if (q.hasOrder() || q.hasLimit() || q.hasOffset()) {
		auto ordering = q.getOrdering();
		mem::String orderField;
		mem::String schemeName = scheme.getName().str<mem::Interface>();
		if (q.hasOrder()) {
			if (auto f = scheme.getField(q.getOrderField())) {
				if (f->getType() == db::Type::FullTextView) {
					orderField = mem::toString("__ts_rank_", q.getOrderField());
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
				ordering == db::Ordering::Descending ? stappler::sql::Nulls::Last : stappler::sql::Nulls::None);

		if (q.hasLimit() && q.hasOffset()) {
			o.limit(q.getLimitValue(), q.getOffsetValue());
		} else if (q.hasLimit()) {
			o.limit(q.getLimitValue());
		} else if (q.hasOffset()) {
			o.offset(q.getOffsetValue());
		}
	}
}

void SqlQuery::writeQueryReqest(SqlQuery::SelectFrom &s, const db::QueryList::Item &item) {
	auto &q = item.query;
	if (!item.all && !item.query.empty()) {
		auto w = s.where();
		writeWhere(w, db::Operator::And, *item.scheme, q);
	}

	writeOrdering(s, *item.scheme, q);
}

static void SqlQuery_writeJoin(SqlQuery::SelectFrom &s, const mem::StringView &sqName, const mem::StringView &schemeName, const db::QueryList::Item &item) {
	s.innerJoinOn(sqName, [&] (SqlQuery::WhereBegin &w) {
		mem::StringView fieldName = item.ref
				? ( item.ref->getType() == db::Type::Set ? mem::StringView("__oid") : item.ref->getName() )
				: mem::StringView("__oid");
		w.where(SqlQuery::Field(schemeName, fieldName), db::Comparation::Equal, SqlQuery::Field(sqName, "id"));
	});
}

void SqlQuery::writeFullTextRank(Select &sel, const db::Scheme &scheme, const db::Query &q) {
	for (auto &it : q.getSelectList()) {
		if (auto f = scheme.getField(it.field)) {
			if (f->getType() == db::Type::FullTextView) {
				sel.query->writeBind(db::Binder::FullTextRank{scheme.getName(), f});
				sel.query->getStream() << " AS __ts_rank_" << it.field << ", ";
			}
		}
	}
}

auto SqlQuery::writeFullTextFrom(Select &sel, const db::Scheme &scheme, const db::Query &q) -> SelectFrom {
	auto f = sel.from();
	for (auto &it : q.getSelectList()) {
		if (auto field = scheme.getField(it.field)) {
			if (field->getType() == db::Type::FullTextView) {
				if (!it.searchData.empty()) {
					mem::StringStream queryFrom;
					for (auto &searchIt : it.searchData) {
						if (!queryFrom.empty()) {
							queryFrom  << " && ";
						}
						binder.writeBind(queryFrom, searchIt);
					}
					auto queryStr = queryFrom.weak();
					f = f.from(Query::Field(queryStr).as(mem::toString("__ts_query_", it.field)));
				} else if (it.value1) {
					auto d = field->getSlot<db::FieldFullTextView>();
					auto q = d->parseQuery(it.value1);
					if (!q.empty()) {
						mem::StringStream queryFrom;
						for (auto &searchIt : q) {
							if (!queryFrom.empty()) {
								queryFrom  << " && ";
							}
							binder.writeBind(queryFrom, searchIt);
						}
						auto queryStr = queryFrom.weak();
						f = f.from(Query::Field(queryStr).as(mem::toString("__ts_query_", it.field)));
					}
				}
			}
		}
	}
	return f;
}

auto SqlQuery::writeSelectFrom(GenericQuery &q, const db::QueryList::Item &item, bool idOnly, const mem::StringView &schemeName, const mem::StringView &fieldName, bool isSimpleGet) -> SelectFrom {
	if (idOnly) {
		return q.select(SqlQuery::Field(schemeName, fieldName).as("id")).from(schemeName);
	}

	auto sel = q.select();
	writeFullTextRank(sel, *item.scheme, item.query);
	item.readFields([&] (const mem::StringView &name, const db::Field *) {
		sel = sel.field(SqlQuery::Field(schemeName, name));
	}, isSimpleGet);
	return writeFullTextFrom(sel, *item.scheme, item.query).from(schemeName);
}

auto SqlQuery::writeSelectFrom(Select &sel, db::Worker &worker, const db::Query &q) -> SelectFrom {
	writeFullTextRank(sel, worker.scheme(), q);
	if (worker.shouldIncludeAll()) {
		sel = sel.field(SqlQuery::Field("*"));
	} else {
		worker.readFields(worker.scheme(), q, [&] (const mem::StringView &name, const db::Field *) {
			sel = sel.field(SqlQuery::Field(name));
		});
	}
	return writeFullTextFrom(sel, worker.scheme(), q).from(worker.scheme().getName());
}

void SqlQuery::writeQueryListItem(GenericQuery &q, const db::QueryList &list, size_t idx, bool idOnly, const db::Field *field, bool forSubquery) {
	auto &items = list.getItems();
	const db::QueryList::Item &item = items.at(idx);
	const db::Field *sourceField = nullptr;
	const db::FieldView *viewField = nullptr;
	mem::String refQueryTag;
	if (idx > 0) {
		sourceField = items.at(idx - 1).field;
	}

	if (idx > 0 && !item.ref && sourceField && sourceField->getType() != db::Type::Object) {
		mem::String prevSq = mem::toString("sq", idx - 1);
		const db::QueryList::Item &prevItem = items.at(idx - 1);

		if (sourceField->getType() == db::Type::View) {
			viewField = static_cast<const db::FieldView *>(sourceField->getSlot());
		}
		mem::String tname = viewField
				? mem::toString(prevItem.scheme->getName(), "_f_", prevItem.field->getName(), "_view")
				: mem::toString(prevItem.scheme->getName(), "_f_", prevItem.field->getName());

		mem::String targetIdField = mem::toString(item.scheme->getName(), "_id");
		mem::String sourceIdField = mem::toString(prevItem.scheme->getName(), "_id");

		if (idOnly && item.query.empty()) { // optimize id-only empty request
			q.select(SqlQuery::Field(targetIdField).as("id"))
					.from(tname)
					.innerJoinOn(prevSq, [&] (WhereBegin &w) {
				w.where(sourceIdField, stappler::sql::Comparation::Equal, SqlQuery::Field(prevSq, "id"));
			});
			return;
		}

		refQueryTag = mem::toString("sq", idx, "_ref");
		q.with(refQueryTag, [&] (GenericQuery &sq) {
			sq.select(SqlQuery::Field(targetIdField).as("id"))
					.from(tname).innerJoinOn(prevSq, [&] (WhereBegin &w) {
				w.where(SqlQuery::Field(sourceIdField), stappler::sql::Comparation::Equal, SqlQuery::Field(prevSq, "id"));
			});
		});
	}

	const db::Field * f = field?field:item.field;

	mem::StringView schemeName(item.scheme->getName());
	mem::StringView fieldName( (f && (
		(f->getType() == db::Type::Object && (forSubquery || !idOnly || idx + 1 == items.size()))
		|| f->isFile()))
			? f->getName()
			: mem::StringView("__oid") );

	auto s = writeSelectFrom(q, item, idOnly, schemeName, fieldName, list.hasFlag(db::QueryList::SimpleGet));
	if (idx > 0) {
		if (refQueryTag.empty()) {
			SqlQuery_writeJoin(s, mem::toString("sq", idx - 1), item.scheme->getName(), item);
		} else {
			SqlQuery_writeJoin(s, refQueryTag, item.scheme->getName(), item);
		}
	}
	writeQueryReqest(s, item);
}

void SqlQuery::writeQueryList(const db::QueryList &list, bool idOnly, size_t count) {
	const db::QueryList::Item &item = list.getItems().back();
	if (item.query.hasDelta() && list.isDeltaApplicable()) {
		if (!list.isView()) {
			writeQueryDelta(*item.scheme, stappler::Time::microseconds(item.query.getDeltaToken()), item.fields.getResolves(), false);
		} else {
			writeQueryViewDelta(list, stappler::Time::microseconds(item.query.getDeltaToken()), item.fields.getResolves(), false);
		}
		return;
	} else if (item.query.hasDelta()) {
		messages::error("Query", "Delta is not applicable for this query");
	}

	auto &items = list.getItems();
	count = std::min(items.size(), count);

	GenericQuery q(this);
	size_t i = 0;
	if (count > 0) {
		for (; i < count - 1; ++ i) {
			q.with(mem::toString("sq", i), [&] (GenericQuery &sq) {
				writeQueryListItem(sq, list, i, true, nullptr, true);
			});
		}
	}

	writeQueryListItem(q, list, i, idOnly, nullptr, false);
}

void SqlQuery::writeQueryFile(const db::QueryList &list, const db::Field *field) {
	auto &items = list.getItems();
	auto count = items.size();
	GenericQuery q(this);
	for (size_t i = 0; i < count - 1; ++ i) {
		q.with(mem::toString("sq", i), [&] (GenericQuery &sq) {
			writeQueryListItem(sq, list, i, true);
		});
	}

	q.with(mem::toString("sq", count - 1), [&] (GenericQuery &sq) {
		writeQueryListItem(sq, list, count - 1, true, field);
	});

	auto fileScheme = internals::getFileScheme();
	q.select(SqlQuery::Field::all(fileScheme->getName()))
			.from(fileScheme->getName())
			.innerJoinOn(mem::toString("sq", count - 1), [&] (SqlQuery::WhereBegin &w) {
		w.where(SqlQuery::Field(fileScheme->getName(), "__oid"), db::Comparation::Equal, SqlQuery::Field(mem::toString("sq", count - 1), "id"));
	});
}

void SqlQuery::writeQueryArray(const db::QueryList &list, const db::Field *field) {
	auto &items = list.getItems();
	auto count = items.size();
	GenericQuery q(this);
	for (size_t i = 0; i < count; ++ i) {
		q.with(mem::toString("sq", i), [&] (GenericQuery &sq) {
			writeQueryListItem(sq, list, i, true);
		});
	}

	auto scheme = items.back().scheme;

	q.select(SqlQuery::Field("t", "data"))
			.from(SqlQuery::Field(mem::toString(scheme->getName(), "_f_", field->getName())).as("t"))
			.innerJoinOn(mem::toString("sq", count - 1), [&] (SqlQuery::WhereBegin &w) {
		w.where(SqlQuery::Field("t", mem::toString(scheme->getName(), "_id")), db::Comparation::Equal, SqlQuery::Field(mem::toString("sq", count - 1), "id"));
	});
}

void SqlQuery::writeQueryDelta(const db::Scheme &scheme, const stappler::Time &time, const mem::Set<const db::Field *> &fields, bool idOnly) {
	GenericQuery q(this);
	auto s = q.with("d", [&] (SqlQuery::GenericQuery &sq) {
		sq.select()
			.aggregate("max", Field("time").as("time"))
			.aggregate("max", Field("action").as("action"))
			.field("object")
			.from(SqlHandle::getNameForDelta(scheme))
			.where("time", db::Comparation::GreatherThen, time.toMicroseconds())
			.group("object")
			.order(db::Ordering::Descending, "time");
	}).select();
	if (!idOnly) {
		db::QueryList::readFields(scheme, fields, [&] (const mem::StringView &name, const db::Field *field) {
			s.field(SqlQuery::Field("t", name));
		});
	} else {
		s.field(Field("t", "__oid"));
	}
	s.fields(Field("d", "action").as("__d_action"), Field("d", "time").as("__d_time"), Field("d", "object").as("__d_object"))
		.from(Field(scheme.getName()).as("t"))
		.rightJoinOn("d", [&] (SqlQuery::WhereBegin &w) {
			w.where(Field("d", "object"), db::Comparation::Equal, Field("t", "__oid"));
	});
}

void SqlQuery::writeQueryViewDelta(const db::QueryList &list, const stappler::Time &time, const mem::Set<const db::Field *> &fields, bool idOnly) {
	auto &items = list.getItems();
	const db::QueryList::Item &item = items.back();
	auto prevScheme = items.size() > 1 ? items.at(items.size() - 2).scheme : nullptr;
	auto viewField = items.size() > 1 ? items.at(items.size() - 2).field : items.back().field;
	auto view = static_cast<const db::FieldView *>(viewField->getSlot());

	GenericQuery q(this);
	const db::Scheme &scheme = *item.scheme;
	mem::String deltaName = mem::toString(prevScheme->getName(), "_f_", view->name, "_delta");
	mem::String viewName = mem::toString(prevScheme->getName(), "_f_", view->name, "_view");
	auto s = q.with("dv", [&] (SqlQuery::GenericQuery &sq) {
		uint64_t id = 0;
		mem::String sqName;
		// optimize id-only
		if (items.size() != 2 || items.front().query.getSingleSelectId() == 0) {
			size_t i = 0;
			for (; i < items.size() - 1; ++ i) {
				sq.with(mem::toString("sq", i), [&] (GenericQuery &sq) {
					writeQueryListItem(sq, list, i, true);
				});
			}
			sqName = mem::toString("sq", i - 1);
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
					.where(SqlQuery::Field("tag"), db::Comparation::Equal, id)
					.where(db::Operator::And, "time", db::Comparation::GreatherThen, time.toMicroseconds())
					.group("object").field("tag");
			} else {
				sq.select()
					.aggregate("max", Field("time").as("time"))
					.field("object")
					.field(Field(sqName, "id").as("tag"))
					.from(deltaName)
					.innerJoinOn(sqName, [&] (SqlQuery::WhereBegin &w) {
						w.where(SqlQuery::Field(deltaName, "tag"), db::Comparation::Equal, SqlQuery::Field(sqName, "id"));
				})
					.where("time", db::Comparation::GreatherThen, time.toMicroseconds())
					.group("object").field(SqlQuery::Field(sqName, "id"));;
			}
		}).select().fields(Field("d", "time"), Field("d", "object"), Field("__vid"))
			.from(viewName).rightJoinOn("d", [&] (SqlQuery::WhereBegin &w) {
				w.where(SqlQuery::Field("d", "tag"), db::Comparation::Equal, SqlQuery::Field(viewName, mem::toString(prevScheme->getName(), "_id")))
						.where(db::Operator::And, SqlQuery::Field("d", "object"), db::Comparation::Equal, SqlQuery::Field(viewName, mem::toString(scheme.getName(), "_id")));
			});
	}).select();

	if (!idOnly) {
		db::QueryList::readFields(scheme, fields, [&] (const mem::StringView &name, const db::Field *field) {
			s.field(SqlQuery::Field("t", name));
		});
	} else {
		s.field(Field("t", "__oid"));
	}
	s.fields(Field("dv", "time").as("__d_time"), Field("dv", "object").as("__d_object"), Field("dv", "__vid"))
		.from(Field(view->scheme->getName()).as("t"))
		.rightJoinOn("dv", [&] (SqlQuery::WhereBegin &w) {
			w.where(Field("dv", "object"), db::Comparation::Equal, Field("t", "__oid"));
	});
}

const mem::StringStream &SqlQuery::getQuery() const {
	return stream;
}

db::QueryInterface * SqlQuery::getInterface() const {
	return binder.getInterface();
}

NS_DB_SQL_END
