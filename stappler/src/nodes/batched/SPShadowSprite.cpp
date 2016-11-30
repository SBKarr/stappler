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
#include "SPShadowSprite.h"
#include "renderer/CCTexture2D.h"

NS_SP_BEGIN

static std::map<uint32_t, ShadowSprite::ShadowTexture *> s_textures;

static void createShadowGaussianData(uint8_t *data, uint32_t size) {
	float sigma = sqrtf((size * size) / (-2.0f * logf(1.0f / 255.0f)));

	for (uint32_t i = 0; i < size; i++) {
		for (uint32_t j = 0; j < size; j++) {
			float dist = sqrtf( (i * i) + (j * j) );
			if (dist > size) {
				dist = size;
			}
			data[i * size + j] = (uint8_t)(255.0f * expf( -1.0f * (dist * dist) / (2.0f * sigma * sigma) ));
		}
	}
}

ShadowSprite::ShadowTexture::~ShadowTexture() {
	s_textures.erase(_size);
}

bool ShadowSprite::ShadowTexture::init(uint32_t size) {
	if (!Texture::init(size)) {
		return false;
	}

	s_textures.insert(std::make_pair(_size, this));
	return true;
}

uint8_t *ShadowSprite::ShadowTexture::generateTexture(uint32_t size) {
	auto data = Texture::generateTexture(size);
	createShadowGaussianData(data, size);
	return data;
}

Rc<ShadowSprite::Texture> ShadowSprite::generateTexture(uint32_t r) {
	auto it = s_textures.find(r);
	if (it == s_textures.end()) {
		return Rc<ShadowTexture>::create(r);
	} else {
		return it->second;
	}
}

NS_SP_END
