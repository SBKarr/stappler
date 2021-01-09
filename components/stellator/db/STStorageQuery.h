/**
Copyright (c) 2017-2019 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef STELLATOR_DB_STSTORAGEQUERY_H_
#define STELLATOR_DB_STSTORAGEQUERY_H_

#include "SPSql.h"
#include "SPSearchConfiguration.h"
#include "STStorage.h"

NS_DB_BEGIN

using Operator = stappler::sql::Operator;
using Comparation = stappler::sql::Comparation;
using Ordering = stappler::sql::Ordering;

stappler::Pair<mem::String, bool> encodeComparation(Comparation);
stappler::Pair<Comparation, bool> decodeComparation(const mem::StringView &);

enum class Resolve {
	None = 0,
	Files = 2,
	Sets = 4,
	Objects = 8,
	Arrays = 16,
	Ids = 32,
	Basics = 64,
	Defaults = 128,
	All = Files | Sets | Objects | Arrays | Defaults,
};

SP_DEFINE_ENUM_AS_MASK(Resolve);

class Query : public mem::AllocBase {
public:
	struct Field : public mem::AllocBase {
		mem::String name;
		mem::Vector<Field> fields;

		Field(Field &&);
		Field(const Field &);

		Field &operator=(Field &&);
		Field &operator=(const Field &);

		template <typename Str> Field(Str &&);
		template <typename Str> Field(Str &&, mem::Vector<mem::String> &&);
		template <typename Str> Field(Str &&, std::initializer_list<mem::String> &&);
		template <typename Str> Field(Str &&, mem::Vector<Field> &&);
		template <typename Str> Field(Str &&, std::initializer_list<Field> &&);

		void setName(const char *);
		void setName(const mem::StringView &);
		void setName(const mem::String &);
		void setName(mem::String &&);
		void setName(const Field &);
		void setName(Field &&);
	};

	using FieldsVec = mem::Vector<Field>;

	struct Select : public mem::AllocBase {
		Comparation compare = Comparation::Equal;
		mem::Value value1;
		mem::Value value2;
		mem::String field;
		mem::Vector<stappler::search::SearchData> searchData;

		Select() { }
		Select(const mem::StringView & f, Comparation c, mem::Value && v1, mem::Value && v2);
		Select(const mem::StringView & f, Comparation c, int64_t v1, int64_t v2);
		Select(const mem::StringView & f, Comparation c, const mem::String & v);
		Select(const mem::StringView & f, Comparation c, const mem::StringView & v);
		Select(const mem::StringView & f, Comparation c, mem::Vector<stappler::search::SearchData> && v);
	};

	struct SoftLimit {
		mem::String field;
		size_t limit;
		mem::Value offset;
	};

	static Query all();
	static Query field(int64_t id, const mem::StringView &);
	static Query field(int64_t id, const mem::StringView &, const Query &);

	static Resolve decodeResolve(const mem::StringView &str);
	static mem::String encodeResolve(Resolve);

	Query & select(const mem::StringView &alias);
	Query & select(int64_t id);
	Query & select(const mem::Value &);
	Query & select(mem::Vector<int64_t> &&id);
	Query & select(mem::SpanView<int64_t> id);
	Query & select(std::initializer_list<int64_t> &&id);

	Query & select(const mem::StringView &f, Comparation c, const mem::Value & v1, const mem::Value &v2 = mem::Value());
	Query & select(const mem::StringView &f, const mem::Value & v1); // special case for equality

	Query & select(const mem::StringView &f, Comparation c, int64_t v1);
	Query & select(const mem::StringView &f, Comparation c, int64_t v1, int64_t v2);
	Query & select(const mem::StringView &f, const mem::String & v);
	Query & select(const mem::StringView &f, mem::String && v);

	Query & select(const mem::StringView &f, const mem::Bytes & v);
	Query & select(const mem::StringView &f, mem::Bytes && v);

	Query & select(const mem::StringView &f, mem::Vector<stappler::search::SearchData> && v);

	Query & select(Select &&q);

	Query & order(const mem::StringView &f, Ordering o = Ordering::Ascending, size_t limit = stappler::maxOf<size_t>(), size_t offset = 0);
	Query & softLimit(const mem::StringView &, Ordering, size_t limit, mem::Value &&);

	Query & first(const mem::StringView &f, size_t limit = 1, size_t offset = 0);
	Query & last(const mem::StringView &f, size_t limit = 1, size_t offset = 0);

	Query & limit(size_t l, size_t off);
	Query & limit(size_t l);
	Query & offset(size_t l);

	Query & delta(uint64_t);
	Query & delta(const mem::StringView &);

	template <typename ... Args>
	Query & include(Field &&, Args && ...);

	Query & include(Field &&);
	Query & exclude(Field &&);

	Query & depth(uint16_t);

	Query & forUpdate();

	Query & clearFields();

	bool empty() const;

	mem::StringView getQueryField() const;
	int64_t getQueryId() const;

	int64_t getSingleSelectId() const;
	const mem::Vector<int64_t> & getSelectIds() const;
	mem::StringView getSelectAlias() const;
	const mem::Vector<Select> &getSelectList() const;

	const mem::String & getOrderField() const;
	Ordering getOrdering() const;

	size_t getLimitValue() const;
	size_t getOffsetValue() const;

	const mem::Value &getSoftLimitValue() const;

	bool hasSelectName() const; // id or alias
	bool hasSelectList() const;

	bool hasSelect() const;
	bool hasOrder() const;
	bool hasLimit() const;
	bool hasOffset() const;
	bool hasDelta() const;
	bool hasFields() const;
	bool isForUpdate() const;
	bool isSoftLimit() const;

	uint64_t getDeltaToken() const;

	uint16_t getResolveDepth() const;

	const FieldsVec &getIncludeFields() const;
	const FieldsVec &getExcludeFields() const;

	mem::Value encode() const;

protected:
	mem::String queryField;
	int64_t queryId = 0;

	mem::Vector<int64_t> selectIds;
	mem::String selectAlias;
	mem::Vector<Select> selectList;

	Ordering ordering = Ordering::Ascending;
	mem::String orderField;

	size_t limitValue = stappler::maxOf<size_t>();
	size_t offsetValue = 0;
	mem::Value softLimitValue;

	uint64_t deltaToken;

	uint16_t resolveDepth = 1;

	FieldsVec fieldsInclude;
	FieldsVec fieldsExclude;
	bool update = false;
	bool _softLimit = false;
	bool _selected = false;
};

template <typename Str>
inline Query::Field::Field(Str &&str) {
	setName(std::forward<Str>(str));
}

template <typename Str>
inline Query::Field::Field(Str &&str, mem::Vector<mem::String> &&l) {
	setName(std::forward<Str>(str));
	for (auto &it : l) {
		fields.emplace_back(std::move(it));
	}
}

template <typename Str>
inline Query::Field::Field(Str &&str, std::initializer_list<mem::String> &&l) {
	setName(std::forward<Str>(str));
	for (auto &it : l) {
		fields.emplace_back(mem::String(std::move(it)));
	}
}

template <typename Str>
inline Query::Field::Field(Str &&str, mem::Vector<Field> &&l) : fields(std::move(l)) {
	setName(std::forward<Str>(str));
}

template <typename Str>
inline Query::Field::Field(Str &&str, std::initializer_list<Field> &&l) {
	setName(std::forward<Str>(str));
	for (auto &it : l) {
		fields.emplace_back(std::move(it));
	}
}

template <typename ... Args>
Query & Query::include(Field &&f, Args && ... args) {
	include(std::move(f));
	include(std::forward<Args>(args)...);
	return *this;
}

NS_DB_END

#endif /* STELLATOR_DB_STSTORAGEQUERY_H_ */
