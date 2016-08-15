/*
 * SPRoundedLayer.cpp
 *
 *  Created on: 18 марта 2015 г.
 *      Author: sbkarr
 */

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

Rc<RoundedSprite::Texture> RoundedSprite::generateTexture(uint32_t r) {
	auto it = s_textures.find(r);
	if (it == s_textures.end()) {
		return Rc<RoundedTexture>::create(r);
	} else {
		return it->second;
	}
}

NS_SP_END
