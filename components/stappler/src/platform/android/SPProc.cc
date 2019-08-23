// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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

#include "SPCommon.h"
#include "SPCommonPlatform.h"

#if (ANDROID)

#include <cpu-features.h>

NS_SP_PLATFORM_BEGIN

namespace proc {
bool _isArmNeonSupported() {
#if defined (__arm64__) || defined (__aarch64__)
	return true;
#elif defined (__ARM_NEON__)
	class AnrdoidNeonChecker {
	public:
		AnrdoidNeonChecker()
		{
			if (android_getCpuFamily() == ANDROID_CPU_FAMILY_ARM && (android_getCpuFeatures() & ANDROID_CPU_ARM_FEATURE_NEON) != 0) {
				_isNeonEnabled = true;
			} else {
				_isNeonEnabled = false;
			}
		}
		bool isNeonEnabled() const {return _isNeonEnabled;}
	private:
		bool _isNeonEnabled;
	};
	static AnrdoidNeonChecker checker;
	return checker.isNeonEnabled();
#else
	return false;
#endif
}
}

NS_SP_PLATFORM_END

#endif
