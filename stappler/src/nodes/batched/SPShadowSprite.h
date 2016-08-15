/*
 * SPShadowSprite.h
 *
 *  Created on: 16 нояб. 2014 г.
 *      Author: sbkarr
 */

#ifndef STAPPLER_SRC_EXT_SPSHADOWSPRITE_H_
#define STAPPLER_SRC_EXT_SPSHADOWSPRITE_H_

#include "SPCustomCornerSprite.h"

NS_SP_BEGIN

class ShadowSprite : public CustomCornerSprite {
public:
	class ShadowTexture : public CustomCornerSprite::Texture {
	public:
		virtual bool init(uint32_t size) override;
		virtual ~ShadowTexture();
	    virtual uint8_t *generateTexture(uint32_t size) override;
	};

protected:
	virtual Rc<Texture> generateTexture(uint32_t size);
};

NS_SP_END

#endif /* STAPPLER_SRC_EXT_SPSHADOWSPRITE_H_ */
