// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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

#include "SPBitmap.h"

#include "SPDataReader.h"
#include "SPFilesystem.h"
#include "SPLog.h"
#include "SPCommon.h"
#include "SPBuffer.h"

#include "SPCommonPlatform.h"
#include "SPString.h"

NS_SP_BEGIN

using Color = Bitmap::PixelFormat;
using Alpha = Bitmap::Alpha;

template<Color Source, Color Target>
void convertLine(const uint8_t *in, uint8_t *out, uint32_t ins, uint32_t outs);

template<>
void convertLine<Color::RGB888, Color::RGBA8888>(const uint8_t *in, uint8_t *out, uint32_t ins, uint32_t outs) {
	for (size_t i = 0, l = ins - 2; i < l; i += 3) {
		*out++ = in[i];			//R
		*out++ = in[i + 1];		//G
		*out++ = in[i + 2];		//B
		*out++ = 0xFF;			//A
	}
}

template<>
void convertLine<Color::I8, Color::RGB888>(const uint8_t *in, uint8_t *out, uint32_t ins, uint32_t outs) {
    for (size_t i = 0; i < ins; ++i) {
        *out++ = in[i];     //R
        *out++ = in[i];     //G
        *out++ = in[i];     //B
    }
}

template<>
void convertLine<Color::IA88, Color::RGB888>(const uint8_t *in, uint8_t *out, uint32_t ins, uint32_t outs) {
    for (size_t i = 0, l = ins - 1; i < l; i += 2) {
        *out++ = in[i];     //R
        *out++ = in[i];     //G
        *out++ = in[i];     //B
    }
}

template<>
void convertLine<Color::I8, Color::RGBA8888>(const uint8_t *in, uint8_t *out, uint32_t ins, uint32_t outs) {
	for (size_t i = 0; i < ins; ++i) {
        *out++ = in[i];     //R
        *out++ = in[i];     //G
        *out++ = in[i];     //B
        *out++ = 0xFF;      //A
    }
}

template<>
void convertLine<Color::IA88, Color::RGBA8888>(const uint8_t *in, uint8_t *out, uint32_t ins, uint32_t outs) {
    for (size_t i = 0, l = ins - 1; i < l; i += 2) {
        *out++ = in[i];     //R
        *out++ = in[i];     //G
        *out++ = in[i];     //B
        *out++ = in[i + 1]; //A
    }
}

template<>
void convertLine<Color::I8, Color::IA88>(const uint8_t *in, uint8_t *out, uint32_t ins, uint32_t outs) {
    for (size_t i = 0; i < ins; ++i) {
        *out++ = in[i];
		*out++ = 0xFF;
    }
}

template<>
void convertLine<Color::IA88, Color::A8>(const uint8_t *in, uint8_t *out, uint32_t ins, uint32_t outs) {
    for (size_t i = 1; i < ins; i += 2) {
        *out++ = in[i]; //A
    }
}

template<>
void convertLine<Color::IA88, Color::I8>(const uint8_t *in, uint8_t *out, uint32_t ins, uint32_t outs) {
    for (size_t i = 0, l = ins - 1; i < l; i += 2) {
        *out++ = in[i]; //R
    }
}

template<>
void convertLine<Color::RGBA8888, Color::RGB888>(const uint8_t *in, uint8_t *out, uint32_t ins, uint32_t outs) {
    for (size_t i = 0, l = ins - 3; i < l; i += 4) {
        *out++ = in[i];         //R
        *out++ = in[i + 1];     //G
        *out++ = in[i + 2];     //B
    }
}

template<>
void convertLine<Color::RGB888, Color::I8>(const uint8_t *in, uint8_t *out, uint32_t ins, uint32_t outs) {
    for (size_t i = 0, l = ins - 2; i < l; i += 3) {
        *out++ = (in[i] * 299 + in[i + 1] * 587 + in[i + 2] * 114 + 500) / 1000;  //I =  (R*299 + G*587 + B*114 + 500) / 1000
    }
}

template<>
void convertLine<Color::RGBA8888, Color::I8>(const uint8_t *in, uint8_t *out, uint32_t ins, uint32_t outs) {
    for (size_t i = 0, l = ins - 3; i < l; i += 4) {
        *out++ = (in[i] * 299 + in[i + 1] * 587 + in[i + 2] * 114 + 500) / 1000;  //I =  (R*299 + G*587 + B*114 + 500) / 1000
    }
}

template<>
void convertLine<Color::RGBA8888, Color::A8>(const uint8_t *in, uint8_t *out, uint32_t ins, uint32_t outs) {
    for (size_t i = 0, l = ins -3; i < l; i += 4) {
        *out++ = in[i + 3]; //A
    }
}

