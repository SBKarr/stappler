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

#ifndef STAPPLER_SRC_DRAW_SPPATHNODE_H_
#define STAPPLER_SRC_DRAW_SPPATHNODE_H_

#include "SPDrawCanvas.h"
#include "SPDynamicSprite.h"

NS_SP_EXT_BEGIN(draw)

using Path = layout::Path;
using Style = layout::DrawStyle;
using Image = layout::Image;

class PathNode : public DynamicSprite {
public:
	virtual ~PathNode();

	virtual bool init(Format fmt);

	/** Init with coordinate system (0 - width, 0 - height) */
	virtual bool init(uint16_t width, uint16_t height, Format fmt = Format::A8);
	virtual bool init(layout::Image * = nullptr, Format fmt = Format::A8);

    virtual void visit(cocos2d::Renderer *, const Mat4 &, uint32_t f, ZPath &) override;

    virtual Image::PathRef addPath();
    virtual Image::PathRef addPath(const Path & path);
    virtual Image::PathRef addPath(Path && path);

    virtual Image::PathRef getPath(const StringView &);

    virtual void removePath(const Image::PathRef &);
    virtual void removePath(const StringView &);

    virtual bool clear();

    virtual void setAntialiased(bool value);
    virtual bool isAntialiased() const;

    virtual void setImage(Image *);
    virtual Image * getImage() const;

    virtual uint32_t getBaseWidth();
    virtual uint32_t getBaseHeight();

protected:
	virtual void updateCanvas(layout::Subscription::Flags f);

	Rc<cocos2d::Texture2D> generateTexture(cocos2d::Texture2D *tex, uint32_t w, uint32_t h, Format fmt);

	bool _pathsDirty = false;
	uint16_t _baseWidth = 0;
	uint16_t _baseHeight = 0;
	Format _format = Format::A8;
	Rc<Canvas> _canvas;
	data::Listener<Image> _image;
};

NS_SP_EXT_END(draw)

#endif /* STAPPLER_SRC_DRAW_SPPATHNODE_H_ */
