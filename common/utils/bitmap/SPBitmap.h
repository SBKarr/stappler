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

#ifndef COMMON_UTILS_BITMAP_SPBITMAP_H_
#define COMMON_UTILS_BITMAP_SPBITMAP_H_

#include "SPCommon.h"
#include "SPIO.h"

NS_SP_BEGIN

class Bitmap : public AllocBase {
public:
	enum class ResampleFilter {
		Box, // "box"
		Tent, // "tent"
		Bell, // "bell"
		BSpline, // "b-spline"
		Mitchell, // "mitchell"
		Lanczos3, // "lanczos3"
		Blackman, // "blackman"
		Lanczos4, // "Lanczos4"
		Lanczos6, // "lanczos6"
		Lanczos12, // "lanczos12"
		Kaiser, // "kaiser"
		Gaussian, // "gaussian"
		Catmullrom, // "catmullrom"
		QuadInterp, // "quadratic_interp"
		QuadApprox, // "quadratic_approx"
		QuadMix, // "quadratic_mix"

		Default = Lanczos4,
	};

	enum class FileFormat {
		Png,
		Jpeg,
		WebpLossless,
		WebpLossy,
		Svg,
		Gif,
		Tiff,
		Custom
	};

	enum class Alpha {
		Premultiplied,
		Unpremultiplied,
		Opaque,
	};

	enum class PixelFormat {
		Auto, // used by application, do not use with Bitmap directly
		A8,
		I8,
		IA88,
		RGB888,
		RGBA8888,
	};

	using StrideFn = Function<uint32_t(PixelFormat, uint32_t)>;

	static bool getImageSize(const StringView &file, size_t &width, size_t &height);
	static bool getImageSize(const io::Producer &file, size_t &width, size_t &height);

	static bool isImage(const StringView &file, bool readable = true);
	static bool isImage(const io::Producer &file, bool readable = true);
	static bool isImage(const uint8_t * data, size_t dataLen, bool readable = true);

	static Pair<FileFormat, StringView> detectFormat(const StringView &file);
	static Pair<FileFormat, StringView> detectFormat(const io::Producer &file);
	static Pair<FileFormat, StringView> detectFormat(const uint8_t * data, size_t dataLen);

	static StringView getMimeType(FileFormat);

	static bool check(FileFormat, const uint8_t * data, size_t dataLen);
	static bool check(const StringView &, const uint8_t * data, size_t dataLen);

	static uint8_t getBytesPerPixel(PixelFormat);

public:
	Bitmap();

	// init with jpeg or png data
	Bitmap(const Bytes &, const StrideFn &strideFn = nullptr);
	Bitmap(const uint8_t *, size_t, const StrideFn &strideFn = nullptr);

	// init with raw data
	Bitmap(const uint8_t *, uint32_t width, uint32_t height, PixelFormat = PixelFormat::RGBA8888, Alpha = Alpha::Premultiplied, uint32_t stride = 0);
	Bitmap(const Bytes &, uint32_t width, uint32_t height, PixelFormat = PixelFormat::RGBA8888, Alpha = Alpha::Premultiplied, uint32_t stride = 0);
	Bitmap(Bytes &&, uint32_t width, uint32_t height, PixelFormat = PixelFormat::RGBA8888, Alpha = Alpha::Premultiplied, uint32_t stride = 0);

	Bitmap(const Bitmap &) = delete;
	Bitmap &operator =(const Bitmap &) = delete;

	Bitmap(Bitmap &&);
	Bitmap &operator =(Bitmap &&);

	uint32_t width() const { return _width; }
	uint32_t height() const { return _height; }
	uint32_t stride() const { return _stride; }

	bool hasStrideOffset() const { return _width * getBytesPerPixel(_color) < _stride; }

	Alpha alpha() const { return _alpha; }
	PixelFormat format() const { return _color; }

	const Bytes & data() const { return _data; }

	uint8_t *dataPtr() { return _data.data(); }
	const uint8_t *dataPtr() const { return _data.data(); }

	bool updateStride(const StrideFn &strideFn);
	bool convert(PixelFormat, const StrideFn &strideFn = nullptr);
	bool truncate(PixelFormat, const StrideFn &strideFn = nullptr);

	// init with jpeg or png data
	bool loadData(const uint8_t * data, size_t dataLen, const StrideFn &strideFn = nullptr);
	bool loadData(const Bytes &, const StrideFn &strideFn = nullptr);

	// init with raw data
	void loadBitmap(const uint8_t *d, uint32_t w, uint32_t h, PixelFormat c, Alpha a = Bitmap::Alpha::Unpremultiplied, uint32_t stride = 0);
	void loadBitmap(const Bytes &d, uint32_t w, uint32_t h, PixelFormat c, Alpha a = Bitmap::Alpha::Unpremultiplied, uint32_t stride = 0);
	void loadBitmap(Bytes &&d, uint32_t w, uint32_t h, PixelFormat c, Alpha a = Bitmap::Alpha::Unpremultiplied, uint32_t stride = 0);

