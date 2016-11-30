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

#ifndef __chiertime_federal__SPNetworkTask__
#define __chiertime_federal__SPNetworkTask__

#include "SPTask.h"
#include "SPThread.h"
#include "SPNetworkHandle.h"

NS_SP_BEGIN

class NetworkTask : public Task {
public:
	using Method = NetworkHandle::Method;

	typedef std::function<int(int64_t, int64_t)> ThreadProgressCallback;
	typedef std::function<size_t(char *data, size_t size)> IOCallback;

	static Thread &thread();

public:
    NetworkTask();
    virtual ~NetworkTask();

    virtual bool init(Method method, const std::string &url);
    virtual bool execute() override;
    virtual bool performQuery();

	virtual void run();

	/* Adds HTTP header line to request */
    void addHeader(const std::string &header);
    void addHeader(const std::string &header, const std::string &value);

	void setReceiveFile(const std::string &str, bool resumeDownload);
	void setReceiveCallback(const IOCallback &cb);

	void setSendFile(const std::string &str);
	void setSendCallback(const IOCallback &cb, size_t outSize);
	void setSendData(const std::string &data);
	void setSendData(const Bytes &data);
	void setSendData(Bytes &&data);
	void setSendData(const uint8_t *data, size_t size);
	void setSendData(const data::Value &, data::EncodeFormat fmt = data::EncodeFormat());

    int32_t getResponseCode() const { return (int32_t)_handle.getResponseCode(); }
	const std::string &getUrl() const { return _handle.getUrl(); }
    const std::string &getError() const { return _handle.getError(); }
    const std::string &getContentType() const { return _handle.getContentType(); }
	const std::vector<std::string> &getRecievedHeaders() const { return _handle.getRecievedHeaders(); }

	const std::string &getSendFile() const { return _handle.getSendFile(); }
	const std::string &getReceiveFile() const { return _handle.getReceiveFile(); }

	void setDebug(bool value) { _handle.setDebug(value); }
	const std::stringstream &getDebugData() const { return _handle.getDebugData(); }

	std::string getReceivedHeaderString(const std::string &h);
	int64_t getReceivedHeaderInt(const std::string &h);

	size_t getSize() const { return _size; }
	int64_t getMTime() const { return _mtime; }
	const std::string &getETag() const { return _etag; }

	void setSize(size_t val) { _size = val; }
	void setMTime(int64_t val) { _mtime = val; }
	void setETag(const std::string &val) { _etag = val; }

	NetworkHandle & getHandle() { return _handle; }
	const NetworkHandle & getHandle() const { return _handle; }

protected:
	NetworkHandle _handle;

	uint64_t _mtime = 0;
	size_t _size = 0;
	std::string _etag;

    void setThreadDownloadProgress(const ThreadProgressCallback &callback);
    void setThreadUploadProgress(const ThreadProgressCallback &callback);

	/* network downloads designed to run on their own thread,
	 but you can perform them as tasks on any other thread as usual */
	static Thread _networkThread;
};

NS_SP_END

#endif /* defined(__chiertime_federal__SPNetworkTask__) */
