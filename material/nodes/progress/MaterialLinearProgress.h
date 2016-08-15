/*
 * MaterialLinearProgress.h
 *
 *  Created on: 17 мая 2015 г.
 *      Author: sbkarr
 */

#ifndef LIBS_MATERIAL_NODES_PROGRESS_MATERIALLINEARPROGRESS_H_
#define LIBS_MATERIAL_NODES_PROGRESS_MATERIALLINEARPROGRESS_H_

#include "Material.h"
#include "SPLayer.h"

NS_MD_BEGIN

class LinearProgress : public cocos2d::Node {
public:
	virtual bool init() override;
	virtual void onContentSizeDirty() override;

	virtual void setDeterminate(bool value);
	virtual bool isDeterminate() const;

	virtual void setAnimated(bool value);
	virtual bool isAnimated() const;

	virtual void setProgress(float value);
	virtual float getProgress() const;

	virtual void setLineColor(const Color &);
	virtual void setLineOpacity(uint8_t);

	virtual void setBarColor(const Color &);
	virtual void setBarOpacity(uint8_t);

protected:
	virtual void layoutSubviews();

	bool _animated = false;
	bool _determinate = true;

	float _progress = 0.0f;

	stappler::Layer *_line = nullptr;
	stappler::Layer *_bar = nullptr;
	stappler::Layer *_secondaryBar = nullptr;
};

NS_MD_END

#endif /* LIBS_MATERIAL_NODES_PROGRESS_MATERIALLINEARPROGRESS_H_ */
