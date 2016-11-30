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
