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

#ifndef STAPPLER_SRC_FEATURES_TEXTURE_SPDYNAMICTEXTURE_H_
#define STAPPLER_SRC_FEATURES_TEXTURE_SPDYNAMICTEXTURE_H_

#include "SPDefine.h"
#include "renderer/CCTexture2D.h"

NS_SP_BEGIN

class DynamicTexture : public Ref {
public:
	static bool isPboAvailable();

	using Callback = Function<void(cocos2d::Texture2D *newTex, bool pbo)>;

	~DynamicTexture();

	bool init(const Callback & = nullptr);
	void setBitmap(const Bitmap &);
	void drop();

	cocos2d::Texture2D *getTexture() const;

protected:
	void onBitmap(const Bitmap &);

	void onTexturePbo(const Bitmap &);
	void onTextureCreated(cocos2d::Texture2D *);

	void onTextureUpdated(cocos2d::Texture2D *, uint64_t);
	void onTexturePboUpdated(uint64_t);

	bool isAvaiableForPbo(const Bitmap &, cocos2d::Texture2D *);

	void schedule();
	void unschedule();

	Callback _callback = nullptr;
	Rc<cocos2d::Texture2D> _texture;
	uint64_t _time = 0;
	bool _scheduled = false;
};

NS_SP_END

#endif /* STAPPLER_SRC_FEATURES_TEXTURE_SPDYNAMICTEXTURE_H_ */
