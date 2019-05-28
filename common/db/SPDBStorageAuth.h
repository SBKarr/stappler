/**
Copyright (c) 2018-2019 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef COMMON_DB_SPDBSTORAGEAUTH_H_
#define COMMON_DB_SPDBSTORAGEAUTH_H_

#include "SPDBStorageQuery.h"

NS_DB_BEGIN

class Auth : public mem::AllocBase {
public:
	using NameFieldCallback = mem::Function<stappler::Pair<const Field *, mem::String>(const Scheme &, const mem::StringView &)>;

	Auth(const Scheme &);
	Auth(const Scheme &, const mem::StringView &name, const mem::StringView &password = mem::StringView());
	Auth(const Scheme &, const Field *name, const Field *password = nullptr);

	Auth(const Scheme &, const NameFieldCallback &, const Field *password);
	Auth(const Scheme &, const NameFieldCallback &, const mem::StringView &password = mem::StringView());

	const Scheme &getScheme() const;

	stappler::Pair<const Field *, mem::String> getNameField(const mem::StringView &) const;
	const Field *getPasswordField() const;

	bool authorizeWithPassword(const mem::StringView &input, const mem::Bytes &database, size_t tryCount) const;

protected:
	const Field *detectPasswordField(const Scheme &);

	const Field *_password = nullptr;
	const Field *_name = nullptr;
	NameFieldCallback _nameFieldCallback = nullptr;
	const Scheme *_scheme = nullptr;
};

NS_DB_END

#endif /* SERENITY_SRC_STORAGE_STORAGEAUTH_H_ */
