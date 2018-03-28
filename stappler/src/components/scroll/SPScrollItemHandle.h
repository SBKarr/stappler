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

#ifndef STAPPLER_SRC_COMPONENTS_SCROLL_SPSCROLLITEMHANDLE_H_
#define STAPPLER_SRC_COMPONENTS_SCROLL_SPSCROLLITEMHANDLE_H_

#include "SPScrollController.h"
#include "2d/CCComponent.h"
#include "2d/CCNode.h"
#include "base/CCVector.h"

NS_SP_BEGIN

class ScrollItemHandle : public cocos2d::Component {
public:
	using Item = ScrollController::Item;
	using Callback = Function<void(const Item &)>;

	virtual ~ScrollItemHandle();

	virtual void onNodeInserted(ScrollController *, Item &, size_t index);
	virtual void onNodeUpdated(ScrollController *, Item &, size_t index);
	virtual void onNodeRemoved(ScrollController *, Item &, size_t index);

	void setInsertCallback(const Callback &);
	void setUpdateCallback(const Callback &);
	void setRemoveCallback(const Callback &);

	void resize(float newSize, bool forward = true);

	void setLocked(bool);
	bool isLocked() const;

protected:
	ScrollController *_controller = nullptr;
	size_t _itemIndex = 0;

	Callback _insertCallback;
	Callback _updateCallback;
	Callback _removeCallback;
	bool _isLocked = false;
};

NS_SP_END

#endif /* STAPPLER_SRC_COMPONENTS_SCROLL_SPSCROLLITEMHANDLE_H_ */
