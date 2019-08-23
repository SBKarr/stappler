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

#ifndef LIBS_MATERIAL_NODES_LAYOUT_MATERIALFLEXIBLELAYOUT_H_
#define LIBS_MATERIAL_NODES_LAYOUT_MATERIALFLEXIBLELAYOUT_H_

#include "MaterialLayout.h"

NS_MD_BEGIN

class FlexibleLayout : public Layout {
public:
	using HeightFunction = Function<Pair<float, float>()>; // pair< min, max >

	virtual bool init() override;
	virtual void onContentSizeDirty() override;

	template <typename T, typename ... Args>
	auto setBaseNode(const Rc<T> &ptr, Args && ... args) -> T * {
		setBaseNode(ptr.get(), std::forward<Args>(args)...);
		return ptr.get();
	}

	template <typename T, typename ... Args>
	auto setFlexibleNode(const Rc<T> &ptr, Args && ... args) -> T * {
		setFlexibleNode(ptr.get(), std::forward<Args>(args)...);
		return ptr.get();
	}

	virtual void setBaseNode(ScrollView *node, int zOrder = 2);
	virtual void setFlexibleNode(cocos2d::Node *, int zOrder = 3);

	virtual void setFlexibleAutoComplete(bool value);

	virtual void setFlexibleMinHeight(float height);
	virtual float getFlexibleMinHeight() const;

	virtual void setFlexibleMaxHeight(float height);
	virtual float getFlexibleMaxHeight() const;

	virtual void setFlexibleHeightFunction(const HeightFunction &);
	virtual const HeightFunction &getFlexibleHeightFunction() const;

	virtual void setStatusBarTracked(bool value);
	virtual bool isStatusBarTracked() const;

	virtual void setStatusBarBackgroundColor(const Color &);
	virtual const Color3B &getStatusBarBackgroundColor() const;

	virtual float getFlexibleLevel() const;
	virtual void setFlexibleLevel(float value);
	virtual void setFlexibleLevelAnimated(float value, float duration);
	virtual void setFlexibleHeight(float value);

	virtual void setBaseNodePadding(float);
	virtual float getBaseNodePadding() const;

	float getCurrentFlexibleHeight() const;
	float getCurrentFlexibleMax() const;

	virtual void onPush(ContentLayer *l, bool replace) override;
	virtual void onForegroundTransitionBegan(ContentLayer *l, Layout *overlay) override;

	virtual void setKeyboardTracked(bool);
	virtual bool isKeyboardTracked() const;

	// Безопасный триггер препятствует анимации, пока достаточный объём скролла не пройден
	virtual void setSafeTrigger(bool value);
	virtual bool isSafeTrigger() const;

	virtual void expandFlexibleNode(float extraSpace, float animation = 0.0f);
	virtual void clearFlexibleExpand(float animation = 0.0f);

protected:
	float getStatusBarHeight() const;
	void onStatusBarHeight(float);

	virtual void updateFlexParams();
	virtual void onScroll(float delta, bool finished);
	virtual void onStatusBarNode(const node::Params &);
	virtual void onFlexibleNode(const node::Params &);
	virtual void onBaseNode(const node::Params &, const Padding &, float offset);

	static constexpr int AutoCompleteTag() { return 5; }

	virtual void onKeyboard(bool enabled, const Rect &, float duration);

	bool _flexibleAutoComplete = true;
	bool _flexibleBaseNode = true;
	bool _safeTrigger = true;

	float _flexibleLevel = 1.0f;
	float _flexibleMinHeight = 0.0f;
	float _flexibleMaxHeight = 0.0f;
	float _baseNodePadding = 4.0f;

	float _flexibleExtraSpace = 0.0f;

	HeightFunction _flexibleHeightFunction = nullptr;

	cocos2d::Node *_flexibleNode = nullptr;
	ScrollView *_baseNode = nullptr;

	stappler::Layer *_statusBar = nullptr;
	bool _statusBarTracked = false;
	float _statusBarHeight = nan();

	Size _keyboardSize;
	EventListener *_keyboardEventListener = nullptr;
	bool _trackKeyboard = false;
	bool _keyboardEnabled = false;
};

NS_MD_END

#endif /* LIBS_MATERIAL_NODES_LAYOUT_MATERIALFLEXIBLELAYOUT_H_ */
