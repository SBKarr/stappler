/**
 Copyright (c) 2020 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef COMPONENTS_XENOLITH_CORE_DIRECTOR_XLDIRECTOR_H_
#define COMPONENTS_XENOLITH_CORE_DIRECTOR_XLDIRECTOR_H_

#include "XLEventHeader.h"
#include "XLVk.h"

namespace stappler::xenolith {

class Director : public Ref {
public:
	static EventHeader onProjectionChanged;
	static EventHeader onAfterUpdate;
	static EventHeader onAfterVisit;
	static EventHeader onAfterDraw;

	enum class Projection {
		_2D,
		_3D,
		Euclid,
		Custom,
		Default = Euclid,
	};

	Director();

	virtual ~Director();
	virtual bool init();

	inline vk::View* getView() { return _view; }
	void setView(vk::View *view);

	bool mainLoop(double);

	void update(double);

	/* If waitUntilComplete is set - Director will wait for all tasks in queue (and all tasks, that will be spawned) to complete,
	 * if waitUntilComplete is not set - tasks `onComplete` callback may never be called, so custom data will never been freed  */
	void end();

protected:
	Rc<vk::View> _view;
};

}

#endif /* COMPONENTS_XENOLITH_CORE_DIRECTOR_XLDIRECTOR_H_ */
