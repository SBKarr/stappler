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
		DynamicBatchAI88 = 1 << 2,
		DynamicBatchA8Highp = 1 << 3, // for font on high-density screens

		RawTexture = 1 << 4,
		RawTextureColor = 1 << 5,
		RawTextureColorA8 = 1 << 6,
		RawTextureColorA8Highp = 1 << 7,

		RawRect = 1 << 8,
		RawRectBorder = 1 << 9,
		RawRects = 1 << 10,

		DrawNodeSet = DrawNodeA8 | DynamicBatchI8 | DynamicBatchAI88 | DynamicBatchA8Highp,
		RawDrawingSet = RawTexture | DynamicBatchA8Highp | RawRect | RawRectBorder | RawRects,
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
