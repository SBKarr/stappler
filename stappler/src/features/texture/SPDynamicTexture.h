/*
 * SPDynamicTexture.h
 *
 *  Created on: 14 июня 2016 г.
 *      Author: sbkarr
 */

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
