/**
Copyright (c) 2021 Roman Katuntsev <sbkarr@stappler.org>

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

#include "XLVkViewImpl-linux.h"

#if LINUX

#include <unistd.h>
#include <fcntl.h>
#include <xcb/xcb.h>
#include <sys/eventfd.h>
#include <sys/epoll.h>

namespace stappler::xenolith::vk {

using VkXcbSurfaceCreateFlagsKHR = VkFlags;

struct VkXcbSurfaceCreateInfoKHR {
	VkStructureType               sType;
	const void*                   pNext;
	VkXcbSurfaceCreateFlagsKHR    flags;
	xcb_connection_t*             connection;
	xcb_window_t                  window;
};

using PFN_vkGetPhysicalDeviceXcbPresentationSupportKHR = VkBool32 (*) (VkPhysicalDevice physicalDevice,
		uint32_t queueFamilyIndex, xcb_connection_t *connection, xcb_visualid_t visual_id);

using PFN_vkCreateXcbSurfaceKHR = VkResult (*) (VkInstance instance,
		const VkXcbSurfaceCreateInfoKHR *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface);

struct XcbAtomRequest {
	StringView name;
	bool onlyIfExists;
};

static XcbAtomRequest s_atomRequests[] = {
	{ "WM_PROTOCOLS", true },
	{ "WM_DELETE_WINDOW", false },
};

class XcbView : public LinuxViewInterface {
public:
	static void ReportError(int error);

	XcbView(Rc<Instance>, ViewImpl *, StringView, URect);
	virtual ~XcbView();

	bool valid() const;

	Vector<Pair<VkPhysicalDevice, uint32_t>> getAvailableDevices(const Vector<VkPhysicalDevice> &);

	VkSurfaceKHR createWindowSurface();

	virtual bool run(Rc<PresentationLoop> loop, const Callback<bool(uint64_t)> &cb) override;

	bool pollForEvents();

	virtual void onEventPushed() override;

protected:
	Rc<Instance> _instance;
	ViewImpl *_view = nullptr;
	Rc<PresentationLoop> _loop;
	xcb_connection_t *_connection = nullptr;
	xcb_screen_t *_defaultScreen = nullptr;
	uint32_t _window = 0;

	xcb_atom_t _atoms[sizeof(s_atomRequests) / sizeof(XcbAtomRequest)];

	uint16_t _width = 0;
	uint16_t _height = 0;

	int _eventFd = -1;
	int _socket = -1;
};

void XcbView::ReportError(int error) {
	switch (error) {
	case XCB_CONN_ERROR:
		stappler::log::text("XcbView", "XCB_CONN_ERROR: socket error, pipe error or other stream error");
		break;
	case XCB_CONN_CLOSED_EXT_NOTSUPPORTED:
		stappler::log::text("XcbView", "XCB_CONN_CLOSED_EXT_NOTSUPPORTED: extension is not supported");
		break;
	case XCB_CONN_CLOSED_MEM_INSUFFICIENT:
		stappler::log::text("XcbView", "XCB_CONN_CLOSED_MEM_INSUFFICIENT: out of memory");
		break;
	case XCB_CONN_CLOSED_REQ_LEN_EXCEED:
		stappler::log::text("XcbView", "XCB_CONN_CLOSED_REQ_LEN_EXCEED: too large request");
		break;
	case XCB_CONN_CLOSED_PARSE_ERR:
		stappler::log::text("XcbView", "XCB_CONN_CLOSED_PARSE_ERR: error during parsing display string");
		break;
	case XCB_CONN_CLOSED_INVALID_SCREEN:
		stappler::log::text("XcbView", "XCB_CONN_CLOSED_INVALID_SCREEN: server does not have a screen matching the display");
		break;
	case XCB_CONN_CLOSED_FDPASSING_FAILED:
		stappler::log::text("XcbView", "XCB_CONN_CLOSED_FDPASSING_FAILED: fail to pass some FD");
		break;
	}
}

XcbView::XcbView(Rc<Instance> inst, ViewImpl *v, StringView, URect rect) {
	_instance = inst;
	_view = v;
	int screen_nbr;
	if constexpr (s_printVkInfo) {
		auto d = getenv("DISPLAY");
		if (!d) {
			stappler::log::vtext("XcbView-Info", "DISPLAY is not defined");
		} else {
			// stappler::log::vtext("XcbView-Info", "Using DISPLAY: '", d, "'");
		}
	}
	_connection = xcb_connect (NULL, &screen_nbr); // always not null
	_socket = xcb_get_file_descriptor(_connection); // assume it's non-blocking
	_eventFd = eventfd(0, EFD_NONBLOCK);

	auto err = xcb_connection_has_error(_connection);
	if (err != 0) {
		ReportError(err);
		return;
	}

	auto setup = xcb_get_setup(_connection);
	auto iter = xcb_setup_roots_iterator(setup);
	for (; iter.rem; --screen_nbr, xcb_screen_next(&iter)) {
		if (screen_nbr == 0) {
			_defaultScreen = iter.data;
			break;
		}
	}

	uint32_t mask = /* XCB_CW_BACK_PIXEL | */ XCB_CW_EVENT_MASK;
	uint32_t values[1];
	//values[0] = _defaultScreen->black_pixel;
	values[0] = XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_STRUCTURE_NOTIFY;
		/*XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS |	XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_POINTER_MOTION
			| XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW | XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE
			| XCB_EVENT_MASK_VISIBILITY_CHANGE | XCB_EVENT_MASK_RESIZE_REDIRECT | XCB_EVENT_MASK_FOCUS_CHANGE
			| XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_PROPERTY_CHANGE |
			| XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY
						| XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT
			| XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_RESIZE_REDIRECT  | XCB_EVENT_MASK_FOCUS_CHANGE | XCB_EVENT_MASK_PROPERTY_CHANGE
			| XCB_EVENT_MASK_COLOR_MAP_CHANGE | XCB_EVENT_MASK_OWNER_GRAB_BUTTON */;

	/* Ask for our window's Id */
	_window = xcb_generate_id(_connection);

	_width = rect.width;
	_height = rect.height;

	xcb_create_window(_connection,
		XCB_COPY_FROM_PARENT, // depth (same as root)
		_window, // window Id
		_defaultScreen->root, // parent window
		rect.x, rect.y, rect.width, rect.height,
		0, // border_width
		XCB_WINDOW_CLASS_INPUT_OUTPUT, // class
		_defaultScreen->root_visual, // visual
		mask, values);

	xcb_map_window (_connection, _window);

    xcb_intern_atom_cookie_t atomCookies[sizeof(s_atomRequests) / sizeof(XcbAtomRequest)];

    size_t i = 0;
	for (auto &it : s_atomRequests) {
		atomCookies[i] = xcb_intern_atom( _connection, it.onlyIfExists ? 1 : 0, it.name.size(), it.name.data() );
		++ i;
	}

	xcb_flush(_connection);

    i = 0;
	for (auto &it : atomCookies) {
		auto reply = xcb_intern_atom_reply( _connection, it, nullptr );
		if (reply) {
			_atoms[i] = reply->atom;
			free(reply);
		} else {
			_atoms[i] = 0;
		}
		++ i;
	}

	xcb_change_property( _connection, XCB_PROP_MODE_REPLACE, _window, _atoms[0], 4, 32, 1, &_atoms[1] );
}

