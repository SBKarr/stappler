/**
Copyright (c) 2016-2019 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef SERENITY_SRC_FILTER_OUTPUTFILTER_H_
#define	SERENITY_SRC_FILTER_OUTPUTFILTER_H_

#include "SPStringView.h"
#include "Request.h"

NS_SA_BEGIN

class OutputFilter : public AllocPool {
public:
	using Reader = StringView;

	static void filterRegister();
	static void insert(const Request &);

	OutputFilter(const Request &rctx);

	apr_status_t func(ap_filter_t *f, apr_bucket_brigade *bb);
	int init(ap_filter_t *f);

	bool readRequestLine(Reader & r);
	bool readHeaders(Reader & r);

	data::Value resultForRequest();

protected:
	apr_status_t process(ap_filter_t* f, apr_bucket *e, const char *data, size_t len);
	apr_status_t outputHeaders(ap_filter_t* f, apr_bucket *e, const char *data, size_t len);

	size_t calcHeaderSize() const;
	void writeHeader(ap_filter_t* f, StringStream &) const;

	apr_bucket_brigade *_tmpBB;
	bool _seenEOS = false;
	bool _skipFilter = false;
	Request _request;

	enum class State {
		None,
		FirstLine,
		Headers,
		Body,

		Protocol,
		Code,
		Status,

		HeaderName,
		HeaderValue,
	};

	State _state = State::FirstLine;
	State _subState = State::Protocol;

	char _char, _buf;
	bool _isWhiteSpace;

	int64_t _responseCode = 0;

	apr::table _headers;

	apr::ostringstream _nameBuffer;
	apr::ostringstream _buffer;
	apr::ostringstream _headersBuffer;
	apr_bucket *_bucket = nullptr;
	apr::string _responseLine;
	apr::string _statusText;
};

NS_SA_END

#endif	/* SERENITY_SRC_FILTER_OUTPUTFILTER_H_ */

