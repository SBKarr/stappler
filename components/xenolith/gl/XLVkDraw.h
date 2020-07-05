/**
 Copyright (c) 2020 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef COMPONENTS_XENOLITH_GL_XLVKDRAW_H_
#define COMPONENTS_XENOLITH_GL_XLVKDRAW_H_

#include "XLVkDevice.h"

namespace stappler::xenolith::vk {

class DrawDevice : public VirtualDevice {
public:
	class Requisite : public Ref {
	public:
	protected:
		// statics
		Rc<Buffer> materials; // storage
		Vector<Rc<Buffer>> statics; // storage
		Vector<Rc<Buffer>> uniforms; // uniform

		// dynamics
		Rc<Buffer> draws; // storage
		Rc<Buffer> transforms;  // uniform
		Vector<Rc<Buffer>> dynamics; // storage
		Rc<Buffer> index;
	};

	virtual ~DrawDevice();
	bool init(Rc<Instance>, Rc<Allocator>, VkQueue, uint32_t qIdx);

	bool drawFrame(thread::TaskQueue &, );

protected:
	Rc<Requisite> _current;
	Rc<Requisite> _next;

	VkQueue _queue = VK_NULL_HANDLE;
	size_t _currentFrame = 0;
	uint32_t _queueIdx = 0;

	Vector<VkFence> _inFlightFences;
	Vector<VkFence> _imagesInFlight;
};

}

#endif /* COMPONENTS_XENOLITH_GL_XLVKDRAW_H_ */
