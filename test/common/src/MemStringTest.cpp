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

struct MemStringCtorTest : MemPoolTest {
	MemStringCtorTest() : MemPoolTest("MemStringCtorTest") { }

	virtual bool run(pool_t *pool) {
		StringStream stream;
		size_t count = 0;
		size_t passed = 0;
		stream << "\n";

		runTest(stream, "Default ctor test", count, passed, [&] {
			String s;
			return s.empty() && (s.length() == 0) && (s.size() == 0);
		});

		runTest(stream, "Fill ctor test", count, passed, [&] {
			String s(4, '=');
			stream << s.data();
			return s.size() == 4 && strcmp(s.data(), "====") == 0;
		});

		runTest(stream, "Substring ctor test", count, passed, [&] {
			const String other("Exemplary");
			String s(other, 1, other.length() - 2);
			stream << s.data();
			return s.size() == "xemplar"_len && strcmp(s.data(), "xemplar") == 0;
		});

		runTest(stream, "C-style ctor test", count, passed, [&] {
			String s("C-style string", 7);
			stream << s.data();
			return s.size() == "C-style"_len && strcmp(s.data(), "C-style") == 0;
		});

		runTest(stream, "C-style ctor-with-zero test", count, passed, [&] {
			String s("C-style\0string");
			stream << s.data();
			return s.size() == "C-style"_len && strcmp(s.data(), "C-style") == 0;
		});

		runTest(stream, "Input iterator ctor test", count, passed, [&] {
			char mutable_c_str[] = "another C-style string";
			String s(std::begin(mutable_c_str) + 8, std::end(mutable_c_str) - 1);
			stream << s.data();
			return s.size() == "C-style string"_len && strcmp(s.data(), "C-style string") == 0;
		});

		runTest(stream, "Copy ctor test", count, passed, [&] {
			const String other("Exemplar");
			String s(other);
			stream << s.data();
			return s.size() == "Exemplar"_len && strcmp(s.data(), "Exemplar") == 0;
		});

		runTest(stream, "Move ctor test", count, passed, [&] {
			String s(String("C++ by ") + String("example"));
			stream << s.data();
			return s.size() == "C++ by example"_len && strcmp(s.data(), "C++ by example") == 0;
		});

		runTest(stream, "Init list ctor test", count, passed, [&] {
			String s({ 'C', '-', 's', 't', 'y', 'l', 'e' });
			stream << s.data();
			return s.size() == "C-style"_len && strcmp(s.data(), "C-style") == 0;
		});

		runTest(stream, "Init list ctor test", count, passed, [&] {
			String s(3, std::toupper('a'));
			stream << s.data();
			return s.size() == "AAA"_len && strcmp(s.data(), "AAA") == 0;
		});

		String s;
		runTest(stream, "Fill assign test", count, passed, [&] {
			s.assign(4, '=');
			stream << s.data();
			return s.size() == 4 && strcmp(s.data(), "====") == 0;
		});

		runTest(stream, "Copy assign test", count, passed, [&] {
			const String c("Exemplary");
			s.assign(c);
			stream << s.data();
			return s.size() == "Exemplary"_len && strcmp(s.data(), "Exemplary") == 0;
		});

		runTest(stream, "Substring assign test", count, passed, [&] {
			const String c("Exemplary");
			s.assign(c, 1, c.length() - 2);
			stream << s.data();
			return s.size() == "xemplar"_len && strcmp(s.data(), "xemplar") == 0;
		});

		runTest(stream, "Move assign test", count, passed, [&] {
			s.assign(String("C++ by ") + "example");
			stream << s.data();
			return s.size() == "C++ by example"_len && strcmp(s.data(), "C++ by example") == 0;
		});

		runTest(stream, "C-style assign test", count, passed, [&] {
			s.assign("C-style string", 7);
			stream << s.data();
			return s.size() == "C-style"_len && strcmp(s.data(), "C-style") == 0;
		});

		runTest(stream, "C-style-zero assign test", count, passed, [&] {
			s.assign("C-style\0string");
			stream << s.data();
			return s.size() == "C-style"_len && strcmp(s.data(), "C-style") == 0;
		});

		runTest(stream, "InputIter assign test", count, passed, [&] {
			s.assign({ 'C', '-', 's', 't', 'y', 'l', 'e' });
			stream << s.data();
			return s.size() == "C-style"_len && strcmp(s.data(), "C-style") == 0;
		});

		_desc = stream.str();

		return count == passed;
	}
} _MemStringCtorTest;

