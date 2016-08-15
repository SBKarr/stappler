/*
 * SPHash.cpp
 *
 *  Created on: 16 дек. 2015 г.
 *      Author: sbkarr
 */

#include "SPCommon.h"
#include "SPCommonPlatform.h"

#if (SPAPR && LINUX)

#include <openssl/sha.h>

NS_SP_PLATFORM_BEGIN

namespace hash {
	bool _sha512(Bytes &buf, const Bytes &source) {
		if ( SHA512(source.data(), (long)source.size(), buf.data()) ) {
			return true;
		}
		return false;
	}
}

NS_SP_PLATFORM_END

#endif
