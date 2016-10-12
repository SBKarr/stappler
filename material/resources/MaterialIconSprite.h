/*
 * MaterialIcon.h
 *
 *  Created on: 19 нояб. 2014 г.
 *      Author: sbkarr
 */

#ifndef CLASSES_MATERIAL_MATERIALICON_H_
#define CLASSES_MATERIAL_MATERIALICON_H_

#include "SPDrawPathNode.h"
#include "MaterialColors.h"
#include "MaterialIconSources.h"

#include "2d/CCActionInterval.h"
#include "renderer/CCCustomCommand.h"

#include "SPIconSprite.h"

NS_MD_BEGIN

class IconSprite : public draw::PathNode {
public:
	virtual ~IconSprite() { }

	virtual bool init(IconName name = IconName::None);
	virtual bool init(IconName name, uint32_t, uint32_t);

	virtual void onEnter() override;

	virtual void setIconName(IconName name);
	virtual IconName getIconName() const;

	virtual void setProgress(float progress);
	virtual float getProgress() const;

	virtual void animate();
	virtual void animate(float targetProgress, float duration);

	virtual bool isEmpty() const;
	virtual bool isDynamic() const;
	virtual bool isStatic() const;

protected:
	class DynamicIcon;
	class NavIcon;
	class CircleLoaderIcon;

	virtual void updateCanvas() override;
	virtual void setDynamicIcon(DynamicIcon *);
	virtual void setStaticIcon(IconName);

	float _progress = 0.0f;
	IconName _iconName = IconName::None;
	DynamicIcon *_dynamicIcon = nullptr;
};

NS_MD_END

#endif /* CLASSES_MATERIAL_MATERIALICON_H_ */
