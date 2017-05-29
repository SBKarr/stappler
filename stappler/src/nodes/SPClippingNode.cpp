// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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

#include "SPDefine.h"
#include "SPClippingNode.h"
#include "base/CCDirector.h"
#include "renderer/CCGLProgramCache.h"
#include "renderer/ccGLStateCache.h"

NS_SP_BEGIN

static GLint g_sStencilBits = -1;
// store the current stencil layer (position in the stencil buffer),
// this will allow nesting up to n ClippingNode,
// where n is the number of bits of the stencil buffer.
static GLint s_layer = -1;

bool ClippingNode::init(Node *stencil) {
	if (!Node::init()) {
		return false;
	}

	_stencil = stencil;
    _inverted = false;

    // get (only once) the number of bits of the stencil buffer
	static bool once = true;
	if (once) {
		glGetIntegerv(GL_STENCIL_BITS, &g_sStencilBits);
		if (g_sStencilBits <= 0) {
			log::text("ClippingNode", "Stencil buffer is not enabled.");
		}
		once = false;
	}

	setCascadeOpacityEnabled(true);

	return true;
}

void ClippingNode::setEnabled(bool value) {
	_enabled = value;
}
bool ClippingNode::isEnabled() const {
	return _enabled;
}

void ClippingNode::onEnter() {
	Node::onEnter();

	if (_stencil) {
		_stencil->onEnter();
	} else {
		log::text("ClippingNode", "ClippingNode warning: _stencil is nil.");
	}
}

void ClippingNode::onEnterTransitionDidFinish() {
	Node::onEnterTransitionDidFinish();

	if (_stencil) {
		_stencil->onEnterTransitionDidFinish();
	}
}

void ClippingNode::onExitTransitionDidStart() {
	if (_stencil) {
		_stencil->onExitTransitionDidStart();
	}

	Node::onExitTransitionDidStart();
}

void ClippingNode::onExit() {
	if (_stencil) {
		_stencil->onExit();
	}

	Node::onExit();
}

