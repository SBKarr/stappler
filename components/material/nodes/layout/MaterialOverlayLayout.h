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

#ifndef MATERIAL_NODES_LAYOUT_MATERIALOVERLAYLAYOUT_H_
#define MATERIAL_NODES_LAYOUT_MATERIALOVERLAYLAYOUT_H_

#include "MaterialLayout.h"

NS_MD_BEGIN

class OverlayLayout : public Layout {
public:
	virtual bool init(MaterialNode *, const Size &, const Function<void()> &cb = nullptr);

	virtual void onContentSizeDirty() override;

	virtual MaterialNode *getNode() const;
	virtual const Size &getBoundSize() const;

	virtual void resize(const Size &, bool animated = false);

	virtual void setBackgroundColor(const Color &);
	virtual void setBackgroundOpacity(uint8_t);

	virtual void setKeyboardTracked(bool);
	virtual bool isKeyboardTracked() const;

	virtual void cancel();

	virtual void setOpenAnimation(const Vec2 &origin, const Function<void()> &);

	virtual void onPush(ContentLayer *l, bool replace) override;
	virtual void onPop(ContentLayer *l, bool replace) override;

	virtual Rc<Transition> getDefaultEnterTransition() const override;
	virtual Rc<Transition> getDefaultExitTransition() const override;

protected:
	virtual Size getActualSize(const Size &) const;
	virtual void onKeyboard(bool enabled, const Rect &, float duration);
	virtual void onResized();

	bool _trackKeyboard = false;
	bool _keyboardEnabled = false;
	Size _keyboardSize;
	Size _boundSize;
	Layer *_background = nullptr;
	MaterialNode *_node = nullptr;
	gesture::Listener *_listener = nullptr;
	EventListener *_keyboardEventListener = nullptr;

	bool _animationPending = false;
	Vec2 _animationOrigin;
	Function<void()> _animationCallback;
	Function<void()> _cancelCallback;
};

NS_MD_END

#endif /* MATERIAL_NODES_LAYOUT_MATERIALOVERLAYLAYOUT_H_ */
