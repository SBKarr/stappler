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

#ifndef LIBS_STAPPLER_CORE_SPFORWARD_H_
#define LIBS_STAPPLER_CORE_SPFORWARD_H_

#include "SPLayout.h"
#include "platform/CCStdC.h"

NS_CC_BEGIN

class Renderer;
class Node;

NS_CC_END

NS_SP_BEGIN

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

using Renderer = cocos2d::Renderer;

using Padding = layout::Padding;
using Margin = layout::Margin;

using MovingAverage = layout::MovingAverage;
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

NS_SP_END

NS_CC_BEGIN

class Node;
class Sprite;
class Scene;
class ClippingNode;
class Component;
class TextureAtlas;
class Texture2D;
class GLProgram;
class Image;

class Touch;
class Event;
class EventCustom;
class EventMouse;

class EventListenerTouchOneByOne;
class EventListenerCustom;
class EventListenerKeyboard;
class EventListenerMouse;

class Director;
class Renderer;
class Scheduler;
class TrianglesCommand;
class QuadCommand;
class GroupCommand;
class CustomCommand;

class FiniteTimeAction;
class ActionInterval;

NS_CC_END


NS_SP_BEGIN

class EventHeader;
class Event;
class EventHandlerNode;
class EventHandler;

class Bitmap;
class SyncRWLock;

class Thread;
class Task;
class ThreadManager;
class ThreadHandlerInterface;
class TaskManager;
class TaskStack;

class NetworkTask;
class NetworkDownload;

class Asset;
class AssetRef;
class AssetDownload;
class AssetLibrary;

class Icon;
class IconSet;

class Screen;
class Device;
class ScreenOrientation;

class StoreKit;
class StoreProduct;

class DynamicAtlas;
class DynamicBatchCommand;
class DynamicMaterial;
class DynamicQuadArray;
class GLProgramSet;

class DynamicBatchScene;
class DynamicBatchNode;
class DynamicSprite;
class IconSprite;
class CustomCornerSprite;
struct Gradient;
class Layer;
class RoundedSprite;
class Scale9Sprite;
class ShadowSprite;
class NetworkSprite;

class Overscroll;
class ScrollViewBase;
class ScrollView;

class ClippingNode;
class StrictNode;
class AlwaysDrawedNode;

class EventListener;

class ScrollController;
class ScrollItemHandle;

class Accelerated;
class FadeIn;
class FadeOut;
class ProgressAction;
class Timeout;

NS_SP_END


NS_SP_EXT_BEGIN(data)

class Source;

using Subscription = layout::Subscription;
template <class T> using Binding = layout::Binding<T>;
template <class T> class Listener;

NS_SP_EXT_END(data)


NS_SP_EXT_BEGIN(gesture)

struct Touch;
struct Tap;
struct Press;
struct Swipe;
struct Pinch;
struct Rotate;
struct Wheel;

enum class Type;
enum class Event;

class Listener;

class Recognizer;
class PressRecognizer;
class TapRecognizer;
class TouchRecognizer;
class SwipeRecognizer;
class PinchRecognizer;
class RotateRecognizer;
class WheelRecognizer;

NS_SP_EXT_END(gesture)


NS_SP_EXT_BEGIN(storage)

class Scheme;
class Handle;

NS_SP_EXT_END(storage)


NS_SP_EXT_BEGIN(rich_text)

class Renderer;
class View;
class Drawer;

NS_SP_EXT_END(rich_text)


NS_SP_EXT_BEGIN(draw)

class PathNode;
class Canvas;

NS_SP_EXT_END(draw)


NS_SP_EXT_BEGIN(ime)

struct Cursor;
struct Handler;

NS_SP_EXT_END(ime)


NS_SP_EXT_BEGIN(font)

class FontSource;
class Controller;

NS_SP_EXT_END(font)

#endif /* LIBS_STAPPLER_CORE_SPFORWARD_H_ */
