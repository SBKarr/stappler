/*
 * SPLayer.h
 *
 *  Created on: 07 апр. 2015 г.
 *      Author: sbkarr
 */

#ifndef LIBS_STAPPLER_NODES_BATCHED_SPLAYER_H_
#define LIBS_STAPPLER_NODES_BATCHED_SPLAYER_H_

#include "SPCustomCornerSprite.h"

NS_SP_BEGIN

struct Gradient {
	using Color = Color4B;
	using ColorRef = const Color &;

	static const cocos2d::Vec2 Vertical;
	static const cocos2d::Vec2 Horizontal;

	Gradient();
	Gradient(ColorRef start, ColorRef end, const cocos2d::Vec2 & = Vertical);
	Gradient(ColorRef bl, ColorRef br, ColorRef tl, ColorRef tr);

	cocos2d::Color4B colors[4]; // bl - br - tl - tr
};

class Layer : public CustomCornerSprite {
public:
    virtual bool init(const cocos2d::Color4B & = cocos2d::Color4B(255, 255, 255, 255));

    virtual void setGradient(const Gradient &);
    virtual const Gradient &getGradient() const;

	virtual Rc<Texture> generateTexture(uint32_t size) override;
protected:
	virtual void updateSprites() override;
	virtual void updateColor() override;

	Gradient _gradient;
};

NS_SP_END

#endif /* LIBS_STAPPLER_NODES_BATCHED_SPLAYER_H_ */
