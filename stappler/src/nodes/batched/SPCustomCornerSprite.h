/*
 * SPCustomCornerSprite.h
 *
 *  Created on: 18 марта 2015 г.
 *      Author: sbkarr
 */

#ifndef LIBS_STAPPLER_NODES_BATCHED_SPCUSTOMCORNERSPRITE_H_
#define LIBS_STAPPLER_NODES_BATCHED_SPCUSTOMCORNERSPRITE_H_

#include "SPDynamicBatchNode.h"

NS_SP_BEGIN

/** Custom corner sprite - extremely fast (low-level) implementation of Scale-9
 * it uses generated corner texture to fill content space as scale-9
 *
 * You can implement your own texture generator (like in ShadowSprite or RoundedSprite)
 */

class CustomCornerSprite : public DynamicBatchNode {
public:
	class Texture : public Ref, EventHandler {
	public:
		Texture();
		virtual ~Texture();

		virtual bool init(uint32_t size);
		void reload();

		cocos2d::Texture2D *getTexture() const;
		uint32_t getSize() const;

	protected:
	    virtual uint8_t *generateTexture(uint32_t size);

		uint32_t _size = 1;
		Rc<cocos2d::Texture2D> _tex;
	};

    virtual ~CustomCornerSprite();

    virtual bool init(uint32_t size, float density = 0.0f);
    virtual void onContentSizeDirty() override;
    virtual void setDensity(float density) override;

    virtual void setTextureSize(uint32_t size);
    virtual uint32_t getTextureSize() const;

    virtual uint32_t getDensityTextureSize() const;

    virtual void setDrawCenter(bool value);

protected:
	virtual Rc<Texture> generateTexture(uint32_t size);

	virtual cocos2d::Rect textureRectForGrid(int i, int j);
	virtual cocos2d::Vec2 texturePositionForGrid(int i, int j, float csx, float csy, float expx, float expy);

	virtual bool textureFlipX(int i, int j);
	virtual bool textureFlipY(int i, int j);

	virtual void updateSprites();

    uint32_t _textureSize = 0;

	cocos2d::Size _texContentSize = cocos2d::Size::ZERO;
	cocos2d::Size _minContentSize = cocos2d::Size::ZERO;

	Rc<Texture> _customTexture = nullptr;
	bool _drawCenter = true;
};

NS_SP_END

#endif /* LIBS_STAPPLER_NODES_BATCHED_SPCUSTOMCORNERSPRITE_H_ */
