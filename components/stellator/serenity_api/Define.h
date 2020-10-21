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

#ifndef STELLATOR_SERENITY_API_DEFINE_H_
#define STELLATOR_SERENITY_API_DEFINE_H_

#if STELLATOR

#include "STDefine.h"
#include "STMemory.h"
#include "SPUrl.h"
#include "SPHtmlParser.h"
#include "SPBitmap.h"
#include "SPNetworkHandle.h"

#define NS_SA_BEGIN				namespace stellator::serenity {
#define NS_SA_END				}
#define USING_NS_SA				using namespace stellator::serenity

#define NS_SA_ST_BEGIN			namespace stellator {
#define NS_SA_ST_END			}
#define USING_NS_SA_ST			using namespace stellator

#define NS_SA_EXT_BEGIN(v)		namespace stellator::serenity::v {
#define NS_SA_EXT_END(v)		}
#define USING_NS_SA_EXT(v)		using namespace stappler::serenity::v


namespace stellator::serenity {

using namespace stappler::mem_pool;

using uuid = stappler::memory::uuid;

using ostringstream = stappler::memory::ostringstream;

using AllocPool = stappler::memory::AllocPool;
using User = db::User;
using Buffer = stappler::BufferTemplate<mem::Interface>;
using UrlView = stappler::UrlView;

using ByteOrder = stappler::ByteOrder;
using CharGroupId = stappler::CharGroupId;

template <ByteOrder::Endian Endianess = ByteOrder::Endian::Network>
using DataReader = stappler::BytesViewTemplate<Endianess>;

using BytesView = stappler::BytesView;
using CoderSource = stappler::CoderSource;

using Bitmap = stappler::Bitmap;

template <typename T>
using Rc = stappler::Rc<T>;

using sp_time_exp_t = stappler::sp_time_exp_t;

template <size_t Size>
using StackBuffer = stappler::StackBuffer<Size>;

namespace string { using namespace stappler::string; }
namespace base16 { using namespace stappler::base16; }
namespace base64 { using namespace stappler::base64; }
namespace base64url { using namespace stappler::base64url; }
namespace valid { using namespace stappler::valid; }
namespace filesystem { using namespace stappler::filesystem; }
namespace filepath { using namespace stappler::filepath; }
namespace search { using namespace stappler::search; }
namespace hash { using namespace stappler::hash; }
namespace html { using namespace stappler::html; }
namespace log { using namespace stappler::log; }

using std::forward;
using std::move;
using std::min;
using std::max;
using stappler::toInt;

template <typename T, typename V>
using Pair = std::pair<T, V>;

template <typename T>
using NumericLimits = std::numeric_limits<T>;

template <typename T = float>
inline auto nan() -> T {
	return NumericLimits<T>::quiet_NaN();
}

template <typename T = float>
inline auto epsilon() -> T {
	return NumericLimits<T>::epsilon();
}

template <typename T>
inline auto isnan(T && t) -> bool {
	return std::isnan(std::forward<T>(t));
}

template <class T>
inline constexpr T maxOf() {
	return NumericLimits<T>::max();
}

template <class T>
inline constexpr T minOf() {
	return NumericLimits<T>::min();
}

using stappler::pair;

constexpr TimeInterval operator"" _sec ( unsigned long long int val ) { return TimeInterval::seconds((time_t)val); }
constexpr TimeInterval operator"" _msec ( unsigned long long int val ) { return TimeInterval::milliseconds(val); }
constexpr TimeInterval operator"" _mksec ( unsigned long long int val ) { return TimeInterval::microseconds(val); }

using InputConfig = db::InputConfig;
using NetworkHandle = stappler::NetworkHandle;

}


namespace stellator::serenity::storage {

using Field = db::Field;
using Flags = db::Flags;
using RemovePolicy = db::RemovePolicy;
using Transform = db::Transform;
using Scheme = db::Scheme;
using MaxFileSize = db::MaxFileSize;
using MaxImageSize = db::MaxImageSize;
using Thumbnail = db::Thumbnail;
using Transaction = db::Transaction;
using Adapter = db::Adapter;

using MinLength = db::MinLength;
using MaxLength = db::MaxLength;
using ViewFn = db::ViewFn;
using FieldView = db::FieldView;

using AccessRole = db::AccessRole;
using AccessRoleId = db::AccessRoleId;
using MaxImageSize = db::MaxImageSize;
using ImagePolicy = db::ImagePolicy;

using Query = db::Query;
using Auth = db::Auth;
using Worker = db::Worker;

using Ordering = db::Ordering;
using UpdateFlags = db::UpdateFlags;
using FullTextData = db::FullTextData;

using ReadFilterFn = db::ReadFilterFn;
using WriteFilterFn = db::WriteFilterFn;
using ViewLinkageFn = db::ViewLinkageFn;
using ReplaceFilterFn = db::ReplaceFilterFn;
using FilterFn = db::FilterFn;
using DefaultFn = db::DefaultFn;
using FullTextViewFn = db::FullTextViewFn;
using FullTextQueryFn = db::FullTextViewFn;

using Action = db::Action;

}


namespace stellator::serenity::data {

using Value = mem::Value;
using EncodeFormat = stappler::data::EncodeFormat;
using Stream = stappler::data::Stream;

using stappler::data::read;
using stappler::data::write;
using stappler::data::readFile;

}

#else

#include "SEDefine.h"

#endif

#endif /* STELLATOR_SERENITY_API_DEFINE_H_ */
