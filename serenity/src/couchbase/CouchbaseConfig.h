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

#ifndef SERENITY_SRC_COUCHBASE_COUCHBASECONFIG_H_
#define SERENITY_SRC_COUCHBASE_COUCHBASECONFIG_H_

#include "Define.h"

#ifndef NOCB

NS_SA_EXT_BEGIN(couchbase)

struct Config {
	Config();
	void init(data::Value &&);

	apr::string host;
	apr::string name;
	apr::string password;
	apr::string bucket;

	int min = 0;
	int softMax = 3;
	int hardMax = 8;
	apr_interval_time_t ttl = 30_apr_sec;
};

// Major type value defines how to parse value in slot
enum class StorageType {
	None = 0, // Non-specific - application defined data
	Int, // Plain-text int value
	Set, // Array of values
	Data // data::Value
};

Bytes Key(const Bytes &);
Bytes Key(const String &, bool weak = false);
String Text(const Bytes &, bool weak = false);
Bytes Uuid(const apr::uuid &);
Bytes Oid(uint64_t oid);
Bytes Oid(uint64_t oid, Bytes &&);
Bytes Sid(uint64_t sid);
Bytes Sid(uint64_t sid, Bytes &&);

NS_SA_EXT_END(couchbase)

#endif

#endif /* SERENITY_SRC_COUCHBASE_COUCHBASECONFIG_H_ */
