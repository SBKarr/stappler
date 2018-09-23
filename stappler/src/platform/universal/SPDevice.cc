// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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

#include "SPDevice.h"
#include "SPFilesystem.h"
#include "SPError.h"

#include "base/CCDirector.h"
#include "base/CCScheduler.h"
#include "platform/CCGLView.h"

#include "SPEvent.h"
#include "SPThread.h"
#include "SPString.h"
#include "SPPlatform.h"
#include "SPScreen.h"
#include "SPAssetDownload.h"
#include "SPIME.h"

#include "SPStoreKit.h"
#include "SPAssetLibrary.h"
#include "SPThreadManager.h"
#include "SPTextureCache.h"

#include "base/CCEventDispatcher.h"
#include "base/CCEventType.h"
#include "base/CCEventCustom.h"
#include "base/CCEventKeyboard.h"
#include "base/CCEventListenerCustom.h"
#include "base/CCEventListenerKeyboard.h"

NS_SP_EXT_BEGIN(log)

void __stappler_log(const StringView &tag, CustomLog::Type t, CustomLog::VA &va);

NS_SP_EXT_END(log)


NS_SP_BEGIN

SP_DECLARE_EVENT_CLASS(Device, onDeviceToken);
SP_DECLARE_EVENT_CLASS(Device, onClipboard);
SP_DECLARE_EVENT_CLASS(Device, onNetwork);
SP_DECLARE_EVENT_CLASS(Device, onBackground);
SP_DECLARE_EVENT_CLASS(Device, onFocus);
SP_DECLARE_EVENT_CLASS(Device, onUrlOpened);
SP_DECLARE_EVENT_CLASS(Device, onError);
SP_DECLARE_EVENT_CLASS(Device, onRemoteNotification);
SP_DECLARE_EVENT_CLASS(Device, onAndroidReset);
SP_DECLARE_EVENT_CLASS(Device, onBackKey);
SP_DECLARE_EVENT_CLASS(Device, onRateApplication);
SP_DECLARE_EVENT_CLASS(Device, onScreenshot);
SP_DECLARE_EVENT_CLASS(Device, onLaunchUrl);
SP_DECLARE_EVENT_CLASS(Device, onStatusBar);

static Device *s_device = nullptr;
Device *Device::getInstance() {
	if (!s_device) {
		s_device = new Device();
        s_device->init();
	}
	return s_device;
}

Device::Device() : _deviceLog(&log::__stappler_log) {
    CCAssert(s_device == nullptr, "Device should follow singleton pattern");

    platform::render::_init();
    _userAgent = platform::device::_userAgent();
    _deviceIdentifier  = platform::device::_deviceIdentifier();
    _bundleName = platform::device::_bundleName();
    _applicationName = platform::device::_applicationName();
    _applicationVersion = platform::device::_applicationVersion();
    _userLanguage = platform::device::_userLanguage();
    _isTablet = platform::device::_isTablet();

    _clipboardAvailable = platform::clipboard::_isAvailable();

    _isNetworkOnline = platform::network::_isNetworkOnline();
    platform::network::_setNetworkCallback([this] (bool online) {
        if (online != _isNetworkOnline) {
            this->setNetworkOnline(online);
        }
    });

#ifndef SP_RESTRICT

    auto scheduler = cocos2d::Director::getInstance()->getScheduler();
    scheduler->scheduleUpdate(this, 0, false);

#ifndef __APPLE__
    _keyboardListener = cocos2d::EventListenerKeyboard::create();
    _keyboardListener->onKeyReleased = [this] ( cocos2d::EventKeyboard::KeyCode key, cocos2d::Event*) {
        if (key == cocos2d::EventKeyboard::KeyCode::KEY_BACK) {
            onBackKey(this);
        } else if (key == cocos2d::EventKeyboard::KeyCode::KEY_SCROLL_LOCK) {
            onScreenshot(this);
        }
    };
    _keyboardListener->retain();
    _keyboardListener->setEnabled(true);

    cocos2d::Director::getInstance()->getEventDispatcher()->addEventListenerWithFixedPriority(_keyboardListener, -1);
#endif

#if (ANDROID)
    _backToForegroundlistener = cocos2d::EventListenerCustom::create(EVENT_RENDERER_RECREATED,
        [this] (cocos2d::EventCustom*) {

        ThreadManager::getInstance()->update(0.0f);
        TextureCache::getInstance()->reloadTextures();
    	stappler::log::text("Device", "onAndroidReset");
    	onAndroidReset(this);
    });
    cocos2d::Director::getInstance()->getEventDispatcher()->addEventListenerWithFixedPriority(_backToForegroundlistener, -1);
#endif

#endif
}