template<>
void convertLine<Color::RGB888, Color::IA88>(const uint8_t *in, uint8_t *out, uint32_t ins, uint32_t outs) {
    for (size_t i = 0, l = ins - 2; i < l; i += 3) {
        *out++ = (in[i] * 299 + in[i + 1] * 587 + in[i + 2] * 114 + 500) / 1000;  //I =  (R*299 + G*587 + B*114 + 500) / 1000
        *out++ = 0xFF;
    }
}

template<>
void convertLine<Color::RGBA8888, Color::IA88>(const uint8_t *in, uint8_t *out, uint32_t ins, uint32_t outs) {
    for (size_t i = 0, l = ins - 3; i < l; i += 4) {
        *out++ = (in[i] * 299 + in[i + 1] * 587 + in[i + 2] * 114 + 500) / 1000;  //I =  (R*299 + G*587 + B*114 + 500) / 1000
        *out++ = in[i + 3];
    }
}

template<>
void convertLine<Color::A8, Color::IA88>(const uint8_t *in, uint8_t *out, uint32_t ins, uint32_t outs) {
    for (size_t i = 0; i < ins; ++i) {
        *out++ = 0xFF;
        *out++ = in[i];
    }
}

template<>
void convertLine<Color::A8, Color::RGB888>(const uint8_t *in, uint8_t *out, uint32_t ins, uint32_t outs) {
	memset(out, 0, outs);
}

template<>
void convertLine<Color::A8, Color::RGBA8888>(const uint8_t *in, uint8_t *out, uint32_t ins, uint32_t outs) {
    for (size_t i = 0; i < ins; ++i) {
        *out++ = 0x00;
        *out++ = 0x00;
        *out++ = 0x00;
        *out++ = in[i];
    }
}

template<>
void convertLine<Color::RGB888, Color::A8>(const uint8_t *in, uint8_t *out, uint32_t ins, uint32_t outs) {
	memset(out, 0, outs);
}


template<Color Source, Color Target>
bool convertData(const Bytes &dataVec, Bytes & out, uint32_t inStride, uint32_t outStride) {
	auto dataLen = dataVec.size();
	auto height = dataLen / inStride;
	auto data = dataVec.data();
	out.resize(height * outStride);
	auto outData = out.data();
	for (size_t j = 0; j < height; j ++) {
		convertLine<Source, Target>(data + inStride * j, outData + outStride * j, inStride, outStride);
	}
	return true;
}

uint8_t Bitmap::getBytesPerPixel(PixelFormat c) {
	switch (c) {
	case PixelFormat::A8:
	case PixelFormat::I8:
		return 1;
		break;
	case PixelFormat::IA88:
		return 2;
		break;
	case PixelFormat::RGB888:
		return 3;
		break;
	case PixelFormat::RGBA8888:
		return 4;
		break;
	case PixelFormat::Auto:
		return 0;
		break;
	}
	return 0;
}

Bitmap::Bitmap() { }

Bitmap::Bitmap(const Bytes &vec, const StrideFn &strideFn)
: Bitmap(vec.data(), vec.size(), strideFn) { }

Bitmap::Bitmap(const uint8_t *data, size_t size, const StrideFn &strideFn) {
	if (!loadData(data, size, strideFn)) {
		_data.clear();
	}
}

Bitmap::Bitmap(const uint8_t *d, uint32_t width, uint32_t height, PixelFormat c, Alpha a, uint32_t stride)
: _color(c), _alpha(a), _width(width), _height(height), _stride(max(stride, width * getBytesPerPixel(c))), _data(d, d + _stride * height) {
	SPASSERT(c != PixelFormat::Auto, "Bitmap: Format::Auto should not be used with Bitmap directly");
}

Bitmap::Bitmap(const Bytes &d, uint32_t width, uint32_t height, PixelFormat c, Alpha a, uint32_t stride)
: _color(c), _alpha(a), _width(width), _height(height), _stride(max(stride, width * getBytesPerPixel(c))), _data(d) {
	SPASSERT(c != PixelFormat::Auto, "Bitmap: Format::Auto should not be used with Bitmap directly");
}

Bitmap::Bitmap(Bytes &&d, uint32_t width, uint32_t height, PixelFormat c, Alpha a, uint32_t stride)
: _color(c), _alpha(a), _width(width), _height(height), _stride(max(stride, width * getBytesPerPixel(c))), _data(std::move(d)) {
	SPASSERT(c != PixelFormat::Auto, "Bitmap: Format::Auto should not be used with Bitmap directly");
}

