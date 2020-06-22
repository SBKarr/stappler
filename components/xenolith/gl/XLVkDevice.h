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

#ifndef COMPONENTS_XENOLITH_GL_XLVKDEVICE_H_
#define COMPONENTS_XENOLITH_GL_XLVKDEVICE_H_

#include "XLVkInstance.h"

namespace stappler::xenolith::vk {

class PresentationDevice : public Ref {
public:
	static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

	using ProgramCallback = Function<void(Rc<ProgramModule>)>;

	PresentationDevice();
	virtual ~PresentationDevice();

	bool init(Rc<Instance> instance, Rc<View> v, VkSurfaceKHR, Instance::PresentationOptions &&, VkPhysicalDeviceFeatures);

	bool addProgram(StringView, StringView, ProgramSource, ProgramStage, const ProgramCallback &);
	bool addProgram(FilePath, ProgramSource, ProgramStage, const ProgramCallback &);

	void drawFrame(double);

	Instance *getInstance() const { return _instance; }
	VkDevice getDevice() const { return _device; }

	bool recreateSwapChain();

private:
	friend class ProgramManager;

	bool createSwapChain(VkSurfaceKHR surface);
	bool createDefaultPipeline();

	void cleanupSwapChain();

	size_t _currentFrame = 0;
	Instance::PresentationOptions _options;

	VkSurfaceKHR _surface = VK_NULL_HANDLE;
	VkDevice _device = VK_NULL_HANDLE;

	VkQueue _graphicsQueue = VK_NULL_HANDLE;
	VkQueue _presentQueue = VK_NULL_HANDLE;

	VkSwapchainKHR _swapChain = VK_NULL_HANDLE;
	Vector<VkImage> _swapChainImages;
	Vector<Rc<ImageView>> _swapChainImageViews;
	Vector<Rc<Framebuffer>> _swapChainFramebuffers;

	Map<String, Rc<ProgramModule>> _shaders;

	Rc<Pipeline> _defaultPipeline;
	Rc<PipelineLayout> _defaultPipelineLayout;
	Rc<RenderPass> _defaultRenderPass;
	Rc<CommandPool> _defaultCommandPool;
	Vector<VkCommandBuffer> _defaultCommandBuffers;

	Rc<Instance> _instance;
	Rc<View> _view;

	Vector<VkSemaphore> _imageAvailableSemaphores;
	Vector<VkSemaphore> _renderFinishedSemaphores;
	Vector<VkFence> _inFlightFences;
	Vector<VkFence> _imagesInFlight;
};

}

#endif /* COMPONENTS_XENOLITH_GL_XLVKDEVICE_H_ */
