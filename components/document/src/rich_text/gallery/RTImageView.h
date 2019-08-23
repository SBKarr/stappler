/**
Copyright (c) 2017 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef LIBS_MATERIAL_GUI_RICHTEXTVIEW_MATERIALRICHTEXTIMAGEVIEW_H_
#define LIBS_MATERIAL_GUI_RICHTEXTVIEW_MATERIALRICHTEXTIMAGEVIEW_H_

#include "RTCommonSource.h"
#include "MaterialLayout.h"
#include "RTTooltip.h"

NS_RT_BEGIN

class ImageView : public material::Layout {
public:
	virtual ~ImageView();
	virtual bool init(CommonSource *, const StringView &id, const StringView &src, const StringView &alt = StringView());

	virtual void onContentSizeDirty() override;
	virtual void onEnter() override;

	virtual void close();

protected:
	virtual void acquireImageAsset(const StringView &);

	virtual Rc<Tooltip> constructTooltip(CommonSource *, const Vector<String> &) const;
	virtual bool isSourceValid(CommonSource *, const StringView & src) const;

	virtual void onExpand();
	virtual Rc<cocos2d::Texture2D> readImage(const StringView &);
	virtual void onImage(cocos2d::Texture2D *);
	virtual void onAssetCaptured(const StringView &);

	String _src;
	Tooltip *_tooltip = nullptr;
	material::MenuSourceButton *_expandButton = nullptr;
	bool _expanded = true;

	material::ImageLayer *_sprite = nullptr;
	Rc<CommonSource> _source;
};

NS_RT_END

#endif /* LIBS_MATERIAL_GUI_RICHTEXTVIEW_MATERIALRICHTEXTIMAGEVIEW_H_ */
