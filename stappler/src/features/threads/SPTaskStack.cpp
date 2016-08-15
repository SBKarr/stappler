//
//  SPExecutionManager.cpp
//  chiertime-federal
//
//  Created by SBKarr on 6/23/13.
//
//

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
	push(Rc<Task>::create([func] (Ref *obj, bool success) {
		if (success) { func(); }
	}, target));
}
