// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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

#include "SPDefine.h"
#include "SPRoundedSprite.h"
#include "renderer/CCTexture2D.h"

NS_SP_BEGIN

static std::map<uint32_t, RoundedSprite::RoundedTexture *> s_textures;

static void createRoundedTextureData(uint8_t *data, uint32_t size) {
	for (uint32_t i = 0; i < size; i++) {
		for (uint32_t j = 0; j < size; j++) {
			float dist = sqrtf((float)( (i * i) + (j * j) ));
			auto diff = dist - size;
			if (fabs(diff + 0.5f) <= 0.5f) {
				data[i * size + j] = (uint8_t) ( 255.0f * (1.0f - (diff + 1.0f)) );
			} else if (diff > 0) {
				data[i * size + j] = 0;
			} else {
				data[i * size + j] = 255;
			}
		}
	}
}

RoundedSprite::RoundedTexture::~RoundedTexture() {
	s_textures.erase(_size);
}

bool RoundedSprite::RoundedTexture::init(uint32_t size) {
	if (!Texture::init(size)) {
		return false;
	}

	s_textures.insert(std::make_pair(_size, this));
	return true;
}

uint8_t *RoundedSprite::RoundedTexture::generateTexture(uint32_t size) {
	auto data = Texture::generateTexture(size);
	createRoundedTextureData(data, size);
	return data;
}

bool RoundedSprite::init(uint32_t size, float density) {
	if (!CustomCornerSprite::init(size, density)) {
		return false;
	}

	_stencil = true;

	return true;
}

Rc<RoundedSprite::Texture> RoundedSprite::generateTexture(uint32_t r) {
	auto it = s_textures.find(r);
	if (it == s_textures.end()) {
		return Rc<RoundedTexture>::create(r);
	} else {
		return it->second;
	}
}

NS_SP_END
