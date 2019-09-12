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
#include "SPBitmap.h"
#include "SPSql.h"
#include "SPFilesystem.h"
#include "SPTime.h"
#include "SPData.h"
#include "SPDataStream.h"
#include "Test.h"

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

struct TimeTest : Test {
	TimeTest() : Test("TimeTest") { }

	virtual bool run() {
		StringStream stream;

		auto now = Time::now();

		bool success = true;

		for (int i = 0; i <= 10; ++ i) {
			auto t = now + TimeInterval::milliseconds( (i == 0) ? 0 : rand());

			auto ctime = t.toCTime();
			auto http = t.toHttp();

			auto t1 = Time::fromHttp(http);
			auto t2 = Time::fromRfc(ctime);

			auto ti = TimeInterval::microseconds(rand());

			stream << "\n\t" << t.toSeconds() << " | Rfc822: " << http << " | " << t1.toSeconds() << " " << (t1.toSeconds() == t.toSeconds());
			stream << " | CTime: " << ctime << " | " << t2.toSeconds() << " " << (t2.toSeconds() == t.toSeconds());
			stream << " | " << ti.toMicros();

			if (!(t1.toSeconds() == t.toSeconds() && t2.toSeconds() == t.toSeconds())) {
				success = false;
			}
		}

		_desc = stream.str();
		return success;
	}
} _TimeTest;

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

int parseOptionString(data::Value &ret, const StringView &str, int argc, const char * argv[]) {
	if (str == "help") {
		ret.setBool(true, "help");
	} else if (str == "verbose") {
		ret.setBool(true, "verbose");
	} else if (str == "gencbor") {
		ret.setBool(true, "gencbor");
	}
	return 1;
}

int _spMain(argc, argv) {
	data::Value opts = data::parseCommandLineOptions(argc, argv,
			&parseOptionSwitch, &parseOptionString);
	if (opts.getBool("help")) {
		std::cout << HELP_STRING << "\n";
		return 0;
	}

	if (opts.getBool("verbose")) {
		std::cout << " Current work dir: " << stappler::filesystem::currentDir() << "\n";
		std::cout << " Documents dir: " << stappler::filesystem::documentsPath() << "\n";
		std::cout << " Cache dir: " << stappler::filesystem::cachesPath() << "\n";
		std::cout << " Writable dir: " << stappler::filesystem::writablePath() << "\n";
		std::cout << " Options: " << stappler::data::EncodeFormat::Pretty << opts << "\n";
	}

	if (opts.getBool("gencbor")) {
		auto path = filesystem::currentDir("test.json");
		auto data = data::readFile(path);
		if (data.isArray()) {
			auto cborPath = filesystem::currentDir("data");
			filesystem::remove(cborPath, true, true);
			filesystem::mkdir(cborPath);

			size_t i = 1;
			for (auto &it : data.asArray()) {
				auto cborHexData = toString("d9d9f7", it.getString("hex"));
				auto cborData = base16::decode(cborHexData);
				filesystem::write(filepath::merge(cborPath, toString(i, ".cbor")), base16::decode(cborHexData));

				if (it.hasValue("decoded")) {
					StringStream stream; stream << std::showpoint << std::setprecision(20);
					data::write(stream, it.getValue("decoded"), data::EncodeFormat::Json);
					String str = stream.str();
					filesystem::write(filepath::merge(cborPath, toString(i, ".json")), (const uint8_t *)str.data(), str.size());
				}

				if (it.isString("diagnostic")) {
					auto &str = it.getString("diagnostic");
					filesystem::write(filepath::merge(cborPath, toString(i, ".diag")), (const uint8_t *)str.data(), str.size());
				}

				++ i;
			}
		}
	}

	data::Value null;
	data::Value integer(42);
	data::Value f(42.0);
	data::Value boolFalse(false);
	data::Value boolTrue(true);
	data::Value str("String");

	memory::PoolInterface::StringType pstr("string");
	StringView r(pstr);

	auto mempool = memory::pool::create();
	memory::pool::push(mempool);

	data::ValueTemplate<memory::PoolInterface> pool_val {
		data::ValueTemplate<memory::PoolInterface>(),
		data::ValueTemplate<memory::PoolInterface>(42),
		data::ValueTemplate<memory::PoolInterface>(42.0),
		data::ValueTemplate<memory::PoolInterface>(false),
		data::ValueTemplate<memory::PoolInterface>("String"),
		data::ValueTemplate<memory::PoolInterface>(true),
	};

	data::ValueTemplate<memory::PoolInterface> dict_val {
		pair("hello", data::ValueTemplate<memory::PoolInterface>("test1")),
		pair("world", data::ValueTemplate<memory::PoolInterface>("test2")),
		pair("hello world", data::ValueTemplate<memory::PoolInterface>("test2")),
		pair("[hello world]", data::ValueTemplate<memory::PoolInterface>("test2")),
	};

	Vector<data::Value> vec2;
	vec2.emplace_back(data::Value(true));
	vec2.emplace_back(data::Value(0.5));
	vec2.emplace_back(data::Value("test"));

	vec2.emplace(vec2.begin() + 1, data::Value(1234));

	memory::set<int64_t> set;
	set.emplace(123);
	set.emplace(1234);

	memory::set<memory::string> strset;
	strset.emplace("hello");
	strset.emplace("world");

	memory::map<memory::string, memory::string> strmap;
	strmap.emplace("hello", "test1");
	strmap.emplace("world", "test2");

	memory::vector<int> vecInt;
	vecInt.emplace_back(1);
	vecInt.emplace_back(2);
	vecInt.emplace_back(3);
	vecInt.emplace_back(4);
	vecInt.emplace_back(5);
	vecInt.emplace_back(6);

	auto &args = opts.getValue("args");

	memory::pool::pop();

	if (args.size() > 1) {
		if (args.getString(1) == "all") {
			Test::RunAll();
		} else {
			size_t i = 0;
			for (auto &it : args.asArray()) {
				if (i > 0) {
					if (it.isString()) {
						Test::Run(it.asString());
					}
				}
				++ i;
			}
		}
	} else {
		Test::RunAll();
	}

	return 0;
}
