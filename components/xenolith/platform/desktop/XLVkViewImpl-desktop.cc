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

#include "XLVkViewImpl-desktop.h"

#if (MSYS || CYGWIN || MACOS)

namespace stappler::xenolith::vk {

/*class GLFWEventHandler {
public:

	static void onGLFWMouseCallBack(GLFWwindow* window, int button, int action, int modify) {
		if (_view) {
			_view->onGLFWMouseCallBack(window, button, action, modify);
		}
	}

	static void onGLFWMouseMoveCallBack(GLFWwindow* window, double x, double y) {
		if (_view) {
			_view->onGLFWMouseMoveCallBack(window, x, y);
		}
	}

	static void onGLFWMouseScrollCallback(GLFWwindow* window, double x, double y) {
		if (_view) {
			_view->onGLFWMouseScrollCallback(window, x, y);
		}
	}

	static void onGLFWKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
		if (_view) {
			_view->onGLFWKeyCallback(window, key, scancode, action, mods);
		}
	}

	static void onGLFWCharCallback(GLFWwindow* window, unsigned int character) {
		if (_view) {
			_view->onGLFWCharCallback(window, character);
		}
	}

	static void onGLFWWindowPosCallback(GLFWwindow* windows, int x, int y) {
		if (_view) {
			_view->onGLFWWindowPosCallback(windows, x, y);
		}
	}

	static void onGLFWframebuffersize(GLFWwindow* window, int w, int h) {
		if (_view) {
			_view->onGLFWframebuffersize(window, w, h);
		}
	}

	static void onGLFWWindowSizeFunCallback(GLFWwindow *window, int width, int height) {
		if (_view) {
			_view->onGLFWWindowSizeFunCallback(window, width, height);
		}
	}

	static void setGLViewImpl(VkViewImpl* view) {
		_view = view;
	}

	static void onGLFWWindowIconifyCallback(GLFWwindow* window, int iconified) {
		if (_view) {
			_view->onGLFWWindowIconifyCallback(window, iconified);
		}
	}

	static void onGLFWWindowFocusCallback(GLFWwindow* window, int focus) {
		if (_view) {
			_view->onGLFWWindowFocusCallback(window, focus);
		}
	}

private:
	static VkViewImpl * _view;
};*/

namespace {

using KeyCode = gesture::KeyCode;

static KeyCode s_keyMap[GLFW_KEY_LAST + 1] [[gnu::unused]] = {
	KeyCode::None,				KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None, // 0 - 7
	KeyCode::None,				KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None, // 8 - 15
	KeyCode::None,				KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None, // 16 - 23
	KeyCode::None,				KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None, // 24 - 31
	KeyCode::Key_Space,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::Key_Apostrophe, // 32 - 39
	KeyCode::None,				KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::Key_Comma,		KeyCode::Key_Minus,		KeyCode::Key_Period,	KeyCode::Key_Slash, // 40 - 47
	KeyCode::Key_0,				KeyCode::Key_1,			KeyCode::Key_2,			KeyCode::Key_3,			KeyCode::Key_4,			KeyCode::Key_5,			KeyCode::Key_6,			KeyCode::Key_7, // 48 - 55
	KeyCode::Key_8,				KeyCode::Key_9,			KeyCode::None,			KeyCode::Key_Semicolon, KeyCode::None,			KeyCode::Key_Equal,		KeyCode::None,			KeyCode::None, // 56 - 63
	KeyCode::None,				KeyCode::Key_A,			KeyCode::Key_B,			KeyCode::Key_C,			KeyCode::Key_D,			KeyCode::Key_E,			KeyCode::Key_F,			KeyCode::Key_G, // 64 - 71
	KeyCode::Key_H,				KeyCode::Key_I,			KeyCode::Key_J,			KeyCode::Key_K,			KeyCode::Key_L,			KeyCode::Key_M,			KeyCode::Key_N,			KeyCode::Key_O, // 72 - 79
	KeyCode::Key_P,				KeyCode::Key_Q,			KeyCode::Key_R,			KeyCode::Key_S,			KeyCode::Key_T,			KeyCode::Key_U,			KeyCode::Key_V,			KeyCode::Key_W, // 80 - 87
	KeyCode::Key_X,				KeyCode::Key_Y,			KeyCode::Key_Z,			KeyCode::Key_LeftBracket,KeyCode::Key_BackSlash,KeyCode::Key_RightBracket,KeyCode::None,		KeyCode::None, // 88 - 95
	KeyCode::Key_Grave,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None, // 96 - 103
	KeyCode::None,				KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None, // 104 - 111
	KeyCode::None,				KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None, // 112 - 119
	KeyCode::None,				KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None, // 120 - 127
	KeyCode::None,				KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None, // 128 - 135
	KeyCode::None,				KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None, // 136 - 143
	KeyCode::None,				KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None, // 144 - 151
	KeyCode::None,				KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None, // 152 - 159
	KeyCode::None,				KeyCode::Key_Grave,		KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None, // 160 - 167
	KeyCode::None,				KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None, // 168 - 175
	KeyCode::None,				KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None, // 176 - 183
	KeyCode::None,				KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None, // 184 - 191
	KeyCode::None,				KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None, // 192 - 199
	KeyCode::None,				KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None, // 200 - 207
	KeyCode::None,				KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None, // 208 - 215
	KeyCode::None,				KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None, // 216 - 223
	KeyCode::None,				KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None, // 224 - 231
	KeyCode::None,				KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None, // 232 - 239
	KeyCode::None,				KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None, // 240 - 247
	KeyCode::None,				KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None, // 248 - 255
	KeyCode::Key_Escape,		KeyCode::Key_Enter,		KeyCode::Key_Tab,		KeyCode::Key_Backspace,	KeyCode::Key_Insert,	KeyCode::Key_Delete,	KeyCode::Key_RightArrow,KeyCode::Key_LeftArrow, // 256 - 263
	KeyCode::Key_DownArrow, 	KeyCode::Key_UpArrow,	KeyCode::Key_PgUp,		KeyCode::Key_PgDown,	KeyCode::Key_Home,		KeyCode::Key_End,		KeyCode::None,			KeyCode::None, // 264 - 271
	KeyCode::None,				KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None, // 272 - 279
	KeyCode::Key_CapsLock,		KeyCode::Key_ScrollLock, KeyCode::Key_NumLock,	KeyCode::Key_Print,		KeyCode::Key_Pause,		KeyCode::None,			KeyCode::None,			KeyCode::None, // 280 - 287
	KeyCode::None,				KeyCode::None,			KeyCode::Key_F1,		KeyCode::Key_F2,		KeyCode::Key_F3,		KeyCode::Key_F4,		KeyCode::Key_F5,		KeyCode::Key_F6, // 288 - 295
	KeyCode::Key_F7,			KeyCode::Key_F8,		KeyCode::Key_F9,		KeyCode::Key_F10,		KeyCode::Key_F11,		KeyCode::Key_F12,		KeyCode::None,			KeyCode::None, // 296 - 303
	KeyCode::None,				KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None, // 304 - 311
	KeyCode::None,				KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::None, // 312 - 319
	KeyCode::Key_0,				KeyCode::Key_1,			KeyCode::Key_2,			KeyCode::Key_3,			KeyCode::Key_4,			KeyCode::Key_5,			KeyCode::Key_6,			KeyCode::Key_7, // 320 - 327
	KeyCode::Key_8,				KeyCode::Key_9,			KeyCode::Key_NumPadPeriod,KeyCode::Key_NumPadDivide,KeyCode::Key_NumPadMultiply,KeyCode::Key_NumPadMinus,KeyCode::Key_NumPadPlus, KeyCode::Key_NumPadEnter, // 328 - 335
	KeyCode::Key_NumPadEqual,	KeyCode::None,			KeyCode::None,			KeyCode::None,			KeyCode::Key_Shift,		KeyCode::Key_Ctrl,		KeyCode::Key_Alt,		KeyCode::Key_Hyper, // 336 - 343
	KeyCode::Key_RightShift,	KeyCode::Key_RightCtrl,	KeyCode::Key_RightAlt,	KeyCode::Key_Hyper,		KeyCode::Key_Menu, // 344 - 348
};

static std::mutex s_glfwErrorMutex;
static void onGLFWError(int errorID, const char* errorDesc) {
	log::format("GLFW", "GLFWError #%d Happen, %s\n", errorID, errorDesc);
}

static void onGLFWWindowSizeFunCallback(GLFWwindow *window, int width, int height) {
	if (auto view = reinterpret_cast<ViewImpl *>(glfwGetWindowUserPointer(window))) {
		view->onGLFWWindowSizeFunCallback(window, width, height);
	}
}

static void onGLFWMouseCallBack(GLFWwindow* window, int button, int action, int modify) {
	if (auto view = reinterpret_cast<ViewImpl *>(glfwGetWindowUserPointer(window))) {
		view->onGLFWMouseCallBack(window, button, action, modify);
	}
}

static void onGLFWMouseMoveCallBack(GLFWwindow* window, double x, double y) {
	if (auto view = reinterpret_cast<ViewImpl *>(glfwGetWindowUserPointer(window))) {
		view->onGLFWMouseMoveCallBack(window, x, y);
	}
}

static void onGLFWMouseScrollCallback(GLFWwindow* window, double x, double y) {
	if (auto view = reinterpret_cast<ViewImpl *>(glfwGetWindowUserPointer(window))) {
		view->onGLFWMouseScrollCallback(window, x, y);
	}
}

}

ViewImpl::ViewImpl() {
	_viewName = "Xenolith";

	s_glfwErrorMutex.lock();
	glfwSetErrorCallback(onGLFWError);
	s_glfwErrorMutex.unlock();
	glfwInit();
}

ViewImpl::~ViewImpl() {
	_device = nullptr;
	if (_instance) {
		_instance->vkDestroySurfaceKHR(_instance->getInstance(), _surface, nullptr);
	}
	_instance = nullptr;
	glfwDestroyWindow(_mainWindow);
	glfwTerminate();
}

bool ViewImpl::init(const StringView & viewName, Rect rect) {
	setViewName(viewName);

	if (!glfwVulkanSupported()) {
	    return false;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	//glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	_mainWindow = glfwCreateWindow(rect.size.width, rect.size.height, "Vulkan", _monitor, nullptr);
	glfwSetWindowUserPointer(_mainWindow, this);
	glfwSetFramebufferSizeCallback(_mainWindow, vk::onGLFWWindowSizeFunCallback);
    glfwSetMouseButtonCallback(_mainWindow, vk::onGLFWMouseCallBack);
    glfwSetCursorPosCallback(_mainWindow, vk::onGLFWMouseMoveCallBack);
    glfwSetScrollCallback(_mainWindow, vk::onGLFWMouseScrollCallback);

	auto instance = Instance::create();
	if (!instance) {
		return false;
	}

	_frameWidth = uint32_t(rect.size.width);
	_frameHeight = uint32_t(rect.size.height);

	if (glfwCreateWindowSurface(instance->getInstance(), _mainWindow, nullptr, &_surface) != VK_SUCCESS) {
		log::text("VkView", "Fail to create Vulkan surface for window");
		return false;
	}

	auto opts = instance->getPresentationOptions(_surface);
	if (opts.empty()) {
		log::text("VkView", "No available Vulkan devices for presentation on surface");
		return false;
	}

	auto targetOpts = opts.front();
	selectPresentationOptions(targetOpts);

	auto requiredFeatures = Instance::Features::getOptional();
	requiredFeatures.enableFromFeatures(Instance::Features::getRequired());
	requiredFeatures.disableFromFeatures(targetOpts.features);
	if (targetOpts.features.canEnable(requiredFeatures)) {
		auto device = Rc<PresentationDevice>::create(instance, this, _surface, move(targetOpts), requiredFeatures);
		if (!device) {
			log::text("VkView", "Fail to create Vulkan presentation device");
			return false;
		}

		return View::init(instance, device);
	}

	if (instance) {
		instance->vkDestroySurfaceKHR(instance->getInstance(), _surface, nullptr);
	}
	log::text("VkView", "Unable to create device, not all required features is supported");
	return false;
}

bool ViewImpl::init(const StringView & viewName) {
	_monitor = glfwGetPrimaryMonitor();
	if (nullptr == _monitor) {
		return false;
	}

	const GLFWvidmode* videoMode = glfwGetVideoMode(_monitor);

	return init(viewName, Rect(0, 0, videoMode->width, videoMode->height));
}

bool ViewImpl::init(const StringView & viewname, const GLFWvidmode &videoMode, GLFWmonitor *monitor) {
	_monitor = monitor;
	if (nullptr == _monitor) {
		return false;
	}

	glfwWindowHint(GLFW_REFRESH_RATE, videoMode.refreshRate);
	glfwWindowHint(GLFW_RED_BITS, videoMode.redBits);
	glfwWindowHint(GLFW_BLUE_BITS, videoMode.blueBits);
	glfwWindowHint(GLFW_GREEN_BITS, videoMode.greenBits);

	return init(viewname, Rect(0, 0, videoMode.width, videoMode.height));
}

bool ViewImpl::isVkReady() const {
	return nullptr != _mainWindow;
}

void ViewImpl::end() {
	if (_mainWindow) {
		glfwSetWindowShouldClose(_mainWindow, 1);
		_mainWindow = nullptr;
	}
	// Release self. Otherwise, VkViewImpl could not be freed.
	release();
}

void ViewImpl::swapBuffers() {
	if (_mainWindow) {
		glfwSwapBuffers(_mainWindow);
	}
}

void ViewImpl::pollEvents() {
	glfwPollEvents();
}

void ViewImpl::setIMEKeyboardState(bool open) {

}

bool ViewImpl::windowShouldClose() const {
	return glfwWindowShouldClose(_mainWindow);
}

double ViewImpl::getTimeCounter() const {
	return glfwGetTime();
}

void ViewImpl::setAnimationInterval(double val) {
	_animationInterval = val;
}

bool ViewImpl::run(Application *app, Rc<Director> dir, const Callback<bool(double)> &cb) {
	double nLast = glfwGetTime();
	auto frameCb = [&] () {
		const double nNow = glfwGetTime();
		if (!cb(nNow - nLast)) {
			return false;
		}
		nLast = nNow;
		return true;
	};

	bool ret = true;
	double dt = 0;

	_loop = Rc<PresentationLoop>::alloc(app, this, _device, dir, _animationInterval, [] () -> double {
		return glfwGetTime();
	}, frameCb);

	_loop->begin();

	while (!windowShouldClose()) {
		const auto iv = _animationInterval * 4.0;
		if (!frameCb()) {
			ret = false;
			break;
		}
		dt = glfwGetTime() - nLast;
		if (dt > 0 && dt < iv && _dropFrameDelay.test_and_set()) { // limit framerate
			 std::unique_lock<std::mutex> lock(_glSync);
			_glSyncVar.wait_for(lock, std::chrono::duration<double, std::ratio<1>>(iv - dt));
		}

		std::unique_lock<std::mutex> lock(_glSync);
		pollEvents();
	}
	_loop->end();

	_device->getTable()->vkDeviceWaitIdle(_device->getDevice());

	return ret;
}

void ViewImpl::updateFrameSize() {
	if (_screenSize.width > 0.0f && _screenSize.height > 0.0f) {
		// int frameBufferW = 0, frameBufferH = 0;
		// glfwGetFramebufferSize(_mainWindow, &frameBufferW, &frameBufferH);

		glfwSetWindowSize(_mainWindow, _screenSize.width * _density, _screenSize.height *_density);
	}
}

void ViewImpl::onGLFWWindowSizeFunCallback(GLFWwindow *window, int width, int height) {
	std::cout << "\tonGLFWWindowSizeFunCallback\n"; std::cout.flush();
	if (_loop) {
		if (!_loop->isStalled()) {
			_loop->reset();
		}

		bool changed = false;
		std::lock_guard<PresentationLoop> lock(*_loop);
		if (uint32_t(width) != _frameWidth) {
			_frameWidth = uint32_t(width); changed = true;
		}

		if (uint32_t(height) != _frameHeight) {
			_frameHeight = uint32_t(height); changed = true;
		}

		/*if (_loop->isStalled() || changed) {
			_device->recreateSwapChain(*_loop->getQueue());
			std::cout << "Recreate by call\n"; std::cout.flush();
			_loop->forceFrame(true);
			_loop->reset();
		}*/
	}
}

void ViewImpl::onGLFWMouseCallBack(GLFWwindow* window, int button, int action, int modify) {
	// std::cout << "\tonGLFWMouseCallBack\n";
}

void ViewImpl::onGLFWMouseMoveCallBack(GLFWwindow* window, double x, double y) {
	// std::cout << "\tonGLFWMouseMoveCallBack\n";
}

void ViewImpl::onGLFWMouseScrollCallback(GLFWwindow* window, double x, double y) {
	std::cout << "\tonGLFWMouseScrollCallback\n";
}


void ViewImpl::setScreenSize(float width, float height) {
	View::setScreenSize(width, height);
	updateFrameSize();
}

void ViewImpl::setClipboardString(const StringView &str) {
	glfwSetClipboardString(_mainWindow, str.str().data());
}

StringView ViewImpl::getClipboardString() const {
	auto str = glfwGetClipboardString(_mainWindow);
	if (str) {
		return StringView(str);
	} else {
		return StringView();
	}
}

void ViewImpl::selectPresentationOptions(Instance::PresentationOptions &targetOpts) const {
    for (const auto& availableFormat : targetOpts.formats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
        	targetOpts.formats.clear();
        	targetOpts.formats.emplace_back(VkSurfaceFormatKHR{VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR});
        	break;
        }
    }

	for (const auto& availablePresentMode : targetOpts.presentModes) {
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			targetOpts.presentModes.clear();
			targetOpts.presentModes.emplace_back(VK_PRESENT_MODE_MAILBOX_KHR);
        	break;
		}
	}

