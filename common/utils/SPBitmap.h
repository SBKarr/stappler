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

#ifndef LIBS_STAPPLER_CORE_STRUCT_SPBITMAP_H_
#define LIBS_STAPPLER_CORE_STRUCT_SPBITMAP_H_

#include "SPCommon.h"
#include "SPIO.h"

NS_SP_BEGIN

class Bitmap : public AllocBase {
public:
	enum class Alpha {
		Premultiplied,
		Unpremultiplied,
		Opaque,
	};

	enum class Format {
		Auto, // used by application, do not use with Bitmap directly
		A8,
		I8,
		IA88,
		RGB888,
		RGBA8888,
	};

	using StrideFn = Function<uint32_t(Format, uint32_t)>;

	static bool getImageSize(const String &file, size_t &width, size_t &height);
	static bool getImageSize(const io::Producer &file, size_t &width, size_t &height);

	static bool isImage(const String &file, bool readable = true);
	static bool isImage(const io::Producer &file, bool readable = true);

	static bool isGif(const uint8_t * data, size_t dataLen);
	static bool isPng(const uint8_t * data, size_t dataLen);
	static bool isJpg(const uint8_t * data, size_t dataLen);
	static bool isTiff(const uint8_t * data, size_t dataLen);
	static bool isWebp(const uint8_t * data, size_t dataLen);

	static uint8_t getBytesPerPixel(Format);

	static void savePng(const String &filename, const uint8_t *data, uint32_t width, uint32_t height, uint32_t stride, Format fmt, bool invert = false);
	static Bytes writePng(const uint8_t *data, uint32_t width, uint32_t height, uint32_t stride, Format fmt, bool invert = false);

	static void savePng(const String &filename, const uint8_t *data, uint32_t width, uint32_t height, Format fmt, bool invert = false);
	static Bytes writePng(const uint8_t *data, uint32_t width, uint32_t height, Format fmt, bool invert = false);

	Bitmap();

	// init with jpeg or png data
	Bitmap(const Bytes &, const StrideFn &strideFn = nullptr);
	Bitmap(const uint8_t *, size_t, const StrideFn &strideFn = nullptr);

	// init with raw data
	Bitmap(const uint8_t *, uint32_t width, uint32_t height, Format = Format::RGBA8888, Alpha = Alpha::Premultiplied, uint32_t stride = 0);
	Bitmap(const Bytes &, uint32_t width, uint32_t height, Format = Format::RGBA8888, Alpha = Alpha::Premultiplied, uint32_t stride = 0);
	Bitmap(Bytes &&, uint32_t width, uint32_t height, Format = Format::RGBA8888, Alpha = Alpha::Premultiplied, uint32_t stride = 0);

	Bitmap(const Bitmap &) = delete;
	Bitmap &operator =(const Bitmap &) = delete;

	Bitmap(Bitmap &&);
	Bitmap &operator =(Bitmap &&);

	uint32_t width() const { return _width; }
	uint32_t height() const { return _height; }
	uint32_t stride() const { return _stride; }

	bool hasStrideOffset() const { return _width * getBytesPerPixel(_color) < _stride; }

	Alpha alpha() const { return _alpha; }
	Format format() const { return _color; }

	const Bytes & data() const { return _data; }

	uint8_t *dataPtr() { return _data.data(); }
	const uint8_t *dataPtr() const { return _data.data(); }

	bool updateStride(const StrideFn &strideFn);
	bool convert(Format, const StrideFn &strideFn = nullptr);
	bool truncate(Format, const StrideFn &strideFn = nullptr);

	// init with jpeg or png data
	bool loadData(const uint8_t * data, size_t dataLen, const StrideFn &strideFn = nullptr);
	bool loadData(const Bytes &, const StrideFn &strideFn = nullptr);

	// init with raw data
	void loadBitmap(const uint8_t *d, uint32_t w, uint32_t h, Format c, Alpha a = Bitmap::Alpha::Unpremultiplied, uint32_t stride = 0);
	void loadBitmap(const Bytes &d, uint32_t w, uint32_t h, Format c, Alpha a = Bitmap::Alpha::Unpremultiplied, uint32_t stride = 0);
	void loadBitmap(Bytes &&d, uint32_t w, uint32_t h, Format c, Alpha a = Bitmap::Alpha::Unpremultiplied, uint32_t stride = 0);

	void alloc(uint32_t w, uint32_t h, Format c, Alpha a = Bitmap::Alpha::Unpremultiplied, uint32_t stride = 0);

	operator bool () const { return _data.size() != 0; }

	void clear() { _data.clear(); }
	bool empty() const { return _data.empty(); }
	size_t size() const { return _data.size(); }

	void save(const String &); // only png is now available

protected:
	void setInfo(uint32_t w, uint32_t h, Format c, Alpha a = Bitmap::Alpha::Unpremultiplied, uint32_t stride = 0);

	Format _color = Format::RGBA8888;
	Alpha _alpha = Alpha::Opaque;

	uint32_t _width = 0;
	uint32_t _height = 0;
	uint32_t _stride = 0; // in bytes
	Bytes _data;
};

NS_SP_END

#endif /* LIBS_STAPPLER_CORE_STRUCT_SPBITMAP_H_ */
