/*
 * SPCharGroup.h
 *
 *  Created on: 25 июля 2015 г.
 *      Author: sbkarr
 */

#ifndef LIBS_STAPPLER_CORE_STRUCT_SPCHARGROUP_H_
#define LIBS_STAPPLER_CORE_STRUCT_SPCHARGROUP_H_

#include "SPCharMatching.h"

NS_SP_BEGIN

bool inCharGroup(chars::CharGroupId mask, char16_t);
bool inCharGroupMask(chars::CharGroupId mask, char16_t);
WideString getCharGroup(chars::CharGroupId mask);

NS_SP_END

#endif /* LIBS_STAPPLER_CORE_STRUCT_SPCHARGROUP_H_ */

