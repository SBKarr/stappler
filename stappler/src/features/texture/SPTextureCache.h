/*
 * SPTextureCache.h
 *
 *  Created on: 24 мая 2015 г.
 *      Author: sbkarr
 */

#ifndef LIBS_STAPPLER_FEATURES_TEXTURE_SPTEXTURECACHE_H_
#define LIBS_STAPPLER_FEATURES_TEXTURE_SPTEXTURECACHE_H_

#include "SPDefine.h"
#include "SPEventHandler.h"
#include "base/CCMap.h"

NS_SP_BEGIN

class TextureCache : public EventHandler {
public:
	static constexpr float GetDelayTime() { return 1.0f; }

	using Callback = std::function<void(cocos2d::Texture2D *)>;
	using CallbackVec = std::vector<Callback>;
	using CallbackMap = std::map<std::string, CallbackVec>;

	static TextureCache *getInstance();
	static Thread &thread();

	void update(float dt);

	void addTexture(const std::string &, const Callback & = nullptr, bool forceReload = false);
	void addTexture(Asset *, const Callback & = nullptr, bool forceReload = false);

	bool hasTexture(const std::string &path);

protected:
	TextureCache();

	void registerWithDispatcher();
	void unregisterWithDispatcher();

	bool _registred = false;
	cocos2d::Map<std::string, cocos2d::Texture2D *> _textures;
	std::map<cocos2d::Texture2D *, std::pair<float, std::string>> _texturesScore;
	CallbackMap _callbackMap;
};

NS_SP_END

#endif /* LIBS_STAPPLER_FEATURES_TEXTURE_SPTEXTURECACHE_H_ */
