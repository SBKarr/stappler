/**
Copyright (c) 2018 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef SERENITY_SRC_STORAGE_STORAGEAUTH_H_
#define SERENITY_SRC_STORAGE_STORAGEAUTH_H_

#include "StorageQuery.h"

NS_SA_EXT_BEGIN(storage)

class Auth {
public:
	using NameFieldCallback = Function<Pair<const Field *, String>(const Scheme &, const StringView &)>;

	Auth(const Scheme &);
	Auth(const Scheme &, const StringView &name, const StringView &password = StringView());
	Auth(const Scheme &, const Field *name, const Field *password = nullptr);

	Auth(const Scheme &, const NameFieldCallback &, const Field *password);
	Auth(const Scheme &, const NameFieldCallback &, const StringView &password = StringView());

	const Scheme &getScheme() const;

	Pair<const Field *, String> getNameField(const StringView &) const;
	const Field *getPasswordField() const;

	bool authorizeWithPassword(const StringView &input, const Bytes &database, size_t tryCount) const;

protected:
	const Field *detectPasswordField(const Scheme &);

	const Field *_password = nullptr;
	const Field *_name = nullptr;
	NameFieldCallback _nameFieldCallback = nullptr;
	const Scheme *_scheme = nullptr;
};

NS_SA_EXT_END(storage)

#endif /* SERENITY_SRC_STORAGE_STORAGEAUTH_H_ */
