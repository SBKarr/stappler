/**
Copyright (c) 2016-2019 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef LIBS_STAPPLER_NODES_BATCHED_SPDYNAMICBATCHCOMMAND_H_
#define LIBS_STAPPLER_NODES_BATCHED_SPDYNAMICBATCHCOMMAND_H_

#include "SPDefine.h"
#include "SPDynamicLinearGradient.h"
#include "renderer/CCCustomCommand.h"

NS_SP_BEGIN

class DynamicBatchCommand : public cocos2d::CustomCommand {
public:
	using GLProgram = cocos2d::GLProgram;
	using GLProgramState = cocos2d::GLProgramState;
	using BlendFunc = cocos2d::BlendFunc;

	DynamicBatchCommand(bool b = false);

	void init(float depth, GLProgramState *, BlendFunc, DynamicAtlas *, const Mat4& mv, const std::vector<int> &, bool n,
			bool stencil = false, AlphaTest = AlphaTest(), DynamicLinearGradient *gradient = nullptr);

	void setStencilIndex(uint8_t);

	void useMaterial();
	void execute();
	uint8_t makeStencil();

	GLuint getTextureId() const;
	cocos2d::GLProgramState *getProgram() const;
	const cocos2d::BlendFunc &getBlendFunc() const;
	DynamicAtlas *getAtlas() const;

	const Mat4 &getTransform() const;

	bool isNormalized() const;
	bool isStencil() const;
	bool isBatch() const;

	uint32_t getMaterialId(int32_t groupId) const;
	uint8_t getStencilIndex() const;

	DynamicLinearGradient *getGradient() const;

protected:
	//Material
	GLuint _textureID = 0;
	cocos2d::GLProgramState * _shader = nullptr;
	cocos2d::BlendFunc _blendType;

	DynamicAtlas *_textureAtlas = nullptr;

	// ModelView transform
	Mat4 _mv;
	bool _normalized = false;
	bool _batch = false;
	bool _stencil = false;
	uint8_t _stencilIndex = 0;
	AlphaTest _alphaTest;
	Rc<DynamicLinearGradient> _gradient;
};

NS_SP_END

#endif /* LIBS_STAPPLER_NODES_BATCHED_SPDYNAMICBATCHCOMMAND_H_ */
