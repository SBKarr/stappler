/**
Copyright (c) 2016-2017 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef LAYOUT_INTERNAL_SLRENDERERTYPES_H_
#define LAYOUT_INTERNAL_SLRENDERERTYPES_H_

#include "SLStyle.h"
#include "SLDocument.h"

#include <forward_list>

NS_LAYOUT_BEGIN

namespace RenderFlag {
using Mask = uint32_t;
enum Flag : Mask {
	NoImages = 1 << 0, // do not render images
	NoHeightCheck = 1 << 1, // disable re-rendering when viewport height is changed
	RenderById = 1 << 2, // render nodes by html id instead of spine name
	PaginatedLayout = 1 << 3, // render as book pages instead of html linear layout
	SplitPages = 1 << 4, // render as book pages instead of html linear layout
};
}

struct MediaParameters {
	Size surfaceSize;

	int dpi = 92;
	float density = 1.0f;
	float fontScale = 1.0f;

	style::MediaType mediaType = style::MediaType::Screen;
	style::Orientation orientation = style::Orientation::Landscape;
	style::Pointer pointer = style::Pointer::Coarse;
	style::Hover hover = style::Hover::None;
	style::LightLevel lightLevel = style::LightLevel::Normal;
	style::Scripting scripting = style::Scripting::None;

	RenderFlag::Mask flags = 0;

	Map<CssStringId, String> _options;

	Margin pageMargin;

	Color4B defaultBackground = Color4B(0xfa, 0xfa, 0xfa, 255);

	float getDefaultFontSize() const;

	void addOption(const String &);
	void removeOption(const String &);
	bool hasOption(const String &) const;
	bool hasOption(CssStringId) const;

	Vector<bool> resolveMediaQueries(const Vector<style::MediaQuery> &) const;

	bool shouldRenderImages() const;
};


template <typename T, size_t BytesSize>
struct MemoryStorage {
	static constexpr size_t ALIGN_BASE = 16;
	static constexpr size_t ALIGN_VALUE(size_t size) { return (((size) + ((ALIGN_BASE) - 1)) & ~((ALIGN_BASE) - 1)); }
	static constexpr size_t ALIGN_SIZE = ALIGN_VALUE(sizeof(T));

	struct alignas(16) Bytes {
		std::array<uint8_t, BytesSize - ALIGN_VALUE(sizeof(size_t))> data;
	};

	struct Storage {
		~Storage();

		bool filled() const;

		template <typename ... Args>
		T &emplace(Args && ... args);

		size_t used = 0;
		Bytes bytes;
	};

	MemoryStorage();

	template <typename ... Args>
	T &emplace(Args && ... args);

	size_t size = 0;
	size_t blocks = 0;
	std::forward_list<Storage> storage;
};


template <typename T, size_t Size>
MemoryStorage<T, Size>::Storage::~Storage() {
	for (size_t i = 0; i < used; ++ i) {
		((T *)(&bytes.data[i * ALIGN_SIZE]))->~T();
	}
}

template <typename T, size_t Size>
bool MemoryStorage<T, Size>::Storage::filled() const {
	return used < bytes.data.size() / ALIGN_SIZE;
}

template <typename T, size_t Size>
template <typename ... Args>
auto MemoryStorage<T, Size>::Storage::emplace(Args && ... args) -> T& {
	auto ptr = new (bytes.data.data() + used * ALIGN_SIZE) T(std::forward<Args>(args)...);
	++ used;
	return *ptr;
}

template <typename T, size_t Size>
MemoryStorage<T, Size>::MemoryStorage() { }

template <typename T, size_t Size>
template <typename ... Args>
auto MemoryStorage<T, Size>::emplace(Args && ... args) -> T & {
	if (storage.empty() || storage.front().filled()) {
		storage.emplace_front();
		++ blocks;
	}
	++ size;
	return storage.front().emplace(std::forward<Args>(args)...);
}

NS_LAYOUT_END

#endif /* LAYOUT_INTERNAL_SLRENDERERTYPES_H_ */
