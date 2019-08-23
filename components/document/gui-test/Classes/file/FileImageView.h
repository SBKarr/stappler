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

#ifndef CLASSES_LAYOUTS_FILES_FILEIMAGEVIEW_H_
#define CLASSES_LAYOUTS_FILES_FILEIMAGEVIEW_H_

#include "MaterialLayout.h"
#include "SPDataSource.h"
#include "SPFont.h"

NS_SP_EXT_BEGIN(app)

class FileImageView : public material::Layout {
public:
	virtual bool init(const StringView &src);

	virtual void onContentSizeDirty() override;

	virtual void refresh();
	virtual void close();

protected:
	virtual void onImage(cocos2d::Texture2D *);

	virtual void onFadeOut();
	virtual void onFadeIn();

	String _filePath;
	material::Toolbar *_toolbar = nullptr;
	material::ImageLayer *_sprite = nullptr;
	draw::PathNode *_vectorImage = nullptr;
};

NS_SP_EXT_END(app)

#endif /* CLASSES_LAYOUTS_FILES_FILEIMAGEVIEW_H_ */
