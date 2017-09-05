// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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

#include "SPCommon.h"
#include "SPBitmap.h"
#include "SPDataReader.h"
#include "SPHtmlParser.h"
#include "SPFilesystem.h"
#include "SPCommonPlatform.h"

#include "SPBitmapFormatJpeg.hpp"
#include "SPBitmapFormatPng.hpp"

NS_SP_BEGIN

static bool BitmapFormat_isPng(const uint8_t * data, size_t dataLen) {
    if (dataLen <= 8) {
        return false;
    }

    static const unsigned char PNG_SIGNATURE[] = {0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a};
    return memcmp(PNG_SIGNATURE, data, sizeof(PNG_SIGNATURE)) == 0;
}

static bool BitmapFormat_getPngImageSize(const io::Producer &file, StackBuffer<512> &data, size_t &width, size_t &height) {
	if (BitmapFormat_isPng(data.data(), data.size()) && data.size() >= 24) {
		auto reader = DataReader<ByteOrder::Network>(data.data() + 16, 8);

		width = reader.readUnsigned32();
		height = reader.readUnsigned32();

		return true;
	}
	return false;
}

static bool BitmapFormat_isJpg(const uint8_t * data, size_t dataLen) {
    if (dataLen <= 4) {
        return false;
    }

    static const unsigned char JPG_SOI[] = {0xFF, 0xD8};
    return memcmp(data, JPG_SOI, 2) == 0;
}

static bool BitmapFormat_getJpegImageSize(const io::Producer &file, StackBuffer<512> &data, size_t &width, size_t &height) {
	if (BitmapFormat_isJpg(data.data(), data.size())) {
		size_t offset = 2;
		uint16_t len = 0;
		uint8_t marker = 0;

		auto reader = DataReader<ByteOrder::Network>(data.data() + 2, data.size() - 2);
		while (reader.is((uint8_t) 0xFF)) {
			++reader;
		}

		marker = reader.readUnsigned();
		len = reader.readUnsigned16();

		while (marker < 0xC0 || marker > 0xCF) {
			offset += 2 + len;
			data.clear();

			if (file.seekAndRead(offset, data, 12) != 12) {
				return false;
			}

			if (data.size() >= 12) {
				reader = data.get<DataReader<ByteOrder::Network>>();

				while (reader.is((uint8_t) 0xFF)) {
					++reader;
				}

				marker = reader.readUnsigned();
				len = reader.readUnsigned16();
			} else {
				reader.clear();
				break;
			}
		}

		if (reader >= 5 && marker >= 0xC0 && marker <= 0xCF) {
			++reader;
			height = reader.readUnsigned16();
			width = reader.readUnsigned16();
			return true;
		}

		return false;
	}
	return false;
}

static bool BitmapFormat_isWebpLossless(const uint8_t * data, size_t dataLen) {
    if (dataLen <= 12) {
        return false;
    }

    static const char* WEBP_RIFF = "RIFF";
    static const char* WEBP_WEBP = "WEBPVP8L";

    return memcmp(data, WEBP_RIFF, 4) == 0
    		&& memcmp(static_cast<const unsigned char*>(data) + 8, WEBP_WEBP, 8) == 0;
}

static bool BitmapFormat_getWebpLosslessImageSize(const io::Producer &file, StackBuffer<512> &data, size_t &width, size_t &height) {
	if (BitmapFormat_isWebpLossless(data.data(), data.size())) {
		auto reader = DataReader<ByteOrder::Big>(data.data() + 21, 4);

		auto b0 = reader.readUnsigned();
		auto b1 = reader.readUnsigned();
		auto b2 = reader.readUnsigned();
		auto b3 = reader.readUnsigned();

		// first 14 bits - width, last 14 bits - height

		width = (b0 | ((b1 & 0x3F) << 8)) + 1;
		height = (((b3 & 0xF) << 10) | (b2 << 2) | ((b1 & 0xC0) >> 6)) + 1;

		return true;
	}
	return false;
}

static bool BitmapFormat_isWebp(const uint8_t * data, size_t dataLen) {
    if (dataLen <= 12) {
        return false;
    }

    static const char* WEBP_RIFF = "RIFF";
    static const char* WEBP_WEBP = "WEBP";

    return memcmp(data, WEBP_RIFF, 4) == 0
        && memcmp(static_cast<const unsigned char*>(data) + 8, WEBP_WEBP, 4) == 0;
}

