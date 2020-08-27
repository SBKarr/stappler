/**
Copyright (c) 2020 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef COMPONENTS_XENOLITH_CORE_XLFORWARD_H_
#define COMPONENTS_XENOLITH_CORE_XLFORWARD_H_

#include "SPLayout.h"
#include "SPFilesystem.h"
#include "SPThreadTask.h"
#include "SPData.h"
#include "SPLog.h"
#include "SPSpanView.h"

#include "XLConfig.h"

#define XL_ASSERT(cond)    assert(cond)

#ifndef XLASSERT
#if DEBUG
#define XLASSERT(cond, msg) XL_ASSERT(cond)
#else
#define XLASSERT(cond, msg)
#endif
#endif

namespace stappler::xenolith {

using Vec2 = layout::Vec2;
using Vec3 = layout::Vec3;
using Vec4 = layout::Vec4;
using Size = layout::Size;
using Rect = layout::Rect;

using Mat4 = layout::Mat4;
using Quaternion = layout::Quaternion;

using Color4B = layout::Color4B;
using Color4F = layout::Color4F;
using Color3B = layout::Color3B;

using Padding = layout::Padding;
using Margin = layout::Margin;

template <size_t Count>
using MovingAverage = layout::MovingAverage<Count>;
using FilePath = layout::FilePath;

namespace Anchor {
extern const Vec2 Middle; /** equals to Vec2(0.5, 0.5) */
extern const Vec2 BottomLeft; /** equals to Vec2(0, 0) */
extern const Vec2 TopLeft; /** equals to Vec2(0, 1) */
extern const Vec2 BottomRight; /** equals to Vec2(1, 0) */
extern const Vec2 TopRight; /** equals to Vec2(1, 1) */
extern const Vec2 MiddleRight; /** equals to Vec2(1, 0.5) */
extern const Vec2 MiddleLeft; /** equals to Vec2(0, 0.5) */
extern const Vec2 MiddleTop; /** equals to Vec2(0.5, 1) */
extern const Vec2 MiddleBottom; /** equals to Vec2(0.5, 0) */
}

struct AlphaTest {
	enum State : uint8_t {
		Disabled,
		LessThen,
		GreatherThen,
	};

	State state = State::Disabled;
	uint8_t value = 0;
};

enum ScreenOrientation {
	Landscape = 1,
	LandscapeLeft,
	LandscapeRight,
	Portrait,
	PortraitTop,
	PortraitBottom
};

class Event;
class EventHeader;
class EventHandler;
class EventHandlerNode;

class Director;
class Scene;
class ProgramManager;

using Task = thread::Task;

} // stappler::xenolith

#endif /* COMPONENTS_XENOLITH_CORE_XLFORWARD_H_ */
