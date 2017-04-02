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

	/* file index can be coded as ordered index or negative id
	 * negative_id = - index - 1
	 *
	 * return nullptr if there is no such file */
	static InputFile *getFileFromContext(int64_t);

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

	InputFile * getInputFile(int64_t) const;

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