static bool BitmapFormat_getWebpImageSize(const io::Producer &file, StackBuffer<512> &data, size_t &width, size_t &height) {
	if (BitmapFormat_isWebp(data.data(), data.size())) {
		auto reader = DataReader<ByteOrder::Little>(data.data() + 24, 6);

		auto b0 = reader.readUnsigned();
		auto b1 = reader.readUnsigned();
		auto b2 = reader.readUnsigned();
		auto b3 = reader.readUnsigned();
		auto b4 = reader.readUnsigned();
		auto b5 = reader.readUnsigned();

		// first 14 bits - width, last 14 bits - height

		width = (b0 | (b1 << 8) | (b2 << 8)) + 1;
		height = (b3 | (b4 << 8) | (b5 << 8)) + 1;

		return true;
	}
	return false;
}

static bool BitmapFormat_detectSvg(const StringView &buf, size_t &w, size_t &h) {
	StringView str(buf);
	str.skipUntilString("<svg", false);
	if (!str.empty() && str.is<CharReaderBase::CharGroup<CharGroupId::WhiteSpace>>()) {
		bool found = false;
		bool isSvg = false;
		uint32_t width = 0;
		uint32_t height = 0;
		while (!found && !str.empty()) {
			str.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
			auto key = html::Tag_readAttrName(str, true);
			auto value = html::Tag_readAttrValue(str, true);
			if (!key.empty() && !value.empty()) {
				if (key == "xmlns") {
					if (value.is("http://www.w3.org/2000/svg")) {
						isSvg = true;
					}
				} else if (key == "width") {
					width = size_t(value.readInteger());
				} else if (key == "height") {
					height = size_t(value.readInteger());
				}
				if (isSvg && width && height) {
					found = true;
				}
			}
		}
		if (isSvg) {
			w = width;
			h = height;
		}
		return isSvg;
	}

	return false;
}

static bool BitmapFormat_detectSvg(const StringView &buf) {
	size_t w = 0, h = 0;
	return BitmapFormat_detectSvg(buf, w, h);
}

static bool BitmapFormat_isSvg(const uint8_t * data, size_t dataLen) {
	if (dataLen <= 127) {
		return false;
	}

	return BitmapFormat_detectSvg(StringView((const char *)data, dataLen));
}

static bool BitmapFormat_getSvgImageSize(const io::Producer &file, StackBuffer<512> &data, size_t &width, size_t &height) {
	if (BitmapFormat_detectSvg(StringView((const char *)data.data(), data.size()), width, height)) {
		return true;
	}

	return false;
}

static bool BitmapFormat_isGif(const uint8_t * data, size_t dataLen) {
    if (dataLen <= 8) {
        return false;
    }

    static const unsigned char GIF_SIGNATURE_1[] = {0x47, 0x49, 0x46, 0x38, 0x37, 0x61};
    static const unsigned char GIF_SIGNATURE_2[] = {0x47, 0x49, 0x46, 0x38, 0x39, 0x61};
    return memcmp(GIF_SIGNATURE_1, data, sizeof(GIF_SIGNATURE_1)) == 0
    		|| memcmp(GIF_SIGNATURE_2, data, sizeof(GIF_SIGNATURE_2)) == 0;
}

static bool BitmapFormat_getGifImageSize(const io::Producer &file, StackBuffer<512> &data, size_t &width, size_t &height) {
	if (BitmapFormat_isGif(data.data(), data.size())) {
		auto reader = DataReader<ByteOrder::Little>(data.data() + 6, 4);

		width = reader.readUnsigned16();
		height = reader.readUnsigned16();

		return true;
	}
	return false;
}

static bool BitmapFormat_isTiff(const uint8_t * data, size_t dataLen) {
    if (dataLen <= 4) {
        return false;
    }

    static const char* TIFF_II = "II";
    static const char* TIFF_MM = "MM";

    return (memcmp(data, TIFF_II, 2) == 0 && *(static_cast<const unsigned char*>(data) + 2) == 42 && *(static_cast<const unsigned char*>(data) + 3) == 0) ||
        (memcmp(data, TIFF_MM, 2) == 0 && *(static_cast<const unsigned char*>(data) + 2) == 0 && *(static_cast<const unsigned char*>(data) + 3) == 42);
}

