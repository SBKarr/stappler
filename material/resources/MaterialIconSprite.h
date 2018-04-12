/**
Copyright (c) 2016 Roman Katuntsev <sbkarr@stappler.org>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
**/

#ifndef MATERIAL_RESOURCES_MATERIALICONSPRITE_H_
#define MATERIAL_RESOURCES_MATERIALICONSPRITE_H_

#include "SPDrawPathNode.h"
#include "2d/CCActionInterval.h"
#include "MaterialIconStorage.h"
#include "renderer/CCCustomCommand.h"

NS_MD_BEGIN

class IconSprite : public draw::PathNode {
public:
	virtual ~IconSprite() { }

	enum class SizeHint {
		Small,
		Normal,
		Large
	};

	virtual bool init(IconName name = IconName::None, SizeHint = SizeHint::Normal);

	virtual void onContentSizeDirty() override;
	virtual void visit(cocos2d::Renderer *, const Mat4 &, uint32_t f, ZPath &) override;

	virtual void onEnter() override;

	virtual void setSizeHint(SizeHint);

	virtual void setIconName(IconName name);
	virtual IconName getIconName() const;

	virtual void setProgress(float progress);
	virtual float getProgress() const;
	virtual float getRawProgress() const;

	virtual void animate();
	virtual void animate(float targetProgress, float duration);

	virtual bool isEmpty() const;
	virtual bool isDynamic() const;
	virtual bool isStatic() const;

protected:
	class DynamicIcon;
	class NavIcon;
	class CircleLoaderIcon;
	class ExpandIcon;

	virtual void updateCanvas(layout::Subscription::Flags f) override;
	virtual void setDynamicIcon(DynamicIcon *);
	virtual void setStaticIcon(IconName);

	virtual void onUpdate();

	float _diff = 0.0f;
	float _progress = 0.0f;
	IconName _iconName = IconName::None;
	DynamicIcon *_dynamicIcon = nullptr;
	EventListener *_listener = nullptr;
	IconStorage *_storage = nullptr;
};

NS_MD_END

#endif /* MATERIAL_RESOURCES_MATERIALICONSPRITE_H_ */
