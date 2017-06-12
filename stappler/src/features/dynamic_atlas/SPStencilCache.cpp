// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2017 Roman Katuntsev <sbkarr@stappler.org>

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
#include "SPStencilCache.h"
#include "base/CCDirector.h"
#include "renderer/ccGLStateCache.h"
#include "renderer/CCRenderer.h"
#include "renderer/CCGLProgramCache.h"

NS_SP_BEGIN

static StencilCache *s_instance = nullptr;

static inline GLenum getEnumFromFunc(StencilCache::Func f) {
	switch (f) {
	case StencilCache::Func::Never: return GL_NEVER; break;
	case StencilCache::Func::Less: return GL_LESS; break;
	case StencilCache::Func::LessEqual: return GL_LEQUAL; break;
	case StencilCache::Func::Greater: return GL_GREATER; break;
	case StencilCache::Func::GreaterEqual: return GL_GEQUAL; break;
	case StencilCache::Func::Equal: return GL_EQUAL; break;
	case StencilCache::Func::NotEqual: return GL_NOTEQUAL; break;
	case StencilCache::Func::Always: return GL_ALWAYS; break;
	}
	return GL_ALWAYS;
}

static inline GLenum getEnumFromOp(StencilCache::Op f) {
	switch (f) {
	case StencilCache::Op::Keep: return GL_KEEP; break;
	case StencilCache::Op::Zero: return GL_ZERO; break;
	case StencilCache::Op::Replace: return GL_REPLACE; break;
	case StencilCache::Op::Incr: return GL_INCR; break;
	case StencilCache::Op::IncrWrap: return GL_INCR_WRAP; break;
	case StencilCache::Op::Decr: return GL_DECR; break;
	case StencilCache::Op::DecrWrap: return GL_DECR_WRAP; break;
	case StencilCache::Op::Invert: return GL_INVERT; break;
	}
	return GL_KEEP;
}

static inline StencilCache::Func getFuncFromEnum(GLenum f) {
	switch (f) {
	case GL_NEVER: return StencilCache::Func::Never; break;
	case GL_LESS: return StencilCache::Func::Less; break;
	case GL_LEQUAL: return StencilCache::Func::LessEqual; break;
	case GL_GREATER: return StencilCache::Func::Greater; break;
	case GL_GEQUAL: return StencilCache::Func::GreaterEqual; break;
	case GL_EQUAL: return StencilCache::Func::Equal; break;
	case GL_NOTEQUAL: return StencilCache::Func::NotEqual; break;
	case GL_ALWAYS: return StencilCache::Func::Always; break;
	}
	return StencilCache::Func::Always;
}

static inline StencilCache::Op getOpFromEnum(GLenum f) {
	switch (f) {
	case GL_KEEP: return StencilCache::Op::Keep; break;
	case GL_ZERO: return StencilCache::Op::Zero; break;
	case GL_REPLACE: return StencilCache::Op::Replace; break;
	case GL_INCR: return StencilCache::Op::Incr; break;
	case GL_INCR_WRAP: return StencilCache::Op::IncrWrap; break;
	case GL_DECR: return StencilCache::Op::Decr; break;
	case GL_DECR_WRAP: return StencilCache::Op::DecrWrap; break;
	case GL_INVERT: return StencilCache::Op::Invert; break;
	}
	return StencilCache::Op::Keep;
}

StencilCache *StencilCache::getInstance() {
	if (!s_instance) {
		s_instance = new StencilCache();
	}
	return s_instance;
}

void StencilCache::clear() {
	bool en = _enabled;

	auto state = saveState();
	enable(255);

	drawFullScreenQuadClearStencil(255, false);

	if (!en) {
		disable();
	}

	restoreState(state);
	_value = 0;
}

uint8_t StencilCache::pushStencilLayer() {
	_value += 1;
	enableStencilDrawing(_value, _currentState.writeMask);
	return _value;
}

void StencilCache::enableStencilDrawing(uint8_t value, uint8_t mask) {
	enable(_currentState.writeMask);

	setFunc(Func::Never, value, mask);
	setOp(Op::Replace, Op::Keep, Op::Keep);
}

