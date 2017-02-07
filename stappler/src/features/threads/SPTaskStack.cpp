// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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

#include "SPDefine.h"
#include "SPTaskStack.h"
#include "SPPlatform.h"

USING_NS_SP;

TaskStack::TaskStack() : _flag() {
	_flag.test_and_set();
}

void TaskStack::update(float dt) {
    if (!_flag.test_and_set()) {
        return;
    }

	_mutex.lock();

	std::vector<Rc<Task>> stack;
	stack.swap(_stack);

	_mutex.unlock();

    if (stack.empty()) {
        return;
    }

    for (auto task : stack) {
		task->onComplete();
    }
}

void TaskStack::push(Rc<Task> &&task) {
    if (!task) {
        return;
    }

	_mutex.lock();
	_stack.push_back(std::move(task));
	_mutex.unlock();
	_flag.clear();

	platform::render::_requestRender();
}

void TaskStack::push(const Callback & func, Ref *target) {
	push(Rc<Task>::create([func] (const Task &, bool success) {
		if (success) { func(); }
	}, target));
}
