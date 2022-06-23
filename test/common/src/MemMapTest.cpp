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

struct MemMapIntTest {
	int64_t first;
	int64_t second;

	bool operator == (const MemMapIntTest &r) const {
		return first == r.first && second == r.second;
	}
	bool operator != (const MemMapIntTest &r) const {
		return first != r.first || second != r.second;
	}
};

bool operator < (const MemMapIntTest &l, const MemMapIntTest &r) {
	return (l.first == r.first) ? l.second < r.second : (l.first < r.first);
}

std::ostream &operator<<(std::ostream &out, const MemMapIntTest &loc) {
	out << "(" << loc.first << "," << loc.second << ")";
	return out;
}

struct MemMapTest : MemPoolTest {
	MemMapTest() : MemPoolTest("MemMapTest") { }

	virtual bool run(pool_t *pool) {
		StringStream stream;
		size_t count = 0;
		size_t passed = 0;
		stream << "\n";

		/*memory::set<String> setTest;
		setTest.emplace("One");
		setTest.emplace("Two");
		setTest.emplace("FourFour");
		setTest.emplace("Five");
		setTest.emplace("Nine");

		memory::rbtree::TreeDebug::visit(setTest._tree, std::cout);

		do { auto it = setTest.find("One"); if (it == setTest.end()) { std::cout << "Failed One\n"; } } while (0);
		do { auto it = setTest.find("Two"); if (it == setTest.end()) { std::cout << "Failed Two\n"; } } while (0);
		do { auto it = setTest.find("FourFour"); if (it == setTest.end()) { std::cout << "Failed FourFour\n"; } } while (0);
		do { auto it = setTest.find("Five"); if (it == setTest.end()) { std::cout << "Failed Five\n"; } } while (0);
		do { auto it = setTest.find("Nine"); if (it == setTest.end()) { std::cout << "Failed Nine\n"; } } while (0);*/

		runTest(stream, "lower_bound test", count, passed, [&] {
			memory::set<int> values;
			values.emplace(1);
			values.emplace(3);

			auto it = values.lower_bound(4);
			std::cout << *(--it) << "\n";
			return  *(--it) == 3;
		});

		runTest(stream, "persistent test", count, passed, [&] {
			memory::map<size_t, String> data; data.reserve(4);
			data.set_memory_persistent(true);
			data.emplace(4, "Four");
			data.emplace(1, "One");
			data.emplace(2, "Two");
			data.emplace(9, "Nine");

			data.emplace(5, "Five");
			data.emplace(4, "FourFour");
			data.emplace(6, "Six");
			data.emplace(7, "Seven");

			stream << data.capacity() << " " << data.size() << " ";

			if (data.capacity() != data.size() && data.size() != 7) {
				return false;
			}

			data.erase(7);
			data.erase(5);
			data.erase(4);
			data.erase(6);
			data.erase(9);

			// should remove extra nodes but not preallocated nodes
			data.shrink_to_fit();

			stream << data.capacity() << " " << data.size() << " ";

			if (data.capacity() > 4) {
				return false;
			}

			data.emplace(9, "Nine");
			data.emplace(5, "Five");
			data.emplace(4, "FourFour");
			data.emplace(6, "Six");
			data.emplace(7, "Seven");

			data.clear();

			stream << data.capacity() << " " << data.size() << " ";

			if (data.capacity() == 7 && data.size() != 0) {
				return false;
			}

			data.shrink_to_fit();

			stream << data.capacity() << " " << data.size();

			return data.capacity() == 0 && data.size() == 0;
		});

		runTest(stream, "shrink_to_fit test", count, passed, [&] {
			memory::map<size_t, String> data; data.reserve(6);
			data.emplace(4, "Four");
			data.emplace(1, "One");
			data.emplace(2, "Two");
			data.emplace(9, "Nine");

			data.emplace(5, "Five");
			data.emplace(4, "FourFour");
			data.emplace(6, "Six");
			data.emplace(7, "Seven");

			stream << data.capacity() << " " << data.size() << " ";

			if (data.capacity() != data.size() && data.size() != 7) {
				return false;
			}

			data.clear();

			stream << data.capacity() << " " << data.size() << " ";

			// extra node may or may not persists, but preallocated nodes should be preserved
			if (data.capacity() < 6 && data.size() != 0) {
				return false;
			}

			data.shrink_to_fit();

			stream << data.capacity() << " " << data.size();

			return data.capacity() == 0 && data.size() == 0;
		});

		memory::set<int64_t> intSetTest;
		intSetTest.emplace(2);
		intSetTest.emplace(4);
		intSetTest.emplace(1);
		do {
			//memory::rbtree::TreeDebug::visit(intSetTest._tree, std::cout);
			auto it1 = intSetTest.find(2);
			if (it1 == intSetTest.end()) {
				std::cout << "Failed 1\n";
			}

			auto it2 = intSetTest.find(4);
			if (it2 == intSetTest.end()) {
				std::cout << "Failed 2\n";
			}

			auto it3 = intSetTest.find(1);
			if (it3 == intSetTest.end()) {
				std::cout << "Failed 3\n";
			}
		} while(0);

		memory::set<MemMapIntTest> empTest;
		empTest.emplace(MemMapIntTest{420532968332,16623061247});
		empTest.emplace(MemMapIntTest{421702536327,0});
		empTest.emplace(MemMapIntTest{0,0});

		auto it1 = empTest.find(MemMapIntTest{420532968332,16623061247});
		if (it1 == empTest.end()) {
			std::cout << "Failed 1\n";
		}

		auto it2 = empTest.find(MemMapIntTest{421702536327,0});
		if (it2 == empTest.end()) {
			std::cout << "Failed 2\n";
		}

		auto it3 = empTest.find(MemMapIntTest{0,0});
		if (it3 == empTest.end()) {
			std::cout << "Failed 3\n";
		}

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

			stream << data.capacity() << " " << data.size();

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
