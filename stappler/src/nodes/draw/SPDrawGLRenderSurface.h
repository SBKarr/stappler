/**
Copyright (c) 2018 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef STAPPLER_SRC_NODES_DRAW_SPDRAWGLRENDERSURFACE_H_
#define STAPPLER_SRC_NODES_DRAW_SPDRAWGLRENDERSURFACE_H_

#include "SPDrawGLCacheNode.h"
#include "SPEventHandler.h"
#include "renderer/CCTexture2D.h"

NS_SP_EXT_BEGIN(draw)

class GLRenderSurface : public GLCacheNode, private EventHandler {
public:
	enum class StencilDepthFormat {
		None,
		Depth16,
		Depth24Stencil8,
		Stencil8,
	};

	GLRenderSurface();
	virtual ~GLRenderSurface();

	bool initializeSurface(StencilDepthFormat fmt = StencilDepthFormat::Stencil8);
	void finalizeSurface();
	void dropSurface(); // use drop if graphic context was garbage-collected (on Android)

	virtual void resetSurface();

	bool begin(cocos2d::Texture2D *, const Color4B &color, bool clear);
	void end();

	void flushVectorCanvas(layout::Canvas *);

	bool isValid() const;

	Bitmap read(uint32_t x, uint32_t y, uint32_t w, uint32_t h);
	Bitmap read();

protected:
	void setUniformColor(const Color4B &);
	void setUniformTransform(const Mat4 &);

	void setFillBlending();
	void setStrokeBlending();

	void doSafeClear(const Color4B &color);
	bool doUpdateAttachments(cocos2d::Texture2D *tex, uint32_t w, uint32_t h);

	bool _valid = false;

	StencilDepthFormat _depthFormat = StencilDepthFormat::Stencil8;
	GLbitfield _clearFlags = 0;
	GLuint _fbo = 0;
	GLuint _rbo = 0;
	GLint _oldFbo = 0;

	// 0 - vertexes
	// 1 - fixed quad indexes
	// 2 - free indexes
	GLuint _vbo[3] = { 0, 0, 0 };

	uint32_t _width = 0;
	uint32_t _height = 0;

	Mat4 _viewTransform;
	cocos2d::GLProgram *_drawProgram = nullptr;

	bool _premultipliedAlpha = false;
	cocos2d::Texture2D::PixelFormat _internalFormat = cocos2d::Texture2D::PixelFormat::NONE;
	cocos2d::Texture2D::PixelFormat _referenceFormat = cocos2d::Texture2D::PixelFormat::NONE;

	Mat4 _uniformTransform;
	Color4B _uniformColor;
};

NS_SP_EXT_END(draw)

#endif /* STAPPLER_SRC_NODES_DRAW_SPDRAWGLRENDERSURFACE_H_ */