struct MemStringDataTest : MemPoolTest {
	MemStringDataTest() : MemPoolTest("MemStringDataTest") { }

	virtual bool run(pool_t *pool) {
		StringStream stream;
		size_t count = 0;
		size_t passed = 0;
		stream << "\n";

		runTest(stream, "back() test", count, passed, [&] {
			String s("Exemplary");
			char& back = s.back();
			back = 's';
			stream << s.data();
			return s.size() == "Exemplars"_len && strcmp(s.data(), "Exemplars") == 0;
		});

		runTest(stream, "const back() test", count, passed, [&] {
			const String s("Exemplary");
			char const& back = s.back();
			stream << back;
			return back == 'y';
		});

		runTest(stream, "front() test", count, passed, [&] {
			String s("Exemplary");
			char& fr = s.front();
			fr = 'e';
			stream << s.data();
			return s.size() == "exemplary"_len && strcmp(s.data(), "exemplary") == 0;
		});

		runTest(stream, "const front() test", count, passed, [&] {
			const String s("Exemplary");
			char const& fr = s.front();
			stream << fr;
			return fr == 'E';
		});

		runTest(stream, "data() test", count, passed, [&] {
			const String s("Emplary");

			bool l = s.size() == ::strlen(s.data());
			bool eq1 = std::equal(s.begin(), s.end(), s.data());
			bool eq2 = std::equal(s.data(), s.data() + s.size(), s.begin());
			bool nterm = 0 == *(s.data() + s.size());

			stream << "length: " << l << ", "
					<< "eq1: " << eq1 << ", "
					<< "eq2: " << eq2 << ", "
					<< "nterm: " << nterm;

			return l && eq1 && eq2 && nterm;
		});

		runTest(stream, "operator[] test", count, passed, [&] {
			const String e("Exemplar");

			String r;
			for (unsigned i = e.length() - 1; i != 0; i /= 2) {
				r.push_back(e[i]);
			}

			bool idx = r.size() == "rmx"_len && strcmp(r.data(), "rmx") == 0;
			bool cstr = strcmp(&e[0], "Exemplar") == 0;

			std::string s("Exemplar ");
			s[s.size()-1] = 'y';
			bool set = s.size() == "Exemplary"_len && strcmp(s.data(), "Exemplary") == 0;

			stream << r << " " << &e[0] << " " << s;

			return idx && cstr && set;
		});

		runTest(stream, "at() test", count, passed, [&] {
			String s("message"); // for capacity

			s = "abc";
			s.at(2) = 'x';

			bool set = s.size() == "abx"_len && strcmp(s.data(), "abx") == 0;
			bool cap = (s.capacity() == 7);
			stream << s << " cap: " << s.capacity() << ", size: " << s.size();
			return set && cap;
		});

		_desc = stream.str();

		return count == passed;
	}
} _MemStringDataTest;

struct MemStringIterTest : MemPoolTest {
	MemStringIterTest() : MemPoolTest("MemStringIterTest") { }

