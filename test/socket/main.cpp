// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2016 Roman Katuntsev <sbkarr@stappler.org>

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

#include "SPData.h"
#include "SPString.h"
#include "STRoot.h"

#define HELP_STRING \
	"SocketTest\n" \

USING_NS_SP;

static constexpr auto s_config = R"Config({
	"listen": "127.0.0.1:8080",
	"hosts" : [
		{
			"name": "localhost",
			"admin": "serenity@stappler.org",
			"root": "$WORK_DIR/www",
			"components": [{
				"name": "TestComponent",
				"data": {
					"admin": "serenity@stappler.org",
					"root": "$WORK_DIR/www",
				}
			}],
			"db": {
				"host": "localhost",
				"dbname": "test",
				"user": "postgres",
				"password": "80015385",
			}
		}
	]
})Config";

int parseOptionSwitch(data::Value &ret, char c, const char *str) {
	if (c == 'h') {
		ret.setBool(true, "help");
	}
	return 1;
}

int parseOptionString(data::Value &ret, const String &str, int argc, const char * argv[]) {
	if (str == "help") {
		ret.setBool(true, "help");
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

	stellator::Root * root = stellator::Root::getInstance();
	memory::pool::push(root->pool());

	auto val = data::read<StringView, stellator::mem::Interface>(StringView(s_config));

	memory::pool::pop();

	root->run(val);

	return 0;
}
