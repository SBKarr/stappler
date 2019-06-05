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

#ifndef STELLATOR_CORE_STCONFIG_H_
#define STELLATOR_CORE_STCONFIG_H_

#include "SPCommon.h"
#include "SPTime.h"

namespace stellator::config {

constexpr size_t getMaxVarSize() { return 255; }
constexpr size_t getMaxRequestSize() { return 0; }
constexpr size_t getMaxFileSize() { return 0; }

constexpr auto getInputUpdateTime() { return stappler::TimeInterval::seconds(1); }
constexpr auto getInputUpdateFrequency() { return 0.1f; }

constexpr auto getMaxAuthTime() { return stappler::TimeInterval::seconds(720); }
constexpr auto getMaxLoginFailure() { return 4; }

constexpr size_t getMaxInputPostSize() { return 250_MiB; }
constexpr size_t getMaxInputFileSize() { return 250_MiB; }
constexpr size_t getMaxInputVarSize() { return 8_KiB; }

constexpr size_t getMaxExtraFieldSize() { return 8_KiB; }

constexpr auto getDefaultTextMin() { return 3; }
constexpr auto getDefaultTextMax() { return 256; }

inline auto getDefaultPasswordSalt() { return "StapplerUserPasswordKey"_weak; }
inline auto getInternalPasswordKey() { return "Stappler Password Salt"_weak; }

inline stappler::TimeInterval getKeyValueStorageTime() { return stappler::TimeInterval::seconds(60 * 60 * 24 * 365); }
inline stappler::TimeInterval getInternalsStorageTime() { return stappler::TimeInterval::seconds(60 * 60 * 24 * 30); }

inline auto getStorageInterfaceKey() { return "StorageInterface"; }

constexpr auto getUploadTmpFilePrefix() { return "sa.upload"; }
constexpr auto getUploadTmpImagePrefix() { return "sa.image"; }

constexpr uint16_t getResourceResolverMaxDepth() { return 4; }


constexpr auto getDefaultSessionName() { return "SID"; }
constexpr auto getDefaultSessionKey() { return "SerenitySession"; }


}

#endif /* STELLATOR_CORE_STCONFIG_H_ */
