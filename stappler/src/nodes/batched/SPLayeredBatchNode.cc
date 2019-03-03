// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2016-2018 Roman Katuntsev <sbkarr@stappler.org>

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
#include "SPLayeredBatchNode.h"

#include "renderer/CCRenderer.h"
#include "renderer/CCTexture2D.h"

NS_SP_BEGIN

LayeredBatchNode::~LayeredBatchNode() { }

void LayeredBatchNode::setTextures(const Vector<Rc<cocos2d::Texture2D>> &tex) {
	_textures.clear();
	for (auto &it : tex) {
		updateBlendFunc(it);
		_textures.emplace_back(TextureLayer{it, nullptr, Rc<DynamicQuadArray>::alloc()});
	}

	if (_commands.size() < _textures.size()) {
		auto size = _commands.size();
		_commands.reserve(_textures.size());
		for (size_t i = size; i < _textures.size(); ++ i) {
			_commands.push_back(Rc<CmdWrap>::create());
		}
	}
}
void LayeredBatchNode::setTextures(const Vector<cocos2d::Texture2D *> &tex) {
	_textures.clear();
	for (auto &it : tex) {
		updateBlendFunc(it);
		_textures.emplace_back(TextureLayer{it, nullptr, Rc<DynamicQuadArray>::alloc()});
	}

	if (_commands.size() < _textures.size()) {
		auto size = _commands.size();
		_commands.reserve(_textures.size());
		for (size_t i = size; i < _textures.size(); ++ i) {
			_commands.push_back(Rc<CmdWrap>::create());
		}
	}
}

void LayeredBatchNode::setTextures(const Vector<Rc<cocos2d::Texture2D>> &tex, Vector<Rc<DynamicQuadArray>> &&newQuads) {
	_textures.clear();
	for (size_t i = 0; i < tex.size(); ++i) {
		updateBlendFunc(tex[i]);
		_textures.emplace_back(TextureLayer{tex[i], nullptr, std::move(newQuads[i])});
	}

	if (_commands.size() < _textures.size()) {
		auto size = _commands.size();
		_commands.reserve(_textures.size());
		for (size_t i = size; i < _textures.size(); ++ i) {
			_commands.push_back(Rc<CmdWrap>::create());
		}
	}
}

void LayeredBatchNode::draw(cocos2d::Renderer *renderer, const Mat4 &transform, uint32_t flags, const ZPath &zPath) {
	if (_programDirty) {
		for (auto &it : _textures) {
			updateBlendFunc(it.texture);
		}
	}

	if (_gradientObject) {
		Vec2 pos = transform.transformPoint(Vec2::ZERO);
		Vec2 size = transform.transformPoint(Vec2(_contentSize.width, _contentSize.height)) - pos;
		if (_gradientObject->isAbsolute()) {
			_gradientObject->updateWithSize(Size(size.x, size.y), pos);
		} else {
			_gradientObject->updateWithSize(_contentSize, Vec2::ZERO);
		}
	}

	size_t i = 0;
	for (auto &it : _textures) {
		if (!it.atlas && it.texture) {
			if (auto atlas = Rc<DynamicAtlas>::create(it.texture)) {
				it.atlas = atlas;
				it.atlas->addQuadArray(it.quads);
			}
		}

		if (!it.atlas || !it.texture || !getGLProgram()) {
			continue;
		}

		auto &cmd = _commands[i];
		if (_normalized) {
			Mat4 newMV;
			newMV.m[12] = floorf(transform.m[12]);
			newMV.m[13] = floorf(transform.m[13]);
			newMV.m[14] = floorf(transform.m[14]);

			cmd->cmd.init(_globalZOrder, getGLProgramState(), _blendFunc, it.atlas, newMV, zPath, _normalized, false, _alphaTest, _gradientObject);
		} else {
			cmd->cmd.init(_globalZOrder, getGLProgramState(), _blendFunc, it.atlas, transform, zPath, _normalized, false, _alphaTest, _gradientObject);
		}

		renderer->addCommand(&cmd->cmd);
		++ i;
	}
}

void LayeredBatchNode::updateColor() {
	if (!_textures.empty()) {
	    Color4B color4( _displayedColor.r, _displayedColor.g, _displayedColor.b, _displayedOpacity );

	    for (auto &it : _textures) {
	    	if (!it.quads->empty()) {
	    	    // special opacity for premultiplied textures
	    		if (_opacityModifyRGB) {
	    			color4.r *= _displayedOpacity/255.0f;
	    			color4.g *= _displayedOpacity/255.0f;
	    			color4.b *= _displayedOpacity/255.0f;
	    	    }

	    		for (size_t i = 0; i < it.quads->size(); i++) {
	    			it.quads->setColor(i, color4);
	    		}
	    	}
	    }
	}
}

DynamicQuadArray *LayeredBatchNode::getQuads(cocos2d::Texture2D *tex) {
    for (auto &it : _textures) {
    	if (it.texture == tex) {
    		return it.quads;
    	}
    }
    return nullptr;
}

NS_SP_END
