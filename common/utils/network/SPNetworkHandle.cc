// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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

#include "SPCommon.h"
#include "SPString.h"
#include "SPNetworkHandle.h"

#include "SPStringView.h"
#include "SPFilesystem.h"
#include "SPLog.h"
#include "SPBitmap.h"

#ifdef LINUX
#include <sys/types.h>
#include <sys/xattr.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

#ifdef __MINGW32__
#define CURL_STATICLIB 1
#endif

#include <curl/curl.h>

#define SP_NETWORK_PROGRESS_TIMEOUT stappler::TimeInterval::microseconds(250000ull)

#define SP_NETWORK_LOG(...)
//#define SP_NETWORK_LOG(...) log::format("Network", __VA_ARGS__)

NS_SP_BEGIN

#define SP_TERMINATED_DATA(view) (view.terminated()?view.data():view.str().data())

struct NetworkHandle::Network {
#ifndef SPAPR
	class CurlHandle {
	public:
		CurlHandle() {
			_curl = curl_easy_init();
		}
		~CurlHandle() {
			if (_curl) {
				curl_easy_cleanup(_curl);
				_curl = nullptr;
			}
		}
		CURL *get() {
			return _curl;
		}
		operator bool () {
			return _curl != nullptr;
		}
		void invalidate(CURL * curl) {
			if (_curl == curl) {
				curl_easy_cleanup(_curl);
				_curl = curl_easy_init();
			}
		}
		void reset() {
			if (_curl) {
				curl_easy_reset(_curl);
			}
		}

		CurlHandle(CurlHandle &&other) : _curl(other._curl) {
			other._curl = nullptr;
		}
		CurlHandle & operator = (CurlHandle &&other) {
			_curl = other._curl;
			other._curl = nullptr;
			return *this;
		}

		CurlHandle(const CurlHandle &) = delete;
		CurlHandle & operator = (const CurlHandle &) = delete;
	protected:
		CURL *_curl = nullptr;
	};

	struct CurlThreadLocalKey {
		static void onDestructor(void *ptr) {
			if (ptr) {
				CurlHandle * handle = (CurlHandle *)ptr;
				delete handle;
			}
		}
		CurlThreadLocalKey() {
			pthread_key_create(&_key, &CurlThreadLocalKey::onDestructor);
		}
		~CurlThreadLocalKey() {
			pthread_key_delete(_key);
		}

		CurlHandle * get() {
			CurlHandle * ptr = (CurlHandle *)pthread_getspecific(_key);
			if (!ptr) {
				ptr = new CurlHandle();
				(void) pthread_setspecific(_key, ptr);
			}
			return ptr;
		}

        void invalidate(CURL *curl) {
            CurlHandle * ptr = (CurlHandle *)pthread_getspecific(_key);
            if (ptr) {
                ptr->invalidate(curl);
            }
        }

        void reset(CURL *curl) {
            CurlHandle * ptr = (CurlHandle *)pthread_getspecific(_key);
            if (ptr) {
                ptr->reset();
            }
        }

		pthread_key_t _key;
	};

	static CurlThreadLocalKey s_localCurl;

	static CURL * getHandle(bool reuse) {
		if (reuse) {
			auto t = s_localCurl.get();
			return t->get();
		} else {
			return curl_easy_init();
		}
	}

	static void releaseHandle(CURL *curl, bool reuse, bool success) {
		if (!reuse) {
			curl_easy_cleanup(curl);
		} else {
			if (!success) {
				s_localCurl.invalidate(curl);
            } else {
                s_localCurl.reset(curl);
            }
		}
	}
#else
	static CURL * getHandle(bool reuse) {
		return curl_easy_init();
	}

	static void releaseHandle(CURL *curl, bool reuse, bool success) {
		curl_easy_cleanup(curl);
	}
#endif

	static size_t writeDummy(const void *data, size_t size, size_t nmemb, void *obj) {
		return size * nmemb;
	}

	static size_t writeData(char *data, size_t size, size_t nmemb, void *obj) {
		auto task = static_cast<NetworkHandle *>(obj);
		return task->writeData(data, size * nmemb);
	}

	static size_t writeHeaders(char *data, size_t size, size_t nmemb, void *obj) {
		auto task = static_cast<NetworkHandle *>(obj);
		return task->writeHeaders(data, size * nmemb);
	}

