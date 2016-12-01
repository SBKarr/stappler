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

#ifndef LIBS_MATERIAL_RESOURCES_MATERIALMETRICS_H_
#define LIBS_MATERIAL_RESOURCES_MATERIALMETRICS_H_

#include "Material.h"

NS_MD_BEGIN

enum class MenuMetrics {
	Navigation,
	NormalMobile,
	NormalDesktop,
};

namespace metrics {

bool isTouchDevice();
bool isTablet();
float appBarHeight();
float miniBarHeight();

inline float horizontalIncrement();



inline MenuMetrics menuDefaultMetrics();

inline float menuVerticalPadding(MenuMetrics);
inline float menuItemHeight(MenuMetrics);
inline float menuFirstLeftKeyline(MenuMetrics);
inline float menuSecondLeftKeyline(MenuMetrics);
inline float menuRightKeyline(MenuMetrics);

inline float tabMinWidth() { return 72.0f; }
inline float tabMaxWidth() { return 264.0f; }
inline float tabHeight() { return 48.0f; }

}

NS_MD_END

#include "MaterialMetricsInline.h"

#endif /* LIBS_MATERIAL_RESOURCES_MATERIALMETRICS_H_ */
