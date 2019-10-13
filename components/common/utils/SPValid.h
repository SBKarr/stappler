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

#ifndef COMMON_UTILS_SPVALID_H_
#define COMMON_UTILS_SPVALID_H_

#include "SPCommon.h"
#include "SPStringView.h"

NS_SP_EXT_BEGIN(valid)

using String = memory::PoolInterface::StringType;
using Bytes = memory::PoolInterface::BytesType;

String idnToAscii(const StringView &, bool validate = true);
String idnToUnicode(const StringView &, bool validate = false);

/** Identifier starts with [a-zA-Z_] and can contain [a-zA-Z0-9_\-.@] */
bool validateIdentifier(const StringView &str);

/** Text can contain all characters above 0x1F and \t, \r, \n, \b, \f */
bool validateText(const StringView &str);

bool validateEmail(String &str);
bool validateUrl(String &str);

bool validateNumber(const StringView &str);
bool validateHexadecimial(const StringView &str);
bool validateBase64(const StringView &str);

void makeRandomBytes(uint8_t *, size_t);

template <typename Interface = memory::DefaultInterface>
auto makeRandomBytes(size_t) -> typename Interface::BytesType;

template <typename Interface = memory::DefaultInterface>
auto makePassword(const StringView &str, const StringView &key = StringView()) -> typename Interface::BytesType;

bool validatePassord(const StringView &str, const BytesView &passwd, const StringView &key = StringView());

// Minimal length is 6
template <typename Interface = memory::DefaultInterface>
auto generatePassword(size_t len) -> typename Interface::StringType;

template <typename Interface = memory::DefaultInterface>
auto convertOpenSSHKey(const StringView &) -> typename Interface::BytesType;

NS_SP_EXT_END(valid)

#endif /* COMMON_UTILS_SPVALID_H_ */
