//
//  SPTask.h
//  chiertime-federal
//
//  Created by SBKarr on 6/20/13.
//
//

#ifndef chiertime_federal_SPTask_h
#define chiertime_federal_SPTask_h

#include "SPDefine.h"
#include "base/CCRef.h"

NS_SP_BEGIN

class Task : public cocos2d::Ref {
public: /* typedefs */
	/* Function to be executed in init phase */
	using PrepareCallback = std::function<bool(Ref *)>;

	/* Function to be executed in other thread */
	using ExecuteCallback = std::function<bool(Ref *)>;

	/* Function to be executed after task is performed */
	using CompleteCallback = std::function<void(Ref *, bool)>;

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

	std::vector<PrepareCallback> _prepare;
	std::vector<ExecuteCallback> _execute;
	std::vector<CompleteCallback> _complete;
};

NS_SP_END

#endif
