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

#include "XLApplication.h"
#include "XLPlatform.h"
#include "XLDirector.h"
#include "XLEvent.h"
#include "XLEventHandler.h"

namespace stappler::xenolith {

XL_DECLARE_EVENT_CLASS(Application, onDeviceToken);
XL_DECLARE_EVENT_CLASS(Application, onNetwork);
XL_DECLARE_EVENT_CLASS(Application, onUrlOpened);
XL_DECLARE_EVENT_CLASS(Application, onError);
XL_DECLARE_EVENT_CLASS(Application, onRemoteNotification);
XL_DECLARE_EVENT_CLASS(Application, onRegenerateResources);
XL_DECLARE_EVENT_CLASS(Application, onRegenerateTextures);
XL_DECLARE_EVENT_CLASS(Application, onBackKey);
XL_DECLARE_EVENT_CLASS(Application, onRateApplication);
XL_DECLARE_EVENT_CLASS(Application, onScreenshot);
XL_DECLARE_EVENT_CLASS(Application, onLaunchUrl);

static Application * s_application = nullptr;

Application *Application::getInstance() {
	return s_application;
}

void Application::sleep(double v) {
	Sleep(uint32_t(ceilf(v * 1000)));
}

Application::Application() {
	XLASSERT(s_application == nullptr, "Application should be only one");

	_userAgent = platform::device::_userAgent();
	_deviceIdentifier  = platform::device::_deviceIdentifier();
	_bundleName = platform::device::_bundleName();
	_applicationName = platform::device::_applicationName();
	_applicationVersion = platform::device::_applicationVersion();
	_userLanguage = platform::device::_userLanguage();
	_isNetworkOnline = platform::network::_isNetworkOnline();

	platform::network::_setNetworkCallback([this] (bool online) {
		if (online != _isNetworkOnline) {
			setNetworkOnline(online);
		}
	});

	s_application = this;
}

Application::~Application() { }

bool Application::applicationDidFinishLaunching() {
	_queue = Rc<thread::TaskQueue>::alloc(2);
	if (!_queue->spawnWorkers()) {
		log::text("Application", "Fail to spawn worker threads");
		return false;
	}

	_director = Rc<Director>::create();
	auto view = _director->getView();
	if (!view) {
		if (auto v = Rc<vk::ViewImpl>::create(getBundleName(), Rect(Vec2(0.0f, 0.0f), platform::desktop::getScreenSize()))) {
			_mainView = v.get();
			_director->setView(v);
		}
	}

	return true;
}

void Application::applicationDidReceiveMemoryWarning() {

}

int Application::run(Director *director) {
	if (!applicationDidFinishLaunching()) {
		return 1;
	}

	if (!director) {
		director = _director;
	}

	Rc<vk::View> glview = director->getView();
	if (!glview) {
		return -1;
	}

	glview->run(director, [&] (double val) -> bool {
		update(val);
		director->mainLoop(val);
		return true;
	});

	// Director should still do a cleanup if the window was closed manually.
	if (glview->isVkReady()) {
		director->end();
		director = nullptr;
	}
    return 0;
}

bool Application::openURL(const StringView &url) {
	return platform::interaction::_goToUrl(url, true);
}

void Application::update(double dt) {
	auto id = std::this_thread::get_id();
	if (id != _threadId) {
		thread::ThreadInfo::setMainThread();
		_threadId = id;
	}

	if (_queue) {
		_queue->update();
	}

    if (!_isNetworkOnline) {
        _updateTimer += dt;
        if (_updateTimer >= 10) {
            _updateTimer = -10;
			setNetworkOnline(platform::network::_isNetworkOnline());
        }
    }

    if (_deviceIdentifier.empty()) {
        _deviceIdentifier = platform::device::_deviceIdentifier();
    }
}

void Application::registerDeviceToken(const uint8_t *data, size_t len) {
    registerDeviceToken(base16::encode(CoderSource(data, len)));
}

void Application::registerDeviceToken(const String &data) {
	_deviceToken = data;
	if (!_deviceToken.empty()) {
		onDeviceToken(this, _deviceToken);
	}
}

void Application::setNetworkOnline(bool online) {
	if (_isNetworkOnline != online) {
		_isNetworkOnline = online;
		onNetwork(this, _isNetworkOnline);
		if (!_isNetworkOnline) {
			_updateTimer = 0;
		}
	}
}

bool Application::isNetworkOnline() {
	return _isNetworkOnline;
}

void Application::goToUrl(const StringView &url, bool external) {
	onUrlOpened(this, url);
	platform::interaction::_goToUrl(url, external);
}
void Application::makePhoneCall(const StringView &number) {
	onUrlOpened(this, number);
	platform::interaction::_makePhoneCall(number);
}
void Application::mailTo(const StringView &address) {
	onUrlOpened(this, address);
	platform::interaction::_mailTo(address);
}
void Application::rateApplication() {
	onRateApplication(this);
	platform::interaction::_rateApplication();
}

std::pair<uint64_t, uint64_t> Application::getTotalDiskSpace() {
	return platform::device::_diskSpace();
}
uint64_t Application::getApplicationDiskSpace() {
	auto path = filesystem::writablePath(getBundleName());
	uint64_t size = 0;
	filesystem::ftw(path, [&size] (const StringView &path, bool isFile) {
		if (isFile) {
			size += filesystem::size(path);
		}
	});

	path = filesystem::cachesPath(getBundleName());
	filesystem::ftw(path, [&size] (const StringView &path, bool isFile) {
		if (isFile) {
			size += filesystem::size(path);
		}
	});

	return size;
}

int64_t Application::getApplicationVersionCode() {
	static int64_t version = 0;
	if (version == 0) {
		String str(_applicationVersion);
		int major = 0, middle = 0, minor = 0, state = 0;

		for (char c : str) {
			if (c == '.') {
				state++;
			} else if (c >= '0' && c <= '9') {
				if (state == 0) {
					major = major * 10 + (c - '0');
				} else if (state == 1) {
					middle = middle * 10 + (c - '0');
				} else if (state == 2) {
					minor = minor * 10 + (c - '0');
				} else {
					break;
				}
			} else {
				break;
			}
		}
		version = major * 1000000 + middle * 1000 + minor;
	}
	return version;
}

void Application::notification(const String &title, const String &text) {
	platform::interaction::_notification(title, text);
}

void Application::setLaunchUrl(const StringView &url) {
    _launchUrl = url.str();
}

void Application::processLaunchUrl(const StringView &url) {
    _launchUrl = url.str();
    onLaunchUrl(this, url);
}

StringView Application::getLaunchUrl() const {
    return _launchUrl;
}


bool Application::isMainThread() const {
	return _threadId == std::this_thread::get_id();
}

void Application::performOnMainThread(const Callback &func, Ref *target, bool onNextFrame) {
	if ((isMainThread() || _singleThreaded) && !onNextFrame) {
		func();
	} else {
		_queue->onMainThread(Rc<thread::Task>::create([func] (const thread::Task &, bool success) {
			if (success) { func(); }
		}));
	}
}

void Application::performOnMainThread(Rc<thread::Task> &&task, bool onNextFrame) {
	if ((isMainThread() || _singleThreaded) && !onNextFrame) {
		task->onComplete();
	} else {
		_queue->onMainThread(std::move(task));
	}
}

void Application::perform(const ExecuteCallback &exec, const CompleteCallback &complete, Ref *obj) {
	perform(Rc<Task>::create(exec, complete, obj));
}

void Application::perform(Rc<thread::Task> &&task) {
	if (_singleThreaded) {
		task->setSuccessful(task->execute());
		task->onComplete();
	} else {
		_queue->perform(std::move(task));
	}
}

void Application::perform(Rc<thread::Task> &&task, bool performFirst) {
	if (_singleThreaded) {
		task->setSuccessful(task->execute());
		task->onComplete();
	} else {
		_queue->performWithPriority(std::move(task), performFirst);
	}
}

void Application::performAsync(Rc<Task> &&task) {
	if (_singleThreaded) {
		task->setSuccessful(task->execute());
		task->onComplete();
	} else {
		_queue->performAsync(std::move(task));
	}
}

void Application::setSingleThreaded(bool value) {
	_singleThreaded = value;
}

bool Application::isSingleThreaded() const {
	return _singleThreaded;
}

void Application::addEventListener(const EventHandlerNode *listener) {
	auto it = _eventListeners.find(listener->getEventID());
	if (it != _eventListeners.end()) {
		it->second.insert(listener);
	} else {
		_eventListeners.emplace(listener->getEventID(), std::unordered_set<const EventHandlerNode *>{listener});
	}
}
void Application::removeEventListner(const EventHandlerNode *listener) {
	auto it = _eventListeners.find(listener->getEventID());
	if (it != _eventListeners.end()) {
		it->second.erase(listener);
	}
}

void Application::removeAllListeners() {
	_eventListeners.clear();
}

void Application::dispatchEvent(const Event &ev) {
	if (_eventListeners.size() > 0) {
		auto it = _eventListeners.find(ev.getHeader().getEventID());
		if (it != _eventListeners.end() && it->second.size() != 0) {
			Vector<const EventHandlerNode *> listenersToExecute;
			auto &listeners = it->second;
			for (auto l : listeners) {
				if (l->shouldRecieveEventWithObject(ev.getEventID(), ev.getObject())) {
					listenersToExecute.push_back(l);
				}
			}

			for (auto l : listenersToExecute) {
				l->onEventRecieved(ev);
			}
		}
	}
}

}