XcbView::~XcbView() {
	if (_eventFd) {
		close(_eventFd);
	}
	_defaultScreen = nullptr;
	if (_connection) {
		xcb_disconnect(_connection);
		_connection = nullptr;
	}
	_instance = nullptr;
}

bool XcbView::valid() const {
	return xcb_connection_has_error(_connection) == 0;
}

Vector<Pair<VkPhysicalDevice, uint32_t>> XcbView::getAvailableDevices(const Vector<VkPhysicalDevice> &devs) {
	auto fn = (PFN_vkGetPhysicalDeviceXcbPresentationSupportKHR)_instance->vkGetInstanceProcAddr(
			_instance->getInstance(), "vkGetPhysicalDeviceXcbPresentationSupportKHR");
	if (!fn) {
		stappler::log::text("Vk", "vkGetPhysicalDeviceXcbPresentationSupportKHR not found");
		return Vector<Pair<VkPhysicalDevice, uint32_t>>();
	}

	Vector<Pair<VkPhysicalDevice, uint32_t>> ret;
	for (auto &it : devs) {
		uint32_t value = 0;
		uint32_t queueFamilyCount = 0;
		_instance->vkGetPhysicalDeviceQueueFamilyProperties(it, &queueFamilyCount, nullptr);

		for (uint32_t i = 0; i < queueFamilyCount; ++ i) {
			if (fn(it, i, _connection, _defaultScreen->root_visual)) {
				value |= (1 << i);
			}
		}

		if (value) {
			ret.emplace_back(it, value);
		}
	}
	return ret;
}

