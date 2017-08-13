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

#include "SPCommon.h"
#include "SPString.h"
#include "SPData.h"
#include "Test.h"

NS_SP_BEGIN

struct MemVectorTest : MemPoolTest {
	MemVectorTest() : MemPoolTest("MemVectorTest") { }

	virtual bool run(pool_t *pool) {
		StringStream stream;
		size_t count = 0;
		size_t passed = 0;
		stream << "\n";

		runTest(stream, "Vector<Vector<String>>", count, passed, [&] {
			Vector<Vector<String>> vec;
			vec.reserve(1);

			for (size_t i = 0; i < 5; ++ i) {
				vec.emplace_back(Vector<String>());
				auto &val = vec.back();
				val.reserve(1);
				val.emplace_back("test " + string::ToStringTraits<memory::PoolInterface>::toString(i));
			}

			for (auto &it : vec) {
				stream << it.at(0) << " ";
			}

			return true;
		});

		runTest(stream, "Vector<String>", count, passed, [&] {
			Vector<String> vec;
			vec.emplace_back("FirstString");
			vec.emplace_back("SecondString");
			vec.emplace_back("ThirdString");

			vec.emplace(vec.begin() + 1, "InsertString");

			stream << "sizeof: " << sizeof(String);

			return true;
		});

		runTest(stream, "Vector<data::Value>", count, passed, [&] {
			Vector<Value> vec;
			vec.emplace_back(Value(true));
			vec.emplace_back(Value(0.5));
			vec.emplace_back(Value("test"));

			vec.emplace(vec.begin() + 1, Value(1234));

			stream << "sizeof: " << sizeof(Value);

			return true;
		});

		_desc = stream.str();

		return count == passed;
	}
} _MemVectorTest;

NS_SP_END
