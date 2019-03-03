// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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

#include "SPDefine.h"
#include "SPDynamicBatchCommand.h"

#include "SPDynamicAtlas.h"
#include "SPDynamicQuadArray.h"
#include "SPStencilCache.h"

#include "platform/CCGL.h"
#include "renderer/CCGLProgram.h"
#include "renderer/CCGLProgramState.h"
#include "renderer/CCTexture2D.h"
#include "renderer/ccGLStateCache.h"

#include "xxhash.h"

NS_SP_BEGIN

DynamicBatchCommand::DynamicBatchCommand(bool b) {
	_type = RenderCommand::Type::SP_DYNAMIC_COMMAND;
	func = std::bind(&DynamicBatchCommand::execute, this);
	_batch = b;
}

void DynamicBatchCommand::init(float g, GLProgramState *p, BlendFunc f, DynamicAtlas *a, const Mat4& mv,
		const std::vector<int> &zPath, bool n, bool stencil, AlphaTest alphaTest, DynamicLinearGradient *grad) {
	CCASSERT(p, "shader cannot be nill");
	CCASSERT(a, "textureAtlas cannot be nill");

	auto tex = a->getTexture();
	_globalOrder = g;

	_textureID = tex?tex->getName():0;
	_blendType = f;
	_shader = p;

	_textureAtlas = a;

	_mv = mv;
	_zPath = zPath;
	while (!_zPath.empty() && _zPath.back() == 0) {
		_zPath.pop_back();
	}

	if (_zPath.empty()) {
		log::text("WARNING", "zPath empty");
	}

	_normalized = n;
	_stencil = stencil;
	_stencilIndex = 0;
	_alphaTest = alphaTest;
	_gradient = grad;
}

void DynamicBatchCommand::setStencilIndex(uint8_t st) {
	_stencilIndex = st;
}

void DynamicBatchCommand::useMaterial() {
	if (!_batch) {
		auto &quads = _textureAtlas->getQuads();
		for (auto &it : quads) {
			it->updateTransform(Mat4::IDENTITY);
		}
	}

	if (_alphaTest.state != AlphaTest::Disabled) {
		_shader->setUniformFloat("u_alphaTest", _alphaTest.value / 255.0f);
	}
	if (_gradient) {
		_gradient->updateUniforms(_shader);
	}

	_shader->applyGLProgram(_mv);
	_shader->applyUniforms();

	cocos2d::GL::bindTexture2D(_textureID);
	cocos2d::GL::blendFunc(_blendType.src, _blendType.dst);
}

void DynamicBatchCommand::execute() {
	useMaterial();
	auto s = StencilCache::getInstance();
	if (s->isEnabled()) {
		if (_stencilIndex) {
			s->enableStencilTest(StencilCache::Func::GreaterEqual, _stencilIndex);
		} else {
			s->disableStencilTest();
		}
	}
	_textureAtlas->drawQuads();
}

uint8_t DynamicBatchCommand::makeStencil() {
	useMaterial();
	auto s = StencilCache::getInstance();
	if (s->isEnabled()) {
		_stencilIndex = s->pushStencilLayer();
	}
	_textureAtlas->drawQuads(false);
	return _stencilIndex;
}

GLuint DynamicBatchCommand::getTextureId() const {
	return _textureID;
}
cocos2d::GLProgramState *DynamicBatchCommand::getProgram() const {
	return _shader;
}
const cocos2d::BlendFunc &DynamicBatchCommand::getBlendFunc() const {
	return _blendType;
}
DynamicAtlas *DynamicBatchCommand::getAtlas() const {
	return _textureAtlas;
}

const Mat4 &DynamicBatchCommand::getTransform() const {
	return _mv;
}

bool DynamicBatchCommand::isNormalized() const {
	return _normalized;
}

bool DynamicBatchCommand::isStencil() const {
	return _stencil;
}

bool DynamicBatchCommand::isBatch() const {
	return _batch;
}

uint32_t DynamicBatchCommand::getMaterialId(int32_t groupId) const {
	std::vector<int32_t> intArray;
	intArray.reserve(6 + _zPath.size());

	intArray.push_back( _shader ? reinterpretValue<int32_t>(_shader->getGLProgram()->getProgram()) : 0 );
	intArray.push_back( reinterpretValue<int32_t>(_textureID) );
	intArray.push_back( reinterpretValue<int32_t>(_blendType.src) );
	intArray.push_back( reinterpretValue<int32_t>(_blendType.dst) );
	intArray.push_back( int32_t(_normalized) | (int32_t(_stencil) << 2) | (uint32_t(_alphaTest.state) << 16) | (uint32_t(_alphaTest.value) << 24) );
	intArray.push_back( (int32_t)groupId );

	for (auto &i : _zPath) {
		intArray.push_back(i);
	}

	return XXH32((const void*)intArray.data(), sizeof(int32_t) * (int32_t)intArray.size(), 0);
}

uint8_t DynamicBatchCommand::getStencilIndex() const {
	return _stencilIndex;
}

DynamicLinearGradient *DynamicBatchCommand::getGradient() const {
	return _gradient;
}

NS_SP_END
