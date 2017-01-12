//
//  ReaderWebViewController.m
//  Reader
//
//  Created by SBKarr on 1/7/13.
//
//

#include "SPEvent.h"
#include "SPDevice.h"

#import "SPWebViewController.h"
#import "NSStringPunycodeAdditions.h"

#define BUTTON_X 0.0f

#define BUTTON_Y 0.0f
#define BUTTON_SPACE 10.0f
#define BUTTON_HEIGHT 37.5f

#define BUTTON_WIDTH 37.5f

@interface SPWebViewController () <UIWebViewDelegate>

@end

@interface ReaderTimer : NSObject {
    NSTimeInterval timerValue, timerLastUpdate;
    BOOL isActive;
}

- (NSTimeInterval) getValue;
- (void) setValue: (NSTimeInterval)value;
- (void) reset;

@end

@implementation ReaderTimer

- (id) init {
    if (self = [super init]) {
        timerValue = 0;
        isActive = YES;
        timerLastUpdate = [[NSDate date] timeIntervalSince1970];
        
        
        NSNotificationCenter *notificationCenter = [NSNotificationCenter defaultCenter];
        
        [notificationCenter addObserver:self selector:@selector(enterForeground:) name:UIApplicationWillEnterForegroundNotification object:nil];
        [notificationCenter addObserver:self selector:@selector(enterForeground:) name:UIApplicationDidBecomeActiveNotification object:nil];
        
        [notificationCenter addObserver:self selector:@selector(enterBackground:) name:UIApplicationDidEnterBackgroundNotification object:nil];
        [notificationCenter addObserver:self selector:@selector(enterBackground:) name:UIApplicationWillResignActiveNotification object:nil];
        
        [notificationCenter addObserver:self selector:@selector(enterBackground:) name:UIApplicationWillTerminateNotification object:nil];
        
    }
    return self;
}

- (void) dealloc {
    NSNotificationCenter *notificationCenter = [NSNotificationCenter defaultCenter];
    [notificationCenter removeObserver:self];
}

- (void) enterForeground: (NSNotification *)notification {
    if (!isActive) {
        timerLastUpdate = [[NSDate date] timeIntervalSince1970];
        isActive = YES;
    }
}

- (void) enterBackground: (NSNotification *)notification {
    if (isActive) {
        timerValue += [[NSDate date] timeIntervalSince1970] - timerLastUpdate;
        isActive = false;
    }
}

- (NSTimeInterval) getValue {
    if (isActive) {
        NSTimeInterval now = [[NSDate date] timeIntervalSince1970];
        timerValue += now - timerLastUpdate;
        timerLastUpdate = now;
    }
    return timerValue;
}

- (void) setValue: (NSTimeInterval)value {
    [self reset];
    timerValue = value;
}

- (void) reset {
    isActive = YES;
    timerValue = 0;
    timerLastUpdate = [[NSDate date] timeIntervalSince1970];
}

@end

stappler::EventHeader stappler::WebViewHandle::onOpened("WebView", "WebView.onOpened");
stappler::EventHeader stappler::WebViewHandle::onClosed("WebView", "WebView.onClosed");

@implementation SPWebViewController {
    NSURLRequest *theRequest, *currentRequest;
    NSURL *theURL;
    UIWebView *theWebView;
    UIButton *doneButton, *browserButton, *forwardButton, *backButton;
    UIView *controlView;
    UIActivityIndicatorView *activityIndicator;
    BOOL enabled, shouldDismiss, animationFinished;
    NSTimeInterval timeInterval;
    
    ReaderTimer *viewTimer;
	
	BOOL isVideo;
	BOOL isExternal;

	stappler::WebViewHandle *handle;
}

- (void) beginWithURL:(NSURL *)url {
     theRequest = [[NSURLRequest alloc] initWithURL:url cachePolicy:NSURLRequestReloadIgnoringLocalCacheData timeoutInterval:60.0f];
    currentRequest = theRequest;
    [theWebView loadRequest:theRequest];
}