template <typename Reader>
static bool BitmapFormat_getTiffImageSizeImpl(const io::Producer &file, StackBuffer<512> &data, size_t &width, size_t &height) {
	auto reader = Reader(data.data() + 4, 4);
	auto offset = reader.readUnsigned32();

	data.clear();
	if (file.seekAndRead(offset, data, 2) != 2) {
		return false;
	}
	auto size = Reader(data.data(), 2).readUnsigned16();
	auto dictSize = size * 12;
	offset += 2;
	while (dictSize > 0) {
		data.clear();
		size_t blockSize = min(12 * 21, dictSize);
		if (file.read(data, blockSize) != blockSize) {
			return false;
		}

		auto blocks = blockSize / 12;
		reader = Reader(data.data(), blockSize);

		for (uint16_t i = 0; i < blocks; ++i) {
			auto tagid = reader.readUnsigned16();
			auto type = reader.readUnsigned16();
			auto count = reader.readUnsigned32();
			if (tagid == 256 && count == 1) {
				if (type == 3) {
					width = reader.readUnsigned16();
					reader.offset(2);
				} else if (type == 4) {
					width = reader.readUnsigned32();
				} else {
					reader.offset(4);
				}
			} else if (tagid == 257 && count == 1) {
				if (type == 3) {
					height = reader.readUnsigned16();
					reader.offset(2);
				} else if (type == 4) {
					height = reader.readUnsigned32();
				} else {
					reader.offset(4);
				}
				return true;
			} else {
				if (tagid > 257) {
					return false;
				}
				reader.offset(4);
			}
		}
	}
	return false;
}

static bool BitmapFormat_getTiffImageSize(const io::Producer &file, StackBuffer<512> &data, size_t &width, size_t &height) {
	if (BitmapFormat_isTiff(data.data(), data.size())) {
		if (memcmp(data.data(), "II", 2) == 0) {
			if (BitmapFormat_getTiffImageSizeImpl<DataReader<ByteOrder::Little>>(file, data, width, height)) {
				return true;
			}
		} else {
			if (BitmapFormat_getTiffImageSizeImpl<DataReader<ByteOrder::Big>>(file, data, width, height)) {
				return true;
			}
		}
	}
	return false;
}



static BitmapFormat s_defaultFormats[toInt(Bitmap::FileFormat::Custom)] = {
	BitmapFormat(Bitmap::FileFormat::Png, &BitmapFormat_isPng, &BitmapFormat_getPngImageSize
			, &png::loadPng, &png::writePng, &png::savePng
	),
	BitmapFormat(Bitmap::FileFormat::Jpeg, &BitmapFormat_isJpg, &BitmapFormat_getJpegImageSize
			, &jpeg::loadJpg, &jpeg::writeJpeg, &jpeg::saveJpeg
	),
	BitmapFormat(Bitmap::FileFormat::WebpLossless, &BitmapFormat_isWebpLossless, &BitmapFormat_getWebpLosslessImageSize),
	BitmapFormat(Bitmap::FileFormat::WebpLossy, &BitmapFormat_isWebp, &BitmapFormat_getWebpImageSize),
	BitmapFormat(Bitmap::FileFormat::Svg, &BitmapFormat_isSvg, &BitmapFormat_getSvgImageSize),
	BitmapFormat(Bitmap::FileFormat::Gif, &BitmapFormat_isGif, &BitmapFormat_getGifImageSize),
	BitmapFormat(Bitmap::FileFormat::Tiff, &BitmapFormat_isTiff, &BitmapFormat_getTiffImageSize),
};

static std::mutex s_formatListMutex;
static std::vector<BitmapFormat> s_formatList;

void BitmapFormat::add(const BitmapFormat &fmt) {
	s_formatListMutex.lock();
	s_formatList.emplace_back(fmt);
	s_formatListMutex.unlock();
}

BitmapFormat::BitmapFormat(FileFormat f, const check_fn &c, const size_fn &s, const load_fn &l, const write_fn &wr, const save_fn &sv)
: check_ptr(c), size_ptr(s), load_ptr(l), write_ptr(wr), save_ptr(sv), format(f) {
	assert(f != FileFormat::Custom);
	if (check_ptr && size_ptr) {
		flags |= Recognizable;
	}
	if (load_ptr) {
		flags |= Readable;
	}
	if (save_ptr || write_ptr) {
		flags |= Writable;
	}

	switch (format) {
	case FileFormat::Png: name = "PNG"; break;
	case FileFormat::Jpeg: name = "JPEG"; break;
	case FileFormat::WebpLossless: name = "WebP-lossless"; break;
	case FileFormat::WebpLossy: name = "WebP-lossy"; break;
	case FileFormat::Svg: name = "SVG"; break;
	case FileFormat::Gif: name = "GIF"; break;
	case FileFormat::Tiff: name = "TIFF"; break;
	default: break;
	}
}

