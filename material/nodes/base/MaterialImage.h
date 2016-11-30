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
