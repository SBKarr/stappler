/*
 * SPPathNode.h
 *
 *  Created on: 09 дек. 2014 г.
 *      Author: sbkarr
 */

#ifndef STAPPLER_SRC_DRAW_SPPATHNODE_H_
#define STAPPLER_SRC_DRAW_SPPATHNODE_H_

#include "SPDrawPath.h"
#include "SPDynamicSprite.h"
#include "SPDrawCanvas.h"

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

    virtual void acquireCache();
    virtual void releaseCache();

    virtual uint32_t getBaseWidth();
    virtual uint32_t getBaseHeight();

protected:
	friend class Path;

	virtual void updateCanvas();
    cairo_t *acquireDrawContext(uint32_t w, uint32_t h, Format fmt);
    cocos2d::Texture2D *generateTexture(cocos2d::Texture2D *tex);
    void releaseCanvasCache();

	bool _pathsDirty = true;
	bool _isAntialiased = false;
    cocos2d::Vector<Path *> _paths;

	uint32_t _baseWidth = 0;
	uint32_t _baseHeight = 0;
	Format _format = Format::A8;
	Rc<Canvas> _canvas;
	uint32_t _useCache = 0;
};

NS_SP_EXT_END(draw)

#endif /* STAPPLER_SRC_DRAW_SPPATHNODE_H_ */