	virtual bool run(pool_t *pool) {
		StringStream stream;
		size_t count = 0;
		size_t passed = 0;
		stream << "\n";

		runTest(stream, "begin() test", count, passed, [&] {
			String s("Exemplar");
			*s.begin() = 'e';

			bool b = s.size() == "exemplar"_len && strcmp(s.data(), "exemplar") == 0;
			bool c = *s.cbegin() == 'e';

			stream << s << " " << *s.cbegin();

			return b && c;
		});

		runTest(stream, "end() test", count, passed, [&] {
			String s("Exemparl");
			std::next_permutation(s.begin(), s.end());

			bool b = s.size() == "Exemplar"_len && strcmp(s.data(), "Exemplar") == 0;

			String c;
			std::copy(s.cbegin(), s.cend(), std::back_inserter(c));
			bool e = c.size() == "Exemplar"_len && strcmp(c.data(), "Exemplar") == 0;

			stream << c;

			return b && e;
		});

		runTest(stream, "rbegin() test", count, passed, [&] {
		    String s("Exemplar!");
		    *s.rbegin() = 'y';
			bool b = s.size() == "Exemplary"_len && strcmp(s.data(), "Exemplary") == 0;
			stream << s << " ";

		    String c;
		    std::copy(s.crbegin(), s.crend(), std::back_inserter(c));
		    bool e = c.size() == "yralpmexE"_len && strcmp(c.data(), "yralpmexE") == 0;
			stream << c;

			return b && e;
		});

		runTest(stream, "rend() test", count, passed, [&] {
			String s("A man, a plan, a canal: Panama");

		    String c1;
		    std::copy(s.rbegin(), s.rend(), std::back_inserter(c1));

			bool b = c1.size() == "amanaP :lanac a ,nalp a ,nam A"_len && strcmp(c1.data(), "amanaP :lanac a ,nalp a ,nam A") == 0;
			stream << c1;

		    String c2;
		    std::copy(s.crbegin(), s.crend(), std::back_inserter(c2));

			bool c = c2.size() == "amanaP :lanac a ,nalp a ,nam A"_len && strcmp(c2.data(), "amanaP :lanac a ,nalp a ,nam A") == 0;

			return b && c;
		});

		_desc = stream.str();

		return count == passed;
	}
} _MemStringIterTest;

struct MemStringMetaTest : MemPoolTest {
	MemStringMetaTest() : MemPoolTest("MemStringMetaTest") { }

	virtual bool run(pool_t *pool) {
		StringStream stream;
		size_t count = 0;
		size_t passed = 0;
		stream << "\n";

		runTest(stream, "empty() test", count, passed, [&] {
			String s;
			bool t1 = s.empty();
			stream << "'" << s << "', ";

			s = "Exemplar";
			bool t2 = !s.empty();
			stream << "'" << s << "', ";

			s = "";
			bool t3 = s.empty();
			stream << "'" << s << "'";

			return t1 && t2 && t3;
		});

		runTest(stream, "size() test", count, passed, [&] {
			String s("Exemplar");
			stream << s.size() << ", " << s.length() << ", " << std::distance(s.begin(), s.end());
			return (8 == s.size()) && (s.size() == s.length())
					&& (s.size() == static_cast<std::string::size_type>(std::distance(s.begin(), s.end())));
		});

		runTest(stream, "capacity() test", count, passed, [&] {
			String s {"Exemplar"};
			bool t1 = s.capacity() == 8;

			s += " is an example string.";
			bool t2 = s.capacity() >= 30;

			stream << s << " " << s.capacity();
			return t1 && t2;
		});

		runTest(stream, "reserve() test", count, passed, [&] {
			String s;
			String::size_type new_capacity {100u};
			bool t1 = (new_capacity > s.capacity());

			s.reserve(new_capacity);
			bool t2 = (new_capacity <= s.capacity());
			stream << s.capacity();
			return t1 && t2;
		});

		runTest(stream, "shrink_to_fit() test", count, passed, [&] {
			String s;
			bool t1 = s.capacity() == 0;
			stream << s.capacity() << ", ";
			s.resize(100);
			bool t2 = s.capacity() >= 100;
			stream << s.capacity() << ", ";
			s.clear();
			bool t3 = s.capacity() >= 100;
			stream << s.capacity() << ", ";
			s.shrink_to_fit();
			bool t4 = s.capacity() == 0;
			stream << s.capacity() << "";
			return t1 && t2 && t3 && t4;
		});

		_desc = stream.str();

		return count == passed;
	}
} _MemStringMetaTest;

