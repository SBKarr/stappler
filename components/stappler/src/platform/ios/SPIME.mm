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

#include "SPDefine.h"
#include "SPPlatform.h"

#ifdef IOS
#ifndef SP_RESTRICT

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
#define IMELOG(...) stappler::log::format("IME", __VA_ARGS__)
#else
#define IMELOG(...)
#endif

using Cursor = stappler::ime::Cursor;

@interface SPTextPosition : UITextPosition {
	Cursor _cursor;
}

@property (nonatomic, readwrite) Cursor cursor;

- (id) initWithCursor: (Cursor) c;

@end

@interface SPTextRange : UITextRange {
	Cursor _cursor;
}

@property (nonatomic, readwrite) Cursor cursor;

- (id) initWithCursor: (Cursor) c;
- (BOOL) isEmpty;
- (SPTextPosition *) start;
- (SPTextPosition *) end;

@end

@interface SPNativeInputView : UIView <UIKeyInput, UITextInput> {
	CGRect caretRect;
	BOOL _isActive;
	bool _statusBar;

	std::u16string *_nativeString;
	Cursor *_nativeCursor;
	Cursor _markedRange;
	Cursor _selectedRange;
	bool _markedRangeExists;
}

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
	IMELOG("canBecomeFirstResponder");
    return YES;
}

- (BOOL)becomeFirstResponder {
	IMELOG("becomeFirstResponder");
	BOOL val = [super becomeFirstResponder];
	if (val == YES && _isActive == NO) {
		IMELOG("becomeFirstResponder successful");
		_isActive = val;
        stappler::platform::ime::native::setInputEnabled(true);
		_statusBar = stappler::Screen::getInstance()->isStatusBarEnabled();
		stappler::Screen::getInstance()->setStatusBarEnabled(false);
	}
	return val;
}

- (void)removeFromSuperview {
	IMELOG("removeFromSuperview");
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
	NSData *data = [text dataUsingEncoding:NSUTF16LittleEndianStringEncoding];
	
	auto str = std::u16string((char16_t *)data.bytes, text.length);
	_markedRange = Cursor(_nativeCursor->start, uint32_t(str.size()));
	_markedRangeExists = true;
	stappler::platform::ime::native::insertText(str);
	IMELOG("insertText '%s'", text.UTF8String);
	[self updateNativeString];
}

- (void)deleteBackward {
	_markedRangeExists = false;
	stappler::platform::ime::native::deleteBackward();
	IMELOG("deleteBackward");
	[self updateNativeString];
}

- (void)updateNativeString {
	_nativeString = stappler::ime::getNativeStringPointer();
	_nativeCursor = stappler::ime::getNativeCursorPointer();
}

- (void)setSelectedCursor:(Cursor)c {
	_markedRangeExists = false;
	[self.inputDelegate selectionWillChange:self];
	[self updateNativeString];
	[self.inputDelegate selectionDidChange:self];
}

#pragma mark - UITextInputTrait protocol

-(UITextAutocapitalizationType) autocapitalizationType {
    return UITextAutocapitalizationTypeNone;
}

#pragma mark - UITextInput protocol

#pragma mark UITextInput - properties

@synthesize inputDelegate;
@synthesize tokenizer;

@synthesize autocorrectionType;
@synthesize spellCheckingType;
@synthesize keyboardType;
@synthesize returnKeyType;

- (UITextPosition *) beginningOfDocument {
	IMELOG("beginningOfDocument %u", 0);
	return [[SPTextPosition alloc] initWithCursor:Cursor(0)];
}
- (UITextPosition *) endOfDocument {
	IMELOG("endOfDocument %u", uint32_t(_nativeString->size()));
	return [[SPTextPosition alloc] initWithCursor:Cursor(uint32_t(_nativeString->size()))];
}

/* Text may have a selection, either zero-length (a caret) or ranged.  Editing operations are
 * always performed on the text from this selection.  nil corresponds to no selection. */
- (void)setSelectedTextRange:(UITextRange *)aSelectedTextRange {
	Cursor c = ((SPTextRange *)aSelectedTextRange).cursor;
	IMELOG("setSelectedTextRange %u %u", c.start, c.length);
	stappler::platform::ime::native::cursorChanged(c.start, c.length);
}

- (UITextRange *)selectedTextRange {
	IMELOG("selectedTextRange %u %u", _nativeCursor->start, _nativeCursor->length);
	return [[SPTextRange alloc] initWithCursor:Cursor(*_nativeCursor)];
}

#pragma mark UITextInput - Replacing and Returning Text

