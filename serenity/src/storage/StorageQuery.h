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

#ifndef SERENITY_SRC_STORAGE_STORAGEQUERY_H_
#define SERENITY_SRC_STORAGE_STORAGEQUERY_H_

#include "SPSerenityRequest.h"
#include "Define.h"

NS_SA_EXT_BEGIN(storage)

using Operator = sql::Operator;
using Comparation = sql::Comparation;
using Ordering = sql::Ordering;
using Query = query::Query;

enum class Action {
	Get,
	Set,
	Append,
	Remove,
};

enum class TransactionStatus {
	None,
	Commit,
	Rollback,
};

class QueryFieldResolver {
public:
	enum class Meta {
		None = 0,
		Time = 1,
		Action = 2,
		View = 4,
	};

	QueryFieldResolver();
	QueryFieldResolver(const Scheme &, const Query &, const Vector<String> &extraFields = Vector<String>());

	const Field *getField(const String &) const;
	const Scheme *getScheme() const;
	const Map<String, Field> *getFields() const;
	Meta getMeta() const;

	const Set<const Field *> &getResolves() const;
	const Set<StringView> &getResolvesData() const;

	const Query::FieldsVec *getIncludeVec() const;
	const Query::FieldsVec *getExcludeVec() const;

	QueryFieldResolver next(const StringView &) const;

	operator bool () const;

protected:
	struct Data {
		const Scheme *scheme = nullptr;
		const Map<String, Field> *fields = nullptr;
		const Query::FieldsVec *include = nullptr;
		const Query::FieldsVec *exclude = nullptr;
		Set<const Field *> resolved;
		Set<StringView> resolvedData;
		Map<String, Data> next;
		Meta meta = Meta::None;
	};

	QueryFieldResolver(Data *);
	void doResolve(Data *, const Vector<String> &extraFields, uint16_t depth, uint16_t max);
	void doResolveData(Data *, uint16_t depth, uint16_t max);

	Data *root = nullptr;
};

SP_DEFINE_ENUM_AS_MASK(QueryFieldResolver::Meta);

using FullTextData = search::SearchData;

class QueryList : public AllocBase {
public:
	using FieldCallback = Callback<void(const StringView &name, const Field *f)>;

	struct Item {
		const Scheme *scheme = nullptr;
		const Field *ref = nullptr;
		const Field *field = nullptr;

		bool all = false;
		bool resolved = false;

		Query query;
		QueryFieldResolver fields;

		const Set<const Field *> &getQueryFields() const;
		void readFields(const FieldCallback &) const;
	};

	static void readFields(const Scheme &, const Set<const Field *> &, const FieldCallback &);

public:
	QueryList(const Scheme *);

	bool selectById(const Scheme *, uint64_t);
	bool selectByName(const Scheme *, const StringView &);
	bool selectByQuery(const Scheme *, Query::Select &&);

	bool order(const Scheme *, const StringView &f, storage::Ordering o);
	bool first(const Scheme *, const StringView &f, size_t v);
	bool last(const Scheme *, const StringView &f, size_t v);
	bool limit(const Scheme *, size_t limit);
	bool offset(const Scheme *, size_t offset);

	bool setFullTextQuery(const Field *field, Vector<FullTextData> &&);

	bool setAll();
	bool setField(const Scheme *, const Field *field);
	bool setProperty(const Field *field);

	bool isAll() const;
	bool isRefSet() const;
	bool isObject() const;
	bool isView() const;
	bool empty() const;

	bool isDeltaApplicable() const;

	bool apply(const data::Value &query);
	void resolve(const Vector<String> &);

	uint16_t getResolveDepth() const;
	void setResolveDepth(uint16_t);

	void setDelta(Time);
	Time getDelta() const;

	size_t size() const;

	const Scheme *getPrimaryScheme() const;
	const Scheme *getSourceScheme() const;
	const Scheme *getScheme() const;
	const Field *getField() const;
	const Query &getTopQuery() const;

	const Vector<Item> &getItems() const;

	const Query::FieldsVec &getIncludeFields() const;
	const Query::FieldsVec &getExcludeFields() const;

	QueryFieldResolver getFields() const;

	const data::Value &getExtraData() const;

protected:
	void decodeSelect(const Scheme &, Query &, const data::Value &);
	void decodeOrder(const Scheme &, Query &, const String &, const data::Value &);
	bool decodeInclude(const Scheme &, Query &, const data::Value &);
	bool decodeIncludeItem(const Scheme &, Query &, const data::Value &);

	Vector<Item> queries;
	data::Value extraData;
};

NS_SA_EXT_END(storage)

#endif /* SERENITY_SRC_STORAGE_STORAGEQUERY_H_ */
