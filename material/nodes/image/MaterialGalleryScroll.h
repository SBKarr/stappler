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

#ifndef CLASSES_GUI_GALLERY_GALLERYSCROLL_H_
#define CLASSES_GUI_GALLERY_GALLERYSCROLL_H_

#include "MaterialImageLayer.h"

NS_MD_BEGIN

class GalleryImage : public ImageLayer {
public:
	using TextureCallback = Function<void(cocos2d::Texture2D *)>;
	using ImageCallback = Function<void(const String &, const TextureCallback &)>;

	virtual bool init(const String &, const ImageCallback &);
	virtual void onContentSizeDirty() override;
	virtual void onEnter() override;

	virtual void setLoaderColor(const Color &);

protected:
	Time _textureTime;

	IconSprite *_loader = nullptr;
};

class GalleryScroll : public cocos2d::Node {
public:
	enum Action {
		Tap,
		Swipe,
		Pinch,
		DoubleTap
	};

	using ActionCallback = Function<void(Action)>;
	using TextureCallback = GalleryImage::TextureCallback;
	using ImageCallback = GalleryImage::ImageCallback;
	using PositionCallback = Function<void(float)>;

	virtual bool init(const ImageCallback &cb, const Vector<String> &, size_t selected);
	virtual void onContentSizeDirty() override;

	virtual void setActionCallback(const ActionCallback &);
	virtual const ActionCallback & getActionCallback() const;

	virtual void setPositionCallback(const PositionCallback &);
	virtual const PositionCallback & getPositionCallback() const;

	virtual void setLoaderColor(const Color &);
	virtual const Color &getLoaderColor() const;

protected:
	virtual void reset(size_t id);

	virtual bool onTap(const Vec2 &point, int count);

	virtual bool onSwipeBegin(const Vec2 &point);
	virtual bool onSwipe(const Vec2 &delta);
	virtual bool onSwipeEnd(const Vec2 &velocity);

	virtual bool onPinch(const Vec2 &point, float scale, float velocity, bool isEnded);

	virtual void onMove(float value);
	virtual void onMoveEnd(float value);
	virtual void setProgress(float value);

	virtual void onOverscrollLeft(float);
	virtual void onOverscrollRight(float);
	virtual void onOverscrollTop(float);
	virtual void onOverscrollBottom(float);

	bool _hasPinch = false;
	float _progress = 0.0f; // (-1.0f; 1.0f)
	Vector<String> _images;

	size_t _primaryId = 0;
	size_t _secondaryId = 0;

	GalleryImage *_primary = nullptr;
	GalleryImage *_secondary = nullptr;

	Overscroll *_overscrollTop = nullptr;
	Overscroll *_overscrollBottom = nullptr;
	Overscroll *_overscrollLeft = nullptr;
	Overscroll *_overscrollRight = nullptr;

	cocos2d::Component *_gestureListener = nullptr;

	ActionCallback _actionCallback;
	PositionCallback _positionCallback;
	ImageCallback _imageCallback;

	Color _loaderColor = Color::Black;
};

NS_MD_END

#endif /* CLASSES_GUI_GALLERY_GALLERYSCROLL_H_ */