- (NSString *)textInRange:(UITextRange *)range {
	Cursor c = ((SPTextRange *)range).cursor;
	NSString *ret = [[NSString alloc] initWithBytes:(const void *)(_nativeString->data() + c.start) length:c.length * sizeof(char16_t) encoding:NSUTF16LittleEndianStringEncoding];
	IMELOG("textInRange: %u %u '%s'", c.start, c.length, ret.UTF8String);
	return ret;
}

- (void)replaceRange:(UITextRange *)range withText:(NSString *)theText {
	Cursor c = ((SPTextRange *)range).cursor;
	*_nativeCursor = c;
	if (theText.length == 0) {
		stappler::platform::ime::native::deleteBackward();
	} else {
		stappler::platform::ime::native::insertText(stappler::string::toUtf16(theText.UTF8String));
	}
    IMELOG("replaceRange %u %u '%s' %lu", c.start, c.length, theText.UTF8String, theText.length);
}

- (BOOL)shouldChangeTextInRange:(UITextRange *)range replacementText:(NSString *)text {
	// Cursor c = ((SPTextRange *)range).cursor;
	// IMELOG("shouldChangeTextInRange %u %u %lu '%s'", c.start, c.length, text.length, text.UTF8String);
	return YES;
}
#pragma mark UITextInput - Working with Marked and Selected Text

/* If text can be selected, it can be marked. Marked text represents provisionally
 * inserted text that has yet to be confirmed by the user.  It requires unique visual
 * treatment in its display.  If there is any marked text, the selection, whether a
 * caret or an extended range, always resides within.
 *
 * Setting marked text either replaces the existing marked text or, if none is present,
 * inserts it from the current selection. */

- (void)setMarkedTextRange:(UITextRange *)range {
	_markedRange = ((SPTextRange *)range).cursor;
	_markedRangeExists = true;
    IMELOG("setMarkedTextRange");
}

- (UITextRange *)markedTextRange {
	if (_markedRangeExists) {
		IMELOG("markedTextRange %d %u %u", _markedRangeExists, _markedRange.start, _markedRange.length);
		return [[SPTextRange alloc] initWithCursor:_markedRange]; // Nil if no marked text.
	}
	return nil;
}
- (void)setMarkedTextStyle:(NSDictionary *)markedTextStyle {
    IMELOG("setMarkedTextStyle");
}

- (NSDictionary *)markedTextStyle {
    IMELOG("markedTextStyle");
    return nil;
}
- (void)setMarkedText:(NSString *)markedText selectedRange:(NSRange)selectedRange {
	IMELOG("setMarkedText %s %lu %lu", markedText.UTF8String, (unsigned long)selectedRange.location, (unsigned long)selectedRange.length);
	
	if (_markedRangeExists && _markedRange.length > 0) {
		*_nativeCursor = _markedRange;
	} else {
		_nativeCursor->length = 0;
	}
	auto newString = stappler::string::toUtf16(markedText.UTF8String);
	_markedRange = Cursor(_nativeCursor->start, uint32_t(newString.size()));
	_markedRangeExists = true;

	stappler::platform::ime::native::insertText(newString);
}
- (void)unmarkText {
	_markedRangeExists = false;
}

#pragma mark Methods for creating ranges and positions.

- (UITextRange *)textRangeFromPosition:(UITextPosition *)fromPosition toPosition:(UITextPosition *)toPosition {
	Cursor first = ((SPTextPosition *)fromPosition).cursor;
	Cursor second = ((SPTextPosition *)toPosition).cursor;

    IMELOG("textRangeFromPosition %u %u", first.start, second.start);
    return [[SPTextRange alloc] initWithCursor:Cursor(first.start, second.start - first.start)];
}
- (UITextPosition *)positionFromPosition:(UITextPosition *)position offset:(NSInteger)offset {
	Cursor first = ((SPTextPosition *)position).cursor;
	if ((offset < 0 && std::abs(offset) > first.start) || (offset + first.start > _nativeString->size() - 1)) {
		return nil;
	}
	IMELOG("positionFromPosition %u %ld %u", first.start, (long)offset, uint32_t(first.start + offset));
    return [[SPTextPosition alloc] initWithCursor:Cursor(uint32_t(first.start + offset))];
}
- (UITextPosition *)positionFromPosition:(UITextPosition *)position inDirection:(UITextLayoutDirection)direction offset:(NSInteger)offset {
	Cursor first = ((SPTextPosition *)position).cursor;
	if ((offset < 0 && std::abs(offset) > first.start) || (offset + first.start > _nativeString->size() - 1)) {
		return nil;
	}
	IMELOG("positionFromPosition2 %u %ld %u", first.start, (long)offset, uint32_t(first.start + offset));
	return [[SPTextPosition alloc] initWithCursor:Cursor(uint32_t(first.start + offset))];
}