- (id)initWithURL:(NSURL *)url isExternal:(BOOL)external {
    if (self = [super initWithNibName:nil bundle:nil]) {
        viewTimer = [[ReaderTimer alloc] init];
		isExternal = external;
        
        theURL = url;
        isVideo = NO;

		handle = nullptr;
    }
    return self;
}

- (void)viewDidLoad {
	[super viewDidLoad];
	
    [[NSURLCache sharedURLCache] removeAllCachedResponses];
	
	self.view.backgroundColor = [UIColor scrollViewTexturedBackgroundColor];
    self.view.multipleTouchEnabled = YES;
    
    CGRect webViewRect = self.view.bounds;
    webViewRect.size.height -= BUTTON_HEIGHT;
    
    theWebView = [[UIWebView alloc] initWithFrame:webViewRect];
    theWebView.autoresizesSubviews = YES;
    theWebView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    
    if (!WV_DEPRECATED_VERSION) {
        theWebView.suppressesIncrementalRendering = NO;
    }
    theWebView.delegate = self;
    theWebView.multipleTouchEnabled = YES;
    theWebView.scalesPageToFit = YES;
    [self.view addSubview:theWebView];

    
    CGRect controlViewRect = CGRectMake(0, self.view.bounds.size.height - BUTTON_HEIGHT, self.view.bounds.size.width, BUTTON_HEIGHT);
    controlView = [[UIView alloc] initWithFrame:controlViewRect];
    controlView.autoresizesSubviews = YES;
    controlView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleTopMargin;

    UIImage *controlBGImage = [UIImage imageNamed:@"common/reader/WebBG"];
	UIImageView *background = [[UIImageView alloc] initWithImage:controlBGImage];
	background.frame = controlView.bounds;
    background.autoresizingMask = UIViewAutoresizingFlexibleWidth;
	background.contentMode = UIViewContentModeScaleToFill;
	[controlView addSubview:background];
	//[controlView sendSubviewToBack:background];

    CGRect buttonFrame = CGRectMake(BUTTON_X, BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT);
    
    doneButton = [UIButton buttonWithType:UIButtonTypeCustom];
    doneButton.frame = buttonFrame;
    [doneButton setImage:[UIImage imageNamed:@"common/reader/WebExit"] forState:UIControlStateNormal];
    [doneButton addTarget:self action:@selector(doneButtonTapped:) forControlEvents:UIControlEventTouchDownVersionCompatible];
    doneButton.autoresizingMask = UIViewAutoresizingNone;
    doneButton.backgroundColor = [UIColor clearColor];
    doneButton.tintColor = [UIColor clearColor];
    [controlView addSubview:doneButton];
    
    
    buttonFrame = CGRectMake(self.view.bounds.size.width - BUTTON_X - BUTTON_WIDTH, BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT);
	
	if (isExternal == YES) {
		browserButton = [UIButton buttonWithType:UIButtonTypeCustom];
		browserButton.frame = buttonFrame;
		[browserButton setImage:[UIImage imageNamed:@"common/reader/WebBrowser"] forState:UIControlStateNormal];
		[browserButton addTarget:self action:@selector(browserButtonTapped:) forControlEvents:UIControlEventTouchDownVersionCompatible];
		browserButton.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin;
		browserButton.backgroundColor = [UIColor clearColor];
		browserButton.tintColor = [UIColor clearColor];
		[controlView addSubview:browserButton];
	}
	
    activityIndicator = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleGray];
    activityIndicator.center = CGPointMake(self.view.bounds.size.width/2, BUTTON_HEIGHT/2);
    activityIndicator.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleRightMargin;
    [controlView addSubview:activityIndicator];
    [activityIndicator startAnimating];
    
    buttonFrame = CGRectMake(self.view.bounds.size.width/2 - BUTTON_WIDTH/2, BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT);
    
    backButton = [UIButton buttonWithType:UIButtonTypeCustom];
    backButton.frame = CGRectMake(buttonFrame.origin.x - BUTTON_WIDTH, buttonFrame.origin.y, buttonFrame.size.width, buttonFrame.size.height);
    [backButton setImage:[UIImage imageNamed:@"common/reader/WebBack"] forState:UIControlStateNormal];
    [backButton addTarget:self action:@selector(backButtonTapped:) forControlEvents:UIControlEventTouchDownVersionCompatible];
    backButton.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleRightMargin;
    backButton.hidden = YES;
    backButton.backgroundColor = [UIColor clearColor];
    backButton.tintColor = [UIColor clearColor];
    [controlView addSubview:backButton];
    
    forwardButton = [UIButton buttonWithType:UIButtonTypeCustom];
    forwardButton.frame = CGRectMake(buttonFrame.origin.x + BUTTON_WIDTH, buttonFrame.origin.y, buttonFrame.size.width, buttonFrame.size.height);;
    [forwardButton setImage:[UIImage imageNamed:@"common/reader/WebForward"] forState:UIControlStateNormal];
    [forwardButton addTarget:self action:@selector(forwardButtonTapped:) forControlEvents:UIControlEventTouchDownVersionCompatible];
    forwardButton.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleRightMargin;
    forwardButton.hidden = YES;
    forwardButton.backgroundColor = [UIColor clearColor];
    forwardButton.tintColor = [UIColor clearColor];
    [controlView addSubview:forwardButton];
    
    [self.view addSubview:controlView];
    
    [self beginWithURL:theURL];
    [self updateButtons];

	handle = new stappler::WebViewHandle(self, theURL.absoluteString.UTF8String);
	stappler::WebViewHandle::onOpened(handle);
    
    enabled = YES;
}

