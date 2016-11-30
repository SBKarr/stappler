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

#ifndef LIBS_MATERIAL_NODES_LAYOUT_MATERIALSIDEBARLAYOUT_H_
#define LIBS_MATERIAL_NODES_LAYOUT_MATERIALSIDEBARLAYOUT_H_

#include "MaterialLayout.h"

NS_MD_BEGIN

class SidebarLayout : public Layout {
public:
	enum Position {
		Left,
		Right
	};

	using WidthCallback = std::function<float(const cocos2d::Size &)>;
	using BoolCallback = std::function<void(bool)>;

	virtual bool init(Position);
	virtual void onContentSizeDirty() override;

	virtual void setNode(MaterialNode *m, int zOrder = 0);
	virtual MaterialNode *getNode() const;

	virtual void setNodeWidth(float);
	virtual float getNodeWidth() const;

	virtual void setNodeWidthCallback(const WidthCallback &);
	virtual const WidthCallback &getNodeWidthCallback() const;

	virtual void setSwallowTouches(bool);
	virtual bool isSwallowTouches() const;

	virtual void setEdgeSwipeEnabled(bool);
	virtual bool isEdgeSwipeEnabled() const;

	virtual void setBackgroundColor(const Color &);
	virtual void setBackgroundActiveOpacity(uint8_t);
	virtual void setBackgroundPassiveOpacity(uint8_t);

	virtual void show();
	virtual void hide(float factor = 1.0f);

	virtual float getProgress() const;

	virtual bool isNodeVisible() const;
	virtual bool isNodeEnabled() const;

	virtual void setNodeVisibleCallback(const BoolCallback &);
	virtual void setNodeEnabledCallback(const BoolCallback &);

protected:
	virtual void setProgress(float);

	virtual void onSwipeDelta(float);
	virtual void onSwipeFinished(float);

	virtual void onNodeEnabled(bool value);
	virtual void onNodeVisible(bool value);

	virtual void stopNodeActions();

	WidthCallback _widthCallback;
	float _nodeWidth = 0.0f;

	Position _position;
	bool _swallowTouches = true;
	bool _edgeSwipeEnabled = true;
	uint8_t _backgroundActiveOpacity = 64;
	uint8_t _backgroundPassiveOpacity = 0;

	gesture::Listener *_listener;

	Layer *_background = nullptr;
	MaterialNode *_node = nullptr;

	BoolCallback _visibleCallback = nullptr;
	BoolCallback _enabledCallback = nullptr;
};

NS_MD_END

#endif /* LIBS_MATERIAL_NODES_LAYOUT_MATERIALSIDEBARLAYOUT_H_ */
