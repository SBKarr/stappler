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

class Binder;
class QueryInterface;

class Binder {
public:
	struct DataField {
		const data::Value &data;
		bool force = false;
	};

	struct FullTextField {
		const data::Value &data;
	};

	struct FullTextRank {
		StringView scheme;
		const Field *field;
	};

	struct TypeString {
		StringView str;
		StringView type;

		template <typename Str, typename Type>
		TypeString(Str && str, Type && type)
		: str(str), type(type) { }
	};

	void setInterface(QueryInterface *);
	QueryInterface * getInterface() const;

	void writeBind(StringStream &, int64_t);
	void writeBind(StringStream &, uint64_t);
	void writeBind(StringStream &, const String &);
	void writeBind(StringStream &, String &&);
	void writeBind(StringStream &, const StringView &);
	void writeBind(StringStream &, const Bytes &);
	void writeBind(StringStream &, Bytes &&);
	void writeBind(StringStream &, const CoderSource &);
	void writeBind(StringStream &, const data::Value &);
	void writeBind(StringStream &, const DataField &);
	void writeBind(StringStream &, const TypeString &);
	void writeBind(StringStream &, const FullTextField &);
	void writeBind(StringStream &, const FullTextRank &);
	void writeBind(StringStream &, const FullTextData &);

	void clear();

protected:
	QueryInterface *_iface = nullptr;
};

class QueryInterface {
public:
	virtual ~QueryInterface() = default;

	virtual void bindInt(Binder &, StringStream &, int64_t) = 0;
	virtual void bindUInt(Binder &, StringStream &, uint64_t) = 0;
	virtual void bindString(Binder &, StringStream &, const String &) = 0;
	virtual void bindMoveString(Binder &, StringStream &, String &&) = 0;
	virtual void bindStringView(Binder &, StringStream &, const StringView &) = 0;
	virtual void bindBytes(Binder &, StringStream &, const Bytes &) = 0;
	virtual void bindMoveBytes(Binder &, StringStream &, Bytes &&) = 0;
	virtual void bindCoderSource(Binder &, StringStream &, const CoderSource &) = 0;
	virtual void bindValue(Binder &, StringStream &, const data::Value &) = 0;
	virtual void bindDataField(Binder &, StringStream &, const Binder::DataField &) = 0;
	virtual void bindTypeString(Binder &, StringStream &, const Binder::TypeString &) = 0;
	virtual void bindFullText(Binder &, StringStream &, const Binder::FullTextField &) = 0;
	virtual void bindFullTextRank(Binder &, StringStream &, const Binder::FullTextRank &) = 0;
	virtual void bindFullTextData(Binder &, StringStream &, const FullTextData &) = 0;

	virtual void clear() = 0;
};

class SqlQuery : public stappler::sql::Query<Binder> {
public:
	using TypeString = Binder::TypeString;

	SqlQuery(QueryInterface *);

	void clear();

	void writeWhere(SqlQuery::SelectWhere &, Operator op, const Scheme &, const query::Query &);
	void writeOrdering(SqlQuery::SelectFrom &, const Scheme &, const query::Query &, const Vector<FullTextData> & = Vector<FullTextData>());

	SelectFrom writeSelectFrom(GenericQuery &q, const QueryList::Item &item, bool idOnly, const StringView &scheme, const StringView &field);

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
};

NS_SA_EXT_END(sql)

#endif /* SERENITY_SRC_DATABASE_SQL_SQLQUERY_H_ */
