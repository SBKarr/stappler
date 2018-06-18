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

#ifndef LIBS_MATERIAL_NODES_SCENE_MATERIALCONTENTLAYER_H_
#define LIBS_MATERIAL_NODES_SCENE_MATERIALCONTENTLAYER_H_

#include "MaterialOverlayLayout.h"
#include "2d/CCActionInterval.h"

NS_MD_BEGIN

class ContentLayer : public cocos2d::Node {
public:
	using Transition = cocos2d::FiniteTimeAction;

	virtual bool init() override;
	virtual void onContentSizeDirty() override;

	// replaced node will be alone in stack, so, no need for exit transition
	virtual void replaceNode(Layout *, Transition *enter = nullptr);
	virtual void pushNode(Layout *, Transition *enter = nullptr, Transition *exit = nullptr);

	virtual void replaceTopNode(Layout *, Transition *enter = nullptr, Transition *exit = nullptr);
	virtual void popNode(Layout *);

	virtual bool pushOverlayNode(OverlayLayout *, Transition *enter = nullptr, Transition *exit = nullptr);
	virtual bool popOverlayNode(OverlayLayout *);

	virtual Layout *getRunningNode();
	virtual Layout *getPrevNode();

	virtual bool popLastNode();
	virtual bool isActive() const;

	virtual bool onBackButton();
	virtual size_t getNodesCount() const;

	virtual const Vector<Rc<Layout>> &getLayouts() const;

protected:
	virtual void pushNodeInternal(Layout *, Transition *enter, Transition *exit, const Function<void()> &cb);

	virtual void eraseNode(Layout *);
	virtual void eraseOverlay(OverlayLayout *);
	virtual void replaceNodes();
	virtual void updateNodesVisibility();

	Vector<Rc<Layout>> _nodes;
	Map<Layout *, Rc<Transition>> _nodeExit;

	Vector<Rc<OverlayLayout>> _overlays;
	Map<OverlayLayout *, Rc<Transition>> _overlayExit;
};

NS_MD_END

#endif /* LIBS_MATERIAL_NODES_SCENE_MATERIALCONTENTLAYER_H_ */
