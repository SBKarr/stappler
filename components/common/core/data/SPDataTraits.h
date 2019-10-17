/**
Copyright (c) 2016-2019 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef STAPPLER_SRC_CORE_DATA_SPDATATRAITS_H_
#define STAPPLER_SRC_CORE_DATA_SPDATATRAITS_H_

#include "SPCommon.h"

NS_SP_EXT_BEGIN(data)

template <typename T>
struct StreamTraits {
	using success = char;
	using failure = long;

	InvokerCallTest_MakeCallTest(onNextValue, success, failure);
	InvokerCallTest_MakeCallTest(onKey, success, failure);
	InvokerCallTest_MakeCallTest(onBeginArray, success, failure);
	InvokerCallTest_MakeCallTest(onEndArray, success, failure);
	InvokerCallTest_MakeCallTest(onBeginDict, success, failure);
	InvokerCallTest_MakeCallTest(onEndDict, success, failure);
	InvokerCallTest_MakeCallTest(onKeyValuePair, success, failure);
	InvokerCallTest_MakeCallTest(onValue, success, failure);
	InvokerCallTest_MakeCallTest(onArrayValue, success, failure);
};

NS_SP_EXT_END(data)

#endif /* STAPPLER_SRC_CORE_DATA_SPDATATRAITS_H_ */
