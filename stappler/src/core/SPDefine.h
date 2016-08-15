//
//  SPDefine.h
//  chieftime-federal
//
//  Created by SBKarr on 26.06.13.
//
//

#ifndef stappler_core_SPDefine_h
#define stappler_core_SPDefine_h

#include "SPDefault.h"
#include "SPConfig.h"

#if (SP_NOSTOREKIT)
#define SP_INTERNAL_STOREKIT_ENABLED 0
#elif (SP_STOREKIT)
#define SP_INTERNAL_STOREKIT_ENABLED 1
#else
#define SP_INTERNAL_STOREKIT_ENABLED SP_CONFIG_STOREKIT
#endif

#ifndef SP_DRAW
#define SP_DRAW SP_CONFIG_DRAW
#endif

NS_SP_BEGIN

inline std::string operator"" _locale ( const char* str, std::size_t len) {
	std::string ret;
	ret.reserve(len + "@Locale:"_len);
	ret.append("@Locale:");
	ret.append(str, len);
	return ret;
}

NS_SP_END

#endif
