/*
 * MaterialRoundedProgress.h
 *
 *  Created on: 30 мая 2015 г.
 *      Author: sbkarr
 */

#ifndef LIBS_MATERIAL_NODES_PROGRESS_MATERIALROUNDEDPROGRESS_H_
#define LIBS_MATERIAL_NODES_PROGRESS_MATERIALROUNDEDPROGRESS_H_

#include "Material.h"
#include "MaterialColors.h"
#include "2d/CCNode.h"

NS_MD_BEGIN

class RoundedProgress : public cocos2d::Node {
public:
	enum Layout {
		Auto,
		Vertical,
		Horizontal
	};

	virtual bool init(Layout = Auto);
	virtual void onContentSizeDirty() override;

	virtual void setLayout(Layout);
	virtual Layout getLayout() const;

	virtual void setInverted(bool);
	virtual bool isInverted() const;

	virtual void setBorderRadius(uint32_t value);
	virtual uint32_t getBorderRadius() const;

	virtual void setProgress(float value, float anim = 0.0f);
	virtual float getProgress() const;

	virtual void setBarScale(float value);
	virtual float getBarScale() const;

	virtual void setLineColor(const Color &);
	virtual void setLineOpacity(uint8_t);

	virtual void setBarColor(const Color &);
	virtual void setBarOpacity(uint8_t);

protected:
	Layout _layout = Horizontal;
	bool _inverted = false;

	float _barScale = 1.0f;
	float _progress = 0.0f;

	stappler::RoundedSprite *_line = nullptr;
	stappler::RoundedSprite *_bar = nullptr;
};

NS_MD_END

#endif /* LIBS_MATERIAL_NODES_PROGRESS_MATERIALROUNDEDPROGRESS_H_ */
