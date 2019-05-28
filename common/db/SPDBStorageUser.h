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

#ifndef COMMON_DB_SPDBSTORAGEUSER_H_
#define COMMON_DB_SPDBSTORAGEUSER_H_

#include "SPDBStorageObject.h"

NS_DB_BEGIN

class User : public Object {
public:
	static User *create(const Adapter &, const mem::StringView &name, const mem::StringView &password);
	static User *setup(const Adapter &, const mem::StringView &name, const mem::StringView &password);
	static User *create(const Adapter &, mem::Value &&);

	static User *get(const Adapter &, const mem::StringView &name, const mem::StringView &password);
	static User *get(const Adapter &, const Scheme &scheme, const mem::StringView &name, const mem::StringView &password);

	static User *get(const Adapter &, uint64_t oid);
	static User *get(const Adapter &, const Scheme &scheme, uint64_t oid);

	User(mem::Value &&, const Scheme &);

	bool validatePassword(const mem::StringView &passwd) const;
	void setPassword(const mem::StringView &passwd);

	bool isAdmin() const;

	mem::StringView getName() const;
};

NS_DB_END

#endif /* COMMON_DB_SPDBSTORAGEUSER_H_ */