struct MemStringModifierTest : MemPoolTest {
	MemStringModifierTest() : MemPoolTest("MemStringModifierTest") { }

	virtual bool run(pool_t *pool) {
		StringStream stream;
		size_t count = 0;
		size_t passed = 0;
		stream << "\n";

		runTest(stream, "clear() test", count, passed, [&] {
			String s {"Exemplar"};
			String::size_type const capacity = s.capacity();

			stream << "'" << s << "' " << capacity << " ";
			s.clear();
			stream << "'" << s << "' " << capacity;
			return (s.capacity() == capacity) && (s.empty()) && (s.size() == 0);
		});

		runTest(stream, "push_back() test", count, passed, [&] {
			String s("Exem");
			s.push_back('p');
			s.push_back('l');
			s.push_back('a');
			s.push_back('r');

			stream << s << " " << s.capacity() << " " << s.size();

			return s.size() == "Exemplar"_len && strcmp(s.data(), "Exemplar") == 0 && s.capacity() >= s.size();
		});

		runTest(stream, "pop_back() test", count, passed, [&] {
			String s("Exemplar");
			auto cap = s.capacity();
			s.pop_back();
			s.pop_back();
			s.pop_back();
			s.pop_back();

			stream << s << " " << s.capacity() << " " << s.size();

			return s.size() == "Exem"_len && strcmp(s.data(), "Exem") == 0 && s.capacity() == cap;
		});

		String a = "0123456789abcdefghij";
		runTest(stream, "substr(size_t) test", count, passed, [&] {
			String sub1 = a.substr(10);
			stream << sub1;
			return sub1.size() == "abcdefghij"_len && strcmp(sub1.data(), "abcdefghij") == 0;
		});

		runTest(stream, "substr(size_t, size_t) test", count, passed, [&] {
			String sub1 = a.substr(5, 3);
			stream << sub1;
			return sub1.size() == "567"_len && strcmp(sub1.data(), "567") == 0;
		});

		runTest(stream, "substr() extra size test", count, passed, [&] {
			String sub1 = a.substr(a.size() - 3, 50);
			stream << sub1;
			return sub1.size() == "hij"_len && strcmp(sub1.data(), "hij") == 0;
		});

		runTest(stream, "substr() extra pos test", count, passed, [&] {
			String sub1 = a.substr(a.size() + 3, 50);
			stream << sub1;
			return sub1.empty();
		});

		runTest(stream, "copy() test", count, passed, [&] {
			String foo("quuuux");
			char bar[7];
			foo.copy(bar, sizeof bar);
			bar[6] = '\0';
			stream << bar;
			return strcmp(bar, "quuuux") == 0;
		});

		runTest(stream, "replace() test", count, passed, [&] {
			String str("The quick brown fox jumps over the lazy dog.");
			str.replace(10, 5, "red"); // (5)
			str.replace(str.begin(), str.begin() + 3, 1, 'A');
			stream << str;
			return str.size() == "A quick red fox jumps over the lazy dog."_len && strcmp(str.data(), "A quick red fox jumps over the lazy dog.") == 0;
		});

		runTest(stream, "resize() truncate test", count, passed, [&] {
			String str( "Where is the end?" );
			str.resize( 8 );
			stream << str;
			return str.size() == "Where is"_len && strcmp(str.data(), "Where is") == 0;
		});

		runTest(stream, "resize() fill test", count, passed, [&] {
			String str( "Ha" );
			str.resize( 8, 'a' );
			stream << str;
			return str.size() == "Haaaaaaa"_len && strcmp(str.data(), "Haaaaaaa") == 0;
		});

		_desc = stream.str();

		return count == passed;
	}
} _MemStringModifierTest;

struct MemStringInsertTest : MemPoolTest {
	MemStringInsertTest() : MemPoolTest("MemStringInsertTest") { }

