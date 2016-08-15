/*
 * SPAprUuid.h
 *
 *  Created on: 14 февр. 2016 г.
 *      Author: sbkarr
 */

#ifndef COMMON_APR_SPAPRUUID_H_
#define COMMON_APR_SPAPRUUID_H_

#include "SPAprString.h"

#if SPAPR

NS_SP_EXT_BEGIN(apr)

struct uuid : AllocPool {
	static uuid generate() {
		apr_uuid_t ret;
		apr_uuid_get(&ret);
		return ret;
	}

	uuid() {
		memset(_uuid.data, 0, 16);
	}

	uuid(const string &str) {
		apr_uuid_parse(&_uuid, str.data());
	}

	uuid(const Bytes &b) {
		if (b.size() == 16) {
			memcpy(_uuid.data, b.data(), 16);
		}
	}

	uuid(const apr_uuid_t &u) : _uuid(u) { }
	uuid(const uuid &u) : _uuid(u._uuid) { }
	uuid &operator= (const uuid &u) { _uuid = u._uuid; return *this; }
	uuid &operator= (const Bytes &b) {
		if (b.size() == 16) {
			memcpy(_uuid.data, b.data(), 16);
		}
		return *this;
	}

	string str() const {
		string ret; ret.reserve(APR_UUID_FORMATTED_LENGTH);
		apr_uuid_format(ret.data(), &_uuid);
		return ret;
	}

	Bytes bytes() const {
		Bytes ret; ret.resize(16);
		memcpy(ret.data(), _uuid.data, 16);
		return ret;
	}

	const uint8_t *data() const { return _uuid.data; }
	size_t size() const { return 16; }

	apr_uuid_t _uuid;
};

NS_SP_EXT_END(apr)
#endif

#endif /* COMMON_APR_SPAPRUUID_H_ */
