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

#ifndef EXTENSIONS_DOCUMENT_SRC_RICH_TEXT_COMMON_RTDRAWERREQUEST_H_
#define EXTENSIONS_DOCUMENT_SRC_RICH_TEXT_COMMON_RTDRAWERREQUEST_H_

#include "RTCommon.h"
#include "RTCommonSource.h"

NS_RT_BEGIN

class Request : public cocos2d::Ref {
public:
	using Callback = std::function<void(cocos2d::Texture2D *)>;

	static Rc<cocos2d::Texture2D> make(Drawer *, CommonSource *, Result *);

	// draw normal texture
	bool init(Drawer *, CommonSource *, Result *, const Rect &, const Callback &, cocos2d::Ref *);

	// draw thumbnail texture, where scale < 1.0f - resample coef
	bool init(Drawer *, CommonSource *, Result *, const Rect &, float, const Callback &, cocos2d::Ref *);

	void run();

	Rc<cocos2d::Texture2D> makeTexture();

protected:
	void onAssetCaptured();

	void prepareBackgroundImage(const Rect &bbox, const Background &bg);

	void draw(cocos2d::Texture2D *);
	void onDrawed(cocos2d::Texture2D *);

	Rect getRect(const Rect &) const;

	void drawRef(const Rect &bbox, const Link &);
	void drawPath(const Rect &bbox, const layout::Path &);
	void drawRectangle(const Rect &bbox, const Color4B &);
	void drawBitmap(const Rect &bbox, cocos2d::Texture2D *bmp, const Background &bg, const Size &box);
	void drawFillerImage(const Rect &bbox, const Background &bg);
	void drawBackgroundImage(const Rect &bbox, const Background &bg);
	void drawBackgroundColor(const Rect &bbox, const Background &bg);
	void drawBackground(const Rect &bbox, const Background &bg);
	void drawLabel(const Rect &bbox, const Label &l);
	void drawObject(const Object &obj);

	Rect _rect;
	float _scale = 1.0f;
	float _density = 1.0f;
	bool _isThumbnail = false;
	bool _highlightRefs = false;

	uint16_t _width = 0;
	uint16_t _height = 0;
	uint16_t _stride = 0;

	Rc<Drawer> _drawer;
	Rc<Result> _result;
	Rc<CommonSource> _source;
	Rc<FontSource> _font;

	Map<String, CommonSource::AssetData> _networkAssets;

	Callback _callback = nullptr;
	Rc<cocos2d::Ref> _ref = nullptr;
};

NS_RT_END

#endif /* EXTENSIONS_DOCUMENT_SRC_RICH_TEXT_COMMON_RTDRAWERREQUEST_H_ */