Bitmap::Bitmap(Bitmap &&other)
: _color(other._color), _alpha(other._alpha)
, _width(other._width), _height(other._height), _stride(other._stride)
, _data(std::move(other._data)), _originalFormat(other._originalFormat), _originalFormatName(move(other._originalFormatName)) {
	other._data.clear();
}

Bitmap &Bitmap::operator =(Bitmap &&other) {
	_alpha = other._alpha;
	_color = other._color;
	_width = other._width;
	_height = other._height;
	_stride = other._stride;
	_data = std::move(other._data);
	_originalFormat = other._originalFormat;
	_originalFormatName = move(other._originalFormatName);
	other._data.clear();
	return *this;
}

void Bitmap::loadBitmap(const uint8_t *d, uint32_t w, uint32_t h, PixelFormat c, Alpha a, uint32_t stride) {
	SPASSERT(c != PixelFormat::Auto, "Bitmap: Format::Auto should not be used with Bitmap directly");
	setInfo(w, h, c, a, stride);
	_data.clear();
	_data.resize(_stride * h);
	memcpy(_data.data(), d, _data.size());
	_originalFormat = FileFormat::Custom;
	_originalFormatName.clear();
}

void Bitmap::loadBitmap(const Bytes &d, uint32_t w, uint32_t h, PixelFormat c, Alpha a, uint32_t stride) {
	SPASSERT(c != PixelFormat::Auto, "Bitmap: Format::Auto should not be used with Bitmap directly");
	setInfo(w, h, c, a, stride);
	_data = d;
	_originalFormat = FileFormat::Custom;
	_originalFormatName.clear();
}

void Bitmap::loadBitmap(Bytes &&d, uint32_t w, uint32_t h, PixelFormat c, Alpha a, uint32_t stride) {
	SPASSERT(c != PixelFormat::Auto, "Bitmap: Format::Auto should not be used with Bitmap directly");
	setInfo(w, h, c, a, stride);
	_data = std::move(d);
	_originalFormat = FileFormat::Custom;
	_originalFormatName.clear();
}

void Bitmap::alloc(uint32_t w, uint32_t h, PixelFormat c, Alpha a, uint32_t stride) {
	SPASSERT(c != PixelFormat::Auto, "Bitmap: Format::Auto should not be used with Bitmap directly");
	setInfo(w, h, c, a, stride);
	_data.clear();
	_data.resize(_stride * h);
	_originalFormat = FileFormat::Custom;
	_originalFormatName.clear();
}

void Bitmap::setInfo(uint32_t w, uint32_t h, PixelFormat c, Alpha a, uint32_t stride) {
	SPASSERT(c != PixelFormat::Auto, "Bitmap: Format::Auto should not be used with Bitmap directly");
	_width = w;
	_height = h;
	_stride = max(stride, w * getBytesPerPixel(c));
	_color = c;
	_alpha = a;
}

bool Bitmap::updateStride(const StrideFn &strideFn) {
	uint32_t outStride = (strideFn) ? max(strideFn(_color, _width), _width * getBytesPerPixel(_color)):_width * getBytesPerPixel(_color);
	if (outStride != _stride) {
		Bytes out;
		out.resize(_height * outStride);
		size_t minStride = _width * getBytesPerPixel(_color);
		for (size_t j = 0; j < _height; j ++) {
			memcpy(out.data() + j * outStride, _data.data() + j * _stride, minStride);
		}
		_data = std::move(out);
		_stride = outStride;
		return true;
	}
	return true;
}

