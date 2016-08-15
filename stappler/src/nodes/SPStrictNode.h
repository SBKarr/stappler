//
//  SPStrictNode.h
//  stappler
//
//  Created by SBKarr on 11/29/13.
//  Copyright (c) 2013 SBKarr. All rights reserved.
//

#ifndef __stappler__SPStrictNode__
#define __stappler__SPStrictNode__

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

#endif /* defined(__stappler__SPStrictNode__) */
