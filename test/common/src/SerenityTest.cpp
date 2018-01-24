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
#include "SPTime.h"
#include "SPString.h"
#include "SPData.h"
#include "Test.h"

NS_SP_BEGIN

static constexpr auto TEST_STRING(
R"Test((
	field: value;
	arrayField: array1, array2, array3;
	flagField;
	dictField: (
		field1: value1;
		field2: value2
	)
))Test");

static constexpr auto TEST_STRING_2(
R"Test((
    applications : (
        select : (
            name : org.stappler.stappler
        );
        order : (
            by : mtime;
            asc;
            limit: 10;
            offset: 20;
        );
        fields : name, tags, data.deprecated, issues
    );
    issues : (
        select : 12345;
        order : mtime, desc, 20, 40;
        resolve: ids, files, objects, sets, all;
        exclude: content, data.articles
        delta : (
            token : ASD12345;
            format : minimal
        )
    );
    articles : (
        select :
			~(num, gt, 10),
			~(approved, true);
		delta : ASD12345, full
    )
))Test");

static constexpr auto TEST_STRING_3(
R"Test((
	applications : (
        select : (
            name : org.stappler.stappler
        )fields : name, tags, data.deprecated, issues
    );
))Test");

static constexpr auto TEST_STRING_4 = "(applications(fields:name,tags,data.deprecated,issues;order(asc:t;by:mtime;limit:10;offset:20);select(name:org.stappler.stappler));articles(delta:ASD12345,full;select:~(num,gt,10),~(approved,t));issues(delta(format:minimal;token:ASD12345);exclude:content,data.articles;order:mtime,desc,20,40;resolve:ids,files,objects,sets,all;select:12345))\n";

static constexpr auto TEST_STRING_5 = "(\n"
		"arrayField : array1, (field:value), ~(val1, val2), false;\n"
		")\n";

struct PoolSerenityTest : MemPoolTest {
	PoolSerenityTest() : MemPoolTest("PoolSerenityTest") { }

	virtual bool run(pool_t *pool) {
		StringStream stream;
		size_t count = 0;
		size_t passed = 0;
		stream << "\n";

		auto d = data::read<String, memory::PoolInterface>(String(TEST_STRING_5));
		std::cout << data::EncodeFormat::SerenityPretty << d << "\n" << data::EncodeFormat::Pretty << d << "\n";

		runTest(stream, "First", count, passed, [&] {
			auto t = Time::now();
			auto d = data::read<String, memory::PoolInterface>(String(TEST_STRING));
			auto str = data::toString<memory::PoolInterface>(d, data::EncodeFormat::Serenity);
			auto d2 = data::read<String, memory::PoolInterface>(str);
			auto str2 = data::toString<memory::PoolInterface>(d, data::EncodeFormat::SerenityPretty);
			auto d3 = data::read<String, memory::PoolInterface>(str2);

			stream << (Time::now() - t).toMicroseconds();
			return d == d2 && d2 == d3;
		});

		runTest(stream, "Second", count, passed, [&] {
			auto t = Time::now();
			auto d = data::read<String, memory::PoolInterface>(String(TEST_STRING_2));
			auto str = data::toString<memory::PoolInterface>(d, data::EncodeFormat::Serenity);
			auto d2 = data::read<String, memory::PoolInterface>(str);
			auto str2 = data::toString<memory::PoolInterface>(d, data::EncodeFormat::SerenityPretty);
			auto d3 = data::read<String, memory::PoolInterface>(str2);

			stream << (Time::now() - t).toMicroseconds();
			return d == d2 && d2 == d3;
		});

		runTest(stream, "Third", count, passed, [&] {
			auto t = Time::now();
			auto d = data::read<String, memory::PoolInterface>(String(TEST_STRING_3));
			auto str = data::toString<memory::PoolInterface>(d, data::EncodeFormat::Serenity);
			auto d2 = data::read<String, memory::PoolInterface>(str);
			auto str2 = data::toString<memory::PoolInterface>(d, data::EncodeFormat::SerenityPretty);
			auto d3 = data::read<String, memory::PoolInterface>(str2);

			stream << (Time::now() - t).toMicroseconds();
			return d == d2 && d2 == d3;
		});

		runTest(stream, "F", count, passed, [&] {
			auto t = Time::now();
			auto d = data::read<String, memory::PoolInterface>(String(TEST_STRING_4));
			auto str = data::toString<memory::PoolInterface>(d, data::EncodeFormat::Serenity);
			auto d2 = data::read<String, memory::PoolInterface>(str);
			auto str2 = data::toString<memory::PoolInterface>(d, data::EncodeFormat::SerenityPretty);
			auto d3 = data::read<String, memory::PoolInterface>(str2);

			stream << (Time::now() - t).toMicroseconds();
			return d == d2 && d2 == d3;
		});

		_desc = stream.str();

		return count == passed;
	}
} _PoolSerenityTest;

NS_SP_END
