/*
 * SPRoundedLayer.h
 *
 *  Created on: 18 марта 2015 г.
 *      Author: sbkarr
 */

#ifndef LIBS_STAPPLER_NODES_ROUNDED_SPROUNDEDLAYER_H_
#define LIBS_STAPPLER_NODES_ROUNDED_SPROUNDEDLAYER_H_

#include "SPCustomCornerSprite.h"

NS_SP_BEGIN

class RoundedSprite : public CustomCornerSprite {
public:
	class RoundedTexture : public CustomCornerSprite::Texture {
	public:
		virtual bool init(uint32_t size) override;
		virtual ~RoundedTexture();
	    virtual uint8_t *generateTexture(uint32_t size) override;
	};

protected:
	virtual Rc<Texture> generateTexture(uint32_t size);
};

NS_SP_END

#endif /* LIBS_STAPPLER_NODES_ROUNDED_SPROUNDEDLAYER_H_ */