VkSurfaceKHR XcbView::createWindowSurface() {
	auto fn = (PFN_vkCreateXcbSurfaceKHR)_instance->vkGetInstanceProcAddr(
			_instance->getInstance(), "vkCreateXcbSurfaceKHR");
	if (!fn) {
		stappler::log::text("Vk", "vkCreateXcbSurfaceKHR not found");
		return nullptr;
	}

	VkSurfaceKHR surface;
	VkXcbSurfaceCreateInfoKHR createInfo{VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR, nullptr, 0, _connection, _window};
	if (fn(_instance->getInstance(), &createInfo, nullptr, &surface) != VK_SUCCESS) {
		return nullptr;
	}
	return surface;
}

bool XcbView::run(Rc<PresentationLoop> loop, const Callback<bool(uint64_t)> &cb) {
	_loop = loop;
	bool ret = true;
	auto nLast = platform::device::_clock();
	auto frameCb = [&] () {
		const auto nNow = platform::device::_clock();
		if (!cb(nNow - nLast)) {
			return false;
		}
		nLast = nNow;
		return true;
	};

    struct epoll_event eventEvent;
    eventEvent.data.fd = _eventFd;
    eventEvent.events = EPOLLIN | EPOLLET;

    struct epoll_event socketEvent;
    socketEvent.data.fd = _socket;
    socketEvent.events = EPOLLIN;

	int epollFd = epoll_create1(0);
	auto err = epoll_ctl(epollFd, EPOLL_CTL_ADD, _eventFd, &eventEvent);
	if (err == -1) {
		char buf[256] = { 0 };
		stappler::log::vtext("XcbView", "Failed to start thread worker with socket epoll_ctl(",
				_eventFd, ", EPOLL_CTL_ADD): ", strerror_r(errno, buf, 255), "\n");
	}

	err = epoll_ctl(epollFd, EPOLL_CTL_ADD, _socket, &socketEvent);
	if (err == -1) {
		char buf[256] = { 0 };
		stappler::log::vtext("XcbView", "Failed to start thread worker with pipe epoll_ctl(",
				_socket, ", EPOLL_CTL_ADD): ", strerror_r(errno, buf, 255), "\n");
	}

	do {
		static constexpr auto MaxEvents = 8;

		bool _shouldClose = false;
		std::array<struct epoll_event, MaxEvents> _events;

		while (!_shouldClose) {
			int nevents = epoll_wait(epollFd, _events.data(), MaxEvents, -1);
			if (nevents == -1 && errno != EINTR) {
				char buf[256] = { 0 };
				stappler::log::vtext("XcbView", "epoll_wait() failed with errno ", errno, " (", strerror_r(errno, buf, 255), ")");
				return false;
			} else if (nevents <= 0 && errno == EINTR) {
				continue;
			}

			for (int i = 0; i < nevents; i++) {
				auto fd = _events[i].data.fd;
				if ((_events[i].events & EPOLLERR)) {
					stappler::log::vtext("XcbView", "epoll error on socket ", fd);
					continue;
				}
				if ((_events[i].events & EPOLLIN)) {
					if (fd == _eventFd) {
						uint64_t value = 0;
						auto sz = read(_eventFd, &value, sizeof(uint64_t));
						if (sz == 8 && value) {
							// received app event
							auto evVal = _view->popEvents();
							if ((evVal & ViewEvent::Terminate) != 0) {
								_shouldClose = true;
								continue;
							}
							if ((evVal & ViewEvent::SwapchainRecreationBest) != 0) {
								_view->getDevice()->recreateSwapChain(false);
								_loop->reset();
							} else if ((evVal & ViewEvent::SwapchainRecreation) != 0) {
								_view->getDevice()->recreateSwapChain(true);
								_loop->reset();
							}
							if ((evVal & ViewEvent::Update) != 0) {
								frameCb();
							}
						}
					} else if (fd == _socket) {
						if (!pollForEvents()) {
							_shouldClose = true;
						}
					}
				}
			}
		}

		ret = !_shouldClose;
	} while (0);

	_loop = nullptr;
	return ret;
}

