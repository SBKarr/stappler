/**
Copyright (c) 2016-2017 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef LIBS_STAPPLER_NODES_SPCLIPPINGNODE_H_
#define LIBS_STAPPLER_NODES_SPCLIPPINGNODE_H_

#include "SPDefine.h"
#include "2d/CCNode.h"
#include "platform/CCGL.h"
#include "renderer/CCGroupCommand.h"
#include "renderer/CCCustomCommand.h"

NS_SP_BEGIN

class ClippingNode : public cocos2d::Node {
public:
	virtual bool init(Node *stencil = nullptr);

	virtual void setEnabled(bool value);
	virtual bool isEnabled() const;

	void setStencil(Node *stencil);
	Node* getStencil() const;

	void setInverted(bool inverted);
	bool isInverted() const;

	virtual void visit(cocos2d::Renderer *, const Mat4 &, uint32_t, ZPath &zPath) override;

	virtual void onEnter() override;
	virtual void onEnterTransitionDidFinish() override;
	virtual void onExitTransitionDidStart() override;
	virtual void onExit() override;

	virtual void setCameraMask(unsigned short mask, bool applyChildren = true) override;

protected:
	void drawFullScreenQuadClearStencil();
	void onBeforeVisit();
	void onAfterDrawStencil();
	void onAfterVisit();

	bool _enabled = true;
	Rc<Node> _stencil;
	bool _inverted = false;

	GLboolean _currentStencilEnabled = GL_FALSE;
	GLuint _currentStencilWriteMask = ~0;
	GLenum _currentStencilFunc = GL_ALWAYS;
	GLint _currentStencilRef = 0;
	GLuint _currentStencilValueMask = ~0;
	GLenum _currentStencilFail = GL_KEEP;
	GLenum _currentStencilPassDepthFail = GL_KEEP;
	GLenum _currentStencilPassDepthPass = GL_KEEP;
	GLboolean _currentDepthWriteMask = GL_TRUE;

	GLint _mask_layer_le;

	cocos2d::GroupCommand _groupCommand;
	cocos2d::GroupCommand _stencilGroupCommand;
	cocos2d::CustomCommand _beforeVisitCmd;
	cocos2d::CustomCommand _afterDrawStencilCmd;
	cocos2d::CustomCommand _afterVisitCmd;
};

NS_SP_END

#endif /* LIBS_STAPPLER_NODES_SPCLIPPINGNODE_H_ */
