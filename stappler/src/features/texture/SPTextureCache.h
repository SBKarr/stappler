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


NS_CC_BEGIN

class GLView;

NS_CC_END


NS_SP_BEGIN

class TextureCache : public EventHandler {
public:
	static constexpr float GetDelayTime() { return 1.0f; }

	using Callback = std::function<void(cocos2d::Texture2D *)>;
	using CallbackVec = std::vector<Callback>;
	using CallbackMap = std::map<std::string, CallbackVec>;

	static TextureCache *getInstance();
	static Thread &thread();

	~TextureCache();

	void update(float dt);

	void addTexture(const std::string &, const Callback & = nullptr, bool forceReload = false);
	void addTexture(Asset *, const Callback & = nullptr, bool forceReload = false);

	bool hasTexture(const std::string &path);

	void uploadBitmap(Bitmap &&, const Function<void(cocos2d::Texture2D *)> &, Ref * = nullptr);
	void uploadBitmap(Vector<Bitmap> &&tex, const Function<void(Vector<Rc<cocos2d::Texture2D>> &&tex)> &, Ref * = nullptr);

	/* add texture, that was manually loaded, into cache, useful to ensure cache reloading */
	void addLoadedTexture(const String &, cocos2d::Texture2D *);
	void removeLoadedTexture(const String &);

	bool makeCurrentContext();
	void freeCurrentContext();

	template <typename T>
	auto performWithGL(const T &t) {
		struct ContextHolder {
			ContextHolder(TextureCache *c) : cache(c) {
				enabled = cache->makeCurrentContext();
			}
			~ContextHolder() {
				if (enabled) {
					cache->freeCurrentContext();
				}
			}

			bool enabled = false;
			TextureCache *cache;
		} holder(this);

		if (holder.enabled) {
			return t();
		} else {
			return decltype(t())();
		}
	}

protected:
	TextureCache();

	void registerWithDispatcher();
	void unregisterWithDispatcher();

	void uploadTextureBackground(Rc<cocos2d::Texture2D> &, const Bitmap &);
	void uploadTextureBackground(Vector<Rc<cocos2d::Texture2D>> &, const Vector<Bitmap> &);
	void initThread(cocos2d::GLView *);

	bool _registred = false;
	cocos2d::Map<std::string, cocos2d::Texture2D *> _textures;
	std::map<cocos2d::Texture2D *, std::pair<float, std::string>> _texturesScore;
	CallbackMap _callbackMap;
};

NS_SP_END

#endif /* LIBS_STAPPLER_FEATURES_TEXTURE_SPTEXTURECACHE_H_ */
