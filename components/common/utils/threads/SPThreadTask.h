/**
Copyright (c) 2016-2019 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef COMPONENTS_COMMON_UTILS_THREADS_SPTHREADTASK_H_
#define COMPONENTS_COMMON_UTILS_THREADS_SPTHREADTASK_H_

#include "SPRef.h"
#include "SPMemPriorityQueue.h"

NS_SP_EXT_BEGIN(thread)

class Task : public RefBase<AtomicCounter, memory::StandartInterface> {
public: /* typedefs */
	/* Function to be executed in init phase */
	using PrepareCallback = std::function<bool(const Task &)>;

	/* Function to be executed in other thread */
	using ExecuteCallback = std::function<bool(const Task &)>;

	/* Function to be executed after task is performed */
	using CompleteCallback = std::function<void(const Task &, bool)>;

	using PriorityType = ValueWrapper<memory::PriorityQueue<Rc<Task>>::PriorityType, class PriorityTypeFlag>;

	/* creates empty task with only complete function to be used as callback from other thread */
	bool init(const CompleteCallback &, Ref * = nullptr);
	bool init(CompleteCallback &&, Ref * = nullptr);

	/* creates regular async task without initialization phase */
	bool init(const ExecuteCallback &, const CompleteCallback & = nullptr, Ref * = nullptr);
	bool init(ExecuteCallback &&, CompleteCallback && = nullptr, Ref * = nullptr);

	/* creates regular async task with initialization phase */
	bool init(const PrepareCallback &, const ExecuteCallback &, const CompleteCallback & = nullptr, Ref * = nullptr);
	bool init(PrepareCallback &&, ExecuteCallback &&, CompleteCallback && = nullptr, Ref * = nullptr);

	/* adds one more function to be executed before task is added to queue, functions executed as FIFO */
	void addPrepareCallback(const PrepareCallback &);
	void addPrepareCallback(PrepareCallback &&);

	/* adds one more function to be executed in other thread, functions executed as FIFO */
	void addExecuteCallback(const ExecuteCallback &);
	void addExecuteCallback(ExecuteCallback &&);

	/* adds one more function to be executed when task is performed, functions executed as FIFO */
	void addCompleteCallback(const CompleteCallback &);
	void addCompleteCallback(CompleteCallback &&);

	/* mark this task with tag */
	void setTag(int tag) { _tag = tag; }

	/* returns tag */
	int getTag()  const{ return _tag; }

	/* set default task priority */
	void setPriority(PriorityType::Type priority) { _priority = PriorityType(priority); }

	/* get task priority */
	PriorityType getPriority() const { return _priority; }

	void setTarget(Ref *target) { _target = target; }

	/* get assigned target */
	Ref *getTarget() const { return _target; }

	/* used by task manager to set success state */
	void setSuccessful(bool value) { _isSuccessful = value; }

	/* if task execution was successful */
	bool isSuccessful() const { return _isSuccessful; }

	const std::vector<PrepareCallback> &getPrepareTasks() const { return _prepare; }
	const std::vector<ExecuteCallback> &getExecuteTasks() const { return _execute; }
	const std::vector<CompleteCallback> &getCompleteTasks() const { return _complete; }

public: /* overloads */
	virtual bool prepare() const;

	/** called on worker thread */
	virtual bool execute();

	/** called on UI thread when request is completed */
	virtual void onComplete();

public: /* constructors */
	Task();
	virtual ~Task();

protected:
	bool _isSuccessful = true;
	int _tag = -1;
	PriorityType _priority = PriorityType();

	Rc<Ref> _target;

	std::vector<PrepareCallback> _prepare;
	std::vector<ExecuteCallback> _execute;
	std::vector<CompleteCallback> _complete;
};

NS_SP_EXT_END(thread)

#endif /* COMPONENTS_COMMON_UTILS_THREADS_SPTHREADTASK_H_ */
