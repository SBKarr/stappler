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

#ifndef LIBS_STAPPLER_CORE_TEXTURE_SPTEXTURE_H_
#define LIBS_STAPPLER_CORE_TEXTURE_SPTEXTURE_H_

#include "SPTextureInterface.h"

NS_SP_BEGIN

template <class T>
class Texture : public TextureInterface {
public:
	enum class DataPolicy {
		Copy,
		TransferOwnership,
		Borrowing,
	};

	Texture(uint16_t width, uint16_t height);
	Texture(uint16_t width, uint16_t height, void *data, DataPolicy = DataPolicy::Copy);
	Texture(uint16_t width, uint16_t height, const void *data,
			uint16_t x, uint16_t y, uint16_t patchWidth, uint16_t patchHeight);

	~Texture();

	Texture(Texture<T> &&other);
	Texture<T> &operator=(Texture<T> &&other);

	virtual uint8_t getBytesPerPixel() const { return sizeof(T); }
	virtual PixelFormat getFormat() const { return T::format(); }
	virtual uint8_t getComponentsCount() const { return T::components(); }
	virtual uint8_t getBitsPerComponent() const { return T::bitsPerComponent(); }

	virtual uint16_t getWidth() const {return _width; };
	virtual uint16_t getHeight() const {return _height; };

	virtual cocos2d::Texture2D *generateTexture() const;

	template <class K>
	void patch(uint16_t x, uint16_t y, const Texture<K> &tex);

	template <class K>
	void merge(const int16_t &x, const int16_t &y, const Texture<K> &tex, const cocos2d::Color4B & = cocos2d::Color4B(255, 255, 255, 255));

	void save(const std::string &);

	virtual bool drawRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const cocos2d::Color4B &);
	virtual bool drawOutline(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t outline, const cocos2d::Color4B &);

	bool downsample(uint16_t width, uint16_t height);
	bool downsample(uint16_t width, uint16_t height, Texture<T> &);
	bool downsample(uint16_t width, uint16_t height, T *, uint16_t);

	T * slice(uint16_t x, uint16_t y, uint16_t width, uint16_t height);

	T * _data = nullptr;
	uint16_t _width = 0;
	uint16_t _height = 0;
	bool _dataOwner = true;

	Texture(const Texture<T> &) = delete;
	Texture<T> &operator=(const Texture<T> &) = delete;

protected:
	template <class K>
	void mergePixel(T &, K &, const cocos2d::Color4B &);

	void setPixel(T &, const cocos2d::Color4B &);

	T average(
			const uint32_t &aa, float rAA,
			const uint32_t &ba, float rBA,
			const uint32_t &bb, float rBB,
			const uint32_t &ab, float rAB);
};

template <class T>
Texture<T>::Texture(uint16_t width, uint16_t height) {
	_width = width;
	_height = height;
	_data = new T[_width * _height];
	memset(_data, 0, sizeof(T) * _width * _height);
}
template <class T>
Texture<T>::Texture(uint16_t width, uint16_t height, void *data, DataPolicy policy) {
	_width = width;
	_height = height;
	_data = new T[_width * _height];
	if (policy == DataPolicy::Copy) {
		memcpy(_data, data, getBytesPerPixel() * _width * _height);
	} else {
		if (policy == DataPolicy::Borrowing) {
			_dataOwner = false;
		}
		_data = (T *)data;
	}
}
template <class T>
Texture<T>::Texture(uint16_t width, uint16_t height, const void *data,
		uint16_t x, uint16_t y, uint16_t patchWidth, uint16_t patchHeight) {
	_width = patchWidth;
	_height = patchHeight;

	_data = new T[_width * _height];
	memset(_data, 0, sizeof(T) * _width * _height);

	auto src = static_cast<const T *>(data);
	for (uint16_t line = 0; line < patchHeight; line ++) {
		memcpy((void *)(_data + (_width * line)),
				(const void *)(src + (width * (line + y)) + x), patchWidth * getBytesPerPixel());
	}
}

template <class T>
Texture<T>::~Texture() {
	if (_dataOwner) {
		CC_SAFE_DELETE_ARRAY(_data);
	}
}

template <class T>
Texture<T>::Texture(Texture<T> &&other) {
	_width = other._width;
	_height = other._height;
	_data = other._data;
	_dataOwner = other._dataOwner;
	other._data = nullptr;
}

template <class T>
Texture<T> &Texture<T>::operator=(Texture<T> &&other) {
	_width = other._width;
	_height = other._height;
	_data = other._data;
	_dataOwner = other._dataOwner;
	other._data = nullptr;
	return *this;
}

template <class T>
cocos2d::Texture2D *Texture<T>::generateTexture() const {
	if (_width == 0 || _height == 0) {
		return nullptr;
	}
	cocos2d::Texture2D *tex = new cocos2d::Texture2D();
	if (tex->initWithData(reinterpret_cast<const void *>(_data), getBytesPerPixel() * _width * _height,
			convertPixelFormat(getFormat()), _width, _height)) {
		tex->setAliasTexParameters();
		tex->autorelease();
		return tex;
	}
	delete tex;
	return nullptr;
}

