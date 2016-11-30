// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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

#include "SPScreenOrientation.h"

NS_SP_BEGIN

namespace _ScreenOrientation {
	enum Values : uint8_t {
		_Landscape = 1 | 0,
		_LandscapeLeft = 1 | 4,
		_LandscapeRight = 1 | 8,
		_Portrait = 2 | 0,
		_PortraitTop = 2 | 16,
		_PortraitBottom = 2 | 32,
	};
};

const ScreenOrientation ScreenOrientation::Landscape((int64_t)_ScreenOrientation::_Landscape);
const ScreenOrientation ScreenOrientation::LandscapeLeft((int64_t)_ScreenOrientation::_LandscapeLeft);
const ScreenOrientation ScreenOrientation::LandscapeRight((int64_t)_ScreenOrientation::_LandscapeRight);

const ScreenOrientation ScreenOrientation::Portrait((int64_t)_ScreenOrientation::_Portrait);
const ScreenOrientation ScreenOrientation::PortraitTop((int64_t)_ScreenOrientation::_PortraitTop);
const ScreenOrientation ScreenOrientation::PortraitBottom((int64_t)_ScreenOrientation::_PortraitBottom);

bool ScreenOrientation::isLandscape() const {
	return (bool)((_value & 1) != 0);
}
bool ScreenOrientation::isPortrait() const {
	return (bool)((_value & 2) != 0);
}
bool ScreenOrientation::isTransition(const ScreenOrientation &dev) const {
	return ((dev._value) & (_value)) == 0;
}

ScreenOrientation::ScreenOrientation() {
	_value = _ScreenOrientation::_Portrait;
}
ScreenOrientation::ScreenOrientation(int64_t val) {
	_value = (uint8_t)val;
}

ScreenOrientation::ScreenOrientation(const ScreenOrientation &dev) : _value(dev._value) { }
ScreenOrientation &ScreenOrientation::operator=(const ScreenOrientation &dev) {
	_value = dev._value;
	return *this;
}
ScreenOrientation::ScreenOrientation(ScreenOrientation &&dev) : _value(dev._value) { }
ScreenOrientation &ScreenOrientation::operator=(ScreenOrientation &&dev) {
	_value = dev._value;
	return *this;
}

bool ScreenOrientation::operator== (const ScreenOrientation &dev) const {
	if (_value == _ScreenOrientation::_Landscape || _value == _ScreenOrientation::_Portrait || dev._value == _ScreenOrientation::_Landscape || dev._value == _ScreenOrientation::_Portrait) {
		return (dev._value & _value) != 0;
	} else {
		return dev._value == _value;
	}
}
bool ScreenOrientation::operator!= (const ScreenOrientation &dev) const {
	return !(*this == dev);
}

ScreenOrientation::operator uint8_t() const {
	return _value;
}
ScreenOrientation::operator int64_t() const {
	return _value;
}
int64_t ScreenOrientation::asInt64() const {
	return _value;
}
uint8_t ScreenOrientation::asUint8() const {
	return _value;
}
ScreenOrientation::ScreenOrientation(uint8_t value) : _value(value) { }

NS_SP_END
