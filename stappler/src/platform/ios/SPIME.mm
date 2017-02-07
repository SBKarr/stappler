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

#include "SPPlatform.h"
#include "SPIME.h"
#include "SPScreen.h"
#include "platform/CCPlatformConfig.h"
#include "base/CCDirector.h"

#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>

#import "CCGLView.h"
#import "CCEAGLView-ios.h"

#ifndef SP_IME_DEBUG
#define SP_IME_DEBUG 0
#endif

#if (SP_IME_DEBUG)
#define IMELOG stappler::log
#else
#define IMELOG(...)
#endif


@interface SPNativeInputView : UIView <UIKeyInput, UITextInput> {
	CGRect caretRect;
    NSString *_markedText;
	BOOL _isActive;
	bool _statusBar;
}

@property(nonatomic, readonly) UITextPosition *beginningOfDocument;
@property(nonatomic, readonly) UITextPosition *endOfDocument;
@property(nonatomic, assign) id<UITextInputDelegate> inputDelegate;
@property(nonatomic, readonly) UITextRange *markedTextRange;
@property (nonatomic, copy) NSDictionary *markedTextStyle;
@property(readwrite, copy) UITextRange *selectedTextRange;
@property(nonatomic, readonly) id<UITextInputTokenizer> tokenizer;

- (id) initWithFrame:(CGRect)frame;

- (CGRect) convertRectFromViewToSurface:(CGRect)rect;
- (CGPoint) convertPointFromViewToSurface:(CGPoint)point;

@end

#define IOS_MAX_TOUCHES_COUNT     10

@implementation SPNativeInputView

- (id) initWithFrame:(CGRect)frame {
	if (self = [super initWithFrame:frame]) {
		if ([self respondsToSelector:@selector(setContentScaleFactor:)]) {
			self.contentScaleFactor = [[UIScreen mainScreen] scale];
		}
		_markedText = nil;
	}
    return self;
}

- (void)didMoveToWindow {
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(onUIKeyboardNotification:)
                                                 name:UIKeyboardWillShowNotification object:nil];

    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(onUIKeyboardNotification:)
                                                 name:UIKeyboardDidShowNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(onUIKeyboardNotification:)
                                                 name:UIKeyboardWillHideNotification object:nil];

    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(onUIKeyboardNotification:)
                                                 name:UIKeyboardDidHideNotification object:nil];
}

- (void) dealloc {
    [[NSNotificationCenter defaultCenter] removeObserver:self]; // remove keyboard notification
}

- (CGPoint) convertPointFromViewToSurface:(CGPoint)point {
    CGRect bounds = [self bounds];
	CGSize size = [((CCEAGLView *)self.superview) surfaceSize];

	CGPoint ret;
	ret.x = (point.x - bounds.origin.x) / bounds.size.width * size.width;
	ret.y = (point.y - bounds.origin.y) / bounds.size.height * size.height;

	return ret;
}

- (CGRect) convertRectFromViewToSurface:(CGRect)rect {
    CGRect bounds = [self bounds];
	CGSize size = [((CCEAGLView *)self.superview) surfaceSize];

	CGRect ret;
	ret.origin.x = (rect.origin.x - bounds.origin.x) / bounds.size.width * size.width;
	ret.origin.y = (rect.origin.y - bounds.origin.y) / bounds.size.height * size.height;
	ret.size.width = rect.size.width / bounds.size.width * size.width;
	ret.size.height = rect.size.height / bounds.size.height * size.height;

	return ret;
}

