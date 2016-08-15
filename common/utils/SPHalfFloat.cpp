/*
 * SPHalfFloat.cpp
 *
 *  Created on: 23 нояб. 2015 г.
 *      Author: sbkarr
 */

#include "SPCommon.h"
#include "SPHalfFloat.h"

NS_SP_EXT_BEGIN(halffloat)

float decode(uint16_t half) {
	uint16_t exp = (half >> 10) & 0x1f;
	uint16_t mant = half & 0x3ff;
    double val;

    if (exp == 0) {
        val = ldexp(mant, -24);
    } else if (exp != 31) {
        val = ldexp(mant + 1024, exp - 25);
    } else {
        val = mant == 0 ? NumericLimits<float>::infinity() : stappler::nan();
    }

    return (half & 0x8000) ? -val : val;
}

uint16_t encode(float val) {
    union {
        float f;
        uint32_t i;
    } u32;

    u32.f = val;

    uint16_t bits = (u32.i >> 16) & 0x8000; /* Get the sign */
    uint16_t m = (u32.i >> 12) & 0x07ff; /* Keep one extra bit for rounding */
    uint32_t e = (u32.i >> 23) & 0xff; /* Using int is faster here */

    /* If zero, or denormal, or exponent underflows too much for a denormal
     * half, return signed zero. */
    if (e < 103) {
        return bits;
    }

    /* If NaN, return NaN. If Inf or exponent overflow, return Inf. */
    if (e > 142) {
        bits |= 0x7c00u;
        /* If exponent was 0xff and one mantissa bit was set, it means NaN,
         * not Inf, so make sure we set one mantissa bit too. */
        bits |= (e == 255) && (u32.i & 0x007fffffu);
        return bits;
    }

    /* If exponent underflows but not too much, return a denormal */
    if (e < 113) {
        m |= 0x0800u;
        /* Extra rounding may overflow and set mantissa to 0 and exponent
         * to 1, which is OK. */
        bits |= (m >> (114 - e)) + ((m >> (113 - e)) & 1);
        return bits;
    }

    bits |= ((e - 112) << 10) | (m >> 1);
    /* Extra rounding. An overflow will set mantissa to 0 and increment
     * the exponent, which is OK. */
    bits += m & 1;
    return bits;
}

NS_SP_EXT_END(halffloat)
