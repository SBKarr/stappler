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

#ifndef LIBS_STAPPLER_FEATURES_DYNAMIC_ATLAS_SPGLPROGRAMSET_H_
#define LIBS_STAPPLER_FEATURES_DYNAMIC_ATLAS_SPGLPROGRAMSET_H_

#include "SPEventHandler.h"
#include "renderer/CCGLProgram.h"

NS_SP_BEGIN

class GLProgramSet : public Ref {
public:
	enum Program : uint32_t {
		DrawNodeA8 = 1 << 0,
		DynamicBatchI8 = 1 << 1,
		DynamicBatchR8ToA8 = 1 << 2,
		DynamicBatchAI88 = 1 << 3,
		DynamicBatchA8Highp = 1 << 4, // for font on high-density screens
		DynamicBatchColor = 1 << 5,

		RawTexture = 1 << 6,
		RawTextureColor = 1 << 7,
		RawTextureColorA8 = 1 << 8,
		RawTextureColorA8Highp = 1 << 9,

		RawRect = 1 << 10,
		RawRectBorder = 1 << 11,
		RawRects = 1 << 12,
		RawAAMaskR = 1 << 13,
		RawAAMaskRGBA = 1 << 14,

		DrawNodeSet = DrawNodeA8 | DynamicBatchI8 | DynamicBatchAI88 | DynamicBatchA8Highp | DynamicBatchColor | DynamicBatchR8ToA8,
		RawDrawingSet = RawTexture | DynamicBatchA8Highp | RawRect | RawRectBorder | RawRects | RawAAMaskR | RawAAMaskRGBA,
		DynamicBatchR8ToI8 = DynamicBatchI8,
	};

	bool init();
	bool init(uint32_t mask);

	cocos2d::GLProgram *getProgram(Program);

	GLProgramSet();

	void reloadPrograms();

protected:
	Map<Program, Rc<cocos2d::GLProgram>> _programs;
};

NS_SP_END

#endif /* LIBS_STAPPLER_FEATURES_DYNAMIC_ATLAS_SPGLPROGRAMSET_H_ */
