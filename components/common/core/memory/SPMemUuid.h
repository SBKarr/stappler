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

#ifndef COMMON_CORE_MEMORY_SPMEMUUID_H_
#define COMMON_CORE_MEMORY_SPMEMUUID_H_

#include "SPBytesView.h"
#include "SPMemString.h"
#include "SPMemVector.h"
#include "SPStringView.h"

NS_SP_EXT_BEGIN(memory)

struct uuid : AllocPool {
	static constexpr size_t FormattedLength = 36;

	using uuid_t = std::array<uint8_t, 16>;

	static bool parse(uuid_t &, const char *);
	static void format(char *, const uuid_t &);
	static uuid generate();

	uuid() {
		memset(_uuid.data(), 0, 16);
	}

	uuid(StringView str) {
		parse(_uuid, str.data());
	}

	template <ByteOrder::Endian Order>
	uuid(BytesViewTemplate<Order> b) {
		if (b.size() == 16) {
			memcpy(_uuid.data(), b.data(), 16);
		}
	}

	uuid(const uuid_t &u) : _uuid(u) { }
	uuid(const uuid &u) : _uuid(u._uuid) { }

	uuid &operator= (const uuid &u) { _uuid = u._uuid; return *this; }

	uuid &operator= (const memory::string &str) {
		parse(_uuid, str.data());
		return *this;
	}

	uuid &operator= (const std::string &str) {
		parse(_uuid, str.data());
		return *this;
	}

	uuid &operator= (const memory::vector<uint8_t> &b) {
		if (b.size() == 16) {
			memcpy(_uuid.data(), b.data(), 16);
		}
		return *this;
	}

	uuid &operator= (const std::vector<uint8_t> &b) {
		if (b.size() == 16) {
			memcpy(_uuid.data(), b.data(), 16);
		}
		return *this;
	}

	template <typename Str = string>
	auto str() const -> Str {
		char buf[FormattedLength] = { 0 };
		format(buf, _uuid);
		return Str(buf, FormattedLength);
	}

	template <typename B = memory::vector<uint8_t>>
	auto bytes() const -> B {
		return B(_uuid.data(), _uuid.data() + 16);
	}

	uuid_t array() const {
		return _uuid;
	}

	const uint8_t *data() const { return _uuid.data(); }
	size_t size() const { return 16; }

	uuid_t _uuid;
};

NS_SP_EXT_END(memory)

#endif /* COMMON_CORE_MEMORY_SPMEMUUID_H_ */
