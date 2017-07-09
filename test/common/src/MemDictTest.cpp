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

struct MemDictTest : MemPoolTest {
	MemDictTest() : MemPoolTest("MemDictTest") { }

	virtual bool run(pool_t *pool) {
		StringStream stream;
		size_t count = 0;
		size_t passed = 0;
		stream << "\n";

		memory::dict<size_t, String> vec;
		vec.emplace(1, "One");
		vec.emplace(2, "Two");
		vec.emplace(4, "FourFour");
		vec.emplace(5, "Five");
		vec.emplace(9, "Nine");

		memory::dict<size_t, String> vec2;
		vec2.emplace(1, "One");
		vec2.emplace(2, "Two");
		vec2.emplace(3, "Three");
		vec2.emplace(4, "FourFour");
		vec2.emplace(5, "Five");
		vec2.emplace(6, "Six");
		vec2.emplace(7, "Seven");
		vec2.emplace(9, "Nine");
		vec2.emplace(10, "Ten");

		runTest(stream, "copy test", count, passed, [&] {
			memory::dict<size_t, String> data(vec);
			for (auto &it : data) {
				stream << it.first << " " << it.second << " ";
			}

			return vec == data;
		});

		runTest(stream, "emplace test", count, passed, [&] {
			memory::dict<size_t, String> data;
			data.emplace(4, "Four");
			data.emplace(1, "One");
			data.emplace(2, "Two");
			data.emplace(9, "Nine");
			data.emplace(5, "Five");
			data.emplace(4, "FourFour");

			for (auto &it : data) {
				stream << it.first << " " << it.second << " ";
			}

			return vec == data;
		});

		runTest(stream, "find test", count, passed, [&] {
			auto it1 = vec.find(4);
			auto it2 = vec.find(9);
			auto it3 = vec.find(1);
			auto it4 = vec.find(8);

			return (it1->first == 4 && it1->second == "FourFour")
					&& (it2->first == 9 && it2->second == "Nine")
					&& (it3->first == 1 && it3->second == "One")
					&& it4 == vec.end();
		});

		runTest(stream, "emplace_hint test", count, passed, [&] {
			memory::dict<size_t, String> data(vec);
			data.emplace_hint(data.end(), 10, "Ten");
			data.emplace_hint(data.end(), 7, "Seven");
			data.emplace_hint(data.find(7), 6, "Six");
			data.emplace_hint(data.find(7), 3, "Three");

			for (auto &it : data) {
				stream << it.second << " ";
			}
			return data == vec2;
		});

		runTest(stream, "emplace sub test", count, passed, [&] {
			data::ValueTemplate<memory::PoolInterface> val;
			val.setString("One", "One");
			val.setString("Two", "Two");
			val.setString("Three", "Three");
			val.setString("Four", "Four");

			val.asDict().reserve(10);

			val.setValue(std::move(val.getValue("Three")), "Green");
			stream << data::EncodeFormat::Pretty << val << "\n";

			data::ValueTemplate<memory::PoolInterface> test;
			test.setString("Four", "Four");
			test.setString("Three", "Green");
			test.setString("One", "One");
			test.setValue(data::Value(), "Three");
			test.setString("Two", "Two");

			return val == test;
		});

		_desc = stream.str();

		return count == passed;
	}
} _MemDictTest;

NS_SP_END
