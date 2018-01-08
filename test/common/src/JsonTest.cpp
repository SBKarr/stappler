// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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

#include "SPDefine.h"
#include "SPTime.h"
#include "SPCommon.h"
#include "SPString.h"
#include "SPData.h"
#include "Test.h"

static constexpr auto JsonNumberTestString(
R"JsonString({"neg1":-1;"neg2": -123456})JsonString");

NS_SP_BEGIN

struct PoolJsonTest : MemPoolTest {
	PoolJsonTest() : MemPoolTest("PoolJsonTest") { }

	virtual bool run(pool_t *pool) {
		StringStream stream;
		size_t count = 0;
		size_t passed = 0;
		stream << "\n";
		size_t ntests = 32;

		runTest(stream, "StreamJsonTest", count, passed, [&] {
			auto t = Time::now();
			auto d = data::readFile<memory::PoolInterface>(filesystem::currentDir("test/app.json"));

			stream << (Time::now() - t).toMicroseconds();
			return true;
		});

		runTest(stream, "StdJsonTest", count, passed, [&] {
			auto data = filesystem::readTextFile(filesystem::currentDir("test/app.json"));

			uint64_t v = 0;
			for (size_t i = 0; i < ntests; ++i) {
				auto t = Time::now();
				auto d1 = data::read(data);
				v += (Time::now() - t).toMicroseconds();
			}
			stream << v / ntests;
			return true;
		});

		runTest(stream, "PoolJsonTest", count, passed, [&] {
			memory::pool::clear(pool);
			auto data = filesystem::readTextFile(filesystem::currentDir("test/app.json"));

			uint64_t v = 0;
			for (size_t i = 0; i < ntests; ++i) {
				memory::pool::clear(pool);
				auto t = Time::now();
				auto d = data::read<std::string, memory::PoolInterface>(data);
				v += (Time::now() - t).toMicroseconds();
			}
			stream << v / ntests << " "
					 << memory::pool::get_allocated_bytes(pool) << " "
					 << memory::pool::get_return_bytes(pool) << " "
					 << memory::pool::get_opts_bytes(pool);
			return true;
		});

		runTest(stream, "CompareJsonTest", count, passed, [&] {
			memory::pool::clear(pool);
			auto t = Time::now();
			auto data = filesystem::readTextFile(filesystem::currentDir("test/app.json"));
			stream << (Time::now() - t).toMicroseconds() << " ";

			t = Time::now();
			auto d2 = data::read<std::string, memory::PoolInterface>(data);
			stream << (Time::now() - t).toMicroseconds() << " ";

			t = Time::now();
			auto d1 = data::read(data);
			stream << (Time::now() - t).toMicroseconds() << " ";

			bool ret = (data::Value(d2) == d1);
			stream << ret;

			return ret;
		});

		_desc = stream.str();

		return count == passed;
	}
} _PoolJsonTest;

struct JsonNumbersTest : Test {
	JsonNumbersTest() : Test("JsonNumbersTest") { }

	virtual bool run() override {
		StringStream stream;
		stream << "\n";

		auto d = data::read(String(JsonNumberTestString));

		stream << data::EncodeFormat::Pretty << d << "\n";

		_desc = stream.str();

		return true;
	}

} JsonNumbersTest;

NS_SP_END