	virtual bool run(pool_t *pool) {
		StringStream stream;
		size_t count = 0;
		size_t passed = 0;
		stream << "\n";

		String s = "xmplr";

		runTest(stream, "Insert range test", count, passed, [&] {
			s.insert(0, 1, 'E');
			stream << s;
			return s.size() == "Exmplr"_len && strcmp(s.data(), "Exmplr") == 0;
		});

		runTest(stream, "Insert c-string test", count, passed, [&] {
			s.insert(2, "e");
			stream << s;
			return s.size() == "Exemplr"_len && strcmp(s.data(), "Exemplr") == 0;
		});

		runTest(stream, "Insert string test", count, passed, [&] {
			s.insert(6, String("a"));
			stream << s;
			return s.size() == "Exemplar"_len && strcmp(s.data(), "Exemplar") == 0;
		});

		runTest(stream, "Insert string range test", count, passed, [&] {
			s.insert(8, String(" is an example string."),  0, 14);
			stream << s;
			return s.size() == "Exemplar is an example"_len && strcmp(s.data(), "Exemplar is an example") == 0;
		});

		runTest(stream, "Insert iterator test", count, passed, [&] {
			s.insert(s.cbegin() + s.find_first_of('n') + 1, ':');
			stream << s;
			return s.size() == "Exemplar is an: example"_len && strcmp(s.data(), "Exemplar is an: example") == 0;
		});

		runTest(stream, "Insert iterator with count test", count, passed, [&] {
			s.insert(s.cbegin() + s.find_first_of(':') + 1, 2, '=');
			stream << s;
			return s.size() == "Exemplar is an:== example"_len && strcmp(s.data(), "Exemplar is an:== example") == 0;
		});

		runTest(stream, "Insert iterator range test", count, passed, [&] {
			String seq = " string";
			s.insert(s.begin() + s.find_last_of('e') + 1, std::begin(seq), std::end(seq));
			stream << s;
			return s.size() == "Exemplar is an:== example string"_len && strcmp(s.data(), "Exemplar is an:== example string") == 0;
		});

		runTest(stream, "Insert iterator InitList test", count, passed, [&] {
			String seq = " string";
			s.insert(s.cbegin() + s.find_first_of('g') + 1, {'.'});
			stream << s;
			return s.size() == "Exemplar is an:== example string."_len && strcmp(s.data(), "Exemplar is an:== example string.") == 0;
		});

		_desc = stream.str();

		return count == passed;
	}
} _MemStringInsertTest;

struct MemStringEraseTest : MemPoolTest {
	MemStringEraseTest() : MemPoolTest("MemStringEraseTest") { }

	virtual bool run(pool_t *pool) {
		StringStream stream;
		size_t count = 0;
		size_t passed = 0;
		stream << "\n";

		String s = "This is an example";

		runTest(stream, "Erase range test", count, passed, [&] {
			s.erase(0, 5); // Erase "This "
			stream << s;
			return s.size() == "is an example"_len && strcmp(s.data(), "is an example") == 0;
		});

		runTest(stream, "Erase c-string test", count, passed, [&] {
			s.erase(std::find(s.begin(), s.end(), ' '));
			stream << s;
			return s.size() == "isan example"_len && strcmp(s.data(), "isan example") == 0;
		});

		runTest(stream, "Erase string test", count, passed, [&] {
			s.erase(s.find(' '));
			stream << s;
			return s.size() == "isan"_len && strcmp(s.data(), "isan") == 0;
		});

		_desc = stream.str();

		return count == passed;
	}
} _MemStringEraseTest;

struct MemStringAppendTest : MemPoolTest {
	MemStringAppendTest() : MemPoolTest("MemStringAppendTest") { }

