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

#ifndef STAPPLER_SRC_NODES_DRAW_SPDRAWCANVAS_H_
#define STAPPLER_SRC_NODES_DRAW_SPDRAWCANVAS_H_

#include "SPDrawGLRenderSurface.h"
#include "renderer/CCTexture2D.h"
#include "SPBuffer.h"
#include "SLImage.h"
#include "SLCanvas.h"

NS_SP_EXT_BEGIN(draw)

using Format = cocos2d::Texture2D::PixelFormat;

class Canvas : public layout::Canvas, protected GLRenderSurface {
public:
	using Path = layout::Path;

	virtual ~Canvas();

	virtual bool init(StencilDepthFormat fmt = StencilDepthFormat::Stencil8);

	bool begin(cocos2d::Texture2D *, const Color4B &, bool clear = true);
	void end();

	virtual void flush() override;

	Rc<cocos2d::Texture2D> captureImage(const layout::Image &, Format = Format::A8);
	Rc<cocos2d::Texture2D> captureContents(cocos2d::Node *, Format, float density = 0.0f);
	Bitmap captureTexture(cocos2d::Texture2D *);

	void drop();

	using GLRenderSurface::read;

protected:
	void doSafeClear(const Color4B &color);
	bool doUpdateAttachments(cocos2d::Texture2D *tex, uint32_t w, uint32_t h);

	TimeInterval _tessAccum;
	TimeInterval _glAccum;
	uint32_t _contourVertex;
	uint32_t _fillVertex;
};

NS_SP_EXT_END(draw)

#endif /* STAPPLER_SRC_NODES_DRAW_SPDRAWCANVASLIBTESS2_H_ */
