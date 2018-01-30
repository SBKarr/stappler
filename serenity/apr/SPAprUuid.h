/**
Copyright (c) 2016 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef COMMON_APR_SPAPRUUID_H_
#define COMMON_APR_SPAPRUUID_H_

#include "SPMemString.h"

#if SPAPR

NS_SP_EXT_BEGIN(apr)

using string = memory::string;

struct uuid : AllocPool {
	static uuid generate() {
		apr_uuid_t ret;
		apr_uuid_get(&ret);
		return ret;
	}

	uuid() {
		memset(_uuid.data, 0, 16);
	}

	uuid(const string &str) {
		apr_uuid_parse(&_uuid, str.data());
	}

	uuid(const Bytes &b) {
		if (b.size() == 16) {
			memcpy(_uuid.data, b.data(), 16);
		}
	}

	uuid(const apr_uuid_t &u) : _uuid(u) { }
	uuid(const uuid &u) : _uuid(u._uuid) { }
	uuid &operator= (const uuid &u) { _uuid = u._uuid; return *this; }
	uuid &operator= (const Bytes &b) {
		if (b.size() == 16) {
			memcpy(_uuid.data, b.data(), 16);
		}
		return *this;
	}

	string str() const {
		string ret; ret.resize(APR_UUID_FORMATTED_LENGTH);
		apr_uuid_format(ret.data(), &_uuid);
		return ret;
	}

	Bytes bytes() const {
		Bytes ret; ret.resize(16);
		memcpy(ret.data(), _uuid.data, 16);
		return ret;
	}

	std::array<uint8_t, 16> array() const {
		std::array<uint8_t, 16> ret;
		memcpy(ret.data(), _uuid.data, 16);
		return ret;
	}

	const uint8_t *data() const { return _uuid.data; }
	size_t size() const { return 16; }

	apr_uuid_t _uuid;
};

NS_SP_EXT_END(apr)
#endif

#endif /* COMMON_APR_SPAPRUUID_H_ */
