/*
 * SPShadowSprite.cpp
 *
 *  Created on: 16 нояб. 2014 г.
 *      Author: sbkarr
 */

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
