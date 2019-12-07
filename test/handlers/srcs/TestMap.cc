// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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
#include "ServerComponent.h"
#include "RequestHandler.h"

NS_SA_EXT_BEGIN(test)

class TestHandlerMapVariant1 : public HandlerMap::Handler {
public:
	virtual bool isPermitted() override { return true; }

	virtual mem::Value onData() override {
		return mem::Value({
			pair("params", mem::Value(_params)),
			pair("query", mem::Value(_queryFields)),
			pair("query", mem::Value(_inputFields))
		});
	}
};

class TestHandlerMap : public HandlerMap {
public:
	TestHandlerMap() {
		addHandler("Variant1", Request::Method::Get, "/map/:id/:page", SA_HANDLER(TestHandlerMapVariant1))
				.addQueryFields({
			db::Field::Integer("intValue", db::Flags::Required),
			db::Field::Integer("mtime", db::Flags::AutoMTime),
		});

		addHandler("Variant1Post", Request::Method::Post, "/map/:id/:page", SA_HANDLER(TestHandlerMapVariant1))
				.addQueryFields({
			db::Field::Integer("intValue", db::Flags::Required),
			db::Field::Integer("mtime", db::Flags::AutoMTime),
		}).addInputFields({
			db::Field::Text("text", db::Flags::Required),
			db::Field::File("file", db::MaxFileSize(2_MiB)),
		});
	}
};


NS_SA_EXT_END(test)
