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

#ifndef STAPPLER_SRC_PLATFORM_UNIVERSAL_SPAPPLICATION_H_
#define STAPPLER_SRC_PLATFORM_UNIVERSAL_SPAPPLICATION_H_

#include "platform/CCApplication.h"
#include "SPDefine.h"

NS_SP_BEGIN

#ifndef SP_RESTRICT
class Application : public cocos2d::Application {
public:
	static bool isApplicationExists();

	Application();
	virtual ~Application();

	virtual bool applicationDidFinishLaunching() override;
	virtual void applicationDidEnterBackground() override;
	virtual void applicationWillEnterForeground() override;

	virtual void applicationFocusGained() override;
	virtual void applicationFocusLost() override;

	virtual void applicationFrameBegin() override;
	virtual void applicationFrameEnd() override;

	virtual void applicationDidReceiveMemoryWarning() override;

protected:
	memory::pool_t *_applicationPool = nullptr;
	memory::pool_t *_framePool = nullptr;
};
#endif

NS_SP_END

#endif /* STAPPLER_SRC_PLATFORM_UNIVERSAL_SPAPPLICATION_H_ */
