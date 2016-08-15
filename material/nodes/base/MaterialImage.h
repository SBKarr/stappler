/*
 * MaterialImage.h
 *
 *  Created on: 19 нояб. 2014 г.
 *      Author: sbkarr
 */

#ifndef CLASSES_MATERIAL_NODES_MATERIALIMAGE_H_
#define CLASSES_MATERIAL_NODES_MATERIALIMAGE_H_

#include "MaterialNode.h"
#include "SPDynamicSprite.h"

NS_MD_BEGIN

class MaterialImage : public MaterialNode {
public:
	using Autofit = stappler::DynamicSprite::Autofit;
	using ImageSizeCallback = std::function<void(uint32_t width, uint32_t height)>;

	virtual ~MaterialImage();
	virtual bool init(const std::string &file, float density = 0.0f);
	virtual void setContentSize(const cocos2d::Size &) override;
	virtual void visit(cocos2d::Renderer *r, const cocos2d::Mat4 &t, uint32_t f, ZPath &zPath) override;

	virtual void onEnter() override;
	virtual void onExit() override;

	virtual void setAutofit(Autofit);
	virtual void setAutofitPosition(const cocos2d::Vec2 &);

	virtual const std::string &getUrl() const;
	virtual void setUrl(const std::string &, bool force = false);
	virtual void setImageSizeCallback(const ImageSizeCallback &);

    virtual stappler::DynamicSprite * getSprite() const;
    virtual stappler::NetworkSprite * getNetworkSprite() const;
    virtual cocos2d::Texture2D *getTexture() const;

protected:
	virtual void onNetworkSprite();

	stappler::DynamicSprite *_sprite = nullptr;
	stappler::NetworkSprite *_network = nullptr;

	bool _init = false;

	ImageSizeCallback _callback;
};

NS_MD_END

#endif /* CLASSES_MATERIAL_NODES_MATERIALIMAGE_H_ */
