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

#ifndef COMMON_UTILS_SQL_SPSQLWHERE_H_
#define COMMON_UTILS_SQL_SPSQLWHERE_H_

#include "SPSql.h"

NS_SP_EXT_BEGIN(sql)

template <typename Stream>
inline void Query_writeOperator(Stream &stream, Operator op) {
	switch (op) {
	case Operator::And: stream << "AND"; break;
	case Operator::Or: stream << "OR"; break;
	}
}

template <typename Binder, typename Interface, typename Value>
inline void Query_writeComparation(Query<Binder, Interface> &q, typename Interface::StringStreamType &stream,
		const typename Query<Binder, Interface>::Field &f, Comparation cmp, Value &&v1, Value &&v2) {
	stream << "(";
	switch (cmp) {
	case Comparation::LessThen:
		if (!f.source.empty()) { stream << f.source << "."; }
		stream << '"' << f.name << '"' << '<';
		q.writeBind(forward<Value>(v1));
		break;
	case Comparation::LessOrEqual:
		if (!f.source.empty()) { stream << f.source << "."; }
		stream << '"' << f.name << '"' << "<=";
		q.writeBind(forward<Value>(v1));
		break;
	case Comparation::Equal:
		if (!f.source.empty()) { stream << f.source << "."; }
		stream << '"' << f.name << '"' << '=';
		q.writeBind(forward<Value>(v1));
		break;
	case Comparation::NotEqual:
		if (!f.source.empty()) { stream << f.source << "."; }
		stream << '"' << f.name << '"' << "!=";
		q.writeBind(forward<Value>(v1));
		break;
	case Comparation::GreatherOrEqual:
		if (!f.source.empty()) { stream << f.source << "."; }
		stream << '"' << f.name << '"' << ">=";
		q.writeBind(forward<Value>(v1));
		break;
	case Comparation::GreatherThen:
		if (!f.source.empty()) { stream << f.source << "."; }
		stream << '"' << f.name << '"' << '>';
		q.writeBind(forward<Value>(v1));
		break;
	case Comparation::BetweenValues:
		if (!f.source.empty()) { stream << f.source << "."; }
		stream << '"' << f.name << '"' << '>';
		q.writeBind(forward<Value>(v1));
		stream << " AND ";
		if (!f.source.empty()) { stream << f.source << "."; }
		stream << '"' << f.name << '"' << '<';
		q.writeBind(forward<Value>(v2));
		break;
	case Comparation::BetweenEquals:
		if (!f.source.empty()) { stream << f.source << "."; }
		stream << '"' << f.name << '"' << ">=";
		q.writeBind(forward<Value>(v1));
		stream << " AND ";
		if (!f.source.empty()) { stream << f.source << "."; }
		stream << '"' << f.name << '"' << "<=";
		q.writeBind(forward<Value>(v2));
		break;
	case Comparation::NotBetweenValues:
		if (!f.source.empty()) { stream << f.source << "."; }
		stream << '"' << f.name << '"' << "<=";
		q.writeBind(forward<Value>(v1));
		stream << " OR ";
		if (!f.source.empty()) { stream << f.source << "."; }
		stream << '"' << f.name << '"' << ">=";
		q.writeBind(forward<Value>(v2));
		break;
	case Comparation::NotBetweenEquals:
		if (!f.source.empty()) { stream << f.source << "."; }
		stream << '"' << f.name << '"' << '<';
		q.writeBind(forward<Value>(v1));
		stream << " OR ";
		if (!f.source.empty()) { stream << f.source << "."; }
		stream << '"' << f.name << '"' << '>';
		q.writeBind(forward<Value>(v2));
		break;
	case Comparation::Includes:
		if (!f.source.empty()) { stream << f.source << "."; }
		stream << '"' << f.name << '"' << "@@";
		q.writeBind(forward<Value>(v1));
		break;
	}
	stream << ")";
}

template <typename Binder, typename Interface>
template <typename Clause>
template <typename Value>
auto Query<Binder, Interface>::WhereClause<Clause>::where(Operator op, const Field &field, Comparation cmp, Value &&val) -> Clause & {
	if (this->state == State::None) { this->state = State::Some; } else { Query_writeOperator(this->query->stream, op); }
	Query_writeComparation(*(this->query), this->query->stream, field, cmp, std::forward<Value>(val), std::forward<Value>(val));
	return (Clause &)*this;
}

template <typename Binder, typename Interface>
template <typename Clause>
template <typename Value>
auto Query<Binder, Interface>::WhereClause<Clause>::where(Operator op, const Field &field, Comparation cmp, Value &&val1, Value &&val2) -> Clause & {
	if (this->state == State::None) { this->state = State::Some; } else { Query_writeOperator(this->query->stream, op); }
	Query_writeComparation(*(this->query), this->query->stream, field, cmp, std::forward<Value>(val1), std::forward<Value>(val2));
	return (Clause &)*this;
}

template <typename Binder, typename Interface>
template <typename Clause>
template <typename Callback>
auto Query<Binder, Interface>::WhereClause<Clause>::parenthesis(Operator op, const Callback &cb) -> Clause & {
	if (this->state == State::None) { this->state = State::Some; } else { Query_writeOperator(this->query->stream, op); }
	this->query->stream << "(";
	auto state = this->state;
	this->state = State::None;
	WhereBegin tmp(this->query);
	cb(tmp);
	this->state = state;
	this->query->stream << ")";
	return (Clause &)*this;
}

template <typename Binder, typename Interface>
template <typename ... Args>
auto Query<Binder, Interface>::WhereBegin::where(Args && ... args) -> WhereContinue {
	WhereContinue q(this->query);
	q.where(sql::Operator::And, std::forward<Args>(args)...);
	return q;
}

template <typename Binder, typename Interface>
auto Query<Binder, Interface>::WhereBegin::where() -> WhereContinue {
	return WhereContinue(this->query);
}

template <typename Binder, typename Interface>
template <typename Callback>
auto Query<Binder, Interface>::WhereBegin::whereParentesis(const Callback &cb) -> WhereContinue {
	WhereContinue q(this->query);
	q.parenthesis(sql::Operator::And, cb);
	return q;
}

template <typename Binder, typename Interface>
template <typename Clause>
template <typename Value>
auto Query<Binder, Interface>::SetClause<Clause>::set(const StringView &f, Value &&v) -> Clause & {
	if (this->state == State::None) { this->state = State::Some; } else { this->query->stream << ","; }
	this->query->stream << " \"" << f << "\"=";
	this->query->writeBind(forward<Value>(v));
	return (Clause &)*this;
}

template <typename Binder, typename Interface>
template <typename Clause>
template <typename Value>
auto Query<Binder, Interface>::SetClause<Clause>::set(const StringView &t, const StringView &f, Value && v) -> Clause & {
	if (this->state == State::None) { this->state = State::Some; } else { this->query->stream << ","; }
	this->query->stream << " " << t << ".\"" << f << "\"=";
	this->query->writeBind(forward<Value>(v));
	return (Clause &)*this;
}

template <typename Binder, typename Interface>
template <typename Clause>
auto Query<Binder, Interface>::SetClause<Clause>::def(const StringView &f) -> Clause & {
	if (this->state == State::None) { this->state = State::Some; } else { this->query->stream << ","; }
	this->query->stream << " \"" << f << "\"=DEFAULT";
	return (Clause &)*this;
}

NS_SP_EXT_END(sql)

#endif /* COMMON_UTILS_SQL_SPSQLWHERE_H_ */
