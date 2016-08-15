/*
 * GalleryScroll.h
 *
 *  Created on: 9 февр. 2016 г.
 *      Author: sbkarr
 */

#ifndef CLASSES_GUI_GALLERY_GALLERYSCROLL_H_
#define CLASSES_GUI_GALLERY_GALLERYSCROLL_H_

#include "MaterialImageLayer.h"

NS_MD_BEGIN

class GalleryImage : public ImageLayer {
public:
	using TextureCallback = std::function<void(cocos2d::Texture2D *)>;
	using ImageCallback = std::function<void(const std::string &, const TextureCallback &)>;

	virtual bool init(const std::string &, const ImageCallback &);
	virtual void onContentSizeDirty() override;
	virtual void onEnter() override;

protected:
	Time _textureTime;
	material::DynamicIcon *_loader = nullptr;
};

class GalleryScroll : public cocos2d::Node {
public:
	enum Action {
		Tap,
		Swipe,
		Pinch,
		DoubleTap
	};

	using ActionCallback = std::function<void(Action)>;
	using TextureCallback = std::function<void(cocos2d::Texture2D *)>;
	using ImageCallback = std::function<void(const std::string &, const TextureCallback &)>;
	using PositionCallback = std::function<void(float)>;

	virtual bool init(const ImageCallback &cb, const std::vector<std::string> &, size_t selected);
	virtual void onContentSizeDirty() override;

	virtual void setActionCallback(const ActionCallback &);
	virtual const ActionCallback & getActionCallback() const;

	virtual void setPositionCallback(const PositionCallback &);
	virtual const PositionCallback & getPositionCallback() const;

protected:
	virtual void reset(size_t id);

	virtual bool onTap(const cocos2d::Vec2 &point, int count);

	virtual bool onSwipeBegin(const cocos2d::Vec2 &point);
	virtual bool onSwipe(const cocos2d::Vec2 &delta);
	virtual bool onSwipeEnd(const cocos2d::Vec2 &velocity);

	virtual bool onPinch(const cocos2d::Vec2 &point, float scale, float velocity, bool isEnded);

	virtual void onMove(float value);
	virtual void onMoveEnd(float value);
	virtual void setProgress(float value);

	virtual void onOverscrollLeft(float);
	virtual void onOverscrollRight(float);
	virtual void onOverscrollTop(float);
	virtual void onOverscrollBottom(float);

	bool _hasPinch = false;
	float _progress = 0.0f; // (-1.0f; 1.0f)
	std::vector<std::string> _images;

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
};

NS_MD_END

#endif /* CLASSES_GUI_GALLERY_GALLERYSCROLL_H_ */
