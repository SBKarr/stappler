/**
Copyright (c) 2018 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef SERENITY_SRC_DATABASE_SQL_SQLQUERY_H_
#define SERENITY_SRC_DATABASE_SQL_SQLQUERY_H_

#include "SqlResult.h"
#include "SPSerenityRequest.h"
#include "StorageQuery.h"

NS_SA_EXT_BEGIN(sql)

using namespace storage;
using namespace stappler::sql;

class SqlQuery : public stappler::sql::Query<Binder> {
public:
	using TypeString = Binder::TypeString;

	SqlQuery(QueryInterface *);

	void clear();

	void writeWhere(SqlQuery::SelectWhere &, Operator op, const Scheme &, const query::Query &);
	void writeOrdering(SqlQuery::SelectFrom &, const Scheme &, const query::Query &);

	SelectFrom writeSelectFrom(GenericQuery &q, const QueryList::Item &item, bool idOnly, const StringView &scheme, const StringView &field, bool isSimpleGet = false);
	SelectFrom writeSelectFrom(Select &sel, Worker &, const query::Query &);

	void writeQueryReqest(SqlQuery::SelectFrom &s, const QueryList::Item &item);
	void writeQueryListItem(GenericQuery &sq, const QueryList &list, size_t idx, bool idOnly, const storage::Field *field = nullptr, bool forSubquery = false);
	void writeQueryList(const QueryList &query, bool idOnly, size_t count = maxOf<size_t>());
	void writeQueryFile(const QueryList &query, const storage::Field *field);
	void writeQueryArray(const QueryList &query, const storage::Field *field);

	void writeQueryDelta(const Scheme &, const Time &, const Set<const storage::Field *> &fields, bool idOnly);
	void writeQueryViewDelta(const QueryList &list, const Time &, const Set<const storage::Field *> &fields, bool idOnly);

	template <typename T>
	friend auto & operator << (SqlQuery &query, const T &value) {
		return query.stream << value;
	}

	const StringStream &getQuery() const;
	QueryInterface * getInterface() const;

protected:
	void writeFullTextRank(Select &sel, const Scheme &scheme, const query::Query &q);
	SelectFrom writeFullTextFrom(Select &sel, const Scheme &scheme, const query::Query &q);
};

NS_SA_EXT_END(sql)

#endif /* SERENITY_SRC_DATABASE_SQL_SQLQUERY_H_ */