	void alloc(uint32_t w, uint32_t h, PixelFormat c, Alpha a = Bitmap::Alpha::Unpremultiplied, uint32_t stride = 0);

	operator bool () const { return _data.size() != 0; }

	void clear() { _data.clear(); }
	bool empty() const { return _data.empty(); }
	size_t size() const { return _data.size(); }

	FileFormat getOriginalFormat() const { return _originalFormat; }
	String getOriginalFormatName() const { return _originalFormatName; }

	bool save(const String &, bool invert = false);
	bool save(FileFormat, const String &, bool invert = false);
	bool save(const String &name, const String &path, bool invert = false);

	Bytes write(FileFormat = FileFormat::Png, bool invert = false);
	Bytes write(const String &name, bool invert = false);

	// resample with default filter (usually Lanczos4)
	Bitmap resample(uint32_t width, uint32_t height, uint32_t stride = 0) const;

	Bitmap resample(ResampleFilter, uint32_t width, uint32_t height, uint32_t stride = 0) const;

protected:
	void setInfo(uint32_t w, uint32_t h, PixelFormat c, Alpha a = Bitmap::Alpha::Unpremultiplied, uint32_t stride = 0);

	PixelFormat _color = PixelFormat::RGBA8888;
	Alpha _alpha = Alpha::Opaque;

	uint32_t _width = 0;
	uint32_t _height = 0;
	uint32_t _stride = 0; // in bytes
	Bytes _data;

	FileFormat _originalFormat = FileFormat::Custom;
	String _originalFormatName;
};

class BitmapFormat {
public:
	using Color = Bitmap::PixelFormat;
	using Alpha = Bitmap::Alpha;
	using FileFormat = Bitmap::FileFormat;

	enum Flags {
		None = 0,
		Recognizable = 1 << 0,
		Readable = 1 << 1,
		Writable = 1 << 2,
	};

	using check_fn = bool (*) (const uint8_t * data, size_t dataLen);
	using size_fn = bool (*) (const io::Producer &file, StackBuffer<512> &, size_t &width, size_t &height);

	using load_fn = bool (*) (const uint8_t *data, size_t size, Bytes &outputData,
			Color &color, Alpha &alpha, uint32_t &width, uint32_t &height,
			uint32_t &stride, const Bitmap::StrideFn &strideFn);

	using write_fn = Bytes (*) (const uint8_t *data, uint32_t width, uint32_t height, uint32_t stride,
			Color format, bool invert);

	using save_fn = bool (*) (const String &, const uint8_t *data, uint32_t width, uint32_t height, uint32_t stride,
			Color format, bool invert);

	static void add(const BitmapFormat &);

	BitmapFormat(FileFormat, const check_fn&, const size_fn &, const load_fn & = nullptr,
			const write_fn & = nullptr, const save_fn & = nullptr);

	BitmapFormat(const String &, const check_fn&, const size_fn &, const load_fn & = nullptr,
			const write_fn & = nullptr, const save_fn & = nullptr);

	const String &getName() const;

	bool isRecognizable() const;
	bool isReadable() const;
	bool isWritable() const;

	Flags getFlags() const;
	FileFormat getFormat() const;

	bool is(const uint8_t * data, size_t dataLen) const;
	bool getSize(const io::Producer &file, StackBuffer<512> &, size_t &width, size_t &height) const;

	bool load(const uint8_t *data, size_t size, Bytes &outputData,
			Color &color, Alpha &alpha, uint32_t &width, uint32_t &height,
			uint32_t &stride, const Bitmap::StrideFn &strideFn);

	Bytes write(const uint8_t *data, uint32_t width, uint32_t height, uint32_t stride,
			Color format, bool invert);

	bool save(const String &, const uint8_t *data, uint32_t width, uint32_t height, uint32_t stride,
			Color format, bool invert);

	check_fn getCheckFn() const;
	size_fn getSizeFn() const;
	load_fn getLoadFn() const;
	write_fn getWriteFn() const;
	save_fn getSaveFn() const;

protected:
	check_fn check_ptr = nullptr;
	size_fn size_ptr = nullptr;
	load_fn load_ptr = nullptr;
	write_fn write_ptr = nullptr;
	save_fn save_ptr = nullptr;

	Flags flags = None;
	FileFormat format = FileFormat::Custom;
	String name;
};

SP_DEFINE_ENUM_AS_MASK(BitmapFormat::Flags)

NS_SP_END

#endif /* COMMON_UTILS_SPBITMAP_H_ */
