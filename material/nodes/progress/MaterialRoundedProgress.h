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

#ifndef LIBS_MATERIAL_NODES_PROGRESS_MATERIALROUNDEDPROGRESS_H_
#define LIBS_MATERIAL_NODES_PROGRESS_MATERIALROUNDEDPROGRESS_H_

#include "Material.h"
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
