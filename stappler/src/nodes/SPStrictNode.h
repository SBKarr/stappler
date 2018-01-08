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

#ifndef STAPPLER_SRC_NODES_SPSTRICTNODE_H_
#define STAPPLER_SRC_NODES_SPSTRICTNODE_H_

#include "SPDefine.h"
#include "2d/CCNode.h"
#include "renderer/CCGroupCommand.h"
#include "renderer/CCCustomCommand.h"

NS_SP_BEGIN

class StrictNode : public cocos2d::Node {
public:
	virtual bool init() override;
    virtual void visit(cocos2d::Renderer *, const Mat4 &, uint32_t, ZPath &) override;

	virtual bool isClippingEnabled();
	virtual void setClippingEnabled(bool value);

	virtual void setClippingOutline(float top, float left, float bottom, float right);
	virtual void setClippingOutlineTop(float value);
	virtual void setClippingOutlineLeft(float value);
	virtual void setClippingOutlineBottom(float value);
	virtual void setClippingOutlineRight(float value);

	virtual void setClipOnlyNodesBelow(bool value);

protected:
	virtual cocos2d::Rect getViewRect();
    virtual void onBeforeDraw();
    virtual void onAfterDraw();

	cocos2d::Rect _parentScissorRect;
    bool _scissorRestored = false;
	bool _isClippingEnabled = true;
	bool _clipOnlyNodesBelow = false;

	float _outlineTop = 0.0f;
	float _outlineLeft = 0.0f;
	float _outlineBottom = 0.0f;
	float _outlineRight = 0.0f;

    cocos2d::CustomCommand _beforeVisitCmd;
    cocos2d::CustomCommand _afterVisitCmd;
    cocos2d::GroupCommand _groupCommand;
};

/* this node (and it's children) will ignore scissor rules */
class AlwaysDrawedNode : public cocos2d::Node {
public:
	virtual bool init() override;
    virtual void visit(cocos2d::Renderer *renderer, const Mat4 &, uint32_t, ZPath &) override;

protected:
    virtual void onBeforeDraw();
    virtual void onAfterDraw();

    cocos2d::CustomCommand _beforeVisitCmd;
    cocos2d::CustomCommand _afterVisitCmd;
    cocos2d::GroupCommand _groupCommand;
	cocos2d::Rect _scissorRect;
	bool _scissorEnabled = false;
};

NS_SP_END

#endif
