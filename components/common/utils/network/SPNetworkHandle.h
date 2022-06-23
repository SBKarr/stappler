/**
Copyright (c) 2016-2020 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef COMMON_UTILS_NETWORK_SPNETWORKHANDLE_H_
#define COMMON_UTILS_NETWORK_SPNETWORKHANDLE_H_

#include "SPCommon.h"
#include "SPTime.h"
#include "SPData.h"

// CURL predef;

using CURL = void;
using CURLSH = void;
struct curl_slist;

NS_SP_BEGIN

class NetworkHandle {
public:
	enum class Method {
	    Unknown,
	    Get,
	    Post,
	    Put,
	    Delete,
	    Head,
		Smtp
	};

	enum class AuthMethod {
		Basic,
		Digest,
		PKey, // custom serenity method
	};

	struct Context {
		void *userdata = nullptr;
		CURL *curl = nullptr;
		CURLSH *share = nullptr;
		NetworkHandle *handle = nullptr;

		curl_slist *headers = nullptr;
		curl_slist *mailTo = nullptr;

		FILE *inputFile = nullptr;
		FILE *outputFile = nullptr;
		size_t inputPos = 0;

		int code = 0;
		bool success = false;
		std::array<char, 256> error = { 0 };
	};

	using ProgressCallback = Function<int(int64_t, int64_t)>;
	using IOCallback = Function<size_t(char *data, size_t size)>;

	static size_t getActiveHandles();

public:
	NetworkHandle();
	virtual ~NetworkHandle();

	NetworkHandle(NetworkHandle &&) = default;
	NetworkHandle &operator=(NetworkHandle &&) = default;

	virtual bool init(Method method, const StringView &url);
	bool perform();
	bool perform(const Callback<bool(CURL *)> &onBeforePerform, const Callback<bool(CURL *)> &onAfterPerform);

	void setRootCertificateFile(const StringView &);
	void setCookieFile(const StringView &);
	void setUserAgent(const StringView &);
	void setUrl(const StringView &);

	void clearHeaders();
	void addHeader(const StringView &header);
	void addHeader(const StringView &header, const StringView &value);
	const Vector<String> &getRequestHeaders() const;

	void setMailFrom(const StringView &);
	void addMailTo(const StringView &);
	bool setAuthority(const StringView &user, const StringView &passwd = StringView(), AuthMethod = AuthMethod::Basic);
	bool setPrivateKeyAuth(const BytesView &priv);

	// user: [proxyhost]:[port], auth: [name]:[passwd]
	void setProxy(const StringView &proxy, const StringView &auth);

	void setReceiveFile(const StringView &str, bool resumeDownload);
	void setReceiveCallback(const IOCallback &cb);
	void setResumeDownload(bool value);

	void setSendSize(size_t);
	void setSendFile(const StringView &str, const StringView &type = StringView());
	void setSendCallback(const IOCallback &cb, size_t outSize);
	void setSendData(const StringView &data);
	void setSendData(const Bytes &data);
	void setSendData(Bytes &&data);
	void setSendData(const uint8_t *data, size_t size);
	void setSendData(const data::Value &, data::EncodeFormat fmt = data::EncodeFormat());

	StringView getReceivedHeaderString(const StringView &h) const;
	int64_t getReceivedHeaderInt(const StringView &h) const;

	long getResponseCode() const { return _responseCode; }
	long getErrorCode() const { return _errorCode; }
	StringView getError() const { return _error; }

	Method getMethod() const { return _method; }
	StringView getUrl() const { return _url; }
	StringView getRootCertificateFile() const { return _rootCertFile; }
	StringView getCookieFile() const { return _cookieFile; }
	StringView getUserAgent() const { return _userAgent; }
	StringView getContentType() const { return _contentType; }
	const Vector<String> &getRecievedHeaders() const { return _recievedHeaders; }
	StringView getSendFile() const { return _sendFileName; }
	StringView getReceiveFile() const { return _receiveFileName; }

	void setDebug(bool value) { _debug = value; }
	void setReuse(bool value) { _reuse = value; }
	void setShared(bool value) { _shared = value; }
	void setSilent(bool value) { _silent = value; }
	const StringStream &getDebugData() const { return _debugData; }

	void setDownloadProgress(const ProgressCallback &callback);
	void setUploadProgress(const ProgressCallback &callback);

	void setConnectTimeout(int);
	void setLowSpeedLimit(int time, size_t limit);

	virtual bool prepare(Context *, const Callback<bool(CURL *)> &onBeforePerform);
	virtual bool finalize(Context *, const Callback<bool(CURL *)> &onAfterPerform);

protected:
	struct Network;

	friend struct Network;
	friend class NetworkMultiHandle;

	AuthMethod _authMethod = AuthMethod::Basic;
	String _user;
	String _password;
	String _from;
	String _keySign;
	Vector<String> _recv;

	String _url;

	String _cookieFile;
	String _rootCertFile;
	String _userAgent;

	long _errorCode = 0;
	String _contentType;
	String _error;

	long _responseCode;

	bool _isRequestPerformed;
	bool _debug = false;
	bool _reuse = true;
	bool _shared = false;
	bool _verifyHost = true;

	Vector<String> _sendedHeaders;
	Vector<String> _recievedHeaders;
	Map<String, String> _parsedHeaders;

	ProgressCallback _uploadProgress = nullptr;
	ProgressCallback _downloadProgress = nullptr;

	int64_t _uploadProgressValue = 0;
	Time _uploadProgressTiming;

	int64_t _downloadProgressValue = 0;
	Time _downloadProgressTiming;

	String _receiveFileName;
	IOCallback _receiveCallback = nullptr;
	int64_t _receiveOffset = 0;
	bool _resumeDownload = false;

	String _sendFileName;
	IOCallback _sendCallback = nullptr;
	Bytes _sendData;
	size_t _sendSize = 0;
	size_t _sendOffset = 0;

	StringStream _debugData;

	bool _silent = false;

	Pair<FILE *, size_t> openFile(const String &filename, bool readOnly = false, bool resume = false);

	int _connectTimeout = 20;
	int _lowSpeedTime = 120;
	int _lowSpeedLimit = 10_KiB;

	String _proxyAddress;
	String _proxyAuth;

	void *_sharedHandle = nullptr;

protected:
	/* protocol parameter */
	Method _method;

	int progressUpload(int64_t ultotal, int64_t ulnow);
	int progressDownload(int64_t dltotal, int64_t dlnow);

	size_t writeData(char *data, size_t size);
	size_t writeHeaders(const char *data, size_t size);
	size_t writeDebug(char *data, size_t size);
	size_t readData(char *data, size_t size);

	bool setupCurl(CURL *curl, char *errorBuffer);
	bool setupDebug(CURL *curl, bool debug);
	bool setupRootCert(CURL *curl, const StringView &certPath);
	bool setupHeaders(CURL *curl, const Vector<String> &vec, curl_slist **headers, const StringView &);
	bool setupUserAgent(CURL *curl, const StringView &agent);
	bool setupUser(CURL *curl, const StringView &user, const StringView &password, AuthMethod);
	bool setupFrom(CURL *curl, const StringView &from);
	bool setupRecv(CURL *curl, const Vector<String> &vec, curl_slist **mailTo);
	bool setupProgress(CURL *curl, bool progress);
	bool setupCookies(CURL *curl, const StringView &cookiePath);
	bool setupProxy(CURL *curl, const StringView &proxy, const StringView &auth);

	bool setupReceive(CURL *curl, FILE * & inputFile, size_t &inputPos);

	bool setupMethodGet(CURL *curl);
	bool setupMethodHead(CURL *curl);
	bool setupMethodPost(CURL *curl, FILE * & outputFile);
	bool setupMethodPut(CURL *curl, FILE * & outputFile);
	bool setupMethodDelete(CURL *curl);
	bool setupMethodSmpt(CURL *curl, FILE * & outputFile);
};

class NetworkMultiHandle {
public:
	void addHandle(NetworkHandle *, void *);

	// sync interface:
	// returns completed handles, so it can be immediately recharged with addHandle
	bool perform(const Callback<bool(NetworkHandle *, void *)> &);

protected:
	Vector<Pair<NetworkHandle *, void *>> pending;
};

NS_SP_END

#endif /* COMMON_UTILS_NETWORK_SPNETWORKHANDLE_H_ */