	static size_t writeDebug(CURL *handle, curl_infotype type, char *data, size_t size, void *userptr) {
		auto task = static_cast<NetworkHandle *>(userptr);
		return task->writeDebug(data, size);
	}

	static size_t readData(char *data, size_t size, size_t nmemb, void *obj) {
		if (obj != NULL) {
			auto task = static_cast<NetworkHandle *>(obj);
			return task->readData(data, size * nmemb);
		} else {
			return 0;
		}
	}

	static int progress(void *obj, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
		auto task = static_cast<NetworkHandle *>(obj);
		int uProgress = task->progressUpload(ultotal, ulnow);
		int dProgress = task->progressDownload(dltotal, dlnow);
		if (ultotal == ulnow || ultotal == 0) {
			return dProgress;
		} else {
			return uProgress;
		}
	}
};

#ifdef LINUX

static bool NetworkHandle_setUserAttributes(FILE *file, const StringView &str, Time mtime) {
	if (int fd = fileno(file)) {
		auto err = fsetxattr(fd, "user.mime_type", str.data(), str.size(), XATTR_CREATE);
		if (err != 0) {
			err = fsetxattr(fd, "user.mime_type", str.data(), str.size(), XATTR_REPLACE);
			if (err != 0) {
				std::cout << "Fail to set mime type attribute (" << err << ")\n";
				return false;
			}
		}

		if (mtime) {
			struct timespec times[2];
			times[0].tv_nsec = UTIME_OMIT;

			times[1].tv_sec = time_t(mtime.sec());
			times[1].tv_nsec = (mtime.toMicroseconds() - Time::seconds(mtime.sec()).toMicroseconds()) * 1000;
			futimens(fd, times);
		}

		return true;
	}
	return false;
}

static String NetworkHandle_getUserMime(const StringView &filename) {
	String path = filepath::absolute(filename);

	char buf[1_KiB] = { 0 };
	auto vallen = getxattr(path.data(), "user.mime_type", buf, 1_KiB);
	if (vallen == -1) {
		return String();
	}

	return StringView(buf, vallen).str();
}

#endif


#ifndef SPAPR
NetworkHandle::Network::CurlThreadLocalKey NetworkHandle::Network::s_localCurl;
#endif

NetworkHandle::NetworkHandle() {
    _method = Method::Unknown;

    _responseCode = -1;
    _isRequestPerformed = false;
    _error = CURLE_OK;

#ifdef LINUX
    _rootCertFile = "/etc/ssl/certs/";
#endif
}

NetworkHandle::~NetworkHandle() { }

bool NetworkHandle::init(Method method, const StringView &url) {
	if (url.size() == 0 || method == Method::Unknown) {
		return false;
	}

	_url = url.str();
	_method = method;

	SP_NETWORK_LOG("init with url: %s", _url.data());

	return true;
}

