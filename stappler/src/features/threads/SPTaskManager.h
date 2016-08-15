//
//  SPTaskManager.h
//  chiertime-federal
//
//  Created by SBKarr on 6/20/13.
//
//

#ifndef __chiertime_federal__SPTaskManager__
#define __chiertime_federal__SPTaskManager__

#include "SPTask.h"
#include "SPThread.h"
#include "base/CCVector.h"

NS_SP_BEGIN

class TaskManager {
public:
    TaskManager(Thread *thread);
    TaskManager(const std::string &name, uint32_t count, uint32_t threadId);
    virtual ~TaskManager();

    TaskManager(const TaskManager &);
	TaskManager &operator=(const TaskManager &);

    TaskManager(TaskManager &&);
	TaskManager &operator=(TaskManager &&);

    virtual void perform(Rc<Task> &&task);
    virtual void perform(Rc<Task> &&task, int tag);

    virtual void performWithPriority(Rc<Task> &&task, bool performFirst);
    virtual void performWithPriority(Rc<Task> &&task, bool performFirst, int priority);
    virtual void performWithPriority(Rc<Task> &&task, bool performFirst, int priority, int tag);
protected:
	bool spawnWorkers();

	class TaskQueue;
	class Worker;

	uint32_t _maxWorkers = 1;
	uint32_t _threadId = std::numeric_limits<uint32_t>::max();
	std::vector<Worker *> _workers;
	TaskQueue *_queue = nullptr;
	std::string _name;
};

NS_SP_END

#endif /* defined(__chiertime_federal__SPTaskManager__) */
