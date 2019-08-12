/**
Copyright (c) 2019 Roman Katuntsev <sbkarr@stappler.org>

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
#include "SPugContext.h"

NS_SA_EXT_BEGIN(test)

class TestUploadHandler : public RequestHandler {
public:
	virtual bool isRequestPermitted(Request & rctx) override {
		return true;
	}

	virtual int onTranslateName(Request &rctx) override {
		data::Value none;
		auto val = none.getValue("test");
		val = data::Value("string");

		if (_subPath == "/upload.html") {
			if (auto scheme = rctx.server().getScheme("objects")) {
				return rctx.runPug("templates/upload.pug", [&] (pug::Context &c, const pug::Template &tpl) -> bool {
					auto objs = scheme->select(rctx.storage(), storage::Query());
					c.set("objs", move(objs));
					return true;
				});
			}
		}
		return HTTP_NOT_FOUND;
	}
};

NS_SA_EXT_END(test)
