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

#define Data_MakeStreamInvoker(Name) \
		template <typename T, bool Value> struct StreamInvoker_ ## Name; \
		template <typename T> struct StreamInvoker_ ## Name <T, false> { \
			template <typename ... Args> static inline void call(T &t, Args && ...args) { } \
		}; \
		template <typename T> struct StreamInvoker_ ## Name <T, true> { \
			template <typename ... Args> static inline void call(T &t, Args && ...args) { t.Name(std::forward<Args>(args)...);} \
		};

#define Data_MakeStreamCallTest(Name, Success, Failure) \
	private: \
		template <typename C> static Success CallTest_ ## Name( typeof(&C::Name) ); \
		template <typename C> static Failure CallTest_ ## Name(...); \
	public: \
		static const bool hasMethod_ ## Name = sizeof(CallTest_ ## Name<T>(0)) == sizeof(success);

#define Data_MakeStreamCallMethod(Name, Type) \
		template <typename ... Args> \
		static inline void Name(Type &t, Args && ... args) { \
			StreamInvoker_ ## Name <Type, sizeof(CallTest_ ## Name<Type>(0)) == sizeof(success)>::call(t, std::forward<Args>(args)...); \
		}

Data_MakeStreamInvoker(onNextValue);
Data_MakeStreamInvoker(onKey);
Data_MakeStreamInvoker(onBeginArray);
Data_MakeStreamInvoker(onEndArray);
Data_MakeStreamInvoker(onBeginDict);
Data_MakeStreamInvoker(onEndDict);
Data_MakeStreamInvoker(onKeyValuePair);
Data_MakeStreamInvoker(onValue);
Data_MakeStreamInvoker(onArrayValue);

template <typename T>
struct StreamTraits {
	using success = char;
	using failure = long;

	Data_MakeStreamCallTest(onNextValue, success, failure);
	Data_MakeStreamCallTest(onKey, success, failure);
	Data_MakeStreamCallTest(onBeginArray, success, failure);
	Data_MakeStreamCallTest(onEndArray, success, failure);
	Data_MakeStreamCallTest(onBeginDict, success, failure);
	Data_MakeStreamCallTest(onEndDict, success, failure);
	Data_MakeStreamCallTest(onKeyValuePair, success, failure);
	Data_MakeStreamCallTest(onValue, success, failure);
	Data_MakeStreamCallTest(onArrayValue, success, failure);

public:
	Data_MakeStreamCallMethod(onNextValue, T);
	Data_MakeStreamCallMethod(onKey, T);
	Data_MakeStreamCallMethod(onBeginArray, T);
	Data_MakeStreamCallMethod(onEndArray, T);
	Data_MakeStreamCallMethod(onBeginDict, T);
	Data_MakeStreamCallMethod(onEndDict, T);
	Data_MakeStreamCallMethod(onKeyValuePair, T);
	Data_MakeStreamCallMethod(onValue, T);
	Data_MakeStreamCallMethod(onArrayValue, T);
};

NS_SP_EXT_END(data)

#endif /* STAPPLER_SRC_CORE_DATA_SPDATATRAITS_H_ */
