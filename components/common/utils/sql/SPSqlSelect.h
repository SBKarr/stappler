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

#ifndef COMMON_UTILS_SPSQLSELECT_H_
#define COMMON_UTILS_SPSQLSELECT_H_

#include "SPSql.h"

NS_SP_EXT_BEGIN(sql)

template <typename Binder, typename Interface>
auto Query<Binder, Interface>::Select::all() -> Select & {
	if (this->state == State::None) { this->state = State::Some; } else { this->query->stream << ","; }
	this->query->stream << " *";
	return *this;
}

template <typename Binder, typename Interface>
auto Query<Binder, Interface>::Select::count() -> Select & {
	if (this->state == State::None) { this->state = State::Some; } else { this->query->stream << ","; }
	this->query->stream << " COUNT(*)";
	return *this;
}

template <typename Binder, typename Interface>
auto Query<Binder, Interface>::Select::count(const String &alias) -> Select & {
	if (this->state == State::None) { this->state = State::Some; } else { this->query->stream << ","; }
	this->query->stream << " COUNT(*) AS \"" << alias << "\"";
	return *this;
}

template <typename Binder, typename Interface>
template <typename Clause>
template <typename ...Args>
auto Query<Binder, Interface>::FieldsClause<Clause>::fields(const Field &f, Args && ... args) -> Clause & {
	Expand<Clause>::fields((Clause &)*this, f, std::forward<Args>(args)...);
	return (Clause &)*this;
}

template <typename Binder, typename Interface>
template <typename Clause>
auto Query<Binder, Interface>::FieldsClause<Clause>::field(const Field &f) -> Clause & {
	switch (this->state) {
	case State::None:
		this->query->stream << " ";
		this->state = State::Some;
		break;
	case State::Init:
		this->query->stream << "(";
		this->state = State::Some;
		break;
	case State::Some:
		this->query->stream << ", ";
		break;
	}
	this->query->writeBind(f);
	return (Clause &)*this;
}

template <typename Binder, typename Interface>
template <typename Clause>
auto Query<Binder, Interface>::FieldsClause<Clause>::aggregate(const StringView &func, const Field &f) -> Clause & {
	switch (this->state) {
	case State::None:
		this->query->stream << " ";
		this->state = State::Some;
		break;
	case State::Init:
		this->query->stream << "(";
		this->state = State::Some;
		break;
	case State::Some:
		this->query->stream << ", ";
		break;
	}
	this->query->writeBind(func, f);
	return (Clause &)*this;
}

template <typename Binder, typename Interface>
auto Query<Binder, Interface>::Select::from() -> Query<Binder, Interface>::SelectFrom {
	if (this->state == State::None) {
		this->query->stream << " *";
	}
	this->query->stream << " FROM";
	return SelectFrom{this->query};
}

template <typename Binder, typename Interface>
auto Query<Binder, Interface>::Select::from(const Field &field) -> SelectFrom {
	if (this->state == State::None) {
		this->query->stream << " *";
	}
	this->query->stream << " FROM " << field.name;
	this->query->target = field.name;
	if (!field.alias.empty()) {
		this->query->stream << " " << field.alias;
	}
	return SelectFrom(this->query, State::Some);
}

template <typename Binder, typename Interface>
template <typename ... Args>
auto Query<Binder, Interface>::Select::from(const Field &field, Args && ... args) -> SelectFrom {
	auto f = from();
	Expand<SelectFrom>::from(f, field, forward<Args>(args)...);
	return f;
}

template <typename Binder, typename Interface>
auto Query<Binder, Interface>::SelectFrom::from(const Field &field) -> SelectFrom & {
	if (this->state == State::None) { this->state = State::Some; } else { this->query->stream << ","; }
	this->query->stream << " " << field.name;
	if (this->query->target.empty()) {
		this->query->target = field.name;
	}
	if (!field.alias.empty()) {
		this->query->stream << " " << field.alias;
	}
	return *this;
}

template <typename Binder, typename Interface>
template <typename ... Args>
auto Query<Binder, Interface>::SelectFrom::from(const Field &field, Args && ... args) -> SelectFrom & {
	Expand<SelectFrom>::from(*this, forward<Args>(args)...);
	return *this;
}

template <typename Binder, typename Interface>
template <typename Callback>
auto Query<Binder, Interface>::SelectFrom::innerJoinOn(const StringView &s, const Callback &cb) -> SelectFrom & {
	if (this->state == State::Some) {
		this->query->stream << " INNER JOIN " << s << " ON(";
		WhereBegin tmp(this->query);
		cb(tmp);
		this->query->stream << ")";
	}
	return *this;
}

template <typename Binder, typename Interface>
template <typename Callback>
auto Query<Binder, Interface>::SelectFrom::leftJoinOn(const StringView &s, const Callback &cb) -> SelectFrom & {
	if (this->state == State::Some) {
		this->query->stream << " LEFT OUTER JOIN " << s << " ON(";
		WhereBegin tmp(this->query);
		cb(tmp);
		this->query->stream << ")";
	}
	return *this;
}

template <typename Binder, typename Interface>
template <typename Callback>
auto Query<Binder, Interface>::SelectFrom::rightJoinOn(const StringView &s, const Callback &cb) -> SelectFrom & {
	if (this->state == State::Some) {
		this->query->stream << " RIGHT OUTER JOIN " << s << " ON(";
		WhereBegin tmp(this->query);
		cb(tmp);
		this->query->stream << ")";
	}
	return *this;
}

template <typename Binder, typename Interface>
template <typename Callback>
auto Query<Binder, Interface>::SelectFrom::fullJoinOn(const StringView &s, const Callback &cb) -> SelectFrom & {
	if (this->state == State::Some) {
		this->query->stream << " FULL OUTER JOIN " << s << " ON(";
		WhereBegin tmp(this->query);
		cb(tmp);
		this->query->stream << ")";
	}
	return *this;
}