// Pass the touches to the superview
#pragma mark CCEAGLView - Touch Delegate
- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event {
    intptr_t ids[IOS_MAX_TOUCHES_COUNT] = {0};
    float xs[IOS_MAX_TOUCHES_COUNT] = {0.0f};
    float ys[IOS_MAX_TOUCHES_COUNT] = {0.0f};

    int i = 0;
    for (UITouch *touch in touches) {
        ids[i] = (intptr_t)touch;
        xs[i] = [touch locationInView: [touch view]].x * self.contentScaleFactor;
        ys[i] = [touch locationInView: [touch view]].y * self.contentScaleFactor;
        ++i;
    }

    auto glview = cocos2d::Director::getInstance()->getOpenGLView();
    glview->handleTouchesBegin(i, ids, xs, ys);
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event {
    intptr_t ids[IOS_MAX_TOUCHES_COUNT] = {0};
    float xs[IOS_MAX_TOUCHES_COUNT] = {0.0f};
    float ys[IOS_MAX_TOUCHES_COUNT] = {0.0f};

    int i = 0;
    for (UITouch *touch in touches) {
        ids[i] = (intptr_t)touch;
        xs[i] = [touch locationInView: [touch view]].x * self.contentScaleFactor;
        ys[i] = [touch locationInView: [touch view]].y * self.contentScaleFactor;
        ++i;
    }

    auto glview = cocos2d::Director::getInstance()->getOpenGLView();
    glview->handleTouchesMove(i, ids, xs, ys);
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event {
    intptr_t ids[IOS_MAX_TOUCHES_COUNT] = {0};
    float xs[IOS_MAX_TOUCHES_COUNT] = {0.0f};
    float ys[IOS_MAX_TOUCHES_COUNT] = {0.0f};

    int i = 0;
    for (UITouch *touch in touches) {
        ids[i] = (intptr_t)touch;
        xs[i] = [touch locationInView: [touch view]].x * self.contentScaleFactor;;
        ys[i] = [touch locationInView: [touch view]].y * self.contentScaleFactor;;
        ++i;
    }

    auto glview = cocos2d::Director::getInstance()->getOpenGLView();
    glview->handleTouchesEnd(i, ids, xs, ys);
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event {
    intptr_t ids[IOS_MAX_TOUCHES_COUNT] = {0};
    float xs[IOS_MAX_TOUCHES_COUNT] = {0.0f};
    float ys[IOS_MAX_TOUCHES_COUNT] = {0.0f};

    int i = 0;
    for (UITouch *touch in touches) {
        ids[i] = (intptr_t)touch;
        xs[i] = [touch locationInView: [touch view]].x * self.contentScaleFactor;;
        ys[i] = [touch locationInView: [touch view]].y * self.contentScaleFactor;;
        ++i;
    }

    auto glview = cocos2d::Director::getInstance()->getOpenGLView();
    glview->handleTouchesCancel(i, ids, xs, ys);
}

#pragma mark - UIView - Responder

- (BOOL)canBecomeFirstResponder {
    return YES;
}

- (BOOL)becomeFirstResponder {
	BOOL val = [super becomeFirstResponder];
	if (val == YES && _isActive == NO) {
		_isActive = val;
        stappler::platform::ime::native::setInputEnabled(true);
		_statusBar = stappler::Screen::getInstance()->isStatusBarEnabled();
		stappler::Screen::getInstance()->setStatusBarEnabled(false);
	}
	return val;
}

- (void)removeFromSuperview {
	[super removeFromSuperview];
	if (_isActive == YES) {
        stappler::platform::ime::native::setInputEnabled(false);
		stappler::Screen::getInstance()->setStatusBarEnabled(_statusBar);
	}
	_isActive = NO;
}

#pragma mark - UIKeyInput protocol

- (BOOL)hasText {
    return stappler::platform::ime::native::hasText()?YES:NO;
}

- (void)insertText:(NSString *)text {
    if (nil != _markedText) {
        _markedText = nil;
    }
	NSData *data = [text dataUsingEncoding:NSUTF16LittleEndianStringEncoding];
    stappler::platform::ime::native::insertText(std::u16string((char16_t *)data.bytes, text.length));
}

- (void)deleteBackward {
    if (nil != _markedText) {
        _markedText = nil;
    }
    stappler::platform::ime::native::deleteBackward();
}

#pragma mark - UITextInputTrait protocol

-(UITextAutocapitalizationType) autocapitalizationType {
    return UITextAutocapitalizationTypeNone;
}

#pragma mark - UITextInput protocol

#pragma mark UITextInput - properties

@synthesize beginningOfDocument;
@synthesize endOfDocument;
@synthesize inputDelegate;
@synthesize markedTextRange;
@synthesize markedTextStyle;
// @synthesize selectedTextRange;       // must implement
@synthesize tokenizer;

/* Text may have a selection, either zero-length (a caret) or ranged.  Editing operations are
 * always performed on the text from this selection.  nil corresponds to no selection. */
- (void)setSelectedTextRange:(UITextRange *)aSelectedTextRange {
    IMELOG("UITextRange:setSelectedTextRange");
}

- (UITextRange *)selectedTextRange {
    return [[UITextRange alloc] init];
}

#pragma mark UITextInput - Replacing and Returning Text

- (NSString *)textInRange:(UITextRange *)range {
    IMELOG("textInRange: %s", range.description.UTF8String);
    return @"";
}
- (void)replaceRange:(UITextRange *)range withText:(NSString *)theText {
    IMELOG("replaceRange");
}

#pragma mark UITextInput - Working with Marked and Selected Text

/* If text can be selected, it can be marked. Marked text represents provisionally
 * inserted text that has yet to be confirmed by the user.  It requires unique visual
 * treatment in its display.  If there is any marked text, the selection, whether a
 * caret or an extended range, always resides within.
 *
 * Setting marked text either replaces the existing marked text or, if none is present,
 * inserts it from the current selection. */

- (void)setMarkedTextRange:(UITextRange *)markedTextRange {
    IMELOG("setMarkedTextRange");
}

- (UITextRange *)markedTextRange {
    IMELOG("markedTextRange");
    return nil; // Nil if no marked text.
}
- (void)setMarkedTextStyle:(NSDictionary *)markedTextStyle {
    IMELOG("setMarkedTextStyle");
}

- (NSDictionary *)markedTextStyle {
    IMELOG("markedTextStyle");
    return nil;
}
- (void)setMarkedText:(NSString *)markedText selectedRange:(NSRange)selectedRange {
    IMELOG("setMarkedText");
    if (markedText == _markedText) {
        return;
    }
    _markedText = markedText;
}
- (void)unmarkText {
	IMELOG("unmarkText");
    if (nil == _markedText) {
        return;
    }
	NSData *data = [_markedText dataUsingEncoding:NSUTF16LittleEndianStringEncoding];
    stappler::platform::ime::native::insertText(std::u16string((char16_t *)data.bytes, _markedText.length));
    _markedText = nil;
}

#pragma mark Methods for creating ranges and positions.

- (UITextRange *)textRangeFromPosition:(UITextPosition *)fromPosition toPosition:(UITextPosition *)toPosition {
    IMELOG("textRangeFromPosition");
    return nil;
}
- (UITextPosition *)positionFromPosition:(UITextPosition *)position offset:(NSInteger)offset {
    IMELOG("positionFromPosition");
    return nil;
}
- (UITextPosition *)positionFromPosition:(UITextPosition *)position inDirection:(UITextLayoutDirection)direction offset:(NSInteger)offset {
    IMELOG("positionFromPosition");
    return nil;
}

/* Simple evaluation of positions */
- (NSComparisonResult)comparePosition:(UITextPosition *)position toPosition:(UITextPosition *)other {
    IMELOG("comparePosition");
    return (NSComparisonResult)0;
}
- (NSInteger)offsetFromPosition:(UITextPosition *)from toPosition:(UITextPosition *)toPosition {
    IMELOG("offsetFromPosition");
    return 0;
}

- (UITextPosition *)positionWithinRange:(UITextRange *)range farthestInDirection:(UITextLayoutDirection)direction {
    IMELOG("positionWithinRange");
    return nil;
}
- (UITextRange *)characterRangeByExtendingPosition:(UITextPosition *)position inDirection:(UITextLayoutDirection)direction {
    IMELOG("characterRangeByExtendingPosition");
    return nil;
}

#pragma mark Writing direction

- (UITextWritingDirection)baseWritingDirectionForPosition:(UITextPosition *)position inDirection:(UITextStorageDirection)direction {
    IMELOG("baseWritingDirectionForPosition");
    return UITextWritingDirectionNatural;
}
- (void)setBaseWritingDirection:(UITextWritingDirection)writingDirection forRange:(UITextRange *)range {
    IMELOG("setBaseWritingDirection");
}

#pragma mark Geometry

/* Geometry used to provide, for example, a correction rect. */
- (CGRect)firstRectForRange:(UITextRange *)range {
    IMELOG("firstRectForRange");
    return CGRectNull;
}
- (CGRect)caretRectForPosition:(UITextPosition *)position {
    IMELOG("caretRectForPosition");
    return CGRectNull;
}

#pragma mark Hit testing

/* JS - Find the closest position to a given point */
- (UITextPosition *)closestPositionToPoint:(CGPoint)point {
    IMELOG("closestPositionToPoint");
    return nil;
}
- (UITextPosition *)closestPositionToPoint:(CGPoint)point withinRange:(UITextRange *)range {
    IMELOG("closestPositionToPoint");
    return nil;
}
- (UITextRange *)characterRangeAtPoint:(CGPoint)point {
    IMELOG("characterRangeAtPoint");
    return nil;
}

- (NSArray *)selectionRectsForRange:(UITextRange *)range {
    IMELOG("selectionRectsForRange");
    return nil;
}

#pragma mark - UIKeyboard notification

- (void)onUIKeyboardNotification:(NSNotification *)notif {
    NSString * type = notif.name;

    NSDictionary* info = [notif userInfo];
    CGRect end = [self convertRect:[[info objectForKey:UIKeyboardFrameEndUserInfoKey] CGRectValue] fromView:self];
    double aniDuration = [[info objectForKey:UIKeyboardAnimationDurationUserInfoKey] doubleValue];
	
	CGSize viewSize = self.frame.size;
	
	CGSize size = end.size;
	if (size.width < size.height) {
		size = CGSizeMake(size.height, size.width);
	}
	
	CGRect rect = CGRectMake(0, 0, size.width, size.height);
	
    auto glview = cocos2d::Director::getInstance()->getOpenGLView();
    float scaleX = glview->getScaleX();
	float scaleY = glview->getScaleY();

    if (self.contentScaleFactor == 2.0f) {
        rect = CGRectApplyAffineTransform(rect, CGAffineTransformScale(CGAffineTransformIdentity, 2.0f, 2.0f));
    }

    // Convert to desigin coordinate
    rect = CGRectApplyAffineTransform(rect, CGAffineTransformScale(CGAffineTransformIdentity, 1.0f/scaleX, 1.0f/scaleY));

    if (UIKeyboardWillShowNotification == type) {
        stappler::platform::ime::native::onKeyboardShow(cocos2d::Rect(rect.origin.x, rect.origin.y, rect.size.width, rect.size.height), aniDuration);
    } else if (UIKeyboardDidShowNotification == type) {
        caretRect = rect;
        caretRect.origin.y = viewSize.height - (caretRect.origin.y + caretRect.size.height + [UIFont smallSystemFontSize]);
        caretRect.size.height = 0;
    } else if (UIKeyboardWillHideNotification == type) {
		stappler::platform::ime::native::onKeyboardHide(aniDuration);
    } else if (UIKeyboardDidHideNotification == type) {
        caretRect = CGRectZero;
    }
}

- (void) layoutSubviews {
	CGSize size = [((CCEAGLView *)self.superview) surfaceSize];
	self.frame = CGRectMake(0, 0, size.width / self.contentScaleFactor, size.height / self.contentScaleFactor);
	[super layoutSubviews];
}

@end

NS_SP_PLATFORM_BEGIN

namespace ime {
	UIView *_IMEView = nil;

	void _updateCursor(uint32_t pos, uint32_t len) {

	}

	void _updateText(const std::u16string &str, uint32_t pos, uint32_t len, int32_t t) {

	}

	void _runWithText(const std::u16string &str, uint32_t pos, uint32_t len, int32_t t) {
		if (!_IMEView) {
			UIWindow *window = [UIApplication sharedApplication].keyWindow;
			UIViewController *rootViewController = window.rootViewController;
			UIView *view = rootViewController.view;

			CGRect frame = view.frame;
			frame.origin.x = 0.0f; frame.origin.y = 0.0f;

			_IMEView = [[SPNativeInputView alloc] initWithFrame:frame];
			[view addSubview:_IMEView];
			[_IMEView becomeFirstResponder];
		}
	}

	void _cancel() {
		if (_IMEView) {
			[_IMEView removeFromSuperview];
			_IMEView = nil;
		}
	}
}

NS_SP_PLATFORM_END
