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

#ifndef STELLATOR_DB_SQL_STSQLQUERY_H_
#define STELLATOR_DB_SQL_STSQLQUERY_H_

#include "STStorageInterface.h"
#include "STSqlResult.h"

NS_DB_SQL_BEGIN

class SqlQuery : public stappler::sql::Query<db::Binder, mem::Interface> {
public:
	using TypeString = db::Binder::TypeString;

	SqlQuery(db::QueryInterface *);

	void clear();

	void writeWhere(SqlQuery::SelectWhere &, db::Operator op, const db::Scheme &, const db::Query &);
	void writeOrdering(SqlQuery::SelectFrom &, const db::Scheme &, const db::Query &);

	SelectFrom writeSelectFrom(GenericQuery &q, const db::QueryList::Item &item, bool idOnly, const mem::StringView &scheme, const mem::StringView &field, bool isSimpleGet = false);
	SelectFrom writeSelectFrom(Select &sel, db::Worker &, const db::Query &);

	void writeQueryReqest(SqlQuery::SelectFrom &s, const db::QueryList::Item &item);
	void writeQueryListItem(GenericQuery &sq, const db::QueryList &list, size_t idx, bool idOnly, const db::Field *field = nullptr, bool forSubquery = false);
	void writeQueryList(const db::QueryList &query, bool idOnly, size_t count = stappler::maxOf<size_t>());
	void writeQueryFile(const db::QueryList &query, const db::Field *field);
	void writeQueryArray(const db::QueryList &query, const db::Field *field);

	void writeQueryDelta(const db::Scheme &, const stappler::Time &, const mem::Set<const db::Field *> &fields, bool idOnly);
	void writeQueryViewDelta(const db::QueryList &list, const stappler::Time &, const mem::Set<const db::Field *> &fields, bool idOnly);

	template <typename T>
	friend auto & operator << (SqlQuery &query, const T &value) {
		return query.stream << value;
	}

	const mem::StringStream &getQuery() const;
	db::QueryInterface * getInterface() const;

	void writeFullTextRank(Select &sel, const db::Scheme &scheme, const db::Query &q);
	SelectFrom writeFullTextFrom(Select &sel, const db::Scheme &scheme, const db::Query &q);
};

NS_DB_SQL_END

#endif /* STELLATOR_DB_SQL_STSQLQUERY_H_ */
