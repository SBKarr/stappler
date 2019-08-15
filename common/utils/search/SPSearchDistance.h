/**
Copyright (c) 2017 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef COMMON_UTILS_SEARCH_SPSEARCHDISTANCE_H_
#define COMMON_UTILS_SEARCH_SPSEARCHDISTANCE_H_

#include "SPStringView.h"

NS_SP_EXT_BEGIN(search)

// Edit (Levenshtein) Distance calculation and alignment, used by search index and transforms
// See: https://en.wikipedia.org/wiki/Levenshtein_distance

class Distance : public memory::AllocPool {
public:
	enum class Value : uint8_t {
		Match,
		Insert,
		Delete,
		Replace
	};

	class Storage : public memory::AllocPool {
	public:
		struct Struct {
			uint8_t v1 : 2;
			uint8_t v2 : 2;
			uint8_t v3 : 2;
			uint8_t v4 : 2;

			void set(uint8_t idx, Value value) {
				switch (idx) {
				case 0: v1 = toInt(value); break;
				case 1: v2 = toInt(value); break;
				case 2: v3 = toInt(value); break;
				case 3: v4 = toInt(value); break;
				default: break;
				}
			}
			Value get(uint8_t idx) const {
				switch (idx) {
				case 0: return Value(v1); break;
				case 1: return Value(v2); break;
				case 2: return Value(v3); break;
				case 3: return Value(v4); break;
				default: break;
				}
				return Value::Match;
			}
		};

		struct Size {
			size_t size : sizeof(size_t) * 8 - 1;
			size_t vec : 1;
		};

		using Array = std::array<Struct, sizeof(Bytes) / sizeof(Struct)>;
		using Vec = Vector<Struct>;

		static Storage merge(const Storage &, const Storage &);

		Storage() noexcept;
		~Storage() noexcept;

		Storage(const Storage &) noexcept;
		Storage(Storage &&) noexcept;

		Storage &operator=(const Storage &) noexcept;
		Storage &operator=(Storage &&) noexcept;

		bool empty() const;
		size_t size() const;
		size_t capacity() const;

		void reserve(size_t);
		void emplace_back(Value);
		void reverse();

		Value at(size_t) const;
		void set(size_t, Value);

		void clear();

	protected:
		bool isVecStorage() const;
		bool isVecStorage(size_t) const;

		Size _size;
		union {
			Array _array;
			Vec _bytes;
		};
	};

	Distance() noexcept;
	Distance(const StringView &origin, const StringView &canonical, size_t maxDistance = maxOf<size_t>());

	Distance(const Distance &) noexcept;
	Distance(Distance &&) noexcept;

	Distance &operator=(const Distance &) noexcept;
	Distance &operator=(Distance &&) noexcept;

	bool empty() const;

	size_t size() const;

	// calculate position difference from canonical to original
	int32_t diff_original(size_t pos, bool forward = false) const;

	// calculate position difference from original to canonical
	int32_t diff_canonical(size_t pos, bool forward = false) const;

	memory::string info() const;

protected:
	Storage _storage;
};

NS_SP_EXT_END(search)

#endif // COMMON_UTILS_SEARCH_SPSEARCHDISTANCE_H_
