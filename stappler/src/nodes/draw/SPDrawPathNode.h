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

#include "SPDrawPath.h"
#include "SPDynamicSprite.h"
#include "SPDrawCanvasNanoVG.h"

NS_SP_EXT_BEGIN(draw)

class PathNode : public DynamicSprite {
public:
	virtual ~PathNode();

	virtual bool init(uint32_t width, uint32_t height, Format fmt = Format::A8);
    virtual void visit(cocos2d::Renderer *renderer, const cocos2d::Mat4 &parentTransform, uint32_t parentFlags, ZPath &zPath) override;

    virtual void addPath(Path *path);
    virtual void removePath(Path *path);
    virtual void clear();

    virtual void setAntialiased(bool value);
    virtual bool isAntialiased() const;

    const cocos2d::Vector<Path *> &getPaths() const;

    virtual uint32_t getBaseWidth();
    virtual uint32_t getBaseHeight();

protected:
	friend class Path;

	virtual void updateCanvas();
	Rc<cocos2d::Texture2D> generateTexture(cocos2d::Texture2D *tex, uint32_t w, uint32_t h, Format fmt);

	bool _pathsDirty = true;
	bool _isAntialiased = false;
    cocos2d::Vector<Path *> _paths;

	uint32_t _baseWidth = 0;
	uint32_t _baseHeight = 0;
	Format _format = Format::A8;
	Rc<Canvas> _canvas;
};

NS_SP_EXT_END(draw)

#endif /* STAPPLER_SRC_DRAW_SPPATHNODE_H_ */
