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

#ifndef SERENITY_SRC_CORE_NETWORKING_H_
#define SERENITY_SRC_CORE_NETWORKING_H_

#include "SPNetworkHandle.h"
#include "Define.h"

NS_SA_EXT_BEGIN(network)

class Handle : public NetworkHandle {
public:
	Handle(Method method, const String &url);

	data::Value performDataQuery();
	data::Value performDataQuery(const data::Value &, data::EncodeFormat = data::EncodeFormat::DefaultFormat);
	bool performCallbackQuery(const IOCallback &);
	bool performProxyQuery(Request &);
};

class Mail : public NetworkHandle {
public:
	Mail(const String &url, const String &user, const String &passwd);
	Mail(const data::Value &config);

	bool send(apr::ostringstream &stream);
};

NS_SA_EXT_END(network)

#endif /* SERENITY_SRC_CORE_NETWORKING_H_ */