void ClippingNode::drawFullScreenQuadClearStencil() {
	auto director = cocos2d::Director::getInstance();

	director->pushMatrix(cocos2d::MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW);
	director->loadIdentityMatrix(cocos2d::MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW);

	director->pushMatrix(cocos2d::MATRIX_STACK_TYPE::MATRIX_STACK_PROJECTION);
	director->loadIdentityMatrix(cocos2d::MATRIX_STACK_TYPE::MATRIX_STACK_PROJECTION);

	Vec2 vertices[] = { Vec2(-1.0f, -1.0f), Vec2(1.0f, -1.0f), Vec2(1.0f, 1.0f), Vec2(-1.0f, 1.0f) };

	auto glProgram = cocos2d::GLProgramCache::getInstance()->getGLProgram(
			cocos2d::GLProgram::SHADER_NAME_POSITION_U_COLOR);

	int colorLocation = glProgram->getUniformLocation("u_color");
	CHECK_GL_ERROR_DEBUG();

	Color4F color(1, 1, 1, 1);

	glProgram->use();
	glProgram->setUniformsForBuiltins();
	glProgram->setUniformLocationWith4fv(colorLocation, (GLfloat*) &color.r, 1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	cocos2d::GL::enableVertexAttribs(cocos2d::GL::VERTEX_ATTRIB_FLAG_POSITION);
	glVertexAttribPointer(cocos2d::GLProgram::VERTEX_ATTRIB_POSITION, 2, GL_FLOAT, GL_FALSE, 0, vertices);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	CHECK_GL_ERROR_DEBUG();

	CC_INCREMENT_GL_DRAWN_BATCHES_AND_VERTICES(1, 4);

	director->popMatrix(cocos2d::MATRIX_STACK_TYPE::MATRIX_STACK_PROJECTION);
	director->popMatrix(cocos2d::MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW);
}

void ClippingNode::visit(Renderer *renderer, const Mat4 &parentTransform, uint32_t parentFlags, ZPath &zPath) {
	if (!_visible) {
		return;
	}

	if (!_enabled) {
		cocos2d::Node::visit(renderer, parentTransform, parentFlags, zPath);
		return;
	}

	uint32_t flags = processParentFlags(parentTransform, parentFlags);

	// IMPORTANT:
	// To ease the migration to v3.0, we still support the Mat4 stack,
	// but it is deprecated and your code should not rely on it
	auto director = cocos2d::Director::getInstance();
	CCASSERT(nullptr != director, "Director is null when seting matrix stack");
	director->pushMatrix(cocos2d::MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW);
	director->loadMatrix(cocos2d::MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW, _modelViewTransform);

	//Add group command
	zPath.push_back(getLocalZOrder());

	_beforeVisitCmd.init(_globalZOrder, zPath);
	_beforeVisitCmd.func = CC_CALLBACK_0(ClippingNode::onBeforeVisit, this);
	renderer->addCommand(&_beforeVisitCmd);

	_stencilGroupCommand.init(_globalZOrder, zPath);
	renderer->addCommand(&_stencilGroupCommand);

	renderer->pushGroup(_stencilGroupCommand.getRenderQueueID());

	_stencil->visit(renderer, _modelViewTransform, flags, zPath);

	renderer->popGroup();

	_afterDrawStencilCmd.init(_globalZOrder, zPath);
	_afterDrawStencilCmd.func = CC_CALLBACK_0(ClippingNode::onAfterDrawStencil, this);
	renderer->addCommand(&_afterDrawStencilCmd);

	_groupCommand.init(_globalZOrder, zPath);
	renderer->addCommand(&_groupCommand);

	renderer->pushGroup(_groupCommand.getRenderQueueID());
	int i = 0;
	bool visibleByCamera = isVisitableByVisitingCamera();

	if (!_children.empty()) {
		sortAllChildren();
		// draw children zOrder < 0
		for (; i < _children.size(); i++) {
			auto node = _children.at(i);

			if (node && node->getLocalZOrder() < 0) {
				node->visit(renderer, _modelViewTransform, flags, zPath);
			} else {
				break;
			}
		}
		// self draw
		if (visibleByCamera) {
			this->draw(renderer, _modelViewTransform, flags, zPath);
		}

		for (auto it = _children.cbegin() + i; it != _children.cend(); ++it) {
			(*it)->visit(renderer, _modelViewTransform, flags, zPath);
		}
	} else if (visibleByCamera) {
		this->draw(renderer, _modelViewTransform, flags, zPath);
	}

	renderer->popGroup();
	_afterVisitCmd.init(_globalZOrder, zPath);
	_afterVisitCmd.func = CC_CALLBACK_0(ClippingNode::onAfterVisit, this);
	renderer->addCommand(&_afterVisitCmd);

	zPath.pop_back();
	director->popMatrix(cocos2d::MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW);
}

void ClippingNode::setCameraMask(unsigned short mask, bool applyChildren) {
	Node::setCameraMask(mask, applyChildren);

	if (_stencil) {
		_stencil->setCameraMask(mask, applyChildren);
	}
}

cocos2d::Node* ClippingNode::getStencil() const {
	return _stencil;
}

void ClippingNode::setStencil(Node *stencil) {
	_stencil = stencil;
}

bool ClippingNode::isInverted() const {
	return _inverted;
}

void ClippingNode::setInverted(bool inverted) {
	_inverted = inverted;
}

void ClippingNode::onBeforeVisit() {
	s_layer++;

	GLint mask_layer = 0x1 << s_layer;
	GLint mask_layer_l = mask_layer - 1;
	_mask_layer_le = mask_layer | mask_layer_l;

	// manually save the stencil state
	_currentStencilEnabled = glIsEnabled(GL_STENCIL_TEST);
	glGetIntegerv(GL_STENCIL_WRITEMASK, (GLint *) &_currentStencilWriteMask);
	glGetIntegerv(GL_STENCIL_FUNC, (GLint *) &_currentStencilFunc);
	glGetIntegerv(GL_STENCIL_REF, &_currentStencilRef);
	glGetIntegerv(GL_STENCIL_VALUE_MASK, (GLint *) &_currentStencilValueMask);
	glGetIntegerv(GL_STENCIL_FAIL, (GLint *) &_currentStencilFail);
	glGetIntegerv(GL_STENCIL_PASS_DEPTH_FAIL, (GLint *) &_currentStencilPassDepthFail);
	glGetIntegerv(GL_STENCIL_PASS_DEPTH_PASS, (GLint *) &_currentStencilPassDepthPass);

	glEnable(GL_STENCIL_TEST);
	CHECK_GL_ERROR_DEBUG();

	glStencilMask(mask_layer);
	glGetBooleanv(GL_DEPTH_WRITEMASK, &_currentDepthWriteMask);

	// disable depth test while drawing the stencil
	//glDisable(GL_DEPTH_TEST);
	// disable update to the depth buffer while drawing the stencil,
	// as the stencil is not meant to be rendered in the real scene,
	// it should never prevent something else to be drawn,
	// only disabling depth buffer update should do
	glDepthMask(GL_FALSE);

	///////////////////////////////////
	// CLEAR STENCIL BUFFER

	// manually clear the stencil buffer by drawing a fullscreen rectangle on it
	// setup the stencil test func like this:
	// for each pixel in the fullscreen rectangle
	//     never draw it into the frame buffer
	//     if not in inverted mode: set the current layer value to 0 in the stencil buffer
	//     if in inverted mode: set the current layer value to 1 in the stencil buffer
	glStencilFunc(GL_NEVER, mask_layer, mask_layer);
	glStencilOp(!_inverted ? GL_ZERO : GL_REPLACE, GL_KEEP, GL_KEEP);

	// draw a fullscreen solid rectangle to clear the stencil buffer
	//ccDrawSolidRect(Vec2::ZERO, ccpFromSize([[Director sharedDirector] winSize]), Color4F(1, 1, 1, 1));
	drawFullScreenQuadClearStencil();

	///////////////////////////////////
	// DRAW CLIPPING STENCIL

	// setup the stencil test func like this:
	// for each pixel in the stencil node
	//     never draw it into the frame buffer
	//     if not in inverted mode: set the current layer value to 1 in the stencil buffer
	//     if in inverted mode: set the current layer value to 0 in the stencil buffer
	glStencilFunc(GL_NEVER, mask_layer, mask_layer);
	glStencilOp(!_inverted ? GL_REPLACE : GL_ZERO, GL_KEEP, GL_KEEP);

	// enable alpha test only if the alpha threshold < 1,
	// indeed if alpha threshold == 1, every pixel will be drawn anyways

	//Draw _stencil
}

void ClippingNode::onAfterDrawStencil() {
	// restore the depth test state
	glDepthMask(_currentDepthWriteMask);
	//if (currentDepthTestEnabled) {
	//    glEnable(GL_DEPTH_TEST);
	//}

	///////////////////////////////////
	// DRAW CONTENT

	// setup the stencil test func like this:
	// for each pixel of this node and its childs
	//     if all layers less than or equals to the current are set to 1 in the stencil buffer
	//         draw the pixel and keep the current layer in the stencil buffer
	//     else
	//         do not draw the pixel but keep the current layer in the stencil buffer
	glStencilFunc(GL_EQUAL, _mask_layer_le, _mask_layer_le);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	// draw (according to the stencil test func) this node and its childs
}

void ClippingNode::onAfterVisit() {
	///////////////////////////////////
	// CLEANUP

	// manually restore the stencil state
	glStencilFunc(_currentStencilFunc, _currentStencilRef, _currentStencilValueMask);
	glStencilOp(_currentStencilFail, _currentStencilPassDepthFail, _currentStencilPassDepthPass);
	glStencilMask(_currentStencilWriteMask);
	if (!_currentStencilEnabled) {
		glDisable(GL_STENCIL_TEST);
	}

	// we are done using this layer, decrement
	s_layer--;
}

NS_SP_END
