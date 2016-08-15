/*
 * MaterialBackgroundLayer.h
 *
 *  Created on: 16 марта 2015 г.
 *      Author: sbkarr
 */

#ifndef LIBS_MATERIAL_NODES_SCENE_MATERIALBACKGROUNDLAYER_H_
#define LIBS_MATERIAL_NODES_SCENE_MATERIALBACKGROUNDLAYER_H_

#include "Material.h"
#include "2d/CCLayer.h"

NS_MD_BEGIN

class BackgroundLayer : public cocos2d::Layer {
public:
	virtual bool init() override;
	virtual void onContentSizeDirty() override;

	virtual void setTexture(cocos2d::Texture2D *);

	virtual void setColor(const cocos2d::Color3B &) override;

protected:
	cocos2d::LayerColor *_layer = nullptr;
	cocos2d::Sprite *_sprite = nullptr;
};

NS_MD_END

#endif /* LIBS_MATERIAL_NODES_SCENE_MATERIALBACKGROUNDLAYER_H_ */
