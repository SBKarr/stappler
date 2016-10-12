/*
 * SPForward.h
 *
 *  Created on: 11 сент. 2015 г.
 *      Author: sbkarr
 */

#ifndef LIBS_STAPPLER_CORE_SPFORWARD_H_
#define LIBS_STAPPLER_CORE_SPFORWARD_H_

#include "SPCommon.h"

#include "SPLog.h"
#include "SPTime.h"
#include "SPData.h"

#include "math/CCGeometry.h"
#include "base/ccTypes.h"
#include "platform/CCStdC.h"

NS_CC_BEGIN

class Renderer;
class Ref;

NS_CC_END

NS_SP_BEGIN

using Vec2 = cocos2d::Vec2;
using Vec3 = cocos2d::Vec3;
using Vec4 = cocos2d::Vec4;
using Size = cocos2d::Size;
using Rect = cocos2d::Rect;

using Mat4 = cocos2d::Mat4;
using Quaternion = cocos2d::Quaternion;

using Color4B = cocos2d::Color4B;
using Color4F = cocos2d::Color4F;
using Color3B = cocos2d::Color3B;

using Renderer = cocos2d::Renderer;
using Ref = cocos2d::Ref;

NS_SP_END

#include "SPRc.h"
#include "SPPadding.h"
#include "SPMovingAverage.h"
#include "SPString.h"

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
class EventHandlerInterface;

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

class Font;
class FontSet;
class FontSource;

class Icon;
class IconSet;

class Screen;
class Device;
class ScreenOrientation;

class StoreKit;
class StoreProduct;

class Image;
class TextureInterface;
template <class T> class Texture;
class TextureFBO;
class TextureRef;

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

class RichLabel;

class Overscroll;
class ScrollViewBase;
class ScrollView;

class ClippingNode;
class StrictNode;
class AlwaysDrawedNode;

class EventListener;

class ScrollController;

class Accelerated;
class FadeIn;
class FadeOut;
class ProgressAction;
class Timeout;

using FilePath = ValueWrapper<String, class FilePathTag>;

NS_SP_END


NS_SP_EXT_BEGIN(data)

class Source;

class Subscription;
template <class T> class Binding;
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

class Formatter;
class HyphenMap;
class Document;
class Result;
class Node;
class Reader;
class Source;

struct MultipartParser;
struct MediaParameters;
class MediaResolver;

struct Object;
struct Layout;

class Renderer;
class View;
class Drawer;

NS_SP_EXT_END(rich_text)


NS_SP_EXT_BEGIN(draw)

class Path;
class PathNode;
class Canvas;

NS_SP_EXT_END(draw)


NS_SP_EXT_BEGIN(ime)

struct Cursor;
struct Handler;

NS_SP_EXT_END(ime)


NS_SP_EXT_BEGIN(font)

class FontLayout;
class FontTexture;
class Source;

NS_SP_EXT_END(font)

#endif /* LIBS_STAPPLER_CORE_SPFORWARD_H_ */
