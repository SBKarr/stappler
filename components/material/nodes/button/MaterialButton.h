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

#ifndef CLASSES_MATERIAL_NODES_MATERIALBUTTON_H_
#define CLASSES_MATERIAL_NODES_MATERIALBUTTON_H_

#include "MaterialNode.h"
#include "MaterialLabel.h"
#include "MaterialMenuSource.h"
#include "SPDrawPathNode.h"

NS_MD_BEGIN

class ButtonHighLight : public Node {
public:
	virtual bool init(Node *stencil, const Function<void()> &b, const Function<void()> &e, const Function<void(float)> &);
	virtual void onContentSizeDirty() override;

	virtual void animateSelection(const Vec2 &);
	virtual void animateDeselection();
	virtual void dropSpawn();

	virtual void setSelected(bool value, bool instant = false);
	virtual bool isSelected() const;

	virtual void setSpawnDelay(float);
	virtual void setAnimationColor(Color);
	virtual void setAnimationOpacity(uint8_t);

	virtual void setClipNode(Node *, const Function<void(Size)> &);

protected:
	virtual void beginSelection();
	virtual void updateSelection(float progress, uint8_t op);
	virtual void endSelection();
	virtual float getSelectionProgress() const;

	virtual draw::Image::PathRef beginSpawn();
	virtual void updateSpawn(float progress, draw::Image::PathRef & path, const Size &size, const Vec2 &point);
	virtual void endSpawn();

	ClippingNode *_clip = nullptr;
	RoundedSprite *_background = nullptr;

	draw::PathNode *_animationNode = nullptr;
	uint32_t _animationCount = 0;
	uint8_t _animationOpacity = 22;
	float _animationProgress = 0.0f;
	Color _animationColor = Color::Black;
	float _spawnDelay = 0.0f;
	bool _selected = false;
	bool _highlighted = false;

	Function<void()> _beginCallback;
	Function<void()> _endCallback;
	Function<void(float)> _progressCallback;
	Function<void(Size)> _sizeCallback;
};

class Button : public MaterialNode {
public:
	using TapCallback = Function<void()>;
	using TouchFilter = Function<bool(const Vec2 &)>;

	enum Style {
		Raised,
		FlatWhite,
		FlatBlack,
	};

	virtual ~Button() { }

	virtual bool init(const TapCallback &tapCallback = nullptr, const TapCallback &longTapCallback = nullptr);
	virtual void onContentSizeDirty() override;

	virtual void setStyle(Style style);
	virtual Style getStyle() const;

	virtual void setPriority(int32_t value);
	virtual int32_t getPriority() const;

	virtual void setTouchPadding(float);
	virtual float getTouchPadding() const;

	virtual void setTapCallback(const TapCallback & tapCallback);
	virtual void setLongTapCallback(const TapCallback & longTapCallback);
	virtual void setTouchFilter(const TouchFilter &);

	virtual void setSwallowTouches(bool value);
	virtual void setGesturePriority(int32_t);
	virtual void setAnimationOpacity(uint8_t);
	virtual void setSpawnDelay(float);

	virtual bool onPressBegin(const Vec2 &location);
    virtual void onLongPress();
	virtual bool onPressEnd();
	virtual void onPressCancel();

	virtual void setEnabled(bool value);
	virtual bool isEnabled() const;

	virtual void setSelected(bool value, bool instant = false);
	virtual bool isSelected() const;

	virtual void setRaisedDefaultZIndex(float value);
	virtual void setRaisedActiveZIndex(float value);

	virtual void setMenuSource(MenuSource *);
	virtual MenuSource * getMenuSource() const;

	virtual void setMenuSourceButton(MenuSourceButton *);
	virtual MenuSourceButton *getMenuSourceButton() const;

	virtual void setFormController(FormController *);
	virtual FormController *getFormController() const;

	virtual const Vec2 & getTouchPoint() const;

	virtual void cancel();

	virtual gesture::Listener * getListener() const;
	virtual ButtonHighLight *getHighlight() const;

protected:
	virtual void updateStyle();
	virtual void updateEnabled();

	virtual void updateFromSource();

	virtual void onLightLevel() override;

	virtual void onOpenMenuSource();

	TapCallback _tapCallback = nullptr;
	TapCallback _longTapCallback = nullptr;
	TouchFilter _touchFilter = nullptr;
	gesture::Listener *_listener = nullptr;

	ButtonHighLight *_highlight = nullptr;

	Vec2 _touchPoint;

	Style _style = FlatBlack;
	bool _enabled = true;

	float _raisedDefaultZIndex = 1.5f;
	float _raisedActiveZIndex = 3.0f;

	Rc<MenuSource>_floatingMenuSource;
	data::Listener<MenuSourceButton> _source;

	EventListener *_formEventListener = nullptr;
	FormController *_formController = nullptr;
};

NS_MD_END

#endif /* CLASSES_MATERIAL_NODES_MATERIALBUTTON_H_ */
