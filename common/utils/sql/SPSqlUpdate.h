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

#ifndef COMMON_UTILS_SQL_SPSQLUPDATE_H_
#define COMMON_UTILS_SQL_SPSQLUPDATE_H_

#include "SPSql.h"

NS_SP_EXT_BEGIN(sql)

template <typename Binder>
template <typename ... Args>
auto Query<Binder>::Update::where(Args && ... args) -> UpdateWhere {
	this->query->stream << " WHERE";
	UpdateWhere q(this->query);
	q.where(sql::Operator::And, forward<Args>(args)...);
	return q;
};

template <typename Binder>
auto Query<Binder>::Update::where() -> UpdateWhere {
	this->query->stream << " WHERE";
	return UpdateWhere(this->query);
}

template <typename Binder>
auto Query<Binder>::Update::returning() -> Returning {
	this->query->stream << " RETURNING";
	return Returning(this->query);
}

template <typename Binder>
auto Query<Binder>::UpdateWhere::returning() -> Returning {
	this->query->stream << " RETURNING";
	return Returning(this->query);
}

template <typename Binder>
auto Query<Binder>::update(const StringView & field) -> Update {
	stream << "UPDATE " << field << " SET";
	return Update(this);
}

template <typename Binder>
auto Query<Binder>::update(const StringView &field, const StringView &alias) -> Update {
	stream << "UPDATE " << field << " AS " << alias << " SET";
	return Update(this);
}


template <typename Binder>
template <typename ... Args>
auto Query<Binder>::Delete::where(Args && ... args) -> DeleteWhere {
	this->query->stream << " WHERE";
	DeleteWhere q(this->query);
	q.where(sql::Operator::And, forward<Args>(args)...);
	return q;
};

template <typename Binder>
auto Query<Binder>::Delete::where() -> DeleteWhere {
	this->query->stream << " WHERE";
	return DeleteWhere(this->query);
}

template <typename Binder>
auto Query<Binder>::Delete::returning() -> Returning {
	this->query->stream << " RETURNING";
	return Returning(this->query);
}

template <typename Binder>
auto Query<Binder>::DeleteWhere::returning() -> Returning {
	this->query->stream << " RETURNING";
	return Returning(this->query);
}

template <typename Binder>
auto Query<Binder>::remove(const StringView & field) -> Delete {
	stream << "DELETE FROM " << field;
	return Delete(this);
}

template <typename Binder>
auto Query<Binder>::remove(const StringView &field, const StringView &alias) -> Delete {
	stream << "DELETE FROM " << field << " AS " << alias;
	return Delete(this);
}

NS_SP_EXT_END(sql)

#endif /* COMMON_UTILS_SQL_SPSQLUPDATE_H_ */
