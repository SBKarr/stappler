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

#ifndef SACONTENTFILTER_H
#define	SACONTENTFILTER_H

#include "Request.h"
#include "InputFile.h"
#include "SPData.h"
#include "SPCharReader.h"
#include "SPUrlencodeParser.h"

NS_SA_BEGIN

class InputParser : public AllocPool {
public:
	InputParser(const InputConfig &, size_t);
	virtual ~InputParser() { }

	virtual void run(const char *str, size_t len) = 0;
	virtual void finalize() = 0;
	virtual void cleanup();

	virtual data::Value &getData() {
		return root;
	}

	virtual apr::array<InputFile> &getFiles() {
		return files;
	}

	const InputConfig &getConfig() const;

protected:
	using VarState = UrlencodeParser::VarState;

	data::Value *flushString(CharReaderBase &r, data::Value *, VarState state);

	InputConfig config;
	size_t length;
	data::Value root;
	UrlencodeParser basicParser;
	apr::array<InputFile> files;
};

class InputFilter : public AllocPool {
public:
	using Reader = CharReaderBase;
	using FilterFunc = Function<void(InputFilter *filter)>;

	enum class Accept {
		None = 0,
		Urlencoded = 1,
		Multipart = 2,
		Json = 3,
		Files = 4
	};

	enum class Exception {
		None,
		TooLarge,
		Unrecognized,
	};

public:
	static void filterRegister();
	static Exception insert(const Request &);

	apr::weak_string getContentType() const;

	size_t getContentLength() const;
	size_t getBytesRead() const;
	size_t getBytesReadSinceUpdate() const;

	Time getStartTime() const;
	TimeInterval getElapsedTime() const;
	TimeInterval getElapsedTimeSinceUpdate() const;

	size_t getFileBufferSize() const;
	void setFileBufferSize(size_t size);

	bool isFileUploadAllowed() const;
	bool isDataParsingAllowed() const;
	bool isBodySavingAllowed() const;

	bool isCompleted() const;

	const apr::ostringstream & getBody() const;
	data::Value & getData();
	apr::array<InputFile> &getFiles();

	const InputConfig & getConfig() const;

	Request getRequest() const;

protected:
	static apr_status_t filterFunc(ap_filter_t *f, apr_bucket_brigade *bb,
			ap_input_mode_t mode, apr_read_type_e block, apr_off_t readbytes);

	static int filterInit(ap_filter_t *f);

	InputFilter(const Request &, Accept a);

	apr_status_t func(ap_filter_t *f, apr_bucket_brigade *bb,
			ap_input_mode_t mode, apr_read_type_e block, apr_off_t readbytes);

	void step(apr_bucket *b, apr_read_type_e block);
	int init(ap_filter_t *f);

protected:
	Accept _accept = Accept::None;

	Time _time;
	Time _startTime;
	TimeInterval _timer;

	size_t _unupdated = 0;
	size_t _read = 0;
	size_t _contentLength = 0;

	apr::ostringstream _body;

	InputParser *_parser = nullptr;
	Request _request;

	bool _eos = false, _isCompleted = false, _isStarted = false;
};

NS_SA_END

#endif	/* SACONTENTFILTER_H */

