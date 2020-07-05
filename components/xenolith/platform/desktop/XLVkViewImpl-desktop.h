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

#ifndef COMPONENTS_XENOLITH_PLATFORM_DESKTOP_XLVKVIEWIMPL_DESKTOP_H_
#define COMPONENTS_XENOLITH_PLATFORM_DESKTOP_XLVKVIEWIMPL_DESKTOP_H_

#include "XLVkView.h"

#if (MSYS || CYGWIN || LINUX || MACOS)

#include "glfw3.h"

#if (MSYS || CYGWIN)
#ifndef GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#endif
#ifndef GLFW_EXPOSE_NATIVE_WGL
#define GLFW_EXPOSE_NATIVE_WGL
#endif
#include "glfw3native.h"
#endif // MSYS || CYGWIN

#if (MACOS)
#ifndef GLFW_EXPOSE_NATIVE_NSGL
#define GLFW_EXPOSE_NATIVE_NSGL
#endif
#ifndef GLFW_EXPOSE_NATIVE_COCOA
#define GLFW_EXPOSE_NATIVE_COCOA
#endif
#include "glfw3native.h"
#endif // MACOS

/*
GLFWAPI GLFWvkproc glfwGetInstanceProcAddress(VkInstance instance, const char* procname);
GLFWAPI int glfwGetPhysicalDevicePresentationSupport(VkInstance instance, VkPhysicalDevice device, uint32_t queuefamily);
GLFWAPI VkResult glfwCreateWindowSurface(VkInstance instance, GLFWwindow* window, const VkAllocationCallbacks* allocator, VkSurfaceKHR* surface);
*/

namespace stappler::xenolith::vk {

class ViewImpl : public View {
public:
	ViewImpl();
    virtual ~ViewImpl();

    bool init(const StringView & viewName, Rect rect);
    bool init(const StringView & viewName);
    bool init(const StringView & viewname, const GLFWvidmode &videoMode, GLFWmonitor *monitor);

	virtual void end() override;
	virtual bool isVkReady() const override;
	virtual void swapBuffers() override;
	virtual void pollEvents() override;
	virtual void setIMEKeyboardState(bool open) override;
	virtual bool windowShouldClose() const override;
	virtual double getTimeCounter() const override;

	virtual void setAnimationInterval(double) override;
	virtual bool run(const Callback<bool(double)> &) override;

	virtual void setScreenSize(float width, float height) override;

	virtual void setClipboardString(const StringView &) override;
	virtual StringView getClipboardString() const override;

	virtual void selectPresentationOptions(Instance::PresentationOptions &opts) const override;

	void onGLFWWindowSizeFunCallback(GLFWwindow *window, int width, int height);

protected:
	void updateFrameSize();

	/*void onGLFWError(int errorID, const char* errorDesc);
	void onGLFWMouseCallBack(GLFWwindow* window, int button, int action, int modify);
	void onGLFWMouseMoveCallBack(GLFWwindow* window, double x, double y);
	void onGLFWMouseScrollCallback(GLFWwindow* window, double x, double y);
	void onGLFWKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
	void onGLFWCharCallback(GLFWwindow* window, unsigned int character);
	void onGLFWWindowPosCallback(GLFWwindow* windows, int x, int y);
	void onGLFWframebuffersize(GLFWwindow* window, int w, int h);
	void onGLFWWindowIconifyCallback(GLFWwindow* window, int iconified);
	void onGLFWWindowFocusCallback(GLFWwindow* window, int focus);*/

	GLFWwindow* _mainWindow = nullptr;
	GLFWmonitor* _monitor = nullptr;
	VkSurfaceKHR _surface = nullptr;

	uint32_t _frameWidth;
	uint32_t _frameHeight;

	double _animationInterval = 1.0 / 120.0;
};

}
#endif

#endif /* COMPONENTS_XENOLITH_PLATFORM_DESKTOP_XLVKVIEWIMPL_DESKTOP_H_ */
