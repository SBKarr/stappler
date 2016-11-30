/**
Copyright (c) 2016 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef __chiertime_federal__SPExecutionStack__
#define __chiertime_federal__SPExecutionStack__

#include "SPTask.h"

NS_SP_BEGIN

/*
 Task stack used to execute functions on single thread
 */

class TaskStack {
protected:
	using Callback = std::function<void()>;

    std::mutex _mutex;

	std::vector<Rc<Task>> _stack;
    std::atomic_flag _flag;
    
public:
    TaskStack();
    void update(float dt);
    void push(Rc<Task> &&task);
	void push(const Callback &func, cocos2d::Ref *target = nullptr);
};

NS_SP_END

#endif /* defined(__chiertime_federal__SPExecutionManager__) */
