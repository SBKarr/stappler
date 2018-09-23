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

#ifndef __stappler__SPDevice__
#define __stappler__SPDevice__

#include "base/CCRef.h"

#include "SPEventHeader.h"
#include "SPEventHandler.h"
#include "SPSocketServer.h"

/*
 Device is abstract from environment, where the application is runned in.

 Device abstract not only physical device abstraction, but underlaying platform abstraction.
 It means, in contains function to get information about physical device, applcation platform
 version, bundle name, etc...

 It provides:
  - physical device information (screen, identifier, tablet or phone)
  - underlaying OS information (user agent, default language)
  - application platform (bundle name, version)

  - OS clipboard support

  - SP engine special device function:
    - Global clipping feature (enables/disables usage of clipping by GL_SCISSOR_TEST)

  - OS specifics
    - Android hardvare buttons

 */

NS_SP_BEGIN

class Device : public Ref, public EventHandler {
public:
	static Device *getInstance();

	static EventHeader onClipboard;
	static EventHeader onDeviceToken;

	static EventHeader onNetwork;
	static EventHeader onBackground;
	static EventHeader onFocus;

	static EventHeader onUrlOpened;
	static EventHeader onError;

	static EventHeader onRemoteNotification;
	static EventHeader onAndroidReset;

	static EventHeader onBackKey;
    static EventHeader onRateApplication;
    static EventHeader onScreenshot;

    static EventHeader onLaunchUrl;
    static EventHeader onStatusBar;

public:
	/* device information */

	/* phone/tablet check
	 on iOS - uses UI_USER_INTERFACE_IDIOM
	 on Android - xLarge devices (starting from Nexus 7 tablet)
	 */
	bool isTablet() const { return _isTablet; }

	bool isTouchDevice() const { return _isTouchDevice; }

	/* device/OS specific user agent, that used by default browser */
    const String &getUserAgent() const { return _userAgent; }

	/* unique device identifier (UDID). Use OS feature or generates UUID */
    const String &getDeviceIdentifier() const { return _deviceIdentifier; }

	/* Device notification token on APNS or GCM */
    const String &getDeviceToken() const { return _deviceToken; }

	/* application bundle name (reverce-DNS ordered usually) */
    const String &getBundleName() const { return _bundleName; }

	/* Human-readable application name, that displayed on application list of device */
    const String &getApplicationName() const { return _applicationName; }

	/* Application version string as in info.plist or manifest */
    const String &getApplicationVersion() const { return _applicationVersion; }

	/* Returns applcation version, encoded in integer N*XXXYYY, where N+ - major verion,
	XXX - middle version, YYY - minor version, 1.15.1 = 1 015 001 */
	int64_t getApplicationVersionCode();

	/* Device user locale string (like en-gb, fr-fr) */
    const String &getUserLanguage() const { return _userLanguage; }

	/* Device token for APNS/GCM */
    void registerDeviceToken(const uint8_t *data, size_t len);

	/* Device token for APNS/GCM */
    void registerDeviceToken(const String &data);

public:	/* global scissor support for StrictNodes */
	bool isScissorAvailable() const { return _clippingAvailable; }
    void setScissorAvailable(bool value);

public:	/* OS clipboard support */
	bool isClipboardAvailable();
	String getStringFromClipboard();
	void copyStringToClipboard(const String &value);

public: /* device optional features */
    void keyBackClicked();

	void update(float dt);

public: /* networking */
	void setNetworkOnline(bool isOnline);
	bool isNetworkOnline();

public: /* background/foreground */
	bool isInBackground();
	void didEnterBackground();
	void willEnterForeground();

	bool hasFocus() const;
	void onFocusGained();
	void onFocusLost();

public: /* application actions */
	void goToUrl(const StringView &url, bool external = true);
	void makePhoneCall(const StringView &number);
	void mailTo(const StringView &address);
	void rateApplication();

public:
	std::pair<uint64_t, uint64_t> getTotalDiskSpace();
	uint64_t getApplicationDiskSpace();

	void error(const String &errorHandle, const String &errorMessage);

	/* device local notification */
	void notification(const String &title, const String &text);

	void onDirectorStarted();

public: /* launch with url options */
    // set in launch process by AppController/Activity/etc...
    void setLaunchUrl(const StringView &);

    // called, when we should open recieved url with launched application
    // produce onLaunchUrl event, url can be read from event or by getLaunchUrl()
    void processLaunchUrl(const StringView &);

    StringView getLaunchUrl() const;

public:
    bool listen(uint16_t port = 0);

private:
	String _userAgent;
	String _deviceIdentifier;
	String _deviceToken;
	String _bundleName;
	String _applicationName;
	String _applicationVersion;
	String _userLanguage;

    String _launchUrl;

    bool _isTablet;
    bool _isTouchDevice = true;

    bool _clippingAvailable = true;
	bool _clipboardAvailable = false;

	float _updateTimer = 0;
	bool _isNetworkOnline = false;
	bool _inBackground = false;
	bool _hasFocus = true;

	bool _logTransferEnabled = false;
	SocketServer _serv;
	cocos2d::EventListenerKeyboard *_keyboardListener = nullptr;

    static void log(const StringView &tag, const StringView &str);

private:
	log::CustomLog _deviceLog;
	friend class AppDelegate;
	Device();
    void init();

#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
    cocos2d::EventListenerCustom* _backToForegroundlistener = nullptr;
#endif
};

NS_SP_END

#endif /* defined(__stappler__SPScreen__) */