- (void)doneButtonTapped:(UIButton *)button {
    if (button.hidden || button.alpha != 1.0f) return;
    [self dismissWebView];
}

- (void)browserButtonTapped:(UIButton *)button {
    if (button.hidden || button.alpha != 1.0f) return;
    [self dismissWebView];
    [[UIApplication sharedApplication] openURL:currentRequest.URL];
}

- (void)backButtonTapped:(UIButton *)button {
    if (button.hidden || button.alpha != 1.0f || theWebView.loading) return;
    [theWebView goBack];
}

- (void)forwardButtonTapped:(UIButton *)button {
    if (button.hidden || button.alpha != 1.0f || theWebView.loading) return;
    [theWebView goForward];
}

- (void)updateButtons {
    if (backButton.hidden) {
        if (theWebView.canGoBack) {
            backButton.hidden = NO;
            backButton.alpha = 0.0f;
            backButton.userInteractionEnabled = YES;
            [UIView animateWithDuration:0.5
                             animations:^{backButton.alpha = 1.0f;}];
        }
    } else {
        if (!theWebView.canGoBack) {
            backButton.userInteractionEnabled = NO;
            [UIView animateWithDuration:0.5
                             animations:^{backButton.alpha = 1.0f;}
                             completion:^(BOOL finished) {backButton.hidden = YES;}];
        }
    }
    
    if (forwardButton.hidden) {
        if (theWebView.canGoForward) {
            forwardButton.hidden = NO;
            forwardButton.alpha = 0.0f;
            forwardButton.userInteractionEnabled = YES;
            [UIView animateWithDuration:0.5
                             animations:^{forwardButton.alpha = 1.0f;}];
        }
    } else {
        if (!theWebView.canGoForward) {
            forwardButton.userInteractionEnabled = NO;
            [UIView animateWithDuration:0.5
                             animations:^{forwardButton.alpha = 1.0f;}
                             completion:^(BOOL finished) {forwardButton.hidden = YES;}];
        }
    }
    if (browserButton.hidden) {
        if (currentRequest) {
            browserButton.hidden = NO;
            browserButton.alpha = 0.0f;
            browserButton.userInteractionEnabled = YES;
            [UIView animateWithDuration:0.5
                             animations:^{browserButton.alpha = 1.0f;}];
        }
    } else {
        if (!currentRequest) {
            browserButton.userInteractionEnabled = NO;
            [UIView animateWithDuration:0.5
                             animations:^{forwardButton.alpha = 1.0f;}
                             completion:^(BOOL finished) {browserButton.hidden = YES;}];
        }
    }
}

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];
}