/* Simple evaluation of positions */
- (NSComparisonResult)comparePosition:(UITextPosition *)position toPosition:(UITextPosition *)other {
	Cursor first = ((SPTextPosition *)position).cursor;
	Cursor second = ((SPTextPosition *)other).cursor;
	NSComparisonResult res;
	if (first.start == second.start) {
		res = NSOrderedSame;
	} else if (first.start < second.start) {
		res = NSOrderedDescending;
	} else {
		res = NSOrderedAscending;
	}
	IMELOG("comparePosition %u %u %ld", first.start, second.start, (long)res);
	return res;
}
- (NSInteger)offsetFromPosition:(UITextPosition *)from toPosition:(UITextPosition *)toPosition {
	Cursor first = ((SPTextPosition *)from).cursor;
	Cursor second = ((SPTextPosition *)toPosition).cursor;
    IMELOG("offsetFromPosition %u %u", first.start, second.start);
    return second.start - first.start;
}

- (UITextPosition *)positionWithinRange:(UITextRange *)range farthestInDirection:(UITextLayoutDirection)direction {
	Cursor first = ((SPTextRange *)range).cursor;
    IMELOG("positionWithinRange %u %u", first.start, first.length);
    return [[SPTextPosition alloc] initWithCursor:Cursor(first.start)];
}
- (UITextRange *)characterRangeByExtendingPosition:(UITextPosition *)position inDirection:(UITextLayoutDirection)direction {
	Cursor first = ((SPTextPosition *)position).cursor;
    IMELOG("characterRangeByExtendingPosition %u", first.start);
    return [[SPTextRange alloc] initWithCursor:Cursor(first.start, 1)];
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
	IMELOG("onUIKeyboardNotification");
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
        IMELOG("UIKeyboardWillShowNotification %f %f %f %f - %f", rect.origin.x, rect.origin.y, rect.size.width, rect.size.height, aniDuration);
    } else if (UIKeyboardDidShowNotification == type) {
        IMELOG("UIKeyboardDidShowNotification %f", aniDuration);
        caretRect = rect;
        caretRect.origin.y = viewSize.height - (caretRect.origin.y + caretRect.size.height + [UIFont smallSystemFontSize]);
        caretRect.size.height = 0;
    } else if (UIKeyboardWillHideNotification == type) {
		stappler::platform::ime::native::onKeyboardHide(aniDuration);
        IMELOG("UIKeyboardWillHideNotification %f", aniDuration);
    } else if (UIKeyboardDidHideNotification == type) {
        IMELOG("UIKeyboardDidHideNotification %f", aniDuration);
        caretRect = CGRectZero;
    }
}

- (void) layoutSubviews {
	CGSize size = [((CCEAGLView *)self.superview) surfaceSize];
	self.frame = CGRectMake(0, 0, size.width / self.contentScaleFactor, size.height / self.contentScaleFactor);
	[super layoutSubviews];
}

@end

@implementation SPTextPosition
@synthesize cursor=_cursor;

- (id) initWithCursor: (stappler::ime::Cursor) c {
	if (self = [super init]) {
		_cursor = c;
	}
	return self;
}

@end


@implementation SPTextRange
@synthesize cursor=_cursor;

- (id) initWithCursor: (stappler::ime::Cursor) c {
	if (self = [super init]) {
		_cursor = c;
	}
	return self;
}

- (BOOL) isEmpty {
	if (_cursor.length == 0) {
		return YES;
	}
	return NO;
}

- (SPTextPosition *) start {
	IMELOG("[SPTextRange start] %u %u", _cursor.start, _cursor.length);
	return [[SPTextPosition alloc] initWithCursor:stappler::ime::Cursor(_cursor.start, 0)];
}

- (SPTextPosition *) end {
	IMELOG("[SPTextRange end] %u %u", _cursor.start, _cursor.length);
	return [[SPTextPosition alloc] initWithCursor:stappler::ime::Cursor(_cursor.start + _cursor.length, 0)];
}

@end

namespace stappler::platform::ime {
	using InputType = stappler::ime::InputType;
	SPNativeInputView *_IMEView = nil;
	int32_t _inputFlags = 0;
	
