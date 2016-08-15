//
//  SPExecutionManager.h
//  chiertime-federal
//
//  Created by SBKarr on 6/23/13.
//
//

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
