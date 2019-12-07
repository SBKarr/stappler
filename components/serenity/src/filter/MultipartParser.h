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

#ifndef SERENITY_SRC_FILTER_MULTIPARTPARSER_H_
#define SERENITY_SRC_FILTER_MULTIPARTPARSER_H_

#include "InputFilter.h"
#include "SPBuffer.h"
#include "SPDataStream.h"

NS_SA_BEGIN

class MultipartParser : public InputParser {
public:
	using Reader = StringView;

	MultipartParser(const db::InputConfig &, size_t, const mem::StringView &);

	virtual void run(const char *str, size_t len) override;
	virtual void finalize() override;

protected:
	enum class State {
		Begin,
		BeginBlock,
		HeaderLine,
		Data,
		End,
	};

	enum class Data {
		Var,
		File,
		FileAsData,
		Skip,
	};

	enum class Header {
		Begin,
		ContentDisposition,
		ContentDispositionParams,
		ContentDispositionName,
		ContentDispositionFileName,
		ContentDispositionSize,
		ContentDispositionUnknown,
		ContentType,
		ContentEncoding,
		Unknown,
	};

	enum class Literal {
		None,
		Plain,
		Quoted,
	};

	data::Value * flushVarName(Reader &r);
	void flushLiteral(Reader &r, bool quoted);
	void flushData(Reader &r);

	void readBegin(Reader &r);
	void readBlock(Reader &r);
	void readHeaderBegin(Reader &r);
	void readHeaderContentDisposition(Reader &r);
	void readHeaderContentDispositionParam(Reader &r);
	void readHeaderValue(Reader &r);
	void readHeaderDummy(Reader &r);

	void readPlainLiteral(Reader &r);
	void readQuotedLiteral(Reader &r);
	void readHeaderContentDispositionValue(Reader &r);
	void readHeaderContentDispositionDummy(Reader &r);
	void readHeader(Reader &r);
	void readData(Reader &r);

	apr::string name;
	apr::string file;
	apr::string type;
	apr::string encoding;
	size_t size = 0;

	State state = State::Begin;
	Header header = Header::Begin;
	Literal literal = Literal::None;
	Data data = Data::Skip;
	size_t match = 0;
	apr::string boundary;
	Buffer buf;
	data::StreamBuffer dataBuf;
};

NS_SA_END

#endif /* SERENITY_SRC_FILTER_MULTIPARTPARSER_H_ */
