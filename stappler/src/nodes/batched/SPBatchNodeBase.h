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

#ifndef STAPPLER_SRC_NODES_BATCHED_SPBATCHNODEBASE_H_
#define STAPPLER_SRC_NODES_BATCHED_SPBATCHNODEBASE_H_

#include "SPDefine.h"
#include "base/CCProtocols.h"
#include "2d/CCNode.h"

NS_SP_BEGIN

class BatchNodeBase : public cocos2d::Node {
public:
	virtual ~BatchNodeBase();

	virtual bool init(float density = 0.0f);

	void setBlendFunc(const cocos2d::BlendFunc &blendFunc);
	const cocos2d::BlendFunc &getBlendFunc() const;

	virtual void setOpacityModifyRGB(bool modify) override;
	virtual bool isOpacityModifyRGB(void) const override;

	virtual bool isNormalized() const;
	virtual void setNormalized(bool value);

	virtual void setDensity(float density);
	virtual float getDensity() const;

protected:
	virtual void updateBlendFunc(cocos2d::Texture2D *);

	virtual cocos2d::GLProgramState *getProgramStateColor() const;
	virtual cocos2d::GLProgramState *getProgramStateA8() const;
	virtual cocos2d::GLProgramState *getProgramStateI8() const;
	virtual cocos2d::GLProgramState *getProgramStateAI88() const;
	virtual cocos2d::GLProgramState *getProgramStateFullColor() const;

	bool _opacityModifyRGB = false;
	bool _normalized = false;

	cocos2d::BlendFunc _blendFunc;
	float _density = 1.0f;
};

NS_SP_END

#endif /* STAPPLER_SRC_NODES_BATCHED_SPBATCHNODEBASE_H_ */
