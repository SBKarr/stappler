/**
 Copyright (c) 2020-2021 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef COMPONENTS_XENOLITH_GL_XLVKPRESENTATIONLOOP_H_
#define COMPONENTS_XENOLITH_GL_XLVKPRESENTATIONLOOP_H_

#include "XLVkPresentationDevice.h"

namespace stappler::xenolith::vk {

struct PresentationTask;
struct PresentationTimer;

class PresentationLoop : public thread::ThreadHandlerInterface {
public:
	PresentationLoop(Application *, Rc<View>, Rc<PresentationDevice>, Rc<Director>, uint64_t frameMicroseconds);

	virtual void threadInit() override;
	virtual bool worker() override;

	void pushTask(PresentationEvent, Rc<Ref> && = Rc<Ref>(), data::Value && = data::Value(), Function<void()> && = nullptr);

	void schedule(Function<bool(Vector<PresentationTask> &)> &&);
	void schedule(Function<bool(Vector<PresentationTask> &)> &&, uint64_t);

	void setInterval(uint64_t);
	void recreateSwapChain();

	void begin();
	void end(bool success = true);
	void reset();

	void requestPipeline(Rc<PipelineRequest>, Function<void()> &&);

	std::thread::id getThreadId() const { return _threadId; }

protected:
	void runTimers(uint64_t dt, Vector<PresentationTask> &t);

	Application *_application = nullptr;
	Rc<View> _view;
	Rc<PresentationDevice> _device; // logical presentation device
	Rc<Director> _director;

	std::atomic<uint64_t> _frameTimeMicroseconds = 1000'000 / 60;
	std::atomic<uint64_t> _rate;
	std::thread _thread;
	std::thread::id _threadId;

	Vector<PresentationTask> _tasks;
	Vector<PresentationTimer> _timers;
	Rc<thread::TaskQueue> _frameQueue;
	Rc<thread::TaskQueue> _dataQueue;
	memory::pool_t *_pool = nullptr;

	std::atomic<bool> _running = false;
	std::mutex _mutex;
	std::condition_variable _cond;
};

}

#endif /* COMPONENTS_XENOLITH_GL_XLVKPRESENTATIONLOOP_H_ */