	void updateViewFlags(SPNativeInputView *view, int32_t t) {
		UIReturnKeyType returnKey = UIReturnKeyDone;

		InputType type = InputType(t & toInt(InputType::ClassMask));
		switch (type) {
			case InputType::Date_Date:
			case InputType::Date_DateTime:
			case InputType::Date_Time:
				view.keyboardType = UIKeyboardTypeNumbersAndPunctuation;
				break;
			case InputType::Number_Numbers:
				view.keyboardType = UIKeyboardTypeNumberPad;
				break;
			case InputType::Number_Decimial:
			case InputType::Number_Signed:
				view.keyboardType = UIKeyboardTypeNumbersAndPunctuation;
				break;
			case InputType::Phone:
				view.keyboardType = UIKeyboardTypePhonePad;
				break;
			case InputType::Text_Text:
				view.keyboardType = UIKeyboardTypeDefault;
				break;
			case InputType::Text_Search:
				view.keyboardType = UIKeyboardTypeWebSearch;
				returnKey = UIReturnKeySearch;
				break;
			case InputType::Text_Punctuation:
				view.keyboardType = UIKeyboardTypeNumbersAndPunctuation;
				break;
			case InputType::Text_Email:
				view.keyboardType = UIKeyboardTypeEmailAddress;
				break;
			case InputType::Text_Url:
				view.keyboardType = UIKeyboardTypeURL;
				returnKey = UIReturnKeyGo;
				break;
			default:
				view.keyboardType = UIKeyboardTypeDefault;
				break;
		}
		
		if (t & toInt(InputType::AutoCorrectionBit)) {
			view.autocorrectionType = UITextAutocorrectionTypeYes;
		} else {
			view.autocorrectionType = UITextAutocorrectionTypeNo;
		}

		if (t & toInt(InputType::PasswordBit)) {
			view.spellCheckingType = UITextSpellCheckingTypeNo;
		} else {
			view.spellCheckingType = UITextSpellCheckingTypeDefault;
		}

		if (t & toInt(InputType::MultiLineBit)) {
			returnKey = UIReturnKeyDefault;
		} else {
			InputType ret = InputType(t & toInt(InputType::ReturnKeyMask));
			switch (ret) {
			case InputType::Empty: break;
			case InputType::ReturnKeyDefault: returnKey = UIReturnKeyDefault; break;
			case InputType::ReturnKeyGo: returnKey = UIReturnKeyGo; break;
			case InputType::ReturnKeyGoogle: returnKey = UIReturnKeyGoogle; break;
			case InputType::ReturnKeyJoin: returnKey = UIReturnKeyJoin; break;
			case InputType::ReturnKeyNext: returnKey = UIReturnKeyNext; break;
			case InputType::ReturnKeyRoute: returnKey = UIReturnKeyRoute; break;
			case InputType::ReturnKeySearch: returnKey = UIReturnKeySearch; break;
			case InputType::ReturnKeySend: returnKey = UIReturnKeySend; break;
			case InputType::ReturnKeyYahoo: returnKey = UIReturnKeyYahoo; break;
			case InputType::ReturnKeyDone: returnKey = UIReturnKeyDone; break;
			case InputType::ReturnKeyEmergencyCall: returnKey = UIReturnKeyEmergencyCall; break;
			default: returnKey = UIReturnKeyDefault; break;
			}
		}

		view.returnKeyType = returnKey;
	}

	void _updateCursor(uint32_t pos, uint32_t len) {
		[_IMEView setSelectedCursor:Cursor(pos, len)];
	}

	void _updateText(const std::u16string &str, uint32_t pos, uint32_t len, int32_t t) {
		IMELOG("_updateText");
		[_IMEView resignFirstResponder];
		updateViewFlags(_IMEView, t);
		_inputFlags = t;
		[_IMEView updateNativeString];
		[_IMEView becomeFirstResponder];
	}

	void _runWithText(const std::u16string &str, uint32_t pos, uint32_t len, int32_t t) {
		if (!_IMEView) {
			IMELOG("_runWithText");
			UIWindow *window = [UIApplication sharedApplication].keyWindow;
			UIViewController *rootViewController = window.rootViewController;
			UIView *view = rootViewController.view;

			CGRect frame = view.frame;
			frame.origin.x = 0.0f; frame.origin.y = 0.0f;

			_IMEView = [[SPNativeInputView alloc] initWithFrame:frame];
			[view addSubview:_IMEView];
			updateViewFlags(_IMEView, t);
			_inputFlags = t;
			[_IMEView updateNativeString];
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

#endif
#endif
