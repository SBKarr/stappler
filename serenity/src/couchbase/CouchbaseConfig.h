/*
 * CouchbaseConfig.h
 *
 *  Created on: 17 февр. 2016 г.
 *      Author: sbkarr
 */

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
