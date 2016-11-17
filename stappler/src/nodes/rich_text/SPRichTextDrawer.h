/*
 * SPRichTextDrawer.h
 *
 *  Created on: 03 авг. 2015 г.
 *      Author: sbkarr
 */

#ifndef LIBS_STAPPLER_NODES_RICH_TEXT_SPRICHTEXTDRAWER_H_
#define LIBS_STAPPLER_NODES_RICH_TEXT_SPRICHTEXTDRAWER_H_

#include "SPRichTextLayout.h"
#include "SPRichTextSource.h"
#include "SPRichTextResult.h"
#include "SPThread.h"
#include "SPDraw.h"

NS_SP_EXT_BEGIN(rich_text)

class Drawer : public cocos2d::Ref {
public:
	using ObjectVec = std::vector<Object>;
	using Callback = std::function<void(cocos2d::Texture2D *)>;
	using Font = font::Source;

	static Thread &thread();

	bool init();
	void free();

	// draw normal texture
	bool draw(Source *, Result *, const Rect &, const Callback &, cocos2d::Ref *);

	// draw thumbnail texture, where scale < 1.0f - resample coef
	bool thumbnail(Source *, Result *, const Rect &, float, const Callback &, cocos2d::Ref *);

	void update();
	void clearCache();

public:
	cocos2d::Texture2D *getBitmap(const String &);
	void addBitmap(const String &str, cocos2d::Texture2D *bmp);
	void performUpdate();

	bool begin(cocos2d::Texture2D *, const Color4B &);
	void end();

	void cleanup();

	void setColor(const Color4B &);
	void setLineWidth(float);

	void drawRectangle(const Rect &, draw::Style);
	void drawRectangleFill(const Rect &);
	void drawRectangleOutline(const Rect &);
	void drawRectangleOutline(const Rect &, bool top, bool right, bool bottom, bool left);

	void drawTexture(const Rect &bbox, cocos2d::Texture2D *, const Rect &texRect);

	void drawCharRects(Font *, const font::FormatSpec &, const Rect & bbox, float scale);
	void drawChars(Font *, const font::FormatSpec &, const Rect & bbox);
	void drawRects(const Rect &, const Vector<Rect> &);

	void drawCharsQuads(const Vector<Rc<cocos2d::Texture2D>> &, Vector<Rc<DynamicQuadArray>> &&, const Rect & bbox);

protected:
	void drawCharsEffects(Font *, const font::FormatSpec &, const Rect & bbox);
	void drawCharsQuads(cocos2d::Texture2D *, DynamicQuadArray *, const Rect & bbox);

	void bindTexture(GLuint);
	void useProgram(GLuint);
	void enableVertexAttribs(uint32_t);
	void blendFunc(GLenum sfactor, GLenum dfactor);
	void blendFunc(const cocos2d::BlendFunc &);

	void drawResizeBuffer(size_t count);

	Mat4 _projection;

	GLuint _fbo = 0;
	uint32_t _width = 0;
	uint32_t _height = 0;

	GLuint _drawBufferVBO[2] = { 0, 0 };
	size_t _drawBufferSize = 0;

	Color4B _color = Color4B(0, 0, 0, 0);
	float _lineWidth = 1.0f;

	uint32_t _attributeFlags = 0;

	GLuint _currentTexture = 0;
	GLuint _currentProgram = 0;

	GLenum _blendingSource;
	GLenum _blendingDest;

	Time _updated;
	bool _cacheUpdated = false;
	Map<String, Pair<Rc<cocos2d::Texture2D>, Time>> _cache;
};

NS_SP_EXT_END(rich_text)

#endif /* LIBS_STAPPLER_NODES_RICH_TEXT_SPRICHTEXTDRAWER_H_ */
