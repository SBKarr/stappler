/*
 * SPHalfFloat.h
 *
 *  Created on: 23 нояб. 2015 г.
 *      Author: sbkarr
 */

#ifndef STAPPLER_SRC_CORE_STRUCT_SPHALFFLOAT_H_
#define STAPPLER_SRC_CORE_STRUCT_SPHALFFLOAT_H_

#include "SPCommon.h"

NS_SP_EXT_BEGIN(halffloat)

// see https://en.wikipedia.org/wiki/Half_precision_floating-point_format

constexpr uint16_t nan() { return (uint16_t)0x7e00; }
constexpr uint16_t posinf() { return (uint16_t)(31 << 10); }
constexpr uint16_t neginf() { return (uint16_t)(63 << 10); }

float decode(uint16_t half);
uint16_t encode(float val);


NS_SP_EXT_END(halffloat)

#endif /* STAPPLER_SRC_CORE_STRUCT_SPHALFFLOAT_H_ */
