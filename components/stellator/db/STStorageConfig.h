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

#ifndef STELLATOR_DB_STSTORAGECONFIG_H_
#define STELLATOR_DB_STSTORAGECONFIG_H_

#include "SPCommon.h"
#include "SPTime.h"

#define NS_DB_BEGIN		namespace db {
#define NS_DB_END		}

#define NS_DB_SQL_BEGIN	namespace db { namespace sql {
#define NS_DB_SQL_END	} }

#define NS_DB_PQ_BEGIN	namespace db { namespace pq {
#define NS_DB_PQ_END	} }

#ifdef SPAPR

// use Serenity config

#include "SEConfig.h"
#include "SPAprFileStream.h"

NS_DB_BEGIN

using file_t = stappler::apr::file;

namespace config {

using namespace stappler::serenity::config;

}

NS_DB_END

#elif STELLATOR

#include "STConfig.h"
#include "SPFilesystem.h"

NS_DB_BEGIN

using file_t = stappler::filesystem::ifile;

constexpr stappler::TimeInterval operator"" _sec ( unsigned long long int val ) { return stappler::TimeInterval::seconds((time_t)val); }
constexpr stappler::TimeInterval operator"" _msec ( unsigned long long int val ) { return stappler::TimeInterval::milliseconds(val); }
constexpr stappler::TimeInterval operator"" _mksec ( unsigned long long int val ) { return stappler::TimeInterval::microseconds(val); }

namespace config {

using namespace stellator::config;

}

NS_DB_END

#else

// use custom config

#include "SPFilesystem.h"

NS_DB_BEGIN

using file_t = stappler::filesystem::ifile;

constexpr stappler::TimeInterval operator"" _sec ( unsigned long long int val ) { return stappler::TimeInterval::seconds((time_t)val); }
constexpr stappler::TimeInterval operator"" _msec ( unsigned long long int val ) { return stappler::TimeInterval::milliseconds(val); }
constexpr stappler::TimeInterval operator"" _mksec ( unsigned long long int val ) { return stappler::TimeInterval::microseconds(val); }

namespace config {

constexpr auto getMaxAuthTime() { return 720_sec; }
constexpr auto getMaxLoginFailure() { return 4; }

constexpr size_t getMaxInputPostSize() { return 250_MiB; }
constexpr size_t getMaxInputFileSize() { return 250_MiB; }
constexpr size_t getMaxInputVarSize() { return 8_KiB; }

constexpr size_t getMaxExtraFieldSize() { return 8_KiB; }

constexpr auto getDefaultTextMin() { return 3; }
constexpr auto getDefaultTextMax() { return 256; }

inline auto getDefaultPasswordSalt() { return "StapplerUserPasswordKey"_weak; }
inline auto getInternalPasswordKey() { return "Stappler Password Salt"_weak; }

inline stappler::TimeInterval getKeyValueStorageTime() { return stappler::TimeInterval::seconds(60 * 60 * 24 * 365); } // one year
inline stappler::TimeInterval getInternalsStorageTime() { return stappler::TimeInterval::seconds(60 * 60 * 24 * 30); }

constexpr auto getStorageInterfaceKey() { return "ST.StorageInterface"; }
constexpr auto getTransactionPrefixKey() { return "ST.Tr."; } // limit for 6 chars to use with SOO opts (6 + 16 < 23)
constexpr auto getTransactionStackKey() { return "ST.Transaction.Stack"; }

constexpr auto getUploadTmpFilePrefix() { return "sa.upload"; }
constexpr auto getUploadTmpImagePrefix() { return "sa.image"; }

constexpr uint16_t getResourceResolverMaxDepth() { return 4; }

}

NS_DB_END

#endif

#endif /* STELLATOR_DB_STSTORAGECONFIG_H_ */
