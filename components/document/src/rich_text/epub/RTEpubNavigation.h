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

#ifndef CLASSES_LAYOUTS_FILES_FILEEPUBNAVIGATION_H_
#define CLASSES_LAYOUTS_FILES_FILEEPUBNAVIGATION_H_

#include "RTCommon.h"
#include "MaterialNode.h"

NS_RT_BEGIN

class EpubNavigation : public cocos2d::Node {
public:
	using Callback = std::function<void(float)>;

	virtual bool init(const Callback & = nullptr);
	virtual void onContentSizeDirty() override;

	virtual void setView(View *view);
	virtual void setReaderProgress(float p);
	virtual void onResult(layout::Result *);

	virtual void open();
	virtual void close();

	virtual void clearBookmarks();
	virtual void addBookmark(float start, float end);

protected:
	virtual void onButton();

	virtual void onOpenProgress(float val);
	virtual void onManualProgress(float p, float a = 0.0f);

	virtual float getProgressForPoint(const Vec2 &) const;
	virtual void propagateProgress(float value) const;

	class Scroll;
	class Bookmarks;

	View *_view = nullptr;
	Scroll *_scroll = nullptr;

	material::ButtonIcon *_icon = nullptr;
	material::RoundedProgress *_progress = nullptr;
	Bookmarks *_bookmarks = nullptr;

	float _openProgress = 0.0f;
	gesture::Listener *_listener = nullptr;
	bool _touchCaptured = false;
	bool _autoHideView = false;

	float _readerProgress = 0.0f;
	Callback _callback = nullptr;
};

NS_RT_END

#endif /* CLASSES_LAYOUTS_FILES_FILEEPUBNAVIGATION_H_ */
