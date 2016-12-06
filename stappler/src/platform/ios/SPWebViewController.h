//
//  ReaderWebViewController.h
//  Reader
//
//  Created by SBKarr on 1/7/13.
//
//

#include "SPEventHeader.h"
#include "base/CCRef.h"

#ifdef __OBJC__

#import <UIKit/UIKit.h>

#define WV_DEPRECATED_VERSION ([[UIDevice currentDevice] systemVersion].floatValue < 6)

#define UIControlEventTouchDownVersionCompatible \
(UIControlEventTouchUpInside) // for 5.1

#define presentViewController(rootVC, newVC) \
([rootVC presentViewController:newVC animated:YES completion:nil])

#define dismissViewController(VC) \
([VC dismissViewControllerAnimated:YES completion:nil])

@interface SPWebViewController : UIViewController

- (id)initWithURL:(NSURL *)url isExternal:(BOOL)external;

@end

#else

#define nil nullptr;
#define SPWebViewController void;

#endif

NS_SP_BEGIN

class WebViewHandle : public cocos2d::Ref {
public:
	static EventHeader onOpened;
	static EventHeader onClosed;

	WebViewHandle(SPWebViewController *view, const std::string &url)
	: _webView(view), _url(url) { }

	inline int64_t getTimer() { return _timer; }
	inline void setTimer(int64_t value) { _timer = value; }

	inline SPWebViewController *getWebView() { return _webView; }
	inline const std::string &getUrl() { return _url; }
protected:
	SPWebViewController *_webView = nil;
	std::string _url;
	int64_t _timer = 0;
};

NS_SP_END