bool Bitmap::convert(PixelFormat color, const StrideFn &strideFn) {
	if (color == PixelFormat::Auto) {
		color = _color;
	}
	if (_color == color) {
		return updateStride(strideFn);
	}

	bool ret = false;
	Bytes out;
	uint32_t outStride = (strideFn) ? max(strideFn(color, _width), _width * getBytesPerPixel(color)) : _width * getBytesPerPixel(color);

	switch (_color) {
	case PixelFormat::A8:
		switch (color) {
		case PixelFormat::A8: return true; break;
		case PixelFormat::I8: _color = color; return true; break;
		case PixelFormat::IA88: ret = convertData<PixelFormat::A8, PixelFormat::IA88>(_data, out, _stride, outStride); break;
		case PixelFormat::RGB888: ret = convertData<PixelFormat::A8, PixelFormat::RGB888>(_data, out, _stride, outStride); break;
		case PixelFormat::RGBA8888: ret = convertData<PixelFormat::A8, PixelFormat::RGBA8888>(_data, out, _stride, outStride); break;
		case PixelFormat::Auto: return false; break;
		}
		break;
	case PixelFormat::I8:
		switch (color) {
		case PixelFormat::A8: _color = color; return true; break;
		case PixelFormat::I8: return true; break;
		case PixelFormat::IA88: ret = convertData<PixelFormat::I8, PixelFormat::IA88>(_data, out, _stride, outStride); break;
		case PixelFormat::RGB888: ret = convertData<PixelFormat::I8, PixelFormat::RGB888>(_data, out, _stride, outStride); break;
		case PixelFormat::RGBA8888: ret = convertData<PixelFormat::I8, PixelFormat::RGBA8888>(_data, out, _stride, outStride); break;
		case PixelFormat::Auto: return false; break;
		}
		break;
	case PixelFormat::IA88:
		switch (color) {
		case PixelFormat::A8: ret = convertData<PixelFormat::IA88, PixelFormat::A8>(_data, out, _stride, outStride); break;
		case PixelFormat::I8: ret = convertData<PixelFormat::IA88, PixelFormat::I8>(_data, out, _stride, outStride); break;
		case PixelFormat::IA88: return true; break;
		case PixelFormat::RGB888: ret = convertData<PixelFormat::IA88, PixelFormat::RGB888>(_data, out, _stride, outStride); break;
		case PixelFormat::RGBA8888: ret = convertData<PixelFormat::IA88, PixelFormat::RGBA8888>(_data, out, _stride, outStride); break;
		case PixelFormat::Auto: return false; break;
		}
		break;
	case PixelFormat::RGB888:
		switch (color) {
		case PixelFormat::A8: ret = convertData<PixelFormat::RGB888, PixelFormat::A8>(_data, out, _stride, outStride); break;
		case PixelFormat::I8: ret = convertData<PixelFormat::RGB888, PixelFormat::I8>(_data, out, _stride, outStride); break;
		case PixelFormat::IA88: ret = convertData<PixelFormat::RGB888, PixelFormat::IA88>(_data, out, _stride, outStride); break;
		case PixelFormat::RGB888: return true; break;
		case PixelFormat::RGBA8888: ret = convertData<PixelFormat::RGB888, PixelFormat::RGBA8888>(_data, out, _stride, outStride); break;
		case PixelFormat::Auto: return false; break;
		}
		break;
	case PixelFormat::RGBA8888:
		switch (color) {
		case PixelFormat::A8: ret = convertData<PixelFormat::RGBA8888, PixelFormat::A8>(_data, out, _stride, outStride); break;
		case PixelFormat::I8: ret = convertData<PixelFormat::RGBA8888, PixelFormat::I8>(_data, out, _stride, outStride); break;
		case PixelFormat::IA88: ret = convertData<PixelFormat::RGBA8888, PixelFormat::IA88>(_data, out, _stride, outStride); break;
		case PixelFormat::RGB888: ret = convertData<PixelFormat::RGBA8888, PixelFormat::RGB888>(_data, out, _stride, outStride); break;
		case PixelFormat::RGBA8888: return true; break;
		case PixelFormat::Auto: return false; break;
		}
		break;
	case PixelFormat::Auto: return false; break;
	}

	if (ret) {
		_color = color;
		_data = std::move(out);
		_stride = outStride;
	}

	return ret;
}

bool Bitmap::truncate(PixelFormat color, const StrideFn &strideFn) {
	if (color == PixelFormat::Auto) {
		color = _color;
	}
	if (_color == color) {
		return updateStride(strideFn);
	}

	if (getBytesPerPixel(color) == getBytesPerPixel(_color)) {
		_color = color;
		return true;
	}

	auto height = _height;
	auto data = _data.data();
	auto bppIn = getBytesPerPixel(_color);

	Bytes out;
	uint32_t outStride = (strideFn) ? max(strideFn(color, _width), _width * getBytesPerPixel(color)) : _width * getBytesPerPixel(color);
	out.resize(height * outStride);
	auto bppOut = getBytesPerPixel(color);

	auto fillBytes = min(bppIn, bppOut);
	auto clearBytes = bppOut - fillBytes;

	for (size_t j = 0; j < height; j ++) {
		auto inData = data + _stride * j;
		auto outData = out.data() + outStride * j;
	    for (size_t i = 0; i < _stride; i += bppIn) {
	    	for (uint8_t k = 0; k < fillBytes; ++ k) {
		        *outData++ = inData[i * bppIn + k];
	    	}
	    	for (uint8_t k = 0; k < clearBytes; ++ k) {
		        *outData++ = 0;
	    	}
	    }
	}

	_color = color;
	_data = std::move(out);
	_stride = outStride;

	return true;
}

NS_SP_END