- (void)viewDidAppear:(BOOL)animated {
	[super viewDidAppear:animated];
    timeInterval = [[NSDate date] timeIntervalSince1970];
    if (shouldDismiss) {
        [self dismissWebView];
    } else {
        animationFinished = YES;
    }
}

- (void)viewWillDisappear:(BOOL)animated{
	[super viewWillDisappear:animated];
}

- (void)viewDidDisappear:(BOOL)animated {
	[super viewDidDisappear:animated];
}

- (void)viewDidUnload {
	[super viewDidUnload];
    
    theURL = nil;
    
    doneButton = nil; browserButton = nil;
    backButton = nil; forwardButton = nil;
    
    theWebView.delegate = nil;
    theWebView = nil;
    theRequest = nil;
    currentRequest = nil;
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
    return [self.presentingViewController shouldAutorotateToInterfaceOrientation:interfaceOrientation];
}

- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration {
     [self.presentingViewController willRotateToInterfaceOrientation:toInterfaceOrientation duration:duration];
 }
 
- (void)willAnimateRotationToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation duration:(NSTimeInterval)duration {
    [self.presentingViewController willAnimateRotationToInterfaceOrientation:interfaceOrientation duration:duration];
 }
 
 - (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation {
     [self.presentingViewController didRotateFromInterfaceOrientation:fromInterfaceOrientation];
}

- (void)didReceiveMemoryWarning {
	[super didReceiveMemoryWarning];
	
	if (theRequest) {
		[[NSURLCache sharedURLCache] removeCachedResponseForRequest:theRequest];
	}
	if (currentRequest) {
		[[NSURLCache sharedURLCache] removeCachedResponseForRequest:currentRequest];
	}
}

- (BOOL)webView:(UIWebView *)webView shouldStartLoadWithRequest:(NSURLRequest *)request navigationType:(UIWebViewNavigationType)navigationType {
    NSLog(@"%@", request.URL.absoluteString);
    if (!request || !currentRequest || !webView || !theWebView || !enabled) return NO;
    NSString *scheme = [request.URL scheme];
    if ([scheme isEqual:@"http"] || [scheme isEqual:@"https"]) {
        currentRequest = request;
    } else if ([scheme isEqual:[NSString stringWithUTF8String:stappler::Device::getInstance()->getBundleName().c_str()]]) {
        [self dismissWebView];
//[[UIApplication sharedApplication] openURL:request.URL];
        stappler::Device::getInstance()->processLaunchUrl(request.URL.absoluteString.UTF8String);
        return NO;
    }
    
    [self updateButtons];
    return YES;
}

- (void)webViewDidStartLoad:(UIWebView *)webView {
    [activityIndicator startAnimating];
    
    [self updateButtons];
}

- (void)webViewDidFinishLoad:(UIWebView *)webView {
    [activityIndicator stopAnimating];
    
    [self updateButtons];
}

- (void)webView:(UIWebView *)webView didFailLoadWithError:(NSError *)error {
	if (error.code == -999) {
		return;
	}
    [self updateButtons];
    if (!animationFinished) {
        shouldDismiss = YES;
    } else {
        [self dismissWebView];
    }
    UIAlertView *err = [[UIAlertView alloc] initWithTitle:@"Ошибка" message:[NSString stringWithFormat:@"%@: %@", @"Не удается установить соединение", error.localizedDescription] delegate:nil cancelButtonTitle:@"OK" otherButtonTitles:nil];
    [err show];
}

-(void)dismissWebView {
    [theWebView stopLoading];
    [theWebView setDelegate:nil];
	
	if (theRequest) {
		[[NSURLCache sharedURLCache] removeCachedResponseForRequest:theRequest];
	}
	if (currentRequest) {
		[[NSURLCache sharedURLCache] removeCachedResponseForRequest:currentRequest];
	}
	
	if (handle) {
		handle->setTimer([viewTimer getValue] * 1000);
		stappler::WebViewHandle::onClosed(handle);
		delete handle;
		handle = nullptr;
	}

    dismissViewController(self);
    enabled = NO;
}

@end
