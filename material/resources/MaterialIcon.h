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

class StaticIcon : public stappler::IconSprite {
public:
	static StaticIcon *defaultIcon(IconName icon);

	virtual bool init();
	virtual bool init(stappler::Icon *);

	virtual void setIconName(IconName icon);
	virtual IconName getIconName() const;

protected:
	IconName _iconName = IconName::None;
};

class DynamicIcon : public stappler::draw::PathNode {
public:
	enum class Name {
		Navigation,
		Loader,
		Empty,
		None,
	};

	class Icon : public cocos2d::Ref {
	public:
		virtual bool init();
		virtual void redraw(float progress, float diff);

		const cocos2d::Vector<stappler::draw::Path *> &getPaths() const;
		void setParent(DynamicIcon *);

		virtual void animate();
	protected:
		DynamicIcon *_parent = nullptr;
		cocos2d::Vector<stappler::draw::Path *> _paths;
	};

	static DynamicIcon *defaultIcon(Name icon);

	virtual ~DynamicIcon();

	virtual bool init() override;
	virtual bool init(int width, int height);
	virtual void onEnter() override;

	virtual void setProgress(float progress);
	virtual float getProgress();

	virtual void setIconName(Name name);
	virtual Name getIconName() const;

	virtual void animate();
	virtual void animate(float targetProgress, float duration);
	virtual void redraw(float progress, float diff);

protected:
	virtual void setIcon(Icon *i);

	Icon *_icon = nullptr;
	float _progress = 0.0f;
	Name _iconName = Name::None;
};

class DynamicIconProgress : public cocos2d::ActionInterval {
public:
    virtual bool init(float duration, float targetIndex);
    virtual void startWithTarget(cocos2d::Node *t) override;
    virtual void update(float time) override;
    virtual void stop() override;

protected:
    float _sourceProgress = 0;
    float _destProgress = 0;
};

NS_MD_END

#endif /* CLASSES_MATERIAL_MATERIALICON_H_ */