	if (targetOpts.capabilities.currentExtent.width == UINT32_MAX) {
		int width, height;
		glfwGetFramebufferSize(_mainWindow, &width, &height);

		VkExtent2D actualExtent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};

		actualExtent.width = std::max(targetOpts.capabilities.minImageExtent.width, std::min(targetOpts.capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(targetOpts.capabilities.minImageExtent.height, std::min(targetOpts.capabilities.maxImageExtent.height, actualExtent.height));

		targetOpts.capabilities.currentExtent = actualExtent;
	}

	targetOpts.capabilities.maxImageArrayLayers = 1;
}

}

#endif

/*void VkViewImpl::setViewPortInPoints(float x, float y, float w, float h) {
	glViewport((GLint)(x * _density + _viewPortRect.origin.x * _density),
			   (GLint)(y * _density + _viewPortRect.origin.y * _density),
			   (GLsizei)(w * _density),
			   (GLsizei)(h * _density));
}

void VkViewImpl::setScissorInPoints(float x , float y , float w , float h) {
	glScissor((GLint)(x * _scaleX * _retinaFactor * _frameZoomFactor + _viewPortRect.origin.x * _retinaFactor * _frameZoomFactor),
			   (GLint)(y * _scaleY * _retinaFactor  * _frameZoomFactor + _viewPortRect.origin.y * _retinaFactor * _frameZoomFactor),
			   (GLsizei)(w * _scaleX * _retinaFactor * _frameZoomFactor),
			   (GLsizei)(h * _scaleY * _retinaFactor * _frameZoomFactor));
}


void VkViewImpl::onGLFWMouseCallBack(GLFWwindow* window, int button, int action, int modify)
{
	if(GLFW_MOUSE_BUTTON_LEFT == button)
	{
		if(GLFW_PRESS == action)
		{
			_captured = true;
			if (this->getViewPortRect().equals(Rect::ZERO) || this->getViewPortRect().containsPoint(Vec2(_mouseX,_mouseY)))
			{
				intptr_t id = 0;
				this->handleTouchesBegin(1, &id, &_mouseX, &_mouseY);
			}
		}
		else if(GLFW_RELEASE == action)
		{
			if (_captured)
			{
				_captured = false;
				intptr_t id = 0;
				this->handleTouchesEnd(1, &id, &_mouseX, &_mouseY);
			}
		}

		if (_emulatePinch && _ctrlPressed && GLFW_PRESS == action) {
			float pinchX = _pinchX - (_mouseX - _pinchX);
			float pinchY = _pinchY - (_mouseY - _pinchY);
			if (this->getViewPortRect().equals(Rect::ZERO) || this->getViewPortRect().containsPoint(Vec2(pinchX,pinchY))) {
				_isInEmulation = true;
				intptr_t id = 1;
				this->handleTouchesBegin(1, &id, &pinchX, &pinchY);
			}
		}
		if (_isInEmulation && GLFW_RELEASE == action) {
			float pinchX = _pinchX - (_mouseX - _pinchX);
			float pinchY = _pinchY - (_mouseY - _pinchY);
			if (_isInEmulation) {
				_isInEmulation = false;
				intptr_t id = 1;
				this->handleTouchesEnd(1, &id, &pinchX, &pinchY);
			}
		}
	} else if (GLFW_MOUSE_BUTTON_RIGHT == button) {
		if (_emulatePinch) {
			_pinchX = _mouseX;
			_pinchY = _mouseY;
		}
	}

	//Because OpenGL and cocos2d-x uses different Y axis, we need to convert the coordinate here
	float cursorX = (_mouseX - _viewPortRect.origin.x) / _scaleX;
	float cursorY = (_viewPortRect.origin.y + _viewPortRect.size.height - _mouseY) / _scaleY;

	if(GLFW_PRESS == action)
	{
		EventMouse event(EventMouse::MouseEventType::MOUSE_DOWN);
		event.setCursorPosition(cursorX, cursorY);
		event.setMouseButton(button);
		Director::getInstance()->getEventDispatcher()->dispatchEvent(&event);
	}
	else if(GLFW_RELEASE == action)
	{
		EventMouse event(EventMouse::MouseEventType::MOUSE_UP);
		event.setCursorPosition(cursorX, cursorY);
		event.setMouseButton(button);
		Director::getInstance()->getEventDispatcher()->dispatchEvent(&event);
	}
}

void GLViewImpl::onGLFWMouseMoveCallBack(GLFWwindow* window, double x, double y)
{
	_mouseX = (float)x;
	_mouseY = (float)y;

	_mouseX /= this->getFrameZoomFactor();
	_mouseY /= this->getFrameZoomFactor();

	if (_isInRetinaMonitor)
	{
		if (_retinaFactor == 1)
		{
			_mouseX *= 2;
			_mouseY *= 2;
		}
	}

	if (_captured)
	{
		intptr_t id = 0;
		this->handleTouchesMove(1, &id, &_mouseX, &_mouseY);
	}

	if (_isInEmulation) {
		float pinchX = _pinchX - (_mouseX - _pinchX);
		float pinchY = _pinchY - (_mouseY - _pinchY);
		intptr_t id = 1;
		this->handleTouchesMove(1, &id, &pinchX, &pinchY);
	}

	//Because OpenGL and cocos2d-x uses different Y axis, we need to convert the coordinate here
	float cursorX = (_mouseX - _viewPortRect.origin.x) / _scaleX;
	float cursorY = (_viewPortRect.origin.y + _viewPortRect.size.height - _mouseY) / _scaleY;

	EventMouse event(EventMouse::MouseEventType::MOUSE_MOVE);
	// Set current button
	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
	{
		event.setMouseButton(GLFW_MOUSE_BUTTON_LEFT);
	}
	else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
	{
		event.setMouseButton(GLFW_MOUSE_BUTTON_RIGHT);
	}
	else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS)
	{
		event.setMouseButton(GLFW_MOUSE_BUTTON_MIDDLE);
	}
	event.setCursorPosition(cursorX, cursorY);
	Director::getInstance()->getEventDispatcher()->dispatchEvent(&event);
}

void GLViewImpl::onGLFWMouseScrollCallback(GLFWwindow* window, double x, double y)
{
	EventMouse event(EventMouse::MouseEventType::MOUSE_SCROLL);

	//Because OpenGL and cocos2d-x uses different Y axis, we need to convert the coordinate here
	float cursorX = (_mouseX - _viewPortRect.origin.x) / _scaleX;
	float cursorY = (_viewPortRect.origin.y + _viewPortRect.size.height - _mouseY) / _scaleY;
	event.setScrollData((float)x, -(float)y);
	event.setCursorPosition(cursorX, cursorY);
	Director::getInstance()->getEventDispatcher()->dispatchEvent(&event);
}

void GLViewImpl::onGLFWKeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	if (GLFW_REPEAT != action)
	{
		EventKeyboard event(g_keyCodeMap[key], GLFW_PRESS == action);
		auto dispatcher = Director::getInstance()->getEventDispatcher();
		dispatcher->dispatchEvent(&event);

		if (key == GLFW_KEY_LEFT_CONTROL) {
			_ctrlPressed = (GLFW_PRESS == action);
		}
		if (key == GLFW_KEY_LEFT_SHIFT) {
			_shiftPressed = (GLFW_PRESS == action);
		}

		auto d = stappler::platform::desktop::getDensity();

		if (_ctrlPressed && key == GLFW_KEY_F1 && GLFW_RELEASE == action) {
			if (_shiftPressed) {
				glfwSetWindowSize(window, 480 * d, 320 * d);
			} else {
				glfwSetWindowSize(window, 320 * d, 480 * d);
			}
		} else if (_ctrlPressed && key == GLFW_KEY_F2 && GLFW_RELEASE == action) {
			if (_shiftPressed) {
				glfwSetWindowSize(window, 568 * d, 320 * d);
			} else {
				glfwSetWindowSize(window, 320 * d, 568 * d);
			}
		} else if (_ctrlPressed && key == GLFW_KEY_F3 && GLFW_RELEASE == action) {
			if (_shiftPressed) {
				glfwSetWindowSize(window, 667 * d, 375 * d);
			} else {
				glfwSetWindowSize(window, 375 * d, 667 * d);
			}
		} else if (_ctrlPressed && key == GLFW_KEY_F4 && GLFW_RELEASE == action) {
			if (_shiftPressed) {
				glfwSetWindowSize(window, 736 * d, 414 * d);
			} else {
				glfwSetWindowSize(window, 414 * d, 736 * d);
			}
		} else if (_ctrlPressed && key == GLFW_KEY_F5 && GLFW_RELEASE == action) {
			if (_shiftPressed) {
				glfwSetWindowSize(window, 1024 * d, 768 * d);
			} else {
				glfwSetWindowSize(window, 768 * d, 1024 * d);
			}
		}
	}
	if (_imeEnabled) {
		if (GLFW_RELEASE != action && g_keyCodeMap[key] == EventKeyboard::KeyCode::KEY_BACKSPACE) {
			stappler::platform::ime::native::deleteBackward();
		}
		if (GLFW_RELEASE != action &&
				(g_keyCodeMap[key] == EventKeyboard::KeyCode::KEY_ENTER
				|| g_keyCodeMap[key] == EventKeyboard::KeyCode::KEY_KP_ENTER)) {
			stappler::platform::ime::native::insertText(u"\n");
		}
	}
}

void GLViewImpl::onGLFWCharCallback(GLFWwindow *window, unsigned int character)
{
	if (_imeEnabled) {
		char16_t wcharString[2] = { (char16_t) character, 0 };
		std::u16string utf16String(wcharString);
		stappler::platform::ime::native::insertText(utf16String);
	}
}

void GLViewImpl::onGLFWWindowPosCallback(GLFWwindow *windows, int x, int y)
{
	Director::getInstance()->setViewport();
}

void GLViewImpl::onGLFWframebuffersize(GLFWwindow* window, int w, int h)
{
	float frameSizeW = _screenSize.width;
	float frameSizeH = _screenSize.height;
	float factorX = frameSizeW / w * _retinaFactor * _frameZoomFactor;
	float factorY = frameSizeH / h * _retinaFactor * _frameZoomFactor;

	if (fabs(factorX - 0.5f) < FLT_EPSILON && fabs(factorY - 0.5f) < FLT_EPSILON )
	{
		_isInRetinaMonitor = true;
		if (_isRetinaEnabled)
		{
			_retinaFactor = 1;
		}
		else
		{
			_retinaFactor = 2;
		}

		glfwSetWindowSize(window, static_cast<int>(frameSizeW * 0.5f * _retinaFactor * _frameZoomFactor) , static_cast<int>(frameSizeH * 0.5f * _retinaFactor * _frameZoomFactor));
	}
	else if(fabs(factorX - 2.0f) < FLT_EPSILON && fabs(factorY - 2.0f) < FLT_EPSILON)
	{
		_isInRetinaMonitor = false;
		_retinaFactor = 1;
		glfwSetWindowSize(window, static_cast<int>(frameSizeW * _retinaFactor * _frameZoomFactor), static_cast<int>(frameSizeH * _retinaFactor * _frameZoomFactor));
	}
}

void GLViewImpl::onGLFWWindowSizeFunCallback(GLFWwindow *window, int width, int height)
{

	setDesignResolutionSize(width, height, ResolutionPolicy::SHOW_ALL);
	cocos2d::Director::getInstance()->setViewport();
	_screenSize = cocos2d::Size(width, height);
	_pinchX = _screenSize.width / 2;
	_pinchY = _screenSize.height / 2;
	stappler::platform::desktop::setScreenSize(_screenSize);
	stappler::Screen::getInstance()->setNextFrameSize(cocos2d::Size(width, height));
	//stappler::Screen::onOrientation( stappler::Screen::getInstance() );
	//stappler::Screen::getInstance()->initScreenResolution();
}

void GLViewImpl::onGLFWWindowIconifyCallback(GLFWwindow* window, int iconified)
{
	if (iconified == GL_TRUE)
	{
		Application::getInstance()->applicationDidEnterBackground();
	}
	else
	{
		Application::getInstance()->applicationWillEnterForeground();
	}
}

void GLViewImpl::onGLFWWindowFocusCallback(GLFWwindow* window, int focus) {
	if (focus == GL_TRUE) {
		Application::getInstance()->applicationFocusGained();
	} else {
		Application::getInstance()->applicationFocusLost();
	}
}

#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
static bool glew_dynamic_binding()
{
	const char *gl_extensions = (const char*)glGetString(GL_EXTENSIONS);

	// If the current opengl driver doesn't have framebuffers methods, check if an extension exists
	if (glGenFramebuffers == nullptr)
	{
		log("OpenGL: glGenFramebuffers is nullptr, try to detect an extension");
		if (strstr(gl_extensions, "ARB_framebuffer_object"))
		{
			log("OpenGL: ARB_framebuffer_object is supported");

			glIsRenderbuffer = (PFNGLISRENDERBUFFERPROC) wglGetProcAddress("glIsRenderbuffer");
			glBindRenderbuffer = (PFNGLBINDRENDERBUFFERPROC) wglGetProcAddress("glBindRenderbuffer");
			glDeleteRenderbuffers = (PFNGLDELETERENDERBUFFERSPROC) wglGetProcAddress("glDeleteRenderbuffers");
			glGenRenderbuffers = (PFNGLGENRENDERBUFFERSPROC) wglGetProcAddress("glGenRenderbuffers");
			glRenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEPROC) wglGetProcAddress("glRenderbufferStorage");
			glGetRenderbufferParameteriv = (PFNGLGETRENDERBUFFERPARAMETERIVPROC) wglGetProcAddress("glGetRenderbufferParameteriv");
			glIsFramebuffer = (PFNGLISFRAMEBUFFERPROC) wglGetProcAddress("glIsFramebuffer");
			glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC) wglGetProcAddress("glBindFramebuffer");
			glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC) wglGetProcAddress("glDeleteFramebuffers");
			glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC) wglGetProcAddress("glGenFramebuffers");
			glCheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSPROC) wglGetProcAddress("glCheckFramebufferStatus");
			glFramebufferTexture1D = (PFNGLFRAMEBUFFERTEXTURE1DPROC) wglGetProcAddress("glFramebufferTexture1D");
			glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC) wglGetProcAddress("glFramebufferTexture2D");
			glFramebufferTexture3D = (PFNGLFRAMEBUFFERTEXTURE3DPROC) wglGetProcAddress("glFramebufferTexture3D");
			glFramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFERPROC) wglGetProcAddress("glFramebufferRenderbuffer");
			glGetFramebufferAttachmentParameteriv = (PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC) wglGetProcAddress("glGetFramebufferAttachmentParameteriv");
			glGenerateMipmap = (PFNGLGENERATEMIPMAPPROC) wglGetProcAddress("glGenerateMipmap");
		}
		else
		if (strstr(gl_extensions, "EXT_framebuffer_object"))
		{
			log("OpenGL: EXT_framebuffer_object is supported");
			glIsRenderbuffer = (PFNGLISRENDERBUFFERPROC) wglGetProcAddress("glIsRenderbufferEXT");
			glBindRenderbuffer = (PFNGLBINDRENDERBUFFERPROC) wglGetProcAddress("glBindRenderbufferEXT");
			glDeleteRenderbuffers = (PFNGLDELETERENDERBUFFERSPROC) wglGetProcAddress("glDeleteRenderbuffersEXT");
			glGenRenderbuffers = (PFNGLGENRENDERBUFFERSPROC) wglGetProcAddress("glGenRenderbuffersEXT");
			glRenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEPROC) wglGetProcAddress("glRenderbufferStorageEXT");
			glGetRenderbufferParameteriv = (PFNGLGETRENDERBUFFERPARAMETERIVPROC) wglGetProcAddress("glGetRenderbufferParameterivEXT");
			glIsFramebuffer = (PFNGLISFRAMEBUFFERPROC) wglGetProcAddress("glIsFramebufferEXT");
			glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC) wglGetProcAddress("glBindFramebufferEXT");
			glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC) wglGetProcAddress("glDeleteFramebuffersEXT");
			glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC) wglGetProcAddress("glGenFramebuffersEXT");
			glCheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSPROC) wglGetProcAddress("glCheckFramebufferStatusEXT");
			glFramebufferTexture1D = (PFNGLFRAMEBUFFERTEXTURE1DPROC) wglGetProcAddress("glFramebufferTexture1DEXT");
			glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC) wglGetProcAddress("glFramebufferTexture2DEXT");
			glFramebufferTexture3D = (PFNGLFRAMEBUFFERTEXTURE3DPROC) wglGetProcAddress("glFramebufferTexture3DEXT");
			glFramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFERPROC) wglGetProcAddress("glFramebufferRenderbufferEXT");
			glGetFramebufferAttachmentParameteriv = (PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC) wglGetProcAddress("glGetFramebufferAttachmentParameterivEXT");
			glGenerateMipmap = (PFNGLGENERATEMIPMAPPROC) wglGetProcAddress("glGenerateMipmapEXT");
		}
		else
		{
			log("OpenGL: No framebuffers extension is supported");
			log("OpenGL: Any call to Fbo will crash!");
			return false;
		}
	}
	return true;
}
#endif*/