void print_modifiers (uint32_t mask) {
	const char **mod, *mods[] = { "Shift", "Lock", "Ctrl", "Alt", "Mod2", "Mod3",
			"Mod4", "Mod5", "Button1", "Button2", "Button3", "Button4", "Button5" };
	printf("Modifier mask: ");
	for (mod = mods; mask; mask >>= 1, mod++) {
		if (mask & 1) {
			printf("%s", *mod);
		}
	}
	putchar('\n');
}

bool XcbView::pollForEvents() {
	xcb_generic_event_t *e;
	while ((e = xcb_poll_for_event(_connection))) {
		auto et = e->response_type & 0x7f;
		switch (et) {
		case XCB_EXPOSE: {
			xcb_expose_event_t *ev = (xcb_expose_event_t*) e;
			printf("XCB_EXPOSE: Window %d exposed. Region to be redrawn at location (%d,%d), with dimension (%d,%d)\n",
					ev->window, ev->x, ev->y, ev->width, ev->height);
			break;
		}
		case XCB_BUTTON_PRESS: {
			xcb_button_press_event_t *ev = (xcb_button_press_event_t*) e;
			print_modifiers(ev->state);

			switch (ev->detail) {
			case 4:
				printf("Wheel Button up in window %d, at coordinates (%d,%d)\n", ev->event, ev->event_x, ev->event_y);
				break;
			case 5:
				printf("Wheel Button down in window %d, at coordinates (%d,%d)\n", ev->event, ev->event_x, ev->event_y);
				break;
			default:
				printf("Button %d pressed in window %d, at coordinates (%d,%d)\n", ev->detail, ev->event, ev->event_x, ev->event_y);
			}
			break;
		}
		case XCB_BUTTON_RELEASE: {
			xcb_button_release_event_t *ev = (xcb_button_release_event_t*) e;
			print_modifiers(ev->state);
			printf("Button %d released in window %d, at coordinates (%d,%d)\n", ev->detail, ev->event, ev->event_x, ev->event_y);
			break;
		}
		case XCB_MOTION_NOTIFY: {
			// xcb_motion_notify_event_t *ev = (xcb_motion_notify_event_t*) e;
			//printf("Mouse moved in window %ld, at coordinates (%d,%d)\n", ev->event, ev->event_x, ev->event_y);
			break;
		}
		case XCB_ENTER_NOTIFY: {
			xcb_enter_notify_event_t *ev = (xcb_enter_notify_event_t*) e;
			printf("Mouse entered window %d, at coordinates (%d,%d)\n", ev->event, ev->event_x, ev->event_y);
			break;
		}
		case XCB_LEAVE_NOTIFY: {
			xcb_leave_notify_event_t *ev = (xcb_leave_notify_event_t*) e;
			printf("Mouse left window %d, at coordinates (%d,%d)\n", ev->event, ev->event_x, ev->event_y);
			break;
		}
		case XCB_FOCUS_IN: {
			xcb_focus_in_event_t *ev = (xcb_focus_in_event_t*) e;
			printf("XCB_FOCUS_IN: %d\n", ev->event);
			break;
		}
		case XCB_FOCUS_OUT: {
			xcb_focus_in_event_t *ev = (xcb_focus_in_event_t*) e;
			printf("XCB_FOCUS_OUT: %d\n", ev->event);
			break;
		}
		case XCB_KEY_PRESS: {
			xcb_key_press_event_t *ev = (xcb_key_press_event_t*) e;
			print_modifiers(ev->state);
			printf("Key pressed in window %d\n", ev->event);
			break;
		}
		case XCB_KEY_RELEASE: {
			xcb_key_release_event_t *ev = (xcb_key_release_event_t*) e;
			print_modifiers(ev->state);
			printf("Key released in window %d\n", ev->event);
			break;
		}
		case XCB_VISIBILITY_NOTIFY: {
			xcb_visibility_notify_event_t *ev = (xcb_visibility_notify_event_t*) e;
			print_modifiers(ev->state);
			printf("XCB_VISIBILITY_NOTIFY: %d\n", ev->window);
			break;
		}
		case XCB_MAP_NOTIFY : {
			xcb_map_notify_event_t *ev = (xcb_map_notify_event_t*) e;
			printf("XCB_MAP_NOTIFY: %d\n", ev->event);
			break;
		}
		case XCB_REPARENT_NOTIFY : {
			xcb_reparent_notify_event_t *ev = (xcb_reparent_notify_event_t*) e;
			printf("XCB_REPARENT_NOTIFY: %d %d to %d\n", ev->event, ev->window, ev->parent);
			break;
		}
		case XCB_CONFIGURE_NOTIFY : {
			xcb_configure_notify_event_t *ev = (xcb_configure_notify_event_t*) e;
			printf("XCB_CONFIGURE_NOTIFY: %d (%d) rect:%d,%d,%d,%d border:%d override:%d\n", ev->event, ev->window,
					ev->x, ev->y, ev->width, ev->height, uint32_t(ev->border_width), uint32_t(ev->override_redirect));
			if (ev->width != _width || ev->height != _height) {
				_width = ev->width; _height = ev->height;
				_loop->recreateSwapChain();
			}
			break;
		}
		case XCB_RESIZE_REQUEST: {
			xcb_resize_request_event_t *ev = (xcb_resize_request_event_t*) e;
			printf("XCB_RESIZE_REQUEST: %d width:%d height:%d\n", ev->window, ev->width, ev->height);
			if (ev->width != _width || ev->height != _height) {
				_width = ev->width; _height = ev->height;
				_loop->recreateSwapChain();
			}
			break;
		}
        case XCB_CLIENT_MESSAGE: {
        	xcb_client_message_event_t *ev = (xcb_client_message_event_t*) e;
			printf("XCB_CLIENT_MESSAGE: %d of type %d\n", ev->window, ev->type);
			if (ev->type == _atoms[0] && ev->data.data32[0] == _atoms[1]) {
				free(e);
				return false;
			}
			break;
        }
		default:
			/* Unknown event type, ignore it */
			printf("Unknown event: %d\n", et);
			break;
		}
		/* Free the Generic Event */
		free(e);
	}
	return true;
}