void StencilCache::enableStencilTest(Func f, uint8_t value, uint8_t mask) {
	enable(_currentState.writeMask);

	setFunc(f, value, mask);
	setOp(Op::Keep, Op::Keep, Op::Keep);
}

void StencilCache::disableStencilTest(uint8_t mask) {
	enable(_currentState.writeMask);

	setFunc(Func::Always, 0, mask);
	setOp(Op::Keep, Op::Keep, Op::Keep);
}

bool StencilCache::isEnabled() const {
	return _enabled;
}

void StencilCache::enable(uint8_t mask) {
	if (!_enabled) {
		_currentState = _savedState = saveState();

		if (!_currentState.enabled) {
			glEnable(GL_STENCIL_TEST);
			_currentState.enabled = true;
		}
		_enabled = true;
	}

	if (mask != _currentState.writeMask) {
		glStencilMask(mask);
		_currentState.writeMask = mask;
	}
}

void StencilCache::disable() {
	if (_enabled) {
		restoreState(_savedState);
		_enabled = false;
	}
}

StencilCache::State StencilCache::saveState() const {
	State ret;

	ret.enabled = glIsEnabled(GL_STENCIL_TEST);

	GLint writeMask, func, ref, valueMask, sFail, dFail, dPass;

	glGetIntegerv(GL_STENCIL_WRITEMASK, (GLint *) &writeMask);
	glGetIntegerv(GL_STENCIL_FUNC, (GLint *) &func);
	glGetIntegerv(GL_STENCIL_REF, &ref);
	glGetIntegerv(GL_STENCIL_VALUE_MASK, (GLint *) &valueMask);
	glGetIntegerv(GL_STENCIL_FAIL, (GLint *) &sFail);
	glGetIntegerv(GL_STENCIL_PASS_DEPTH_FAIL, (GLint *) &dFail);
	glGetIntegerv(GL_STENCIL_PASS_DEPTH_PASS, (GLint *) &dPass);

	ret.writeMask = uint8_t(writeMask);
	ret.func = getFuncFromEnum(GLenum(func));
	ret.ref = uint8_t(ref);
	ret.valueMask = uint8_t(valueMask);
	ret.stencilFailOp = getOpFromEnum(GLenum(sFail));
	ret.depthFailOp = getOpFromEnum(GLenum(dFail));
	ret.passOp = getOpFromEnum(GLenum(dPass));

	return ret;
}

void StencilCache::restoreState(State state) const {
	glStencilFunc(getEnumFromFunc(state.func), state.ref, state.valueMask);
	glStencilOp(getEnumFromOp(state.stencilFailOp), getEnumFromOp(state.depthFailOp), getEnumFromOp(state.passOp));
	glStencilMask(state.writeMask);
	if (!state.enabled) {
		glDisable(GL_STENCIL_TEST);
	}
}

void StencilCache::drawFullScreenQuadClearStencil(uint8_t mask, bool inverted) {
	setFunc(Func::Never, mask, mask);
	setOp(inverted?Op::Replace:Op::Zero, Op::Keep, Op::Keep);

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

void StencilCache::setFunc(Func f, uint8_t value, uint8_t mask) {
	if (_currentState.func != f || _currentState.ref != value || _currentState.valueMask != mask) {
		_currentState.func = f;
		_currentState.ref = value;
		_currentState.valueMask = mask;

		glStencilFunc(getEnumFromFunc(_currentState.func), _currentState.ref, _currentState.valueMask);
	}
}
void StencilCache::setOp(Op sFail, Op dFail, Op dPass) {
	if (_currentState.stencilFailOp != sFail || _currentState.depthFailOp != dFail || _currentState.passOp != dPass) {
		_currentState.stencilFailOp = sFail;
		_currentState.depthFailOp = dFail;
		_currentState.passOp = dPass;

		glStencilOp(getEnumFromOp(sFail), getEnumFromOp(dFail), getEnumFromOp(dPass));
	}
}

NS_SP_END
