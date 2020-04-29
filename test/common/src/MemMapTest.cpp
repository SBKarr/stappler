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
#include "Test.h"

NS_SP_BEGIN

struct MemMapTest : MemPoolTest {
	MemMapTest() : MemPoolTest("MemMapTest") { }

	virtual bool run(pool_t *pool) {
		StringStream stream;
		size_t count = 0;
		size_t passed = 0;
		stream << "\n";

		memory::map<size_t, String> vec; vec.reserve(5);
		vec.emplace(1, "One");
		vec.emplace(2, "Two");
		vec.emplace(4, "FourFour");
		vec.emplace(5, "Five");
		vec.emplace(9, "Nine");

		memory::map<size_t, String> vec2; vec2.reserve(10);
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
			memory::map<size_t, String> data(vec);
			for (auto &it : data) {
				stream << it.first << " " << it.second << " ";
			}

			return vec == data;
		});

		runTest(stream, "emplace test", count, passed, [&] {
			memory::map<size_t, String> data; data.reserve(6);
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

		runTest(stream, "ptrset test", count, passed, [&] {
			memory::set<memory::map<size_t, String> *> data;

			data.emplace(&vec);
			data.emplace(&vec2);

			return data.find(&vec) != data.end() && data.find(&vec2) != data.end();
		});


		_desc = stream.str();

		return count == passed;
	}
} _MemMapTest;


NS_SP_END
