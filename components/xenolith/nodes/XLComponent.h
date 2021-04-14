/**
Copyright (c) 2021 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef COMPONENTS_XENOLITH_NODES_XLCOMPONENT_H_
#define COMPONENTS_XENOLITH_NODES_XLCOMPONENT_H_

#include "XLDefine.h"

namespace stappler::xenolith {

class Component : public Ref {
public:
	Component(void);
	virtual ~Component(void);
	virtual bool init();

	virtual void onAdded();
	virtual void onRemoved();

	virtual void onEnter();
	virtual void onExit();

    virtual void visit(RenderFrameInfo &, NodeFlags parentFlags);

	virtual void onContentSizeDirty();
	virtual void onTransformDirty();
	virtual void onReorderChildDirty();

	virtual bool isRunning() const;

	virtual bool isEnabled() const;
	virtual void setEnabled(bool b);

	void setOwner(Node *pOwner);
	Node* getOwner() const;

	void setTag(uint64_t);
	uint64_t getTag() const;

protected:
	Node *_owner = nullptr;
	bool _enabled = true;
	bool _running = false;
	uint64_t _tag = InvalidTag;
};

}

#endif /* COMPONENTS_XENOLITH_NODES_XLCOMPONENT_H_ */
