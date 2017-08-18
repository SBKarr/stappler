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

#include "SPFilesystem.h"
#include "SPData.h"
#include "SPDataStream.h"
#include "SPSql.h"

#include "Test.h"

#include <string>
#include <unistd.h>
#include <iostream>
#include <fstream>

static constexpr auto HELP_STRING(
R"HelpString(sptest <options> <command>
Options are one of:
    -v (--verbose)
    -h (--help))HelpString");


NS_SP_BEGIN

struct StringToNumberTest : Test {
	StringToNumberTest() : Test("StringToNumberTest") { }

	template <typename T>
	bool runTest(StringStream &stream, const T &t) {
		auto n = StringToNumber<T>(toString(t));
		stream << "\t" << t << " -> " << toString(t) << " -> " << n << " -> " << (n == t) << "\n";
		return n == t;
	}

	template <typename T>
	bool runFloatTest(StringStream &stream, const T &t) {
		auto n = StringToNumber<T>(toString(t));
		stream << "\t" << t << " -> " << toString(t) << " -> " << n << " -> " << (toString(n) == toString(t)) << "\n";
		return toString(n) == toString(t);
	}

	virtual bool run() {
		StringStream stream;
		size_t pass = 0;

		stream << "\n";

		if (runTest(stream, rand_int64_t())) { ++ pass; }
		if (runTest(stream, rand_uint64_t())) { ++ pass; }
		if (runTest(stream, rand_int32_t())) { ++ pass; }
		if (runTest(stream, rand_uint32_t())) { ++ pass; }
		if (runFloatTest(stream, rand_float())) { ++ pass; }
		if (runFloatTest(stream, rand_double())) { ++ pass; }

		_desc = stream.str();

		if (pass == 6) {
			return true;
		}

		return false;
	}
} _StringToNumberTest;

struct UnicodeTest : Test {
	UnicodeTest() : Test("UnicodeTest") { }

	virtual bool run() {
		String str("Идейные соображения высшего порядка, а также начало повседневной работы по формированию позиции");
		WideString wstr(u"Идейные соображения высшего порядка, а также начало повседневной работы по формированию позиции");
		String strHtml("Идейные&nbsp;&lt;соображения&gt;&amp;работы&#x410;&#x0411;&#1042;&#1043;");

		StringStream stream;
		size_t pass = 0;

		stream << "\n\tUtf8 -> Utf16 -> Utf8 test";
		if (str == string::toUtf8(string::toUtf16(str))) {
			stream << " passed\n";
			++ pass;
		} else {
			stream << " failed\n";
		}

		stream << "\tUtf16 -> Utf8 -> Utf16 test";
		if (wstr == string::toUtf16(string::toUtf8(wstr))) {
			stream << " passed\n";
			++ pass;
		} else {
			stream << " failed\n";
		}

		stream << "\tUtf16Html \"" << string::toUtf8(string::toUtf16Html(strHtml)) << "\"";
		if (u"Идейные <соображения>&работыАБВГ" == string::toUtf16Html(strHtml)) {
			stream << " passed\n";
			++ pass;
		} else {
			stream << " failed\n";
		}
		_desc = stream.str();
		return pass == 3;
	}
} _UnicodeTest;

NS_SP_END

using namespace stappler;

int parseOptionSwitch(data::Value &ret, char c, const char *str) {
	if (c == 'h') {
		ret.setBool(true, "help");
	} else if (c == 'v') {
		ret.setBool(true, "verbose");
	}
	return 1;
}

int parseOptionString(data::Value &ret, const String &str, int argc, const char * argv[]) {
	if (str == "help") {
		ret.setBool(true, "help");
	} else if (str == "verbose") {
		ret.setBool(true, "verbose");
	}
	return 1;
}

int _spMain(argc, argv) {
	data::Value opts = data::parseCommandLineOptions(argc, argv,
			&parseOptionSwitch, &parseOptionString);
	if (opts.getBool("help")) {
		std::cout << HELP_STRING << "\n";
		return 0;
	};

	if (opts.getBool("verbose")) {
		std::cout << " Current work dir: " << stappler::filesystem::currentDir() << "\n";
		std::cout << " Documents dir: " << stappler::filesystem::documentsPath() << "\n";
		std::cout << " Cache dir: " << stappler::filesystem::cachesPath() << "\n";
		std::cout << " Writable dir: " << stappler::filesystem::writablePath() << "\n";
		std::cout << " Options: " << stappler::data::EncodeFormat::Pretty << opts << "\n";
	}

	// Test::RunAll();

	sql::Query<sql::SimpleBinder> query;
	query.select()
			.field("field").field("field", "alias").field("database", "field", "alias")
			.from("table").from("table")
			.where("alias", sql::Comparation::Equal, "value")
			.where(sql::Operator::And, "field", sql::Comparation::Equal, data::Value(1234))
			.where(sql::Operator::Or, "field", sql::Comparation::NotEqual, data::Value(false))
			.where(sql::Operator::Or, "time", sql::Comparation::BetweenValues, data::Value(1234), data::Value(123400))
			.order(sql::Ordering::Descending, "field")
			.limit(12, 16)
			.finalize();
	std::cout << query.getStream().str() << "\n";

	query = sql::Query<sql::SimpleBinder>();
	query.with("query", [] (sql::Query<sql::SimpleBinder>::GenericQuery &q) {
		q.select().all().from("sqtable");
	}).with("query", [] (sql::Query<sql::SimpleBinder>::GenericQuery &q) {
		q.select().all().from("sqtable");
	}).select().fields("field1", "field2").from("table")
		.where("alias", sql::Comparation::Equal, "value")
		.parenthesis(sql::Operator::And, [] (sql::Query<sql::SimpleBinder>::SelectWhere &q) {
			q.where(sql::Operator::Or, "field", sql::Comparation::Equal, "value")
					.where(sql::Operator::Or, "field", sql::Comparation::Equal, "value");
	}).finalize();

	std::cout << query.getStream().str() << "\n";

	sql::Query<sql::SimpleBinder> insertQuery;
	insertQuery.insert("table")
			.field("field1").field("field2")
			.values().value("test1").value("test2")
			.values().value("test3").value("test4")
			.finalize();

	std::cout << insertQuery.getStream().str() << "\n";

	insertQuery = sql::Query<sql::SimpleBinder>();
	insertQuery.insert("table")
			.field("field1").field("field2")
			.values().value("test1").value("test2")
			.onConflictDoNothing()
			.returning().all()
			.finalize();
	std::cout << insertQuery.getStream().str() << "\n";

	insertQuery = sql::Query<sql::SimpleBinder>();
	insertQuery.insert("table")
			.field("field1").field("field2").field("field3")
			.values().value("test1").value("test2").value("test3")
			.onConflict("field1").doUpdate().excluded("field2").def("field3")
			.where("field1", sql::Comparation::Equal, "alias")
			.returning().all()
			.finalize();
	std::cout << insertQuery.getStream().str() << "\n";

	sql::Query<sql::SimpleBinder> updateQuery;
	updateQuery.update("table", "alias")
			.set("field1", "value1").set("field1", "value2")
			.where("field3", sql::Comparation::NotEqual, data::Value(false))
			.returning().all()
			.finalize();
	std::cout << updateQuery.getStream().str() << "\n";

	sql::Query<sql::SimpleBinder> deleteQuery;
	deleteQuery.remove("table", "alias")
			.where("field3", sql::Comparation::NotEqual, data::Value(false))
			.returning().all()
			.finalize();
	std::cout << deleteQuery.getStream().str() << "\n";

	return 0;
}
