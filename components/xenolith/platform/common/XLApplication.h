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

#ifndef COMPONENTS_XENOLITH_PLATFORM_COMMON_XLAPPLICATION_H_
#define COMPONENTS_XENOLITH_PLATFORM_COMMON_XLAPPLICATION_H_

#include "XLDefine.h"
#include "XLEventHeader.h"
#include "XLVk.h"
#include "SPThreadTaskQueue.h"
#include "XLDrawPipeline.h"

namespace stappler::xenolith {

class Application : public Ref {
public:
	static Application *getInstance();

	static EventHeader onDeviceToken;

	static EventHeader onNetwork;

	static EventHeader onUrlOpened;
	static EventHeader onError;

	static EventHeader onRemoteNotification;
	static EventHeader onRegenerateResources;
	static EventHeader onRegenerateTextures;

	static EventHeader onBackKey;
	static EventHeader onRateApplication;
	static EventHeader onScreenshot;

	static EventHeader onLaunchUrl;

	static void sleep(double);

public:
	Application();
	virtual ~Application();

	virtual bool onFinishLaunching();
	virtual void onMemoryWarning();

	virtual void onDeviceInit(vk::PresentationDevice *, const stappler::Callback<void(draw::LoaderStage &&, bool deferred)> &);

	virtual void update(double dt);

public: // Threading, Events
	using Callback = Function<void()>;
	using ExecuteCallback = Function<bool(const Task &)>;
	using CompleteCallback = Function<void(const Task &, bool)>;

	/* Checks if current calling thread is director's main thread */
	bool isMainThread() const;

	/* If current thread is main thread: executes function/task
	   If not: adds function/task to main thread queue */
	void performOnMainThread(const Callback &func, Ref *target = nullptr, bool onNextFrame = false);

	/* If current thread is main thread: executes function/task
	   If not: adds function/task to main thread queue */
    void performOnMainThread(Rc<thread::Task> &&task, bool onNextFrame = false);

	/* Performs action in this thread, task will be constructed in place */
	void perform(const ExecuteCallback &, const CompleteCallback & = nullptr, Ref * = nullptr);

	/* Performs task in thread, identified by id */
    void perform(Rc<thread::Task> &&task);

	/* Performs task in thread, identified by id */
    void perform(Rc<thread::Task> &&task, bool performFirst);

    /* Spawn exclusive thread for task */
	void performAsync(Rc<Task> &&task);

	/* "Single-threaded" mode allow you to perform async tasks on single thread.
	 When "perform" function is called, task and all subsequent callbacks will be
	 executed on current thread. Perform call returns only when task is performed. */
	void setSingleThreaded(bool value);
	bool isSingleThreaded() const;

	void addEventListener(const EventHandlerNode *listener);
	void removeEventListner(const EventHandlerNode *listener);

	void removeAllListeners();

	void dispatchEvent(const Event &ev);

public:
	virtual int run(Director * = nullptr);

	virtual bool openURL(const StringView &url);

	/* device information */

	/* device/OS specific user agent, that used by default browser */
	StringView getUserAgent() const { return _userAgent; }

	/* unique device identifier (UDID). Use OS feature or generates UUID */
	StringView getDeviceIdentifier() const { return _deviceIdentifier; }

	/* Device notification token on APNS or GCM */
	StringView getDeviceToken() const { return _deviceToken; }

	/* application bundle name (reverce-DNS ordered usually) */
	StringView getBundleName() const { return _bundleName; }

	/* Human-readable application name, that displayed in application list */
	StringView getApplicationName() const { return _applicationName; }

	/* Application version string as in info.plist or manifest */
	StringView getApplicationVersion() const { return _applicationVersion; }

	/* Returns applcation version, encoded in integer N*XXXYYY, where N+ - major verion,
	XXX - middle version, YYY - minor version, 1.15.1 = 1 015 001 */
	int64_t getApplicationVersionCode();

	/* Device user locale string (like en-gb, fr-fr) */
	StringView getUserLanguage() const { return _userLanguage; }

	/* Device token for APNS/GCM */
	void registerDeviceToken(const uint8_t *data, size_t len);

	/* Device token for APNS/GCM */
	void registerDeviceToken(const String &data);

public: /* networking */
	void setNetworkOnline(bool isOnline);
	bool isNetworkOnline();

public: /* application actions */
	void goToUrl(const StringView &url, bool external = true);
	void makePhoneCall(const StringView &number);
	void mailTo(const StringView &address);
	void rateApplication();

public:
	Pair<uint64_t, uint64_t> getTotalDiskSpace();
	uint64_t getApplicationDiskSpace();

	/* device local notification */
	void notification(const String &title, const String &text);

public: /* launch with url options */
	// set in launch process by AppController/Activity/etc...
	void setLaunchUrl(const StringView &);

	// called, when we should open recieved url with launched application
	// produce onLaunchUrl event, url can be read from event or by getLaunchUrl()
	void processLaunchUrl(const StringView &);

	StringView getLaunchUrl() const;

public:
	// bool listen(uint16_t port = 0);

private:
	String _userAgent;
	String _deviceIdentifier;
	String _deviceToken;
	String _bundleName;
	String _applicationName;
	String _applicationVersion;
	String _userLanguage;

	String _launchUrl;

	double _updateTimer = 0;
	bool _isNetworkOnline = false;

	Rc<vk::View> _mainView;
	Rc<Director> _director;

	Rc<thread::TaskQueue> _queue;
	std::thread::id _threadId;
	bool _singleThreaded = false;

	std::unordered_map<EventHeader::EventID, std::unordered_set<const EventHandlerNode *>> _eventListeners;
};

}

#endif /* COMPONENTS_XENOLITH_PLATFORM_COMMON_XLAPPLICATION_H_ */
