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
