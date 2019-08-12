/**
Copyright (c) 2019 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef STELLATOR_CORE_STDEFINE_H_
#define STELLATOR_CORE_STDEFINE_H_

#include "STConfig.h"
#include "SPMemory.h"
#include "SPValid.h"

#include "SPug.h"

#include "STInputFile.h"
#include "STStorageScheme.h"
#include "STStorageUser.h"
#include "STStorageFile.h"

#include <typeindex>


namespace stellator {

namespace pug = stappler::pug;
namespace messages = db::messages;

class Root;
class Server;
class Task;
class Request;
class Connection;

class ServerComponent;
class RequestHandler;

class Session;

class InputFilter;

namespace mem {

using namespace stappler::mem_pool;

using MemPool = stappler::memory::MemPool;
using uuid = stappler::memory::uuid;

using ostringstream = stappler::memory::ostringstream;

}


namespace websocket {

class Manager;

}


enum class ResourceType {
	ResourceList,
	ReferenceSet,
	ObjectField,
	Object,
	Set,
	View,
	File,
	Array,
	Search
};

struct InputConfig {
	enum class Require {
		None = 0,
		Data = 1,
		Files = 2,
		Body = 4,
		FilesAsData = 8,
	};

	InputConfig() = default;
	InputConfig(const InputConfig &) = default;
	InputConfig & operator=(const InputConfig &) = default;

	InputConfig(InputConfig &&) = default;
	InputConfig & operator=(InputConfig &&) = default;

	Require required = Require::None;
	size_t maxRequestSize = config::getMaxRequestSize();
	size_t maxVarSize = config::getMaxVarSize();
	size_t maxFileSize = config::getMaxFileSize();

	mem::TimeInterval updateTime = config::getInputUpdateTime();
	float updateFrequency = config::getInputUpdateFrequency();
};

SP_DEFINE_ENUM_AS_MASK(InputConfig::Require)

static constexpr int OK = 0;
static constexpr int DECLINED = -1;
static constexpr int DONE = -2;
static constexpr int SUSPENDED = -3;

static constexpr int HTTP_CONTINUE = 100;
static constexpr int HTTP_SWITCHING_PROTOCOLS = 101;
static constexpr int HTTP_PROCESSING = 102;
static constexpr int HTTP_OK = 200;
static constexpr int HTTP_CREATED = 201;
static constexpr int HTTP_ACCEPTED = 202;
static constexpr int HTTP_NON_AUTHORITATIVE = 203;
static constexpr int HTTP_NO_CONTENT = 204;
static constexpr int HTTP_RESET_CONTENT = 205;
static constexpr int HTTP_PARTIAL_CONTENT = 206;
static constexpr int HTTP_MULTI_STATUS = 207;
static constexpr int HTTP_ALREADY_REPORTED = 208;
static constexpr int HTTP_IM_USED = 226;
static constexpr int HTTP_MULTIPLE_CHOICES = 300;
static constexpr int HTTP_MOVED_PERMANENTLY  = 301;
static constexpr int HTTP_MOVED_TEMPORARILY = 302;
static constexpr int HTTP_SEE_OTHER = 303;
static constexpr int HTTP_NOT_MODIFIED = 304;
static constexpr int HTTP_USE_PROXY = 305;
static constexpr int HTTP_TEMPORARY_REDIRECT = 307;
static constexpr int HTTP_PERMANENT_REDIRECT = 308;
static constexpr int HTTP_BAD_REQUEST = 400;
static constexpr int HTTP_UNAUTHORIZED = 401;
static constexpr int HTTP_PAYMENT_REQUIRED = 402;
static constexpr int HTTP_FORBIDDEN = 403;
static constexpr int HTTP_NOT_FOUND = 404;
static constexpr int HTTP_METHOD_NOT_ALLOWED = 405;
static constexpr int HTTP_NOT_ACCEPTABLE = 406;
static constexpr int HTTP_PROXY_AUTHENTICATION_REQUIRED = 407;
static constexpr int HTTP_REQUEST_TIME_OUT = 408;
static constexpr int HTTP_CONFLICT = 409;
static constexpr int HTTP_GONE = 410;
static constexpr int HTTP_LENGTH_REQUIRED = 411;
static constexpr int HTTP_PRECONDITION_FAILED = 412;
static constexpr int HTTP_REQUEST_ENTITY_TOO_LARGE = 413;
static constexpr int HTTP_REQUEST_URI_TOO_LARGE = 414;
static constexpr int HTTP_UNSUPPORTED_MEDIA_TYPE = 415;
static constexpr int HTTP_RANGE_NOT_SATISFIABLE = 416;
static constexpr int HTTP_EXPECTATION_FAILED = 417;
static constexpr int HTTP_MISDIRECTED_REQUEST = 421;
static constexpr int HTTP_UNPROCESSABLE_ENTITY = 422;
static constexpr int HTTP_LOCKED = 423;
static constexpr int HTTP_FAILED_DEPENDENCY = 424;
static constexpr int HTTP_UPGRADE_REQUIRED = 426;
static constexpr int HTTP_PRECONDITION_REQUIRED = 428;
static constexpr int HTTP_TOO_MANY_REQUESTS = 429;
static constexpr int HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE = 431;
static constexpr int HTTP_UNAVAILABLE_FOR_LEGAL_REASONS = 451;
static constexpr int HTTP_INTERNAL_SERVER_ERROR = 500;
static constexpr int HTTP_NOT_IMPLEMENTED = 501;
static constexpr int HTTP_BAD_GATEWAY = 502;
static constexpr int HTTP_SERVICE_UNAVAILABLE = 503;
static constexpr int HTTP_GATEWAY_TIME_OUT = 504;
static constexpr int HTTP_VERSION_NOT_SUPPORTED = 505;
static constexpr int HTTP_VARIANT_ALSO_VARIES = 506;
static constexpr int HTTP_INSUFFICIENT_STORAGE = 507;
static constexpr int HTTP_LOOP_DETECTED = 508;
static constexpr int HTTP_NOT_EXTENDED = 510;
static constexpr int HTTP_NETWORK_AUTHENTICATION_REQUIRED = 511;

}

#endif /* STELLATOR_CORE_STDEFINE_H_ */
