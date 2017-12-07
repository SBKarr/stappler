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
#include "SPBitmap.h"
#include "SPTime.h"

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

			stream << "\n\t" << t.toSeconds() << " | Rfc822: " << http << " | " << t1.toSeconds() << " " << (t1.toSeconds() == t.toSeconds());
			stream << " | CTime: " << ctime << " | " << t2.toSeconds() << " " << (t2.toSeconds() == t.toSeconds());

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

int parseOptionString(data::Value &ret, const String &str, int argc, const char * argv[]) {
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
	};

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

	auto &args = opts.getValue("args");

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
