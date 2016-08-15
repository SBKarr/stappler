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

NS_SP_EXT_BEGIN(rich_text)

class Drawer : public cocos2d::Ref {
public:
	using ObjectVec = std::vector<Object>;
	using Callback = std::function<void(cocos2d::Texture2D *)>;

	static Thread &thread();

	Drawer();
	~Drawer();

	// draw normal texture
	bool init(Source *, Result *, const Rect &, const Callback &, cocos2d::Ref *);

	// draw thumbnail texture, where scale < 1.0f - resample coef
	bool init(Source *, Result *, const Rect &, float, const Callback &, cocos2d::Ref *);

protected:
	void onAssetCaptured();
	void draw(uint8_t *) const;
	void onDrawed(uint8_t *);

	Rect _rect;
	float _scale = 1.0f;
	bool _isThumbnail = false;

	uint16_t _width = 0;
	uint16_t _height = 0;
	uint16_t _stride = 0;

	Rc<Result> _result;
	Rc<Source> _source;

	Callback _callback = nullptr;
	Rc<cocos2d::Ref> _ref = nullptr;
};

NS_SP_EXT_END(rich_text)

#endif /* LIBS_STAPPLER_NODES_RICH_TEXT_SPRICHTEXTDRAWER_H_ */
