/*
 * SPDrawPrivate.h
 *
 *  Created on: 13 апр. 2016 г.
 *      Author: sbkarr
 */

#ifndef STAPPLER_SRC_NODES_DRAW_SPDRAWCANVAS_H_
#define STAPPLER_SRC_NODES_DRAW_SPDRAWCANVAS_H_

#include "SPDraw.h"

typedef struct _cairo_surface cairo_surface_t;
typedef struct _cairo cairo_t;

NS_SP_EXT_BEGIN(draw)

class Canvas : public cocos2d::Ref {
public:
	Canvas();
	~Canvas();

	bool init(uint32_t width, uint32_t height, Format fmt);
	bool init(uint8_t *, uint32_t width, uint32_t height, uint32_t stride, Format fmt);

	cairo_t *getContext() const;

	cairo_t *acquireContext();
	void releaseContext();

	cocos2d::Texture2D * updateTexture(cocos2d::Texture2D *tex);
	cocos2d::Texture2D * generateTexture(cocos2d::Texture2D *tex);

	bool match(uint32_t w, uint32_t h, Format fmt) const;

	void clear();
	void clear(const cocos2d::Color4B &);

	uint8_t * data() const { return _data; }
	size_t size() const { return _dataLen; }

protected:
	bool _shouldFreeData = true;
	Format _format = Format::A8;

	uint32_t _width = 0;
	uint32_t _height = 0;
	uint32_t _stride = 0;

	size_t _dataLen = 0;
	uint8_t *_data = nullptr;

	uint32_t _canvasStackCount = 0;
	cairo_surface_t *_surface = nullptr;
	cairo_t *_context = nullptr;
};

NS_SP_EXT_END(draw)

#endif /* STAPPLER_SRC_NODES_DRAW_SPDRAWCANVAS_H_ */
