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

#include "SPugContext.h"
#include "Request.h"

NS_SA_BEGIN

void Request::initScriptContext(pug::Context &ctx) {
	pug::VarClass serenityClass;
	serenityClass.staticFunctions.emplace("prettify", [] (pug::VarStorage &, pug::Var *var, size_t argc) -> pug::Var {
		if (var && argc == 1) {
			return pug::Var(data::Value(data::toString(var->readValue(), true)));
		}
		return pug::Var();
	});
	serenityClass.staticFunctions.emplace("timeToHttp", [] (pug::VarStorage &, pug::Var *var, size_t argc) -> pug::Var {
		if (var && argc == 1 && var->readValue().isInteger()) {
			return pug::Var(data::Value(Time::microseconds(var->readValue().asInteger()).toHttp()));
		}
		return pug::Var();
	});
	serenityClass.staticFunctions.emplace("uuidToString", [] (pug::VarStorage &, pug::Var *var, size_t argc) -> pug::Var {
		if (var && argc == 1 && var->readValue().isBytes()) {
			return pug::Var(data::Value(apr::uuid::getStringFromBytes(var->readValue().getBytes())));
		}
		return pug::Var();
	});
	ctx.set("serenity", std::move(serenityClass));
	ctx.set("window", data::Value{
		pair("location", data::Value{
			pair("href", data::Value(getFullHostname() + getUnparsedUri())),
			pair("hostname", data::Value(getHostname())),
			pair("pathname", data::Value(getUnparsedUri())),
			pair("protocol", data::Value(isSecureConnection() ? "https:" : "http:")),
		})
	});
}

NS_SA_END