template <typename Binder, typename Interface>
template <typename ... Args>
auto Query<Binder, Interface>::SelectFrom::where(Args && ... args) -> SelectWhere {
	this->query->stream << " WHERE";
	SelectWhere q(this->query);
	q.where(sql::Operator::And, std::forward<Args>(args)...);
	return q;
}

template <typename Binder, typename Interface>
auto Query<Binder, Interface>::SelectFrom::where() -> SelectWhere {
	this->query->stream << " WHERE";
	return SelectWhere(this->query);
}

template <typename Binder, typename Interface>
auto Query<Binder, Interface>::SelectFrom::group(const Field &f) -> SelectGroup {
	this->query->stream << " GROUP BY ";
	auto g = SelectGroup(this->query);
	g.field(f);
	return g;
}

template <typename Binder, typename Interface>
inline auto Query_writeOrderSt(typename Interface::StringStreamType &stream, Query<Binder, Interface> &query, Ordering ord, const typename Query<Binder, Interface>::Field &field, Nulls n) {
	stream << " ORDER BY ";
	query.writeBind(field, false);
	switch (ord) {
	case Ordering::Ascending: stream << " ASC"; break;
	case Ordering::Descending: stream << " DESC"; break;
	}

	switch (n) {
	case Nulls::None: break;
	case Nulls::First: stream << " NULLS FIRST"; break;
	case Nulls::Last: stream << " NULLS LAST"; break;
	}
}

template <typename Binder, typename Interface>
auto Query<Binder, Interface>::SelectFrom::order(Ordering ord, const Field &field, Nulls n) -> SelectOrder {
	Query_writeOrderSt<Binder>(this->query->stream, *this->query, ord, field, n);
	return SelectOrder(this->query);
}

template <typename Binder, typename Interface>
void Query<Binder, Interface>::SelectFrom::forUpdate() {
	this->query->stream << " FOR UPDATE";
}

template <typename Binder, typename Interface>
template <typename ...Args>
auto Query<Binder, Interface>::SelectGroup::fields(const Field &f, Args && ... args) -> SelectGroup & {
	Expand<SelectGroup>::fields(*this, f, std::forward<Args>(args)...);
	return *this;
}

template <typename Binder, typename Interface>
auto Query<Binder, Interface>::SelectGroup::field(const Field &f) -> SelectGroup & {
	switch (this->state) {
	case State::None:
		this->query->stream << " ";
		this->state = State::Some;
		break;
	case State::Some:
		this->query->stream << ", ";
		break;
	default:
		break;
	}
	this->query->writeBind(f, false);
	return *this;
}

template <typename Binder, typename Interface>
auto Query<Binder, Interface>::SelectGroup::order(Ordering ord, const Field &field, Nulls n) -> SelectOrder {
	Query_writeOrderSt<Binder>(this->query->stream, *this->query, ord, field, n);
	return SelectOrder(this->query);
}

template <typename Binder, typename Interface>
auto Query<Binder, Interface>::SelectWhere::group(const Field &f) -> SelectGroup {
	this->query->stream << " GROUP BY ";
	auto g = SelectGroup(this->query);
	g.field(f);
	return g;
}

template <typename Binder, typename Interface>
auto Query<Binder, Interface>::SelectWhere::order(Ordering ord, const Field &field, Nulls n) -> SelectOrder {
	Query_writeOrderSt<Binder>(this->query->stream, *this->query, ord, field, n);
	return SelectOrder(this->query);
}

template <typename Binder, typename Interface>
void Query<Binder, Interface>::SelectWhere::forUpdate() {
	this->query->stream << " FOR UPDATE";
}

template <typename Binder, typename Interface>
auto Query<Binder, Interface>::SelectOrder::limit(size_t limit, size_t offset) -> SelectPost {
	this->query->stream << " LIMIT " << limit << " OFFSET " << offset;
	return SelectPost(this->query);
}

template <typename Binder, typename Interface>
auto Query<Binder, Interface>::SelectOrder::limit(size_t limit) -> SelectPost  {
	this->query->stream << " LIMIT " << limit;
	return SelectPost(this->query);
}

template <typename Binder, typename Interface>
auto Query<Binder, Interface>::SelectOrder::offset(size_t offset) -> SelectPost {
	this->query->stream << " OFFSET " << offset;
	return SelectPost(this->query);
}

template <typename Binder, typename Interface>
void Query<Binder, Interface>::SelectOrder::forUpdate() {
	this->query->stream << " FOR UPDATE";
}

template <typename Binder, typename Interface>
void Query<Binder, Interface>::SelectPost::forUpdate() {
	this->query->stream << " FOR UPDATE";
}

template <typename Binder, typename Interface>
template <typename ... Args>
auto Query<Binder, Interface>::select(const Field &f, Args && ... args) -> Select {
	auto s = select();
	s.fields(f, forward<Args>(args)...);
	return s;
}

template <typename Binder, typename Interface>
template <typename ... Args>
auto Query<Binder, Interface>::select(Distinct d, const Field &f, Args && ... args) -> Select {
	auto s = select(d);
	s.fields(f, forward<Args>(args)...);
	return s;
}

template <typename Binder, typename Interface>
auto Query<Binder, Interface>::select(Distinct d) -> Select {
	stream << "SELECT";
	switch (d) {
	case Distinct::Distinct:
		stream << " DISTINCT";
		break;
	default:
		break;
	}
	return Select(this);
}

NS_SP_EXT_END(sql)

#endif /* COMMON_UTILS_SPSQLSELECT_H_ */
