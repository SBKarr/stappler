//
//  SPDeviceOrientation.h
//  stappler
//
//  Created by SBKarr on 7/22/14.
//  Copyright (c) 2014 SBKarr. All rights reserved.
//

#ifndef __stappler__SPScreenOrientation__
#define __stappler__SPScreenOrientation__

#include "SPDefine.h"

NS_SP_BEGIN

class ScreenOrientation {
public:
	static const ScreenOrientation Landscape;
	static const ScreenOrientation LandscapeLeft;
	static const ScreenOrientation LandscapeRight;

	static const ScreenOrientation Portrait;
	static const ScreenOrientation PortraitTop;
	static const ScreenOrientation PortraitBottom;

	bool isLandscape() const;
	bool isPortrait() const;
	bool isTransition(const ScreenOrientation &) const;

	ScreenOrientation();
	explicit ScreenOrientation(int64_t val);
	ScreenOrientation(const ScreenOrientation &);
	ScreenOrientation &operator=(const ScreenOrientation &);
	ScreenOrientation(ScreenOrientation &&);
	ScreenOrientation &operator=(ScreenOrientation &&);

	bool operator== (const ScreenOrientation &) const;
	bool operator!= (const ScreenOrientation &) const;

	operator uint8_t() const;
	operator int64_t() const;

	int64_t asInt64() const;
	uint8_t asUint8() const;
private:
	ScreenOrientation(uint8_t value);

	uint8_t _value;
};

NS_SP_END

#endif /* defined(__stappler__SPDeviceOrientation__) */