void XcbView::onEventPushed() {
	uint64_t value = 1;
	write(_eventFd, &value, sizeof(uint64_t));
}

ViewImpl::ViewImpl() { }

ViewImpl::~ViewImpl() {
	_device = nullptr;
	if (_instance) {
		_instance->vkDestroySurfaceKHR(_instance->getInstance(), _surface, nullptr);
	}
	_view = nullptr;
}

bool ViewImpl::init(Rc<Instance> instance, StringView viewName, URect rect) {
	auto v = Rc<XcbView>::alloc(instance, this, viewName, rect);

	uint32_t deviceCount = 0;
	instance->vkEnumeratePhysicalDevices(instance->getInstance(), &deviceCount, nullptr);

	if (deviceCount == 0) {
		log::text("Vk", "failed to find GPUs with Vulkan support!");
		return false;
	}

	Vector<VkPhysicalDevice> devices(deviceCount);
	instance->vkEnumeratePhysicalDevices(instance->getInstance(), &deviceCount, devices.data());

	// find devices, compatible with window
	auto devs = v->getAvailableDevices(devices);
	if (devs.empty()) {
		log::text("Vk", "failed to find GPUs with Vulkan support on X11 server!");
		return false;
	}

	_surface = v->createWindowSurface();
	if (!_surface) {
		log::text("VkView", "Fail to create Vulkan surface for window");
		return false;
	}

	if constexpr (s_printVkInfo) {
		Application::getInstance()->perform([instance, surface = _surface] (const Task &) {
			instance->printDevicesInfo(surface);
			return true;
		}, nullptr, this);
	}

	auto opts = instance->getPresentationOptions(_surface, nullptr, devs);
	if (opts.empty()) {
		log::text("VkView", "No available Vulkan devices for presentation on surface");
		return false;
	}

	_frameWidth = uint32_t(rect.width);
	_frameHeight = uint32_t(rect.height);

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

		_view.set(v);
		return View::init(instance, device);
	}

	instance->vkDestroySurfaceKHR(instance->getInstance(), _surface, nullptr);
	log::text("VkView", "Unable to create device, not all required features is supported");
	return false;
}

