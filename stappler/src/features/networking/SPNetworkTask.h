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

#ifndef STAPPLER_SRC_FEATURES_NETWORKING_SPNETWORKTASK_H_
#define STAPPLER_SRC_FEATURES_NETWORKING_SPNETWORKTASK_H_

#include "SPTask.h"
#include "SPThread.h"
#include "SPNetworkHandle.h"

NS_SP_BEGIN

class NetworkTask : public Task {
public:
	using Method = NetworkHandle::Method;

	using ThreadProgressCallback = NetworkHandle::ProgressCallback;
	using IOCallback = NetworkHandle::IOCallback;

	static Thread &thread();

public:
    NetworkTask();
    virtual ~NetworkTask();

    virtual bool init(Method method, const String &url);
    virtual bool execute() override;
    virtual bool performQuery();

	virtual void run();

    void setAuthority(const String &user, const String &passwd = String());

	/* Adds HTTP header line to request */
    void addHeader(const String &header);
    void addHeader(const String &header, const String &value);

	void setReceiveFile(const String &str, bool resumeDownload);
	void setReceiveCallback(const IOCallback &cb);

	void setSendFile(const String &str);
	void setSendCallback(const IOCallback &cb, size_t outSize);
	void setSendData(const String &data);
	void setSendData(const Bytes &data);
	void setSendData(Bytes &&data);
	void setSendData(const uint8_t *data, size_t size);
	void setSendData(const data::Value &, data::EncodeFormat fmt = data::EncodeFormat());

    int32_t getResponseCode() const { return (int32_t)_handle.getResponseCode(); }
	const String &getUrl() const { return _handle.getUrl(); }
    const String &getError() const { return _handle.getError(); }
    const String &getContentType() const { return _handle.getContentType(); }
	const Vector<String> &getRecievedHeaders() const { return _handle.getRecievedHeaders(); }

	const String &getSendFile() const { return _handle.getSendFile(); }
	const String &getReceiveFile() const { return _handle.getReceiveFile(); }

	void setDebug(bool value) { _handle.setDebug(value); }
	const StringStream &getDebugData() const { return _handle.getDebugData(); }

	String getReceivedHeaderString(const String &h) const;
	int64_t getReceivedHeaderInt(const String &h) const;

	size_t getSize() const { return _size; }
	int64_t getMTime() const { return _mtime; }
	const String &getETag() const { return _etag; }

	void setSize(size_t val) { _size = val; }
	void setMTime(int64_t val) { _mtime = val; }
	void setETag(const String &val) { _etag = val; }

	NetworkHandle & getHandle() { return _handle; }
	const NetworkHandle & getHandle() const { return _handle; }

	void setThread(Thread &);
	Thread &getThread() const;

protected:
	NetworkHandle _handle;

	uint64_t _mtime = 0;
	size_t _size = 0;
	String _etag;
	Thread *_customThread = nullptr;

    void setThreadDownloadProgress(const ThreadProgressCallback &callback);
    void setThreadUploadProgress(const ThreadProgressCallback &callback);

	/* network downloads designed to run on their own thread,
	 but you can perform them as tasks on any other thread as usual */
	static Thread _networkThread;
};

NS_SP_END

#endif /* STAPPLER_SRC_FEATURES_NETWORKING_SPNETWORKTASK_H_ */
