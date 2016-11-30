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

#ifndef LIBS_STAPPLER_NODES_ROUNDED_SPROUNDEDLAYER_H_
#define LIBS_STAPPLER_NODES_ROUNDED_SPROUNDEDLAYER_H_

#include "SPCustomCornerSprite.h"

NS_SP_BEGIN

class RoundedSprite : public CustomCornerSprite {
public:
	class RoundedTexture : public CustomCornerSprite::Texture {
	public:
		virtual bool init(uint32_t size) override;
		virtual ~RoundedTexture();
	    virtual uint8_t *generateTexture(uint32_t size) override;
	};

protected:
	virtual Rc<Texture> generateTexture(uint32_t size);
};

NS_SP_END

#endif /* LIBS_STAPPLER_NODES_ROUNDED_SPROUNDEDLAYER_H_ */
