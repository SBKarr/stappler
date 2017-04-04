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

#ifndef MATERIAL_NODES_SCROLL_MATERIALRECYCLERSCROLL_H_
#define MATERIAL_NODES_SCROLL_MATERIALRECYCLERSCROLL_H_

#include "MaterialScroll.h"

NS_MD_BEGIN

class RecyclerHolder : public cocos2d::Node {
public:
	virtual bool init(const Function<void()> &cb);
	virtual void onContentSizeDirty() override;

	virtual void setEnabled(bool value);
	virtual void setInverted(bool value);

	virtual void setPlaceholderText(const String &value);
	virtual void setPlaceholderButtonText(const String &value);
	virtual void setPlaceholderTextColor(const Color &color);
	virtual void setPlaceholderBackgroundColor(const Color &color);

protected:
	virtual void updateProgress();
	virtual void showEnabled();
	virtual void showDisabled();

	float _progress = 0.0f;
	bool _enabled = false;
	bool _inverted = false;

	Layer *_layer = nullptr;
	IconSprite *_icon = nullptr;
	Label *_label = nullptr;
	ButtonLabel *_button = nullptr;
};

class RecyclerNode : public MaterialNode {
public:
	virtual bool init(RecyclerScroll *scroll, MaterialNode *node, Scroll::Item *item, data::Source::Id id);
	virtual void onContentSizeDirty() override;

	virtual bool isRemoved() const;
	Scroll::Item *getItem() const;
	MaterialNode *getNode() const;

	virtual void setPlaceholderText(const String &value);
	virtual void setPlaceholderButtonText(const String &value);
	virtual void setPlaceholderTextColor(const Color &color);
	virtual void setPlaceholderBackgroundColor(const Color &color);

protected:
	enum State {
		Enabled,
		Prepared,
		Removed,
	};

	virtual void updateProgress();
	virtual bool onSwipeBegin();
	virtual bool onSwipe(float d, float v);
	virtual bool onSwipeEnd(float velocity);
	virtual void onPrepared();
	virtual void onRemoved();
	virtual void onRestore();

	State  _state = Enabled;
	Rc<Scroll::Item> _item;
	float _enabledProgress = 0.0f;
	float _preparedProgress = 0.0f;

	Layer *_layer = nullptr;
	RecyclerHolder *_holder = nullptr;
	MaterialNode *_node = nullptr;
	gesture::Listener *_listener = nullptr;
	RecyclerScroll *_scroll = nullptr;
};

class RecyclerScroll : public Scroll {
public:
	virtual bool init(Source *dataCategory = nullptr);

	virtual void setPlaceholderText(const String &);
	virtual const String & getPlaceholderText() const;

	virtual void setPlaceholderButtonText(const String &);
	virtual const String & getPlaceholderButtonText() const;

	virtual void setPlaceholderTextColor(const Color &);
	virtual const Color & getPlaceholderTextColor() const;

	virtual void setPlaceholderBackgroundColor(const Color &);
	virtual const Color & getPlaceholderBackgroundColor() const;

protected:
	friend class RecyclerHolder;
	friend class RecyclerNode;

	virtual void onSwipeBegin() override;

	virtual Rc<MaterialNode> onItemRequest(const ScrollController::Item &, Source::Id) override;
	virtual ScrollController::Item * getItemForNode(MaterialNode *) const override;

	virtual void removeRecyclerNode(Item *, cocos2d::Node *);
	virtual void scheduleCleanup();
	virtual void unscheduleCleanup();
	virtual void performCleanup();

	virtual void afterCleanup();
	virtual void updateNodes();
	virtual void onItemsRemoved(const Vector<Rc<Item>> &);

	String _placeholderText;
	String _placeholderButton;
	Color _placeholderTextColor;
	Color _placeholderBackgroundColor;
};

NS_MD_END

#endif /* MATERIAL_NODES_SCROLL_MATERIALRECYCLERSCROLL_H_ */
