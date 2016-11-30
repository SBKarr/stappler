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

	virtual cocos2d::GLProgramState *getProgramStateI8() const override;

	virtual void updateCanvas() override;
	virtual void setDynamicIcon(DynamicIcon *);
	virtual void setStaticIcon(IconName);

	float _progress = 0.0f;
	IconName _iconName = IconName::None;
	DynamicIcon *_dynamicIcon = nullptr;
};

NS_MD_END

#endif /* CLASSES_MATERIAL_MATERIALICON_H_ */