void Device::init() {
#ifndef SP_RESTRICT
	//IME::getInstance();
#if (SP_INTERNAL_STOREKIT_ENABLED)
	StoreKit::getInstance();
#endif
	AssetLibrary::getInstance();
#endif

#if (COCOS2D_DEBUG && CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
	log::text("WritablePath", stappler::filesystem::writablePath());
#endif
}

void Device::update(float dt) {
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

void Device::setScissorAvailable(bool value) {
    _clippingAvailable = value;
}

void Device::registerDeviceToken(const uint8_t *data, size_t len) {
    registerDeviceToken(base16::encode(CoderSource(data, len)));
}

void Device::registerDeviceToken(const String &data) {
	_deviceToken = data;
	if (!_deviceToken.empty()) {
		onDeviceToken(this, _deviceToken);
	}
}

void Device::setNetworkOnline(bool online) {
	if (_isNetworkOnline != online) {
		_isNetworkOnline = online;
		onNetwork(this, _isNetworkOnline);
		if (!_isNetworkOnline) {
			_updateTimer = 0;
		}
	}
}

bool Device::isNetworkOnline() {
	return _isNetworkOnline;
}

bool Device::isInBackground() {
	return _inBackground;
}

void Device::didEnterBackground() {
	if (!_inBackground) {
		_inBackground = true;
		onBackground(this, true);
	}
}

void Device::willEnterForeground() {
	if (_inBackground) {
		_inBackground = false;
		onBackground(this, false);

		bool cb = platform::clipboard::_isAvailable();
		if (_clipboardAvailable != cb) {
			_clipboardAvailable = cb;
			onClipboard(this, _clipboardAvailable);
		}
	}
}

bool Device::hasFocus() const {
	return _hasFocus;
}

void Device::onFocusGained() {
	if (!_hasFocus) {
		_hasFocus = true;
		onFocus(this, true);

		bool cb = platform::clipboard::_isAvailable();
		if (_clipboardAvailable != cb) {
			_clipboardAvailable = cb;
			onClipboard(this, _clipboardAvailable);
		}
	}
}

void Device::onFocusLost() {
	if (_hasFocus) {
		_hasFocus = false;
		onFocus(this, false);
	}
}

void Device::goToUrl(const StringView &url, bool external) {
	onUrlOpened(this, url);
	platform::interaction::_goToUrl(url, external);
}
void Device::makePhoneCall(const StringView &number) {
	onUrlOpened(this, number);
	platform::interaction::_makePhoneCall(number);
}
void Device::mailTo(const StringView &address) {
	onUrlOpened(this, address);
	platform::interaction::_mailTo(address);
}
void Device::rateApplication() {
	onRateApplication(this);
	platform::interaction::_rateApplication();
}

std::pair<uint64_t, uint64_t> Device::getTotalDiskSpace() {
	return platform::device::_diskSpace();
}
uint64_t Device::getApplicationDiskSpace() {
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

int64_t Device::getApplicationVersionCode() {
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

void Device::notification(const String &title, const String &text) {
	platform::interaction::_notification(title, text);
}

void Device::onDirectorStarted() {
	platform::device::_onDirectorStarted();

    auto a = platform::clipboard::_isAvailable();
    if (a != _clipboardAvailable) {
    	_clipboardAvailable = a;
    	onClipboard(this, _clipboardAvailable);
    }
}

void Device::setLaunchUrl(const StringView &url) {
    _launchUrl = url.str();
}

void Device::processLaunchUrl(const StringView &url) {
    _launchUrl = url.str();
    onLaunchUrl(this, url);
}

StringView Device::getLaunchUrl() const {
    return _launchUrl;
}

namespace log {

using sp_log_fn = void (*) (const StringView &tag, const StringView &str);

void setCustomLogFn(sp_log_fn fn);

}

bool Device::listen(uint16_t port) {
	if (!_logTransferEnabled) {
		log::setCustomLogFn(&Device::log);
		_logTransferEnabled = true;
	}
	return _serv.listen(port);
}

void Device::log(const StringView &tag, const StringView &str) {
	auto & serv = getInstance()->_serv;
	serv.log(str);
}

bool Device::isClipboardAvailable() {
	return _clipboardAvailable;
}
String Device::getStringFromClipboard() {
	return platform::clipboard::_getString();
}
void Device::copyStringToClipboard(const String &value) {
	platform::clipboard::_copyString(value);
	bool v = platform::clipboard::_isAvailable();
	if (_clipboardAvailable != v) {
		_clipboardAvailable = v;
		onClipboard(this, _clipboardAvailable);
	}
}

void Device::keyBackClicked() {
	platform::interaction::_backKey();
}

void Device::error(const String &errorHandle, const String &errorMessage) {

}

NS_SP_END
