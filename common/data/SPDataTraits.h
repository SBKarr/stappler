/*
 * SPDataTraits.h
 *
 *  Created on: 8 дек. 2015 г.
 *      Author: sbkarr
 */

#ifndef STAPPLER_SRC_CORE_DATA_SPDATATRAITS_H_
#define STAPPLER_SRC_CORE_DATA_SPDATATRAITS_H_

#include "SPCommon.h"

NS_SP_EXT_BEGIN(data)

class Value;

using Array = Vector<Value>;
using Dictionary = Map<String, Value>;

InvokerCallTest_MakeInvoker(Data, onNextValue);
InvokerCallTest_MakeInvoker(Data, onKey);
InvokerCallTest_MakeInvoker(Data, onBeginArray);
InvokerCallTest_MakeInvoker(Data, onEndArray);
InvokerCallTest_MakeInvoker(Data, onBeginDict);
InvokerCallTest_MakeInvoker(Data, onEndDict);
InvokerCallTest_MakeInvoker(Data, onKeyValuePair);
InvokerCallTest_MakeInvoker(Data, onValue);
InvokerCallTest_MakeInvoker(Data, onArrayValue);

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

public:
	InvokerCallTest_MakeCallMethod(Data, onNextValue, T);
	InvokerCallTest_MakeCallMethod(Data, onKey, T);
	InvokerCallTest_MakeCallMethod(Data, onBeginArray, T);
	InvokerCallTest_MakeCallMethod(Data, onEndArray, T);
	InvokerCallTest_MakeCallMethod(Data, onBeginDict, T);
	InvokerCallTest_MakeCallMethod(Data, onEndDict, T);
	InvokerCallTest_MakeCallMethod(Data, onKeyValuePair, T);
	InvokerCallTest_MakeCallMethod(Data, onValue, T);
	InvokerCallTest_MakeCallMethod(Data, onArrayValue, T);
};

NS_SP_EXT_END(data)

#endif /* STAPPLER_SRC_CORE_DATA_SPDATATRAITS_H_ */
