/**
Copyright (c) 2016-2017 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef SERENITY_SRC_OUTPUT_OUTPUT_H_
#define SERENITY_SRC_OUTPUT_OUTPUT_H_

#include "Request.h"

NS_SA_EXT_BEGIN(output)

void formatJsonAsHtml(OutputStream &stream, const data::Value &, bool actionHandling = false);

void writeData(Request &rctx, const data::Value &, bool allowJsonP = true);
void writeData(Request &rctx, std::basic_ostream<char> &stream, const Function<void(const String &)> &ct,
		const data::Value &, bool allowJsonP = true);

int writeResourceFileData(Request &rctx, data::Value &&);
int writeResourceData(Request &rctx, data::Value &&, data::Value && origin);

int writeResourceFileHeader(Request &rctx, const data::Value &);

// write file headers with respect for cache headers (if-none-match, if-modified-since)
// returns true if we shoul write file data or false if we should return HTTP_NOT_MODIFIED
bool writeFileHeaders(Request &rctx, const data::Value &, const String &convertType = String());

NS_SA_EXT_END(output)

#endif /* SERENITY_SRC_OUTPUT_OUTPUT_H_ */
