/*
   Copyright 2013 Roman "SBKarr" Katuntsev, LLC St.Appler

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#ifndef SAOUTPUTFILTER_H
#define	SAOUTPUTFILTER_H

#include "Request.h"
#include "SPCharReader.h"

NS_SA_BEGIN

class OutputFilter : public AllocPool {
public:
	using Reader = CharReaderBase;

	static void filterRegister();
	static void insert(const Request &);

	OutputFilter(const Request &rctx);

	apr_status_t func(ap_filter_t *f, apr_bucket_brigade *bb);
	int init(ap_filter_t *f);

	bool readRequestLine(Reader & r);
	bool readHeaders(Reader & r);

	data::Value resultForRequest();
protected:
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
	apr_bucket *_bucket = nullptr;
	apr::string _responseLine;
	apr::string _statusText;
};

NS_SA_END

#endif	/* SAOUTPUTFILTER_H */

