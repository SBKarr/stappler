/**
Copyright (c) 2016-2018 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef SERENITY_SRC_CORE_SEDEFINE_H_
#define	SERENITY_SRC_CORE_SEDEFINE_H_

#include "SEForward.h"

#include "SPAprAllocator.h"
#include "SPAprArray.h"
#include "SPAprTable.h"
#include "SPAprUri.h"
#include "SPAprMutex.h"

#include "SPRef.h"
#include "SPData.h"

#include "SPug.h"

#include <typeindex>

NS_SA_EXT_BEGIN(mem)

using namespace stappler::mem_pool;

using uuid = stappler::memory::uuid;

using ostringstream = stappler::memory::ostringstream;

using apr::pool::request;
using apr::pool::server;

NS_SA_EXT_END(mem)


NS_SA_BEGIN

enum class ResourceType {
	ResourceList,
	ReferenceSet,
	ObjectField,
	Object,
	Set,
	View,
	File,
	Array,
	Search
};

constexpr apr_time_t operator"" _apr_sec ( unsigned long long int val ) { return val * 1000000; }
constexpr apr_time_t operator"" _apr_msec ( unsigned long long int val ) { return val * 1000; }
constexpr apr_time_t operator"" _apr_mksec ( unsigned long long int val ) { return val; }

NS_SA_END

#endif	/* SERENITY_SRC_CORE_SEDEFINE_H_ */
