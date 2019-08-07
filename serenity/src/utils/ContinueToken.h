/**
Copyright (c) 2019 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef SERENITY_SRC_UTILS_CONTINUETOKEN_H_
#define SERENITY_SRC_UTILS_CONTINUETOKEN_H_

#include "Define.h"

NS_SA_BEGIN

class ContinueToken {
public:
	using Scheme = storage::Scheme;
	using Transaction = storage::Transaction;
	using Query = storage::Query;

	enum Flags {
		None = 0,
		Initial = 1,
		Reverse = 2,
		Inverted = 4,
	};

	ContinueToken() = default;
	ContinueToken(const StringView &f, size_t count, bool reverse);
	ContinueToken(const StringView &);

	ContinueToken(const ContinueToken &) = default;
	ContinueToken(ContinueToken &&) = default;

	ContinueToken &operator=(const ContinueToken &) = default;
	ContinueToken &operator=(ContinueToken &&) = default;

	bool hasPrev() const;
	bool hasNext() const;
	bool isInit() const;

	String encode() const;
	data::Value perform(const Scheme &, const Transaction &, Query &);
	data::Value perform(const Scheme &, const Transaction &, Query &, storage::Ordering ord);

	data::Value performOrdered(const Scheme &, const Transaction &, Query &);

	String encodeNext() const;
	String encodePrev() const;

	size_t getStart() const;
	size_t getEnd() const;
	size_t getTotal() const;

	size_t getNumResults() const;

	bool hasFlag(Flags) const;
	void setFlag(Flags);
	void unsetFlag(Flags);

protected:
	bool hasPrevImpl() const;
	bool hasNextImpl() const;

	String encodeNextImpl() const;
	String encodePrevImpl() const;

	bool _init = false;
	size_t _numResults = 0;
	String field;

	data::Value initVec;

	data::Value firstVec;
	data::Value lastVec;

	size_t count = 0;
	size_t fetched = 0;
	size_t total = 0;

	Flags flags = None;
};

SP_DEFINE_ENUM_AS_MASK(ContinueToken::Flags)

NS_SA_END

#endif /* SERENITY_SRC_UTILS_CONTINUETOKEN_H_ */