#ifdef DEBUG
#define SETOPT(check, curl, name, value) \
	do { \
		int err = CURLE_OK; \
		if (check) { \
			err = curl_easy_setopt(curl, name, value); \
			if (err != CURLE_OK) { \
				SP_NETWORK_LOG("curl_easy_setopt failed: %d %s %s", err, #name, #value); \
				check = false; \
			} \
		} \
	} while (false);
#else
#define SETOPT(check, curl, name, value) check = (check) ? curl_easy_setopt(curl, name, value) == CURLE_OK : false;
#endif

bool NetworkHandle::setupCurl(CURL *curl, char *errorBuffer) {
	bool check = true;

	SETOPT(check, curl, CURLOPT_USE_SSL, CURLUSESSL_TRY);

	SETOPT(check, curl, CURLOPT_NOSIGNAL, 1);
	SETOPT(check, curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_WHATEVER);

	SETOPT(check, curl, CURLOPT_ERRORBUFFER, errorBuffer);
	SETOPT(check, curl, CURLOPT_LOW_SPEED_TIME, _lowSpeedTime);
	SETOPT(check, curl, CURLOPT_LOW_SPEED_LIMIT, _lowSpeedLimit);
	//SETOPT(check, curl, CURLOPT_TIMEOUT, SP_NW_READ_TIMEOUT);
	SETOPT(check, curl, CURLOPT_CONNECTTIMEOUT, _connectTimeout);

#if (DEBUG || LINUX)
	SETOPT(check, curl, CURLOPT_SSL_VERIFYPEER, 0L);
	SETOPT(check, curl, CURLOPT_SSL_VERIFYHOST, 0L);
#else
	SETOPT(check, curl, CURLOPT_SSL_VERIFYPEER, 1L);
	SETOPT(check, curl, CURLOPT_SSL_VERIFYHOST, 2L);
#endif

	SETOPT(check, curl, CURLOPT_URL, _url.c_str());
	SETOPT(check, curl, CURLOPT_RESUME_FROM, 0); // drop byte-ranged GET

	SETOPT(check, curl, CURLOPT_WRITEFUNCTION, Network::writeDummy);
	SETOPT(check, curl, CURLOPT_WRITEDATA, this);

	/* enable all supported built-in compressions */
	SETOPT(check, curl, CURLOPT_ACCEPT_ENCODING, "");

	return check;
}

bool NetworkHandle::setupDebug(CURL *curl, bool debug) {
	bool check = true;
	if (debug) {
		SETOPT(check, curl, CURLOPT_VERBOSE, 1);
		SETOPT(check, curl, CURLOPT_DEBUGFUNCTION, Network::writeDebug);
		SETOPT(check, curl, CURLOPT_DEBUGDATA, this);
	}
	return check;
}

bool NetworkHandle::setupRootCert(CURL *curl, const StringView &certPath) {
	bool check = true;
	if (!certPath.empty()) {
		auto bundle = certPath;
		if (bundle.empty() || bundle.back() != '/') {
			String path = filepath::root(certPath);

			SETOPT(check, curl, CURLOPT_CAINFO, SP_TERMINATED_DATA(bundle));
#ifndef ANDROID
			SETOPT(check, curl, CURLOPT_CAPATH, path.c_str());
#endif
		} else {
			StringView path = bundle.sub(0, bundle.size() - 1);
			SETOPT(check, curl, CURLOPT_CAPATH, SP_TERMINATED_DATA(path));
		}
	}
	return check;
}

bool NetworkHandle::setupHeaders(CURL *curl, const Vector<String> &vec, curl_slist **headers) {
	bool check = true;
	if (vec.size() > 0) {
		for (const auto &str : vec) {
			*headers = curl_slist_append(*headers, str.c_str());
		}
		SETOPT(check, curl, CURLOPT_HTTPHEADER, *headers);
	}

	SETOPT(check, curl, CURLOPT_HEADERFUNCTION, Network::writeHeaders);
	SETOPT(check, curl, CURLOPT_HEADERDATA, this);

	return check;
}

bool NetworkHandle::setupUserAgent(CURL *curl, const StringView &agent) {
	bool check = true;
	if (!agent.empty()) {
		SETOPT(check, curl, CURLOPT_USERAGENT, SP_TERMINATED_DATA(agent));
	} else {
		SETOPT(check, curl, CURLOPT_USERAGENT, "Stappler/0 CURL");
	}
	return check;
}

bool NetworkHandle::setupUser(CURL *curl, const StringView &user, const StringView &password) {
	bool check = true;
	if (!user.empty()) {
		SETOPT(check, curl, CURLOPT_USERNAME, SP_TERMINATED_DATA(user));
		if (!password.empty()) {
			SETOPT(check, curl, CURLOPT_PASSWORD, SP_TERMINATED_DATA(password));
		}
	}
	return check;
}

bool NetworkHandle::setupFrom(CURL *curl, const StringView &from) {
	bool check = true;
	if (_method == Method::Smtp && !from.empty()) {
		SETOPT(check, curl, CURLOPT_MAIL_FROM, SP_TERMINATED_DATA(from));
	}
	return check;
}

bool NetworkHandle::setupRecv(CURL *curl, const Vector<String> &vec, curl_slist **mailTo) {
	bool check = true;
	if (_method == Method::Smtp && vec.size() > 0) {
		for (const auto &str : vec) {
			*mailTo = curl_slist_append(*mailTo, str.c_str());
		}
		SETOPT(check, curl, CURLOPT_MAIL_RCPT, *mailTo);
	}
	return check;
}

bool NetworkHandle::setupProgress(CURL *curl, bool progress) {
	bool check = true;
	if (progress) {
		SETOPT(check, curl, CURLOPT_NOPROGRESS, 0);
		SETOPT(check, curl, CURLOPT_XFERINFOFUNCTION, Network::progress);
		SETOPT(check, curl, CURLOPT_XFERINFODATA, this);
	} else {
		SETOPT(check, curl, CURLOPT_NOPROGRESS, 1);
		SETOPT(check, curl, CURLOPT_XFERINFOFUNCTION, Network::progress);
		SETOPT(check, curl, CURLOPT_XFERINFODATA, NULL);
	}
	return check;
}

bool NetworkHandle::setupCookies(CURL *curl, const StringView &cookiePath) {
	bool check = true;
	if (!cookiePath.empty()) {
		SETOPT(check, curl, CURLOPT_COOKIEFILE, SP_TERMINATED_DATA(cookiePath));
		SETOPT(check, curl, CURLOPT_COOKIEJAR, SP_TERMINATED_DATA(cookiePath));
	}
	return check;
}

bool NetworkHandle::setupReceive(CURL *curl, FILE * & inputFile, size_t &inputPos) {
	bool check = true;
	_receiveOffset = 0;
	if (_method != Method::Head) {
		if (!_receiveFileName.empty()) {
			std::tie(inputFile, inputPos) = openFile(_receiveFileName, false, _resumeDownload);
			if (inputFile) {
				SETOPT(check, curl, CURLOPT_WRITEFUNCTION, NULL);
				SETOPT(check, curl, CURLOPT_WRITEDATA, inputFile);
				if (inputPos != 0 && _resumeDownload) {
					_receiveOffset = inputPos;
					SETOPT(check, curl, CURLOPT_RESUME_FROM, inputPos);
				}
			}
		} else if (_receiveCallback) {
			SETOPT(check, curl, CURLOPT_WRITEFUNCTION, Network::writeData);
			SETOPT(check, curl, CURLOPT_WRITEDATA, this);
		}
	}
	return check;
}


bool NetworkHandle::setupMethodGet(CURL *curl) {
	bool check = true;
    SETOPT(check, curl, CURLOPT_HTTPGET, 1);
    SETOPT(check, curl, CURLOPT_FOLLOWLOCATION, 1);
	return check;
}
bool NetworkHandle::setupMethodHead(CURL *curl) {
	bool check = true;
    SETOPT(check, curl, CURLOPT_HTTPGET, 1);
    SETOPT(check, curl, CURLOPT_FOLLOWLOCATION, 1);
    SETOPT(check, curl, CURLOPT_NOBODY, 1);
	return check;
}

bool NetworkHandle::setupMethodPost(CURL *curl, FILE * & outputFile) {
	bool check = true;
    SETOPT(check, curl, CURLOPT_POST, 1);

	SETOPT(check, curl, CURLOPT_READFUNCTION, NULL);
	SETOPT(check, curl, CURLOPT_READDATA, NULL);
    SETOPT(check, curl, CURLOPT_POSTFIELDS, NULL);
    SETOPT(check, curl, CURLOPT_POSTFIELDSIZE, 0);

	if (!_sendFileName.empty()) {
		size_t size;
		std::tie(outputFile, size) = openFile(_sendFileName, true);
		if (outputFile) {
            SETOPT(check, curl, CURLOPT_READFUNCTION, NULL);
            SETOPT(check, curl, CURLOPT_READDATA, outputFile);
            SETOPT(check, curl, CURLOPT_POSTFIELDSIZE, size);
		}
	} else if (_sendCallback) {
		SETOPT(check, curl, CURLOPT_READFUNCTION, Network::readData);
		SETOPT(check, curl, CURLOPT_READDATA, this);
		SETOPT(check, curl, CURLOPT_POSTFIELDSIZE, _sendSize);
	} else if (_sendData.size() != 0) {
		SETOPT(check, curl, CURLOPT_POSTFIELDS, _sendData.data());
		SETOPT(check, curl, CURLOPT_POSTFIELDSIZE, _sendData.size());
	}
	return check;
}
bool NetworkHandle::setupMethodPut(CURL *curl, FILE * & outputFile) {
	bool check = true;
    SETOPT(check, curl, CURLOPT_UPLOAD, 1);

	SETOPT(check, curl, CURLOPT_READFUNCTION, NULL);
	SETOPT(check, curl, CURLOPT_READDATA, NULL);
    SETOPT(check, curl, CURLOPT_INFILESIZE, 0);

	if (!_sendFileName.empty()) {
		size_t size;
		std::tie(outputFile, size) = openFile(_sendFileName, true);
		if (outputFile) {
            SETOPT(check, curl, CURLOPT_READFUNCTION, NULL);
            SETOPT(check, curl, CURLOPT_READDATA, outputFile);
            SETOPT(check, curl, CURLOPT_INFILESIZE, size);
		}
	} else if (_sendCallback || _sendData.size() != 0) {
		size_t size = (_sendCallback)?_sendSize:_sendData.size();
		SETOPT(check, curl, CURLOPT_READFUNCTION, Network::readData);
		SETOPT(check, curl, CURLOPT_READDATA, this);
		SETOPT(check, curl, CURLOPT_INFILESIZE, size);
	}
	return check;
}
bool NetworkHandle::setupMethodDelete(CURL *curl) {
	bool check = true;
    SETOPT(check, curl, CURLOPT_FOLLOWLOCATION, 1);
    SETOPT(check, curl, CURLOPT_CUSTOMREQUEST, "DELETE");
	return check;
}
bool NetworkHandle::setupMethodSmpt(CURL *curl, FILE * & outputFile) {
	bool check = true;

    SETOPT(check, curl, CURLOPT_UPLOAD, 1);

	SETOPT(check, curl, CURLOPT_READFUNCTION, NULL);
	SETOPT(check, curl, CURLOPT_READDATA, NULL);
    SETOPT(check, curl, CURLOPT_INFILESIZE, 0);

	if (!_sendFileName.empty()) {
		size_t size;
		std::tie(outputFile, size) = openFile(_sendFileName, true);
		if (outputFile) {
            SETOPT(check, curl, CURLOPT_READFUNCTION, NULL);
            SETOPT(check, curl, CURLOPT_READDATA, outputFile);
            SETOPT(check, curl, CURLOPT_INFILESIZE, size);
		}
	} else if (_sendCallback || _sendData.size() != 0) {
		size_t size = (_sendCallback)?_sendSize:_sendData.size();
		SETOPT(check, curl, CURLOPT_READFUNCTION, Network::readData);
		SETOPT(check, curl, CURLOPT_READDATA, this);
		SETOPT(check, curl, CURLOPT_INFILESIZE, size);
	}

    SETOPT(check, curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
	return check;
}

bool NetworkHandle::perform() {
	_isRequestPerformed = false;
	_errorCode = CURLE_OK;

	CURL *curl = Network::getHandle(_reuse);

	curl_slist *headers = nullptr;
	curl_slist *mailTo = nullptr;

	FILE *inputFile = nullptr;
	FILE *outputFile = nullptr;
	size_t inputPos = 0;

	_responseCode = -1;

	if (!curl) {
		return false;
	}

	bool check = true;
	bool success = false;

	char errorBuffer[CURL_ERROR_SIZE];
	memset(errorBuffer, 0, CURL_ERROR_SIZE);

	check = (check) ? setupCurl(curl, errorBuffer) : false;
	check = (check) ? setupDebug(curl, _debug) : false;
	check = (check) ? setupRootCert(curl, _rootCertFile) : false;
	check = (check) ? setupHeaders(curl, _sendedHeaders, &headers) : false;
	check = (check) ? setupUserAgent(curl, _userAgent) : false;
	check = (check) ? setupUser(curl, _user, _password) : false;
	check = (check) ? setupFrom(curl, _from) : false;
	check = (check) ? setupRecv(curl, _recv, &mailTo) : false;
	check = (check) ? setupProgress(curl, _method != Method::Head && (_uploadProgress || _downloadProgress)) : false;
	check = (check) ? setupCookies(curl, _cookieFile) : false;
	check = (check) ? setupReceive(curl, inputFile, inputPos) : false;

    switch (_method) {
        case Method::Get:
        	check = (check) ? setupMethodGet(curl) : false;
            break;
        case Method::Head:
        	check = (check) ? setupMethodHead(curl) : false;
            break;
        case Method::Post:
        	check = (check) ? setupMethodPost(curl, outputFile) : false;
            break;
        case Method::Put:
        	check = (check) ? setupMethodPut(curl, outputFile) : false;
            break;
        case Method::Delete:
        	check = (check) ? setupMethodDelete(curl) : false;
            break;
        case Method::Smtp:
        	check = (check) ? setupMethodSmpt(curl, outputFile) : false;
        	break;
        default:
            break;
    }

    if (!check) {
    	if (!_silent) {
        	log::format("CURL", "Fail to setup %s", _url.c_str());
    	}
        return false;
    }

	_debugData.clear();
	_parsedHeaders.clear();
	_recievedHeaders.clear();

    _errorCode = curl_easy_perform(curl);

	if (headers) {
		curl_slist_free_all(headers);
		headers = nullptr;
	}
	if (mailTo) {
		curl_slist_free_all(mailTo);
		mailTo = nullptr;
	}

	if (CURLE_RANGE_ERROR == _errorCode && _method == Method::Get) {
		size_t allowedRange = size_t(getReceivedHeaderInt("X-Range"));
		if (allowedRange == inputPos) {
			if (!_silent) {
				log::text("CURL", "Get 0-range is not an error, fixed error code to CURLE_OK");
			}
			success = true;
			_errorCode = CURLE_OK;
		}
    }

	if (CURLE_OK == _errorCode) {
        _isRequestPerformed = true;
        if (_method != Method::Smtp) {
            const char *ct = NULL;
            long code = 200;

        	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
        	curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &ct);
			if (ct) {
				_contentType = ct;
#if LINUX
				if (inputFile && !_contentType.empty()) {
					fflush(inputFile);
					NetworkHandle_setUserAttributes(inputFile, _contentType,
							Time::microseconds(getReceivedHeaderInt("X-FileModificationTime")));
					fclose(inputFile);
					inputFile = nullptr;
				}
#endif
			}

            _responseCode = (long)code;

        	SP_NETWORK_LOG("performed: %d %s %ld", (int)_method, _url.c_str(), _responseCode);

	        if (_responseCode == 416) {
	        	size_t allowedRange = size_t(getReceivedHeaderInt("X-Range"));
				if (allowedRange == inputPos) {
					_responseCode = 200;
					if (!_silent) {
						log::text("CURL", toString(_url, ": Get 0-range is not an error, fixed response code to 200"));
					}
				}
	        }

			if (_responseCode >= 200 && _responseCode < 400) {
				success = true;
			} else {
				success = false;
			}
        } else {
    		success = true;
        }
	} else {
		if (!_silent) {
			log::format("CURL", "fail to perform %s: (%ld) %s",  _url.c_str(), _errorCode, errorBuffer);
		}
		_error = errorBuffer;
		success = false;
    }

	Network::releaseHandle(curl, _reuse, success);

	if (inputFile) {
		fflush(inputFile);
		fclose(inputFile);
		inputFile = nullptr;
	}
	if (outputFile) {
		fclose(outputFile);
		outputFile = nullptr;
	}

	return success;
}

void NetworkHandle::setRootCertificateFile(const StringView &str) {
	_rootCertFile = str.str();
}
void NetworkHandle::setCookieFile(const StringView &str) {
	_cookieFile = str.str();
}
void NetworkHandle::setUserAgent(const StringView &str) {
	_userAgent = str.str();
}
void NetworkHandle::setUrl(const StringView &url) {
	_url = url.str();
}

void NetworkHandle::addHeader(const StringView &header) {
    _sendedHeaders.push_back(header.str());
}
void NetworkHandle::addHeader(const StringView &header, const StringView &value) {
    _sendedHeaders.push_back(toString(header, ": ", value));
}

void NetworkHandle::setDownloadProgress(const ProgressCallback &callback) {
	_downloadProgress = callback;
}
void NetworkHandle::setUploadProgress(const ProgressCallback &callback) {
	_uploadProgress = callback;
}

void NetworkHandle::setConnectTimeout(int time) {
	_connectTimeout = time;
}
void NetworkHandle::setLowSpeedLimit(int time, size_t limit) {
	_lowSpeedTime = time;
	_lowSpeedLimit = int(limit);
}

void NetworkHandle::setMailFrom(const StringView &from) {
	_from = from.str();
}
void NetworkHandle::addMailTo(const StringView &to) {
	_recv.push_back(to.str());
}
void NetworkHandle::setAuthority(const StringView &user, const StringView &passwd) {
	_user = user.str();
	_password = passwd.str();
}
void NetworkHandle::setReceiveFile(const StringView &str, bool resumeDownload) {
	_receiveCallback = nullptr;
	_receiveFileName = str.str();
	_resumeDownload = resumeDownload;
}
void NetworkHandle::setReceiveCallback(const IOCallback &cb) {
	_receiveCallback = cb;
	_receiveFileName = "";
	_resumeDownload = false;
}

void NetworkHandle::setResumeDownload(bool value) {
	if (!_receiveFileName.empty()) {
		_resumeDownload = value;
	}
}

void NetworkHandle::setSendSize(size_t size) {
	if (_sendCallback) {
		_sendSize = size;
	}
}
void NetworkHandle::setSendFile(const StringView &str, const StringView &type) {
	_sendFileName = str.str();
	_sendCallback = nullptr;
	_sendData.clear();
	_sendSize = 0;
	if (!type.empty()) {
		addHeader("Content-Type", type);
	} else {
		bool isSet = false;
#if LINUX
		auto t = NetworkHandle_getUserMime(_sendFileName);
		if (!t.empty()) {
			addHeader("Content-Type", t);
			isSet = true;
		}
#endif
		if (!isSet) {
			// try image format
			auto fmt = Bitmap::detectFormat(StringView(_sendFileName));
			switch (fmt.first) {
			case Bitmap::FileFormat::Png: addHeader("Content-Type: image/png"); isSet = true; break;
			case Bitmap::FileFormat::Jpeg: addHeader("Content-Type: image/jpeg"); isSet = true; break;
			case Bitmap::FileFormat::WebpLossless: addHeader("Content-Type: image/webp"); isSet = true; break;
			case Bitmap::FileFormat::WebpLossy: addHeader("Content-Type: image/webp"); isSet = true; break;
			case Bitmap::FileFormat::Svg: addHeader("Content-Type: image/svg+xml"); isSet = true; break;
			case Bitmap::FileFormat::Gif: addHeader("Content-Type: image/gif"); isSet = true; break;
			case Bitmap::FileFormat::Tiff: addHeader("Content-Type: image/tiff"); isSet = true; break;
			case Bitmap::FileFormat::Custom: break;
			}
		}
	}
}
void NetworkHandle::setSendCallback(const IOCallback &cb, size_t outSize) {
	_sendFileName = "";
	_sendCallback = cb;
	_sendData.clear();
	_sendSize = outSize;
}
void NetworkHandle::setSendData(const StringView &data) {
	_sendFileName = "";
	_sendCallback = nullptr;
	_sendData.clear();
	_sendSize = data.size();
	_sendData.assign((uint8_t *)data.data(), (uint8_t *)data.data() + _sendSize);
}
void NetworkHandle::setSendData(const Bytes &data) {
	_sendFileName = "";
	_sendCallback = nullptr;
	_sendData = data;
	_sendSize = data.size();
}
void NetworkHandle::setSendData(Bytes &&data) {
	_sendFileName = "";
	_sendCallback = nullptr;
	_sendData = std::move(data);
	_sendSize = _sendData.size();
}
void NetworkHandle::setSendData(const uint8_t *data, size_t size) {
	_sendFileName = "";
	_sendCallback = nullptr;
	_sendData.clear();
	_sendSize = size;
	_sendData.assign(data, data + size);
}
void NetworkHandle::setSendData(const data::Value &data, data::EncodeFormat fmt) {
	_sendFileName = "";
	_sendCallback = nullptr;
	_sendData = data::write(data, fmt);
	_sendSize = _sendData.size();
	if (fmt.format == data::EncodeFormat::Cbor || fmt.format == data::EncodeFormat::DefaultFormat) {
		addHeader("Content-Type", "application/cbor");
	} else if (fmt.format == data::EncodeFormat::Json || fmt.format == data::EncodeFormat::Pretty) {
		addHeader("Content-Type", "application/json");
	}
}

size_t NetworkHandle::writeData(char *data, size_t size) {
	if (_receiveCallback) {
		return _receiveCallback(data, size);
	}
    return size;
}

size_t NetworkHandle::writeHeaders(const char *data, size_t size) {
	StringView reader(data, size);

	if (!reader.is("\r\n")) {
		if (_method != Method::Smtp) {
			if (!reader.is("HTTP/")) {
				auto name = reader.readUntil<StringView::Chars<':'>>();
				reader ++;

				auto nameStr = name.str(); string::trim(nameStr); string::tolower(nameStr);
				auto valueStr = reader.str(); string::trim(valueStr);

				_parsedHeaders.emplace(std::move(nameStr), std::move(valueStr));
			} else {
				reader.skipUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
				reader.skipUntil<StringView::CharGroup<CharGroupId::Numbers>>();
				reader.readInteger().unwrap([&] (int64_t code) {
					_responseCode = code;
				});
			}
		}

		_recievedHeaders.push_back(String(data, size));
	}

    return size;
}

size_t NetworkHandle::writeDebug(char *data, size_t size) {
	_debugData.write(data, size);
	return size;
}

size_t NetworkHandle::readData(char *data, size_t size) {
	if (_sendCallback) {
		return _sendCallback(data, size);
	} else if (_sendData.size() != 0) {
		size_t remains = _sendSize;
		if (size <= remains) {
			memcpy(data, _sendData.data() + (_sendData.size() - _sendSize), size);
			_sendSize -= size;
			return size;
		} else {
			memcpy(data, _sendData.data() + (_sendData.size() - _sendSize), remains);
			_sendSize = 0;
			return remains;
		}
	}
    return size;
}

int NetworkHandle::progressUpload(int64_t ultotal, int64_t ulnow) {
	auto timing = Time::now();
	if (_uploadProgress && ulnow != _uploadProgressValue
		&& (!_uploadProgressTiming || (timing - _uploadProgressTiming > SP_NETWORK_PROGRESS_TIMEOUT))) {
		_uploadProgressValue = ulnow;
		_uploadProgressTiming = timing;
		return _uploadProgress(ultotal, ulnow);
	}
    return 0;
}

int NetworkHandle::progressDownload(int64_t dltotal, int64_t dlnow) {
	auto timing = Time::now();
	if (_downloadProgress && dlnow != _downloadProgressValue
		&& (!_downloadProgressTiming || (timing - _downloadProgressTiming > SP_NETWORK_PROGRESS_TIMEOUT))) {
		_downloadProgressValue = dlnow;
		_downloadProgressTiming = timing;
		return _downloadProgress(dltotal + _receiveOffset, dlnow + _receiveOffset);
	}
    return 0;
}

Pair<FILE *, size_t> NetworkHandle::openFile(const String &filename, bool readOnly, bool resume) {
	size_t pos = 0;
	FILE *file = nullptr;
	String path = filepath::absolute(filename);
	if (filesystem::exists(path)) {
		pos = filesystem::size(path);
		if (!readOnly) {
			if (!resume) {
				filesystem::remove(path);
				file = filesystem_native::fopen_fn(path, "wb");
			} else {
				file = filesystem_native::fopen_fn(path, "a+b");
			}
		} else {
			file = filesystem_native::fopen_fn(path, "rb");
		}
	} else {
		if (!readOnly) {
			file = filesystem_native::fopen_fn(path, "wb");
		}
	}
    return pair(file, pos);
}

StringView NetworkHandle::getReceivedHeaderString(const StringView &ch) const {
	String h = string::tolower(ch);
	auto i = _parsedHeaders.find(h);
	if (i != _parsedHeaders.end()) {
		return i->second;
	} else {
		return StringView();
	}
}

int64_t NetworkHandle::getReceivedHeaderInt(const StringView &ch) const {
	String h = string::tolower(ch);
	auto i = _parsedHeaders.find(h);
	if (i != _parsedHeaders.end()) {
		if (!i->second.empty()) {
			return atoll(i->second.c_str());
		}
	}
	return 0;
}

#undef SP_TERMINATED_DATA

NS_SP_END
