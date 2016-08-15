//
//  SPScreen.h
//  stappler
//
//  Created by SBKarr on 14.02.14.
//  Copyright (c) 2014 SBKarr. All rights reserved.
//

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

class Device : public cocos2d::Ref, public EventHandler {
public:
	static Device *getInstance();

	static EventHeader onClipboard;
	static EventHeader onDeviceToken;

	static EventHeader onNetwork;
	static EventHeader onBackground;
	static EventHeader onDownloadVersionFailed;

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
    const std::string &getUserAgent() const { return _userAgent; }

	/* unique device identifier (UDID). Use OS feature or generates UUID */
    const std::string &getDeviceIdentifier() const { return _deviceIdentifier; }

	/* Device notification token on APNS or GCM */
    const std::string &getDeviceToken() const { return _deviceToken; }

	/* application bundle name (reverce-DNS ordered usually) */
    const std::string &getBundleName() const { return _bundleName; }

	/* Human-readable application name, that displayed on application list of device */
    const std::string &getApplicationName() const { return _applicationName; }

	/* Application version string as in info.plist or manifest */
    const std::string &getApplicationVersion() const { return _applicationVersion; }

	/* Returns applcation version, encoded in integer N*XXXYYY, where N+ - major verion,
	XXX - middle version, YYY - minor version, 1.15.1 = 1 015 001 */
	int64_t getApplicationVersionCode();

	/* Device user locale string (like en-gb, fr-fr) */
    const std::string &getUserLanguage() const { return _userLanguage; }

	/* Device token for APNS/GCM */
    void registerDeviceToken(const uint8_t *data, size_t len);

	/* Device token for APNS/GCM */
    void registerDeviceToken(const std::string &data);

public:	/* global scissor support for StrictNodes */
	bool isScissorAvailable() const { return _clippingAvailable; }
    void setScissorAvailable(bool value);

public:	/* OS clipboard support */
	bool isClipboardAvailable();
	std::string getStringFromClipboard();
	void copyStringToClipboard(const std::string &value);

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

public: /* application actions */
	void goToUrl(const std::string &url, bool external = true);
	void makePhoneCall(const std::string &number);
	void mailTo(const std::string &address);
	void rateApplication();

public:
	std::pair<uint64_t, uint64_t> getTotalDiskSpace();
	uint64_t getApplicationDiskSpace();

	void error(const std::string &errorHandle, const std::string &errorMessage);

	/* device local notification */
	void notification(const std::string &title, const std::string &text);

	void onDirectorStarted();

public: /* launch with url options */
    // set in launch process by AppController/Activity/etc...
    void setLaunchUrl(const std::string &);

    // called, when we should open recieved url with launched application
    // produce onUrl event, url can be read from event or by getLaunchUrl()
    void processLaunchUrl(const std::string &);

    const std::string &getLaunchUrl() const;

public:
    bool listen(uint16_t port = 0);

private:
	std::string _userAgent;
	std::string _deviceIdentifier;
	std::string _deviceToken;
	std::string _bundleName;
	std::string _applicationName;
	std::string _applicationVersion;
	std::string _userLanguage;

    std::string _launchUrl;

    bool _isTablet;
    bool _isTouchDevice = true;

    bool _clippingAvailable = true;
	bool _clipboardAvailable = false;

	float _updateTimer = 0;
	bool _isNetworkOnline = false;
	bool _inBackground = false;

	bool _logTransferEnabled = false;
	SocketServer _serv;
	cocos2d::EventListenerKeyboard *_keyboardListener = nullptr;

    static void log(const char *, size_t n);

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
