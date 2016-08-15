/*
 * SPAprMemUtils.h
 *
 *  Created on: 19 апр. 2016 г.
 *      Author: sbkarr
 */

#ifndef COMMON_APR_SPAPRMEMUTILS_H_
#define COMMON_APR_SPAPRMEMUTILS_H_

#include "SPAprAllocator.h"

#if SPAPR
NS_APR_BEGIN

template <typename Value>
struct Storage {
	struct Image { Value _value; };

	alignas(__alignof__(Image::_value)) uint8_t _storage[sizeof(Value)];

	Storage() = default;
	Storage(nullptr_t) {}

	void * addr() noexcept { return static_cast<void *>(&_storage); }
	const void * addr() const noexcept { return static_cast<const void *>(&_storage); }

	Value * ptr() noexcept { return static_cast<Value *>(addr()); }
	const Value * ptr() const noexcept { return static_cast<const Value *>(addr()); }

	Value & ref() noexcept { return *ptr(); }
	const Value & ref() const noexcept { return *ptr(); }
};

NS_APR_END
#endif

#endif /* COMMON_APR_SPAPRMEMUTILS_H_ */