BitmapFormat::BitmapFormat(const String &n, const check_fn &c, const size_fn &s, const load_fn &l, const write_fn &wr, const save_fn &sv)
: check_ptr(c), size_ptr(s), load_ptr(l), write_ptr(wr), save_ptr(sv), format(FileFormat::Custom), name(n) {
	if (check_ptr && size_ptr) {
		flags |= Recognizable;
	}
	if (load_ptr) {
		flags |= Readable;
	}
	if (save_ptr || write_ptr) {
		flags |= Writable;
	}
}

const String &BitmapFormat::getName() const {
	return name;
}

bool BitmapFormat::isRecognizable() const {
	return (flags & Recognizable) != None;
}
bool BitmapFormat::isReadable() const {
	return (flags & Readable) != None;
}
bool BitmapFormat::isWritable() const {
	return (flags & Writable) != None;
}

BitmapFormat::Flags BitmapFormat::getFlags() const {
	return flags;
}
BitmapFormat::FileFormat BitmapFormat::getFormat() const {
	return format;
}

bool BitmapFormat::is(const uint8_t * data, size_t dataLen) const {
	if (check_ptr) {
		return check_ptr(data, dataLen);
	}
	return false;
}
bool BitmapFormat::getSize(const io::Producer &file, StackBuffer<512> &buf, size_t &width, size_t &height) const {
	if (size_ptr) {
		return size_ptr(file, buf, width, height);
	}
	return false;
}

bool BitmapFormat::load(const uint8_t *data, size_t size, Bytes &out,
		Color &color, Alpha &alpha, uint32_t &width, uint32_t &height,
		uint32_t &stride, const Bitmap::StrideFn &strideFn) {
	if (load_ptr) {
		return load_ptr(data, size, out, color, alpha, width, height, stride, strideFn);
	}
	return false;
}

Bytes BitmapFormat::write(const uint8_t *data, uint32_t width, uint32_t height, uint32_t stride, Color format, bool invert) {
	if (write_ptr) {
		return write_ptr(data, width, height, stride, format, invert);
	}
	return Bytes();
}

bool BitmapFormat::save(const String &path, const uint8_t *data, uint32_t width, uint32_t height, uint32_t stride, Color format, bool invert) {
	if (save_ptr) {
		return save_ptr(path, data, width, height, stride, format, invert);
	}
	return false;
}

BitmapFormat::check_fn BitmapFormat::getCheckFn() const { return check_ptr; }
BitmapFormat::size_fn BitmapFormat::getSizeFn() const { return size_ptr; }
BitmapFormat::load_fn BitmapFormat::getLoadFn() const { return load_ptr; }
BitmapFormat::write_fn BitmapFormat::getWriteFn() const { return write_ptr; }
BitmapFormat::save_fn BitmapFormat::getSaveFn() const { return save_ptr; }


bool Bitmap::getImageSize(const String &path, size_t &width, size_t &height) {
	auto file = filesystem::openForReading(path);
	return getImageSize(file, width, height);
}

bool Bitmap::getImageSize(const io::Producer &file, size_t &width, size_t &height) {
	StackBuffer<512> data;
	if (file.seekAndRead(0, data, 512) < 32) {
		return false;
	}

	for (int i = 0; i < toInt(Bitmap::FileFormat::Custom); ++i) {
		if (s_defaultFormats[i].isRecognizable() && s_defaultFormats[i].getSize(file, data, width, height)) {
			return true;
		}
	}


	Vector<BitmapFormat::size_fn> fns;

	s_formatListMutex.lock();
	fns.reserve(s_formatList.size());

	for (auto &it : s_formatList) {
		if (it.isRecognizable()) {
			fns.emplace_back(it.getSizeFn());
		}
	}

	s_formatListMutex.unlock();

	for (auto &it : fns) {
		if (it(file, data, width, height)) {
			return true;
		}
	}

	return false;
}

bool Bitmap::isImage(const String &path, bool readable) {
	auto file = filesystem::openForReading(path);
	return isImage(file, readable);
}
bool Bitmap::isImage(const io::Producer &file, bool readable) {
	StackBuffer<512> data;
	if (file.seekAndRead(0, data, 512) < 32) {
		return false;
	}

	for (int i = 0; i < toInt(Bitmap::FileFormat::Custom); ++i) {
		if (s_defaultFormats[i].isRecognizable() && (!readable || s_defaultFormats[i].isReadable())
				&& s_defaultFormats[i].is(data.data(), data.size())) {
			return true;
		}
	}

	Vector<BitmapFormat::check_fn> fns;

	s_formatListMutex.lock();
	fns.reserve(s_formatList.size());

	for (auto &it : s_formatList) {
		if (it.isRecognizable() && (!readable || it.isReadable())) {
			fns.emplace_back(it.getCheckFn());
		}
	}

	s_formatListMutex.unlock();

	for (auto &it : fns) {
		if (it(data.data(), data.size())) {
			return true;
		}
	}

	return false;
}