template <class T>
template <class K>
void Texture<T>::merge(const int16_t &x, const int16_t &y, const Texture<K> &tex, const cocos2d::Color4B &color) {
	if (x > -tex._width && y > -tex._height && x < (int16_t)_width && y < (int16_t)_height) {
		const int16_t firstLine = (y < 0)?(abs(y)):0;
		const int16_t lastLine = (y + tex._height > (int16_t)_height)?(_height - y):tex._height;

		const int16_t firstCol = (x < 0)?(abs(x)):0;
		const int16_t lastCol = (x + tex._width > (int16_t)_width)?(_width - x):tex._width;

		for (uint16_t line = firstLine; line < lastLine; line ++) {
			for (uint16_t px = firstCol; px < lastCol; px ++) {
				mergePixel(_data[(_width * (line + y)) + x + px], tex._data[(line * tex._width) + px], color);
			}
		}
	}
}

template <class T>
bool Texture<T>::drawRect(uint16_t posX, uint16_t posY, uint16_t width, uint16_t height, const cocos2d::Color4B &color) {
	if (posX + width > _width || posY + height > _height) {
		return false;
	}

	for (uint16_t x = posX; x < posX + width; x ++) {
		for (uint16_t y = posY; y < posY + height; y++) {
			setPixel(_data[y * _width + x], color);
		}
	}
	return true;
}

template <class T>
bool Texture<T>::drawOutline(uint16_t posX, uint16_t posY, uint16_t width, uint16_t height, uint16_t outline, const cocos2d::Color4B &color) {
	int16_t outlineX, outlineY, outlineWidth, outlineHeight;

	outlineX = (posX - outline);
	outlineY = (posY - outline);
	outlineWidth = (width + outline * 2);
	outlineHeight = (height + outline * 2);


	for (int16_t x = outlineX; x < outlineX + outlineWidth; x ++) {
		for (int16_t y = outlineY; y < outlineY + outlineHeight; y++) {
			if (x > 0 && x < _width && y > 0 && y < _height
					&& ((x < posX || x >= posX + width) || (y < posY || y >= posY + height))) {
				setPixel(_data[y * _width + x], color);
			}
		}
	}

	return true;
}

template <class T>
T Texture<T>::average(
		const uint32_t &aa, float rAA,
		const uint32_t &ba, float rBA,
		const uint32_t &bb, float rBB,
		const uint32_t &ab, float rAB) {
	uint8_t buf[sizeof(T)];
	for (uint8_t i = 0; i < sizeof(T); i++) {
		buf[i] = (uint8_t) ( (*(reinterpret_cast<uint8_t *>(_data) + aa * sizeof(T) + i)) * rAA
				+  (*(reinterpret_cast<uint8_t *>(_data) + ba * sizeof(T) + i)) * rBA
				+  (*(reinterpret_cast<uint8_t *>(_data) + bb * sizeof(T) + i)) * rBB
				+  (*(reinterpret_cast<uint8_t *>(_data) + ab * sizeof(T) + i)) * rAB );
	}
	T ret;
	ret.set(buf);
	return ret;
}

template <class T>
bool Texture<T>::downsample(uint16_t width, uint16_t height) {
	if (width > _width && height > _height) {
		return false;
	}

	return downsample(width, height, _data, _width);
}

template <class T>
bool Texture<T>::downsample(uint16_t width, uint16_t height, Texture<T> &tex) {
	if (width > tex._width && height > tex._height) {
		return false;
	}

	return downsample(width, height, tex._data, tex._width);
}

template <class T>
bool Texture<T>::downsample(uint16_t width, uint16_t height, T * data, uint16_t dataWidth) {
	float scaleX = (float)width / (float)_width;
	float scaleY = (float)height / (float)_height;

	// first - linear interpolation by x
	for (uint16_t y = 0; y < height; y++) {
		float posY = y;
		float scaledPosY = posY / scaleY;
		uint16_t ya = (uint16_t) floorf(scaledPosY);
		uint16_t yb = (uint16_t) ya + 1;
		float ryA = posY - ya * scaleX;
		float ryB = 1.0f - ryA;

		for (uint16_t x = 0; x < width; x++) {
			float posX = x;
			float scaledPosX = posX / scaleX;
			uint16_t xa = (uint16_t) floorf(scaledPosX);
			uint16_t xb = (uint16_t) xa + 1;
			float rxA = posX - xa * scaleX;
			float rxB = 1.0f - rxA;

			data[x + y * dataWidth] = average(xa + ya * _width, rxA * ryA, xb + ya * _width, rxB * ryA,
					xb + yb * _width, rxB * ryB, xa + yb * _width, rxA * ryB);
		}
	}
	return true;
}

template <class T>
T * Texture<T>::slice(uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
	if (x + width > _width || y + height > _height) {
		return nullptr;
	}

	T * ret = new T[width * height];
	for (auto i = y; i < y + height; i ++) {
		memcpy(ret + i * width, _data + x + (y + i) * _width, sizeof(T) * width);
	}

	return ret;
}

NS_SP_END

#endif /* LIBS_STAPPLER_CORE_TEXTURE_SPTEXTURE_H_ */
