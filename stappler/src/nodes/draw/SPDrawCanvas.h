/*
 * SPDrawPrivate.h
 *
 *  Created on: 13 апр. 2016 г.
 *      Author: sbkarr
 */

#ifndef STAPPLER_SRC_NODES_DRAW_SPDRAWCANVAS_H_
#define STAPPLER_SRC_NODES_DRAW_SPDRAWCANVAS_H_

#include "SPDraw.h"

NS_SP_EXT_BEGIN(draw)

class Canvas : public cocos2d::Ref {
public:
	virtual bool init();

	virtual void begin(cocos2d::Texture2D *, const Color4B &);
	virtual void end();

	virtual void scale(float sx, float sy);
	virtual void translate(float tx, float ty);

	virtual void save();
	virtual void restore();

	virtual void setAntialiasing(bool);
	virtual void setLineWidth(float);

	virtual void pathBegin();
	virtual void pathClose();
	virtual void pathMoveTo(float x, float y);
	virtual void pathLineTo(float x, float y);
	virtual void pathQuadTo(float x1, float y1, float x2, float y2);
	virtual void pathCubicTo(float x1, float y1, float x2, float y2, float x3, float y3);
	virtual void pathArcTo(float xc, float yc, float rx, float ry, float startAngle, float sweepAngle, float rotation);
	virtual void pathAltArcTo(float rx, float ry, float angle, bool largeArc, bool sweep, float x, float y);

	virtual void pathFill(const Color4B &);
	virtual void pathStroke(const Color4B &);
	virtual void pathFillStroke(const Color4B &fill, const Color4B &stroke);

protected:
	uint32_t _width = 0;
	uint32_t _height = 0;
};

NS_SP_EXT_END(draw)

#endif /* STAPPLER_SRC_NODES_DRAW_SPDRAWCANVAS_H_ */
