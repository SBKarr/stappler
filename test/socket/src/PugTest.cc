/**
Copyright (c) 2017-2018 Roman Katuntsev <sbkarr@stappler.org>

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

#include "Define.h"
#include "SPugTemplate.h"

NS_SA_EXT_BEGIN(test)

class TestPugHandler : public RequestHandler {
public:
	virtual bool isRequestPermitted(Request & rctx) override {
		return true;
	}

	virtual int onTranslateName(Request &rctx) override {
		if (_subPath == "/attributes.html") {
			return rctx.runPug("templates/attributes.pug", [&] (pug::Context &c, const pug::Template &tpl) -> bool {
				tpl.describe(std::cout, true);
				return true;
			});
		} else if (_subPath == "/code.html") {
			return rctx.runPug("templates/code.pug", [&] (pug::Context &c, const pug::Template &tpl) -> bool {
				tpl.describe(std::cout, true);
				return true;
			});
		} else if (_subPath == "/var.html") {
			return rctx.runPug("templates/var.pug", [&] (pug::Context &c, const pug::Template &tpl) -> bool {
				tpl.describe(std::cout, true);
				return true;
			});
		} else if (_subPath == "/interp.html") {
			return rctx.runPug("templates/interp.pug", [&] (pug::Context &c, const pug::Template &tpl) -> bool {
				tpl.describe(std::cout, true);
				return true;
			});
		} else if (_subPath == "/if.html") {
			return rctx.runPug("templates/if.pug", [&] (pug::Context &c, const pug::Template &tpl) -> bool {
				tpl.describe(std::cout, true);
				return true;
			});
		} else if (_subPath == "/while.html") {
			return rctx.runPug("templates/while.pug", [&] (pug::Context &c, const pug::Template &tpl) -> bool {
				tpl.describe(std::cout, true);
				return true;
			});
		} else if (_subPath == "/each.html") {
			return rctx.runPug("templates/each.pug", [&] (pug::Context &c, const pug::Template &tpl) -> bool {
				tpl.describe(std::cout, true);
				return true;
			});
		} else if (_subPath == "/include.html") {
			return rctx.runPug("templates/include.pug", [&] (pug::Context &c, const pug::Template &tpl) -> bool {
				tpl.describe(std::cout, true);
				return true;
			});
		}
		return HTTP_NOT_FOUND;
	}
};

NS_SA_EXT_END(test)
