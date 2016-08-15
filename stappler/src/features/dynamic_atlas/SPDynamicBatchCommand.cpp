/*
 * SPDynamicBatchCommand.cpp
 *
 *  Created on: 01 апр. 2015 г.
 *      Author: sbkarr
 */

#include "SPDefine.h"
#include "SPDynamicBatchCommand.h"

#include "SPDynamicAtlas.h"
#include "SPDynamicQuadArray.h"

#include "renderer/CCGLProgram.h"
#include "renderer/CCTexture2D.h"
#include "platform/CCGL.h"
#include "renderer/ccGLStateCache.h"

#include "xxhash.h"

NS_SP_BEGIN

DynamicBatchCommand::DynamicBatchCommand(bool b) {
    _type = RenderCommand::Type::SP_DYNAMIC_COMMAND;
	func = std::bind(&DynamicBatchCommand::execute, this);
	_batch = b;
}

void DynamicBatchCommand::init(float g, GLProgram *p, BlendFunc f, DynamicAtlas *a, const Mat4& mv, const std::vector<int> &zPath, bool n) {
	CCASSERT(p, "shader cannot be nill");
	CCASSERT(a, "textureAtlas cannot be nill");

	_globalOrder = g;
	_textureID = a->getTexture()->getName();
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
}

void DynamicBatchCommand::execute() {
	// Set material
	if (!_batch) {
		auto &quads = _textureAtlas->getQuads();
		for (auto &it : quads) {
			it->updateTransform(Mat4::IDENTITY);
		}
	}

	_shader->use();
	_shader->setUniformsForBuiltins(_mv);
	cocos2d::GL::bindTexture2D(_textureID);
	cocos2d::GL::blendFunc(_blendType.src, _blendType.dst);

	// Draw
	_textureAtlas->drawQuads();
}

GLuint DynamicBatchCommand::getTextureId() const {
	return _textureID;
}
cocos2d::GLProgram *DynamicBatchCommand::getProgram() const {
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

uint32_t DynamicBatchCommand::getMaterialId(int32_t groupId) const {
	std::vector<int32_t> intArray;
	intArray.reserve(6 + _zPath.size());

	intArray.push_back( _shader ? reinterpretValue<int32_t>(_shader->getProgram()) : 0 );
	intArray.push_back( reinterpretValue<int32_t>(_textureID) );
	intArray.push_back( reinterpretValue<int32_t>(_blendType.src) );
	intArray.push_back( reinterpretValue<int32_t>(_blendType.dst) );
	intArray.push_back( (int32_t)_normalized );
	intArray.push_back( (int32_t)groupId );

	for (auto &i : _zPath) {
		intArray.push_back(i);
	}

	return XXH32((const void*)intArray.data(), sizeof(int32_t) * (int32_t)intArray.size(), 0);
}

NS_SP_END
