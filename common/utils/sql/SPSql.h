/**
Copyright (c) 2017 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef COMMON_UTILS_SQLQUERY_H_
#define COMMON_UTILS_SQLQUERY_H_

#include "SPString.h"
#include "SPData.h"

NS_SP_EXT_BEGIN(sql)

enum class Comparation {
	LessThen, // lt
	LessOrEqual, // le
	Equal, // eq
	NotEqual, // neq
	GreatherOrEqual, // ge
	GreatherThen, // gt
	BetweenValues, // bw  field > v1 AND field < v2
	BetweenEquals, // be  field >= v1 AND field <= v2
	NotBetweenValues, // nbw  field < v1 OR field > v2
	NotBetweenEquals, // nbe  field <= v1 OR field >= v2
};

enum class Ordering {
	Ascending,
	Descending,
};

enum class Nulls {
	None,
	First,
	Last,
};

enum class Operator {
	And,
	Or,
};

struct SimpleBinder : public AllocBase {
	void writeBind(StringStream &stream, const data::Value &val) {
		stream << data::toString(val);
	}
	void writeBind(StringStream &stream, const String &val) {
		stream << val;
	}
	void writeBind(StringStream &stream, const Bytes &val) {
		stream << base16::encode(val);
	}
};

template <typename Binder>
class Query : public AllocBase {
public:
	struct Select;
	struct SelectFrom;
	struct SelectWhere;
	struct SelectGroup;
	struct SelectOrder;
	struct SelectPost;

	struct Insert;
	struct InsertValues;
	struct InsertConflict;
	struct InsertUpdateValues;
	struct InsertWhereValues;
	struct InsertPostConflict;

	struct Update;
	struct UpdateWhere;

	struct Delete;
	struct DeleteWhere;

	struct Returning;
	struct WhereBegin;
	struct WhereContinue;

	enum class State {
		None,
		Init,
		Some,
	};

	enum class FinalizationState {
		None,
		Parentesis,
		Quote,
		DoubleQuote,
		Finalized,
	};

	struct RawString {
		String data;
	};

	struct Field {
		static Field all() { return Field("*"); }

		template <typename SourceString>
		static Field all(SourceString && t) { return Field(t, "*"); }

		template <typename SourceString>
		Field(SourceString && str) : name(str) { }

		template <typename SourceString, typename FieldString>
		Field(SourceString &&t, FieldString &&f) : source(t), name(f) { }

		Field & as(const char *str) { alias = StringView(str); return *this; }
		Field & as(const String &str) { alias = StringView(str); return *this; }

		Field & from(const char *str) { source = StringView(str); return *this; }
		Field & from(const String &str) { source = StringView(str); return *this; }

		StringView source;
		StringView name;
		StringView alias;
	};

	template <typename Clause>
	struct Expand {
		template <typename ... VArgs>
		static void fields(Clause &c, const Field &f, VArgs && ... args) {
			c.field(f);
			fields(c, forward<VArgs>(args)...);
		}
		static void fields(Clause &c, const Field &f) {
			c.field(f);
		}

		template <typename ... VArgs>
		static void from(Clause &c, const Field &f, VArgs && ... args) {
			c.from(f);
			from(c, forward<VArgs>(args)...);
		}
		static void from(Clause &c, const Field &f) {
			c.from(f);
		}

		template <typename Value, typename ... VArgs>
		static void values(Clause &c, Value &&v, VArgs && ... args) {
			c.value(forward<Value>(v));
			values(c, forward<VArgs>(args)...);
		}

		template <typename Value>
		static void values(Clause &c, Value &&v) {
			c.value(forward<Value>(v));
		}

		static void values(Clause &c) { }
	};

	struct QueryHandle {
		Query *query = nullptr;
		State state = State::None;

		void finalize();

		QueryHandle(Query *q, State s = State::None) : query(q), state(s) { }
	};

	template <typename Clause>
	struct WhereClause : QueryHandle {
		template <typename Value>
		auto where(Operator, const Field &field, Comparation, Value &&) -> Clause &;

		template <typename Value>
		auto where(Operator, const Field &field, Comparation, Value &&, Value &&) -> Clause &;

		template <typename Callback>
		auto parenthesis(Operator, const Callback &) -> Clause &;

		using QueryHandle::QueryHandle;
	};

	struct WhereBegin : QueryHandle {
		template <typename ... Args>
		auto where(Args && ... args) -> WhereContinue;
		auto where() -> WhereContinue;

		template <typename Callback>
		auto whereParentesis(const Callback &) -> WhereContinue;

		using QueryHandle::QueryHandle;
	};

	struct WhereContinue : WhereClause<WhereContinue> {
		using WhereClause<WhereContinue>::WhereClause;
	};

	template <typename Clause>
	struct SetClause : QueryHandle {
		template <typename Value>
		auto set(const String &f, Value && v) -> Clause &;

		template <typename Value>
		auto set(const String &t, const String &f, Value && v) -> Clause &;

		auto def(const String &f) -> Clause &;

		using QueryHandle::QueryHandle;
	};

	template <typename Clause>
	struct FieldsClause : QueryHandle {
		template <typename ...Args>
		auto fields(const Field &f, Args && ... args) -> Clause &;
		auto field(const Field &f) -> Clause &;
		auto aggregate(const StringView &, const Field &f) -> Clause &;

		using QueryHandle::QueryHandle;
	};

	struct GenericQuery : QueryHandle {
		template <typename Callback>
		auto with(const String &alias, const Callback &) -> GenericQuery &;

		Select select();
		template <typename ... Args>
		Select select(const Field &, Args && ... args);

		Insert insert(const String &);
		Insert insert(const String &, const String &alias);

		Update update(const String &);
		Update update(const String &, const String &alias);

		Delete remove(const String &);
		Delete remove(const String &, const String &alias);

		using QueryHandle::QueryHandle;
	};

	struct Select : FieldsClause<Select> {
		auto all() -> Select &;
		auto count() -> Select &;
		auto count(const String &alias) -> Select &;
		auto from() -> SelectFrom;
		auto from(const Field &field) -> SelectFrom;
		template <typename ... Args>
		auto from(const Field &field, Args && ... args) -> SelectFrom;

		using FieldsClause<Select>::FieldsClause;
	};

	struct SelectFrom : QueryHandle {
		auto from(const Field &field) -> SelectFrom &;
		template <typename ... Args>
		auto from(const Field &field, Args && ... args) -> SelectFrom &;

		template <typename Callback>
		auto innerJoinOn(const String &s, const Callback &cb) -> SelectFrom &;

		template <typename Callback>
		auto leftJoinOn(const String &s, const Callback &cb) -> SelectFrom &;

		template <typename Callback>
		auto rightJoinOn(const String &s, const Callback &cb) -> SelectFrom &;

		template <typename Callback>
		auto fullJoinOn(const String &s, const Callback &cb) -> SelectFrom &;

		template <typename ... Args>
		auto where(Args && ... args) -> SelectWhere;
		auto where() -> SelectWhere;
		auto group(const Field &) -> SelectGroup;
		auto order(Ordering, const Field &, Nulls = Nulls::None) -> SelectOrder;
		void forUpdate();

		using QueryHandle::QueryHandle;
	};

	struct SelectGroup : QueryHandle {
		template <typename ...Args>
		auto fields(const Field &f, Args && ... args) -> SelectGroup &;
		auto field(const Field &) -> SelectGroup &;
		auto order(Ordering, const Field &, Nulls = Nulls::None) -> SelectOrder;

		using QueryHandle::QueryHandle;
	};

	struct SelectWhere : WhereClause<SelectWhere> {
		auto group(const Field &) -> SelectGroup;
		auto order(Ordering, const Field &, Nulls = Nulls::None) -> SelectOrder;
		void forUpdate();

		using WhereClause<SelectWhere>::WhereClause;
	};

	struct SelectOrder : QueryHandle {
		auto limit(size_t limit, size_t offset) -> SelectPost;
		auto limit(size_t limit) -> SelectPost;
		auto offset(size_t offset) -> SelectPost;
		void forUpdate();

		using QueryHandle::QueryHandle;
	};

	struct SelectPost : QueryHandle {
		void forUpdate();

		using QueryHandle::QueryHandle;
	};

	struct Insert : FieldsClause<Insert> {
		template <typename ...Args>
		auto values(Args && ... args) -> InsertValues;

		using FieldsClause<Insert>::FieldsClause;
	};

	struct InsertValues : QueryHandle {
		template <typename Value>
		auto value(Value &&) -> InsertValues &;
		auto def() -> InsertValues &;

		template <typename ...Args>
		auto values(Args && ... args) -> InsertValues &;

		auto onConflict(const String &) -> InsertConflict;
		auto onConflictDoNothing() -> InsertPostConflict;
		auto returning() -> Returning;

		using QueryHandle::QueryHandle;
	};

	struct InsertConflict : QueryHandle {
		auto doNothing() -> InsertPostConflict;
		auto doUpdate() -> InsertUpdateValues;
		using QueryHandle::QueryHandle;
	};

	struct InsertUpdateValues : SetClause<InsertUpdateValues> {
		auto excluded(const String &f) -> InsertUpdateValues &;
		auto excluded(const String &f, const String &v) -> InsertUpdateValues &;
		template <typename ... Args>
		auto where(Args && ... args) -> InsertWhereValues;
		auto where() -> InsertWhereValues;
		auto returning() -> Returning;
		using SetClause<InsertUpdateValues>::SetClause;
	};

	struct InsertWhereValues : WhereClause<InsertWhereValues> {
		auto returning() -> Returning;
		using WhereClause<InsertWhereValues>::WhereClause;
	};

	struct InsertPostConflict : QueryHandle {
		auto returning() -> Returning;
		using QueryHandle::QueryHandle;
	};

	struct Update : SetClause<Update> {
		template <typename ... Args>
		auto where(Args && ... args) -> UpdateWhere;
		auto where() -> UpdateWhere;
		auto returning() -> Returning;
		using SetClause<Update>::SetClause;
	};

	struct UpdateWhere : WhereClause<UpdateWhere> {
		auto returning() -> Returning;
		using WhereClause<UpdateWhere>::WhereClause;
	};

	struct Delete : QueryHandle {
		template <typename ... Args>
		auto where(Args && ... args) -> DeleteWhere;
		auto where() -> DeleteWhere;
		auto returning() -> Returning;
		using QueryHandle::QueryHandle;
	};

	struct DeleteWhere : WhereClause<DeleteWhere> {
		auto returning() -> Returning;
		using WhereClause<DeleteWhere>::WhereClause;
	};

	struct Returning : FieldsClause<Returning> {
		auto all() -> Returning &;
		auto count() -> Returning &;
		auto count(const String &alias) -> Returning &;
		using FieldsClause<Returning>::FieldsClause;
	};

	Query() = default;

	template <typename Callback>
	GenericQuery with(const String &alias, const Callback &);

	Select select();
	template <typename ... Args>
	Select select(const Field &, Args && ... args);

	Insert insert(const String &);
	Insert insert(const String &, const String &alias);

	Update update(const String &);
	Update update(const String &, const String &alias);

	Delete remove(const String &);
	Delete remove(const String &, const String &alias);

	void finalize();

	template <typename Value>
	void writeBind(Value &&);

	void writeBind(const RawString &);
	void writeBind(const Field &);
	void writeBind(const Field &, bool withAlias);
	void writeBind(const StringView &func, const Field &f);

	StringStream &getStream();

protected:
	FinalizationState finalization = FinalizationState::None;
	Binder binder;
	StringStream stream;
	bool subquery = false;
};

template <typename Binder, typename Value>
struct BinderTraits {
	template <typename V>
	static void writeBind(Query<Binder> &q, Binder &b, V &&val) {
		b.writeBind(q.getStream(), forward<V>(val));
	}
};

template <typename Binder>
struct BinderTraits<Binder, typename Query<Binder>::Field> {
	static void writeBind(Query<Binder> &q, Binder &b, const typename Query<Binder>::Field &val) {
		q.writeBind(val);
	}
};

template <typename Binder>
struct BinderTraits<Binder, typename Query<Binder>::RawString> {
	static void writeBind(Query<Binder> &q, Binder &b, const typename Query<Binder>::RawString &val) {
		q.writeBind(val);
	}
};


template <typename Binder>
void Query<Binder>::QueryHandle::finalize() {
	this->query->finalize();
}

template <typename Binder>
template <typename Value>
void Query<Binder>::writeBind(Value &&val) {
	BinderTraits<Binder, typename std::remove_reference<Value>::type>::writeBind(*this, this->binder, forward<Value>(val));
}

template <typename Binder>
void Query<Binder>::writeBind(const RawString &data) {
	stream << data.data;
}

template <typename Binder>
void Query<Binder>::writeBind(const Field &f) {
	writeBind(f, true);
}

template <typename Binder>
void Query<Binder>::writeBind(const Field &f, bool withAlias) {
	if (!f.source.empty()) {
		stream << f.source << ".";
	}
	if (f.name == "*") {
		stream << "*";
	} else {
		stream << "\"" << f.name << "\"";
	}
	if (withAlias && !f.alias.empty()) {
		stream << " AS \"" << f.alias << "\"";
	}
}

template <typename Binder>
void Query<Binder>::writeBind(const StringView &func, const Field &f) {
	stream << func << "(";
	writeBind(f, false);
	stream << ")";
	if (!f.alias.empty()) {
		stream << " AS \"" << f.alias << "\"";
	}
}

template <typename Binder>
StringStream &Query<Binder>::getStream() {
	return stream;
}

template <typename Binder>
void Query<Binder>::finalize() {
	if (subquery) {
		return;
	}

	switch (finalization) {
	case FinalizationState::None: stream << ";"; break;
	case FinalizationState::Parentesis: stream << ");"; break;
	case FinalizationState::Quote: stream << "';"; break;
	case FinalizationState::DoubleQuote: stream << "\";"; break;
	case FinalizationState::Finalized: break;
	}
	finalization = FinalizationState::Finalized;
}


template <typename Binder>
template <typename Callback>
auto Query<Binder>::with(const String &alias, const Callback &cb) -> GenericQuery {
	GenericQuery q(this);
	q.with(alias, cb);
	return q;
}

template <typename Binder>
template <typename Callback>
auto Query<Binder>::GenericQuery::with(const String &alias, const Callback &cb) -> GenericQuery & {
	switch (this->state) {
	case State::None:
		this->query->stream << "WITH ";
		this->state = State::Some;
		break;
	case State::Some:
		this->query->stream << ", ";
		break;
	default:
		break;
	}
	this->query->stream << alias << " AS (";

	auto sq = this->query->subquery;
	this->query->subquery = true;
	GenericQuery q(this->query, State::None);
	cb(q);
	this->query->subquery = sq;

	this->query->stream << ")";
	return *this;
}

template <typename Binder>
auto Query<Binder>::GenericQuery::select() -> Select {
	return this->query->select();
}

template <typename Binder>
template <typename ... Args>
auto Query<Binder>::GenericQuery::select(const Field &f, Args && ... args) -> Select {
	return this->query->select(f, forward<Args>(args)...);
}

template <typename Binder>
auto Query<Binder>::GenericQuery::insert(const String & field) -> Insert {
	return this->query->insert(field);
}

template <typename Binder>
auto Query<Binder>::GenericQuery::insert(const String &field, const String &alias) -> Insert {
	return this->query->insert(field, alias);
}

template <typename Binder>
auto Query<Binder>::GenericQuery::update(const String & field) -> Update {
	return this->query->update(field);
}

template <typename Binder>
auto Query<Binder>::GenericQuery::update(const String &field, const String &alias) -> Update {
	return this->query->update(field, alias);
}

template <typename Binder>
auto Query<Binder>::GenericQuery::remove(const String & field) -> Delete {
	return this->query->remove(field);
}

template <typename Binder>
auto Query<Binder>::GenericQuery::remove(const String &field, const String &alias) -> Delete {
	return this->query->remove(field, alias);
}

NS_SP_EXT_END(sql)

#include "SPSqlInsert.h"
#include "SPSqlUpdate.h"
#include "SPSqlSelect.h"
#include "SPSqlWhere.h"

#endif /* COMMON_UTILS_SQLQUERY_H_ */
