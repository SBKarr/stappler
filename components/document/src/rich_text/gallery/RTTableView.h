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

#ifndef EXTENSIONS_DOCUMENT_SRC_RICH_TEXT_GALLERY_RTTABLEVIEW_H_
#define EXTENSIONS_DOCUMENT_SRC_RICH_TEXT_GALLERY_RTTABLEVIEW_H_

#include "RTCommonSource.h"
#include "MaterialToolbarLayout.h"
#include "RTTooltip.h"

NS_RT_BEGIN

class TableView : public material::ToolbarLayout {
public:
	virtual ~TableView();
	virtual bool init(CommonSource *, const MediaParameters &, const StringView &id);

	virtual void onContentSizeDirty() override;
	virtual void onEnter() override;

	virtual void close();

protected:
	virtual void acquireImageAsset(const StringView &);

	virtual void onImage(cocos2d::Texture2D *);
	virtual void onAssetCaptured(const StringView &);

	uint32_t _min = 0;
	uint32_t _max = 0;
	String _src;
	material::ImageLayer *_sprite = nullptr;
	Rc<CommonSource> _source;
	MediaParameters _media;
};

NS_RT_END

#endif /* EXTENSIONS_DOCUMENT_SRC_RICH_TEXT_GALLERY_RTTABLEVIEW_H_ */
