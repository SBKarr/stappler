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

#include "Material.h"

NS_MD_BEGIN

namespace metrics {

inline float horizontalIncrement() {
	return (isTablet()?64.0f:56.0f);
}

inline MenuMetrics menuDefaultMetrics() {
	return (isTouchDevice()?MenuMetrics::NormalMobile:MenuMetrics::NormalDesktop);
}

inline float menuVerticalPadding(MenuMetrics s) { return (s == MenuMetrics::NormalDesktop)?16.0f:8.0f; }

inline float menuItemHeight(MenuMetrics s) { return (s == MenuMetrics::NormalDesktop)?32.0f:48.0f; }

inline float menuFirstLeftKeyline(MenuMetrics s) {
	switch (s) {
	case MenuMetrics::Navigation: return 16.0f; break;
	case MenuMetrics::NormalMobile: return 16.0f; break;
	case MenuMetrics::NormalDesktop: return 24.0f; break;
	default: break;
	}
	return 0.0f;
}

inline float menuSecondLeftKeyline(MenuMetrics s) {
	switch (s) {
	case MenuMetrics::Navigation: return 72.0f; break;
	case MenuMetrics::NormalMobile: return 64.0f; break;
	case MenuMetrics::NormalDesktop: return 64.0f; break;
	default: break;
	}
	return 0.0f;
}

inline float menuRightKeyline(MenuMetrics s) {
	switch (s) {
	case MenuMetrics::Navigation: return 16.0f; break;
	case MenuMetrics::NormalMobile: return 16.0f; break;
	case MenuMetrics::NormalDesktop: return 24.0f; break;
	default: break;
	}
	return 0.0f;
}

}

NS_MD_END
