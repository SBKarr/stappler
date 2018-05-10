// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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

#include "SPDrawCanvas.h"
#include "SPDefine.h"
#include "SPThreadManager.h"
#include "SPTextureCache.h"
#include "renderer/CCTexture2D.h"
#include "renderer/ccGLStateCache.h"
#include "renderer/CCRenderer.h"
#include "2d/CCNode.h"
#include "base/CCConfiguration.h"
#include "base/CCDirector.h"

NS_SP_EXT_BEGIN(draw)

Canvas::~Canvas() { }

bool Canvas::init(StencilDepthFormat fmt) {
	if (!layout::Canvas::init()) {
		return false;
	}

	return initializeSurface(fmt);
}

bool Canvas::begin(cocos2d::Texture2D *tex, const Color4B &color, bool clear) {
	if (GLRenderSurface::begin(tex, color, clear)) {
		_transform = Mat4::IDENTITY;
		_line.reserve(256);

		_subAccum.clear();
		_tessAccum.clear();
		_glAccum.clear();
		_contourVertex = 0;
		_fillVertex = 0;

		return true;
	}

	return false;
}

void Canvas::end() {
	GLRenderSurface::end();
}

void Canvas::flush() {
	flushVectorCanvas(this);
}

Rc<cocos2d::Texture2D> Canvas::captureContents(cocos2d::Node *node, Format fmt, float density) {
	auto size = node->getContentSize() * density;
	if (size.equals(Size::ZERO)) {
		return nullptr;
	}

	auto director = cocos2d::Director::getInstance();
	auto renderer = director->getRenderer();

	renderer->clearDrawStats();

	auto tex = Rc<cocos2d::Texture2D>::create(fmt, int(floorf(size.width)), int(floorf(size.height)), cocos2d::Texture2D::RenderTarget);
	if (begin(tex, Color4B::BLACK)) {
		director->pushMatrix(cocos2d::MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW);
		director->pushMatrix(cocos2d::MATRIX_STACK_TYPE::MATRIX_STACK_PROJECTION);
		director->loadMatrix(cocos2d::MATRIX_STACK_TYPE::MATRIX_STACK_PROJECTION, _viewTransform);

		Vector<int> newPath;
		node->visit(renderer, Mat4::IDENTITY, 0, newPath);

		renderer->render();

		director->popMatrix(cocos2d::MATRIX_STACK_TYPE::MATRIX_STACK_PROJECTION);
		director->popMatrix(cocos2d::MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW);
		end();
	}

	return tex;
}

Bitmap Canvas::captureTexture(cocos2d::Texture2D *tex) {
	if (begin(tex, Color4B::BLACK, false)) {
		auto ret = read();

		end();

		return ret;
	}

	return Bitmap();
}

void Canvas::drop() {
	GLRenderSurface::dropSurface();
}

NS_SP_EXT_END(draw)
