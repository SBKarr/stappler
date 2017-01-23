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
	virtual bool init(const Size &);

	virtual void onContentSizeDirty() override;

	virtual void pushNode(MaterialNode *);
	virtual MaterialNode *getNode() const;

	virtual void setBoundSize(const Size &);
	virtual const Size &getBoundSize() const;

	virtual void setKeyboardTracked(bool);
	virtual bool isKeyboardTracked() const;

	virtual void cancel();

protected:
	virtual void onKeyboard(bool enabled, const Rect &, float duration);

	bool _trackKeyboard = false;
	bool _keyboardEnabled = false;
	Size _keyboardSize;
	Size _boundSize;
	Layer *_background = nullptr;
	MaterialNode *_node = nullptr;
	gesture::Listener *_listener = nullptr;
	EventListener *_keyboardEventListener = nullptr;
};

NS_MD_END

#endif /* MATERIAL_NODES_LAYOUT_MATERIALOVERLAYLAYOUT_H_ */
