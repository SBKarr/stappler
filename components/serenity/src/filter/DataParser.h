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

#ifndef SERENITY_SRC_FILTER_DATAPARSER_H_
#define SERENITY_SRC_FILTER_DATAPARSER_H_

#include "InputFilter.h"
#include "SPDataStream.h"

NS_SA_BEGIN

class DataParser : public InputParser {
public:
	DataParser(const db::InputConfig &c, size_t s) : InputParser(c, s) { }

	virtual void run(const char *str, size_t len) override {
		stream.write(str, len);
	}
	virtual void finalize() override {
		data = stream.extract();
	}

	virtual data::Value &getData() override {
		return data;
	}

protected:
	data::Value data;
	data::Stream stream;
};

NS_SA_END

#endif /* SERENITY_SRC_FILTER_DATAPARSER_H_ */