	virtual bool run(pool_t *pool) {
		StringStream stream;
		size_t count = 0;
		size_t passed = 0;
		stream << "\n";

		String output;

		runTest(stream, "Append fill test", count, passed, [&] {
		    output.append(3, '*');
			stream << output;
			return output.size() == "***"_len && strcmp(output.data(), "***") == 0;
		});

		runTest(stream, "Append string test", count, passed, [&] {
		    String str = "string";
		    output.append(str);
			stream << output;
			return output.size() == "***string"_len && strcmp(output.data(), "***string") == 0;
		});

		runTest(stream, "Append substring test", count, passed, [&] {
		    String str = "string";
		    output.append(str, 3, 3);
			stream << output;
			return output.size() == "***stringing"_len && strcmp(output.data(), "***stringing") == 0;
		});

		runTest(stream, "Append c-arr test", count, passed, [&] {
		    const char carr[] = "Two and one";
		    output.append(1, ' ').append(carr, 4);
			stream << output;
			return output.size() == "***stringing Two "_len && strcmp(output.data(), "***stringing Two ") == 0;
		});

		runTest(stream, "Append c-string test", count, passed, [&] {
		    const char* cptr = "C-string";
		    output.append(cptr);
			stream << output;
			return output.size() == "***stringing Two C-string"_len && strcmp(output.data(), "***stringing Two C-string") == 0;
		});

		runTest(stream, "Append range test", count, passed, [&] {
		    const char carr[] = "Two and one";
		    output.append(&carr[3], std::end(carr));
			stream << output;
			return output.size() == "***stringing Two C-string and one"_len && strcmp(output.data(), "***stringing Two C-string and one") == 0;
		});

		runTest(stream, "Append InitList test", count, passed, [&] {
		    output.append({ ' ', 'l', 'i', 's', 't' });
			stream << output;
			return output.size() == "***stringing Two C-string and one list"_len && strcmp(output.data(), "***stringing Two C-string and one list") == 0;
		});

		String str;

		runTest(stream, "operator+=(const char *) test", count, passed, [&] {
			str += "This";
			stream << str;
			return str.size() == "This"_len && strcmp(str.data(), "This") == 0;
		});

		runTest(stream, "operator+=(const String &) test", count, passed, [&] {
			str += String(" is ");
			stream << str;
			return str.size() == "This is "_len && strcmp(str.data(), "This is ") == 0;
		});

		runTest(stream, "operator+=(char) test", count, passed, [&] {
			str += 'a';
			stream << str;
			return str.size() == "This is a"_len && strcmp(str.data(), "This is a") == 0;
		});

		runTest(stream, "operator+=(InitList) test", count, passed, [&] {
			str += {' ','s','t','r','i','n','g','.'};
			stream << str;
			return str.size() == "This is a string."_len && strcmp(str.data(), "This is a string.") == 0;
		});

		runTest(stream, "operator+=(double) test", count, passed, [&] {
			str += 76.85;
			stream << str;
			return str.size() == "This is a string.L"_len && strcmp(str.data(), "This is a string.L") == 0;
		});

		_desc = stream.str();

		return count == passed;
	}
} _MemStringAppendTest;

struct MemStringUncommonTest : MemPoolTest {
	MemStringUncommonTest() : MemPoolTest("MemStringUncommonTest") { }

	virtual bool run(pool_t *pool) {
		StringStream stream;
		size_t count = 0;
		size_t passed = 0;
		stream << "\n";

		runTest(stream, "Self-assign test", count, passed, [&] {
			String str("Self-assign test");
			stream << (void *)str.data() << " ";
			str.assign(str.data() + 5, 6);
			bool t1 = strcmp(str.data(), "assign") == 0;
			stream << "'" << str << "' " << (void *)str.data() << " ";

			std::string stdstr("Self-assign test");
			stream << (void *)stdstr.data() << " ";
			stdstr.assign(stdstr.data() + 5, 6);
			bool t2 = strcmp(str.data(), "assign") == 0;
			stream << "'" << stdstr << "' " << (void *)stdstr.data();

			return t1 && t2;
		});

		_desc = stream.str();

		return count == passed;
	}
} _MemStringUncommonTest;

NS_SP_END