bool Bitmap::check(FileFormat fmt, const uint8_t * data, size_t dataLen) {
	assert(fmt != FileFormat::Custom);
	return s_defaultFormats[toInt(fmt)].is(data, dataLen);
}

bool Bitmap::check(const String &name, const uint8_t * data, size_t dataLen) {
	Vector<BitmapFormat::check_fn> fns;

	s_formatListMutex.lock();
	fns.reserve(s_formatList.size());

	for (auto &it : s_formatList) {
		if (it.isRecognizable() && it.getName() == name) {
			fns.emplace_back(it.getCheckFn());
		}
	}

	s_formatListMutex.unlock();

	for (auto &it : fns) {
		if (it(data, dataLen)) {
			return true;
		}
	}

	return false;
}

bool Bitmap::save(const String &path, bool invert) {
	FileFormat fmt = FileFormat::Png;
	auto ext = filepath::lastExtension(path);
	if (ext == "png") {
		fmt = FileFormat::Png;
	} else if (ext == "jpeg" || ext == "jpg") {
		fmt = FileFormat::Jpeg;
	} else if (ext == "webp") {
		fmt = FileFormat::WebpLossless;
	}
	return save(fmt, path, invert);
}
bool Bitmap::save(FileFormat fmt, const String &path, bool invert) {
	auto &support = s_defaultFormats[toInt(fmt)];
	if (support.isWritable()) {
		return support.save(path, _data.data(), _width, _height, _stride, _color, invert);
	}
	return false;
}
bool Bitmap::save(const String &name, const String &path, bool invert) {
	BitmapFormat::save_fn fn = nullptr;

	s_formatListMutex.lock();

	for (auto &it : s_formatList) {
		if (it.getName() == name && it.isWritable()) {
			fn = it.getSaveFn();
		}
	}

	s_formatListMutex.unlock();

	if (fn) {
		return fn(path, _data.data(), _width, _height, _stride, _color, invert);
	}
	return false;
}

Bytes Bitmap::write(FileFormat fmt, bool invert) {
	auto &support = s_defaultFormats[toInt(fmt)];
	if (support.isWritable()) {
		return support.write(_data.data(), _width, _height, _stride, _color, invert);
	}
	return Bytes();
}
Bytes Bitmap::write(const String &name, bool invert) {
	BitmapFormat::write_fn fn = nullptr;

	s_formatListMutex.lock();

	for (auto &it : s_formatList) {
		if (it.getName() == name && it.isWritable()) {
			fn = it.getWriteFn();
		}
	}

	s_formatListMutex.unlock();

	if (fn) {
		return fn(_data.data(), _width, _height, _stride, _color, invert);
	}
	return Bytes();
}

bool Bitmap::loadData(const uint8_t * data, size_t dataLen, const StrideFn &strideFn) {
	for (int i = 0; i < toInt(Bitmap::FileFormat::Custom); ++i) {
		auto &fmt = s_defaultFormats[i];
		if (fmt.is(data, dataLen) && fmt.isReadable()) {
			if (fmt.load(data, dataLen, _data, _color, _alpha, _width, _height, _stride, strideFn)) {
				_originalFormat = FileFormat(i);
				_originalFormatName = fmt.getName();
				return true;
			}
		}
	}

	Vector<BitmapFormat> fns;

	s_formatListMutex.lock();

	fns.reserve(s_formatList.size());
	for (auto &it : s_formatList) {
		if (it.isReadable()) {
			fns.emplace_back(it);
		}
	}

	s_formatListMutex.unlock();

	for (auto &it : fns) {
		if (it.is(data, dataLen) && it.load(data, dataLen, _data, _color, _alpha, _width, _height, _stride, strideFn)) {
			_originalFormat = FileFormat::Custom;
			_originalFormatName = it.getName();
			return true;
		}
	}

	return false;
}

bool Bitmap::loadData(const Bytes &d, const StrideFn &strideFn) {
	return loadData(d.data(), d.size(), strideFn);
}

NS_SP_END
