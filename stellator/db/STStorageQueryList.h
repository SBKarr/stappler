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

#ifndef STELLATOR_DB_STSTORAGEQUERYLIST_H_
#define STELLATOR_DB_STSTORAGEQUERYLIST_H_

#include "STStorageQuery.h"

NS_DB_BEGIN

enum class Action {
	Get,
	Set,
	Append,
	Remove,
	Count,
};

enum class TransactionStatus {
	None,
	Commit,
	Rollback,
};

class QueryFieldResolver : public mem::AllocBase {
public:
	enum class Meta {
		None = 0,
		Time = 1,
		Action = 2,
		View = 4,
	};

	QueryFieldResolver();
	QueryFieldResolver(const Scheme &, const Query &, const mem::Vector<mem::String> &extraFields = mem::Vector<mem::String>());

	const Field *getField(const mem::String &) const;
	const Scheme *getScheme() const;
	const mem::Map<mem::String, Field> *getFields() const;
	Meta getMeta() const;

	const mem::Set<const Field *> &getResolves() const;
	const mem::Set<mem::StringView> &getResolvesData() const;

	const Query::FieldsVec *getIncludeVec() const;
	const Query::FieldsVec *getExcludeVec() const;

	QueryFieldResolver next(const mem::StringView &) const;

	operator bool () const;

protected:
	struct Data {
		const Scheme *scheme = nullptr;
		const mem::Map<mem::String, Field> *fields = nullptr;
		const Query::FieldsVec *include = nullptr;
		const Query::FieldsVec *exclude = nullptr;
		mem::Set<const Field *> resolved;
		mem::Set<mem::StringView> resolvedData;
		mem::Map<mem::String, Data> next;
		Meta meta = Meta::None;
	};

	QueryFieldResolver(Data *);
	void doResolve(Data *, const mem::Vector<mem::String> &extraFields, uint16_t depth, uint16_t max);
	void doResolveData(Data *, uint16_t depth, uint16_t max);

	Data *root = nullptr;
};

SP_DEFINE_ENUM_AS_MASK(QueryFieldResolver::Meta);

using FullTextData = stappler::search::SearchData;

class QueryList : public mem::AllocBase {
public:
	using FieldCallback = stappler::Callback<void(const mem::StringView &name, const Field *f)>;

	enum Flags {
		None,
		SimpleGet = 1 << 0,
	};

	struct Item {
		const Scheme *scheme = nullptr;
		const Field *ref = nullptr;
		const Field *field = nullptr;

		bool all = false;
		bool resolved = false;

		Query query;
		QueryFieldResolver fields;

		const mem::Set<const Field *> &getQueryFields() const;
		void readFields(const FieldCallback &, bool isSimpleGet = false) const;
	};

	static void readFields(const Scheme &, const mem::Set<const Field *> &, const FieldCallback &, bool isSimpleGet = false);

public:
	QueryList(const Scheme *);

	bool selectById(const Scheme *, uint64_t);
	bool selectByName(const Scheme *, const mem::StringView &);
	bool selectByQuery(const Scheme *, Query::Select &&);

	bool order(const Scheme *, const mem::StringView &f, db::Ordering o);
	bool first(const Scheme *, const mem::StringView &f, size_t v);
	bool last(const Scheme *, const mem::StringView &f, size_t v);
	bool limit(const Scheme *, size_t limit);
	bool offset(const Scheme *, size_t offset);

	bool setFullTextQuery(const Field *field, mem::Vector<FullTextData> &&);

	bool setAll();
	bool setField(const Scheme *, const Field *field);
	bool setProperty(const Field *field);

	mem::StringView setQueryAsMtime();

	void clearFlags();
	void addFlag(Flags);
	bool hasFlag(Flags) const;

	bool isAll() const;
	bool isRefSet() const;
	bool isObject() const;
	bool isView() const;
	bool empty() const;

	bool isDeltaApplicable() const;

	bool apply(const mem::Value &query);
	void resolve(const mem::Vector<mem::String> &);

	uint16_t getResolveDepth() const;
	void setResolveDepth(uint16_t);

	void setDelta(stappler::Time);
	stappler::Time getDelta() const;

	size_t size() const;

	const Scheme *getPrimaryScheme() const;
	const Scheme *getSourceScheme() const;
	const Scheme *getScheme() const;
	const Field *getField() const;
	const Query &getTopQuery() const;

	const mem::Vector<Item> &getItems() const;

	const Query::FieldsVec &getIncludeFields() const;
	const Query::FieldsVec &getExcludeFields() const;

	QueryFieldResolver getFields() const;

	const mem::Value &getExtraData() const;

protected:
	void decodeSelect(const Scheme &, Query &, const mem::Value &);
	void decodeOrder(const Scheme &, Query &, const mem::String &, const mem::Value &);
	bool decodeInclude(const Scheme &, Query &, const mem::Value &);
	bool decodeIncludeItem(const Scheme &, Query &, const mem::Value &);

	Flags _flags = Flags::None;
	mem::Vector<Item> queries;
	mem::Value extraData;
};

SP_DEFINE_ENUM_AS_MASK(QueryList::Flags);

NS_DB_END

#endif /* STELLATOR_DB_STSTORAGEQUERYLIST_H_ */
