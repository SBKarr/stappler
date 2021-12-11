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

#ifndef STELLATOR_DB_STCONTINUETOKEN_H_
#define STELLATOR_DB_STCONTINUETOKEN_H_

#include "STStorageQuery.h"

NS_DB_BEGIN

class ContinueToken {
public:
	enum Flags {
		None = 0,
		Initial = 1,
		Reverse = 2,
		Inverted = 4,
	};

	ContinueToken() = default;
	ContinueToken(const mem::StringView &f, size_t count, bool reverse);
	ContinueToken(const mem::StringView &);

	ContinueToken(const ContinueToken &) = default;
	ContinueToken(ContinueToken &&) = default;

	ContinueToken &operator=(const ContinueToken &) = default;
	ContinueToken &operator=(ContinueToken &&) = default;

	operator bool () const { return !field.empty() && count > 0; }

	bool hasPrev() const;
	bool hasNext() const;
	bool isInit() const;

	mem::String encode() const;
	mem::Value perform(const Scheme &, const Transaction &, Query &);
	mem::Value perform(const Scheme &, const Transaction &, Query &, Ordering ord);

	mem::Value performOrdered(const Scheme &, const Transaction &, Query &);

	void refresh(const Scheme &, const Transaction &, Query &);

	mem::String encodeNext() const;
	mem::String encodePrev() const;

	size_t getStart() const;
	size_t getEnd() const;
	size_t getTotal() const;
	size_t getCount() const;
	size_t getFetched() const;
	mem::StringView getField() const;

	size_t getNumResults() const;

	bool hasFlag(Flags) const;
	void setFlag(Flags);
	void unsetFlag(Flags);

	const mem::Value &getFirstVec() const;
	const mem::Value &getLastVec() const;

protected:
	bool hasPrevImpl() const;
	bool hasNextImpl() const;

	mem::String encodeNextImpl() const;
	mem::String encodePrevImpl() const;

	bool _init = false;
	size_t _numResults = 0;
	mem::String field;

	mem::Value initVec;

	mem::Value firstVec;
	mem::Value lastVec;

	size_t count = 0;
	size_t fetched = 0;
	size_t total = 0;

	Flags flags = None;
};

SP_DEFINE_ENUM_AS_MASK(ContinueToken::Flags)

NS_DB_END

#endif /* STELLATOR_DB_STCONTINUETOKEN_H_ */
