/****************************************************************************
Copyright (c) 2013-2014 Chukong Technologies Inc.

http://www.cocos2d-x.org

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
****************************************************************************/

#ifndef __CC_FRAMEWORK_COMPONENT_H__
#define __CC_FRAMEWORK_COMPONENT_H__

#include "base/CCRef.h"
#include "math/CCMath.h"
#include <string>
#include <vector>

NS_CC_BEGIN

class Node;
class Renderer;

class CC_DLL Component : public Ref {
public:
	using ZPath = std::vector<int>;

	Component(void);
	virtual ~Component(void);
	virtual bool init();

	virtual void onAdded();
	virtual void onRemoved();

	virtual void onEnter();
	virtual void onEnterTransitionDidFinish();
	virtual void onExitTransitionDidStart();
	virtual void onExit();

	virtual void onVisit(Renderer *renderer, const Mat4& parentTransform, uint32_t parentFlags, const ZPath &);

	virtual void onContentSizeDirty();
	virtual void onTransformDirty();
	virtual void onReorderChildDirty();

	virtual bool isRunning() const;

	virtual bool isEnabled() const;
	virtual void setEnabled(bool b);

	void setOwner(Node *pOwner);
	Node* getOwner() const;

	void setTag(uint32_t);
	uint32_t getTag() const;

protected:
	Node *_owner = nullptr;
	bool _enabled = true;
	bool _running = false;
	uint32_t _tag = stappler::maxOf<uint32_t>();
};

NS_CC_END

#endif  // __FUNDATION__CCCOMPONENT_H__