bool ViewImpl::init(Rc<Instance> instance, StringView viewName) {
	/*_monitor = glfwGetPrimaryMonitor();
	if (nullptr == _monitor) {
		return false;
	}

	const GLFWvidmode* videoMode = glfwGetVideoMode(_monitor);

	return init(viewName, Rect(0, 0, videoMode->width, videoMode->height));*/
	return false;
}

void ViewImpl::end() {
	// Release self. Otherwise, VkViewImpl could not be freed.
	release();
}

void ViewImpl::setIMEKeyboardState(bool open) {

}

bool ViewImpl::run(Application *app, Rc<Director> dir, const Callback<bool(uint64_t)> &cb) {
	if (!_device) {
		return false;
	}

	_device->createSwapChain(_surface);

	bool ret = true;
	_loop = Rc<PresentationLoop>::alloc(app, this, _device, dir, _frameTimeMicroseconds);
	_loop->begin();

	ret = _view->run(_loop, cb);

	_loop->end();
	_device->getTable()->vkDeviceWaitIdle(_device->getDevice());
	_loop = nullptr;
	return ret;
}

void ViewImpl::setScreenSize(float width, float height) {
	View::setScreenSize(width, height);
}

void ViewImpl::setClipboardString(StringView str) {

}

StringView ViewImpl::getClipboardString() const {
	return StringView();
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
		/*int width, height;
		glfwGetFramebufferSize(_mainWindow, &width, &height);

		VkExtent2D actualExtent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};

		actualExtent.width = std::max(targetOpts.capabilities.minImageExtent.width, std::min(targetOpts.capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(targetOpts.capabilities.minImageExtent.height, std::min(targetOpts.capabilities.maxImageExtent.height, actualExtent.height));

		targetOpts.capabilities.currentExtent = actualExtent;*/
	}

	targetOpts.capabilities.maxImageArrayLayers = 1;
}

void ViewImpl::pushEvent(ViewEvent::Value val) {
	View::pushEvent(val);
	if (_view) {
		_view->onEventPushed();
	}
}

}

#endif
