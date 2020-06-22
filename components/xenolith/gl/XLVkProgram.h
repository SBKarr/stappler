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

#ifndef COMPONENTS_XENOLITH_GL_XLVKPROGRAM_H_
#define COMPONENTS_XENOLITH_GL_XLVKPROGRAM_H_

#include "XLVkDevice.h"

namespace stappler::xenolith::vk {

class ProgramModule : public Ref {
public:
	virtual ~ProgramModule();

	bool init(PresentationDevice &dev, ProgramSource, ProgramStage, FilePath);
	bool init(PresentationDevice &dev, ProgramSource, ProgramStage, StringView data, StringView key);
	bool init(PresentationDevice &dev, ProgramSource, ProgramStage, BytesView data, StringView key);

	void invalidate(PresentationDevice &dev);

	StringView getName() const { return _name; }
	ProgramStage getStage() const { return _stage; }
	VkShaderModule getModule() const { return _shaderModule; }

protected:
	String _name;
	ProgramStage _stage = ProgramStage::None;
	VkShaderModule _shaderModule = VK_NULL_HANDLE;
};

}

#endif /* COMPONENTS_XENOLITH_GL_XLPROGRAMMANAGER_H_ */
