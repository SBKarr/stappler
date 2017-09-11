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

#ifndef LIBS_MATERIAL_NODES_PROGRESS_MATERIALLINEARPROGRESS_H_
#define LIBS_MATERIAL_NODES_PROGRESS_MATERIALLINEARPROGRESS_H_

#include "Material.h"
#include "SPLayer.h"

NS_MD_BEGIN

class LinearProgress : public cocos2d::Node {
public:
	virtual bool init() override;
	virtual void onContentSizeDirty() override;

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
	float _progress = 0.0f;

	stappler::Layer *_line = nullptr;
	stappler::Layer *_bar = nullptr;
};

NS_MD_END

#endif /* LIBS_MATERIAL_NODES_PROGRESS_MATERIALLINEARPROGRESS_H_ */
