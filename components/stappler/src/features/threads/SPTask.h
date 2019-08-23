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

#ifndef LIBS_STAPPLER_FEATURES_THREADS_SPTASK_H
#define LIBS_STAPPLER_FEATURES_THREADS_SPTASK_H

#include "SPDefine.h"
#include "base/CCRef.h"

NS_SP_BEGIN

class Task : public Ref {
public: /* typedefs */
	/* Function to be executed in init phase */
	using PrepareCallback = Function<bool(const Task &)>;

	/* Function to be executed in other thread */
	using ExecuteCallback = Function<bool(const Task &)>;

	/* Function to be executed after task is performed */
	using CompleteCallback = Function<void(const Task &, bool)>;

public: /* interface */
	/* creates empty task with only complete function to be used as callback from other thread */
	bool init(const CompleteCallback &, Ref * = nullptr);

	/* creates regular async task without initialization phase */
	bool init(const ExecuteCallback &, const CompleteCallback & = nullptr, Ref * = nullptr);

	/* creates regular async task with initialization phase */
	bool init(const PrepareCallback &, const ExecuteCallback &, const CompleteCallback & = nullptr, Ref * = nullptr);

	/* adds one more function to be executed before task is added to queue, functions executed as FIFO */
	void addPrepareCallback(const PrepareCallback &);

	/* adds one more function to be executed in other thread, functions executed as FIFO */
	void addExecuteCallback(const ExecuteCallback &);

	/* adds one more function to be executed when task is performed, functions executed as FIFO */
	void addCompleteCallback(const CompleteCallback &);

	/* mark this task with tag */
	void setTag(int tag) { _tag = tag; }

	/* returns tag */
	int getTag()  const{ return _tag; }

	/* set default task priority */
	void setPriority(int priority) { _priority = priority; }

	/* get task priority */
	int getPriority()  const{ return _priority; }

	void setTarget(Ref *target) { _target = target; }

	/* get assigned target */
	Ref *getTarget() const { return _target; }

public: /* behavior */
	/* used by task manager to set success state */
	void setSuccessful(bool value) { _isSuccessful = value; }

	/* if task execution was successful */
	bool isSuccessful() const { return _isSuccessful; }

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
	int _priority = 0;

	Rc<Ref> _target;

	Vector<PrepareCallback> _prepare;
	Vector<ExecuteCallback> _execute;
	Vector<CompleteCallback> _complete;
};

NS_SP_END

#endif
