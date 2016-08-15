//
//  SPScreenOrientation.cpp
//  stappler
//
//  Created by SBKarr on 7/22/14.
//  Copyright (c) 2014 SBKarr. All rights reserved.
//

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
