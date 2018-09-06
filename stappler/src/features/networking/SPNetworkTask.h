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

#include "SPNetworkHandle.h"
#include "SPTask.h"
#include "SPThread.h"

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

    virtual bool init(Method method, const StringView &url);
    virtual bool execute() override;
    virtual bool performQuery();

	virtual void run();

    void setAuthority(const StringView &user, const StringView &passwd = StringView());

	/* Adds HTTP header line to request */
    void addHeader(const StringView &header);
    void addHeader(const StringView &header, const StringView &value);

	void setReceiveFile(const StringView &str, bool resumeDownload);
	void setReceiveCallback(const IOCallback &cb);

	void setSendFile(const StringView &str);
	void setSendCallback(const IOCallback &cb, size_t outSize);
	void setSendData(const StringView &data);
	void setSendData(const Bytes &data);
	void setSendData(Bytes &&data);
	void setSendData(const uint8_t *data, size_t size);
	void setSendData(const data::Value &, data::EncodeFormat fmt = data::EncodeFormat());

    int32_t getResponseCode() const { return (int32_t)_handle.getResponseCode(); }
    StringView getUrl() const { return _handle.getUrl(); }
    StringView getError() const { return _handle.getError(); }
    StringView getContentType() const { return _handle.getContentType(); }
	const Vector<String> &getRecievedHeaders() const { return _handle.getRecievedHeaders(); }

	StringView getSendFile() const { return _handle.getSendFile(); }
	StringView getReceiveFile() const { return _handle.getReceiveFile(); }

	void setDebug(bool value) { _handle.setDebug(value); }
	const StringStream &getDebugData() const { return _handle.getDebugData(); }

	StringView getReceivedHeaderString(const StringView &h) const;
	int64_t getReceivedHeaderInt(const StringView &h) const;

	size_t getSize() const { return _size; }
	int64_t getMTime() const { return _mtime; }
	StringView getETag() const { return _etag; }

	void setSize(size_t val) { _size = val; }
	void setMTime(int64_t val) { _mtime = val; }
	void setETag(const StringView &val) { _etag = val.str(); }

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
