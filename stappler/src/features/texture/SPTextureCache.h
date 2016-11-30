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

#ifndef LIBS_STAPPLER_FEATURES_TEXTURE_SPTEXTURECACHE_H_
#define LIBS_STAPPLER_FEATURES_TEXTURE_SPTEXTURECACHE_H_

#include "SPGLProgramSet.h"
#include "SPEventHandler.h"
#include "SPBitmap.h"

#include "base/CCMap.h"
#include "renderer/CCTexture2D.h"

NS_SP_BEGIN

class TextureCache : public EventHandler {
public:
	static constexpr float GetDelayTime() { return 1.0f; }

	using Callback = Function<void(cocos2d::Texture2D *)>;
	using CallbackVec = Vector<Callback>;
	using CallbackMap = Map<String, CallbackVec>;

	static TextureCache *getInstance();
	static Thread &thread();

	static Rc<cocos2d::Texture2D> uploadTexture(const Bitmap &);
	static cocos2d::Texture2D::PixelFormat getPixelFormat(Bitmap::Format fmt);

	~TextureCache();

	GLProgramSet *getBatchPrograms() const;
	GLProgramSet *getRawPrograms() const;

	void update(float dt);

	void addTexture(const String &, const Callback & = nullptr, bool forceReload = false);
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

	uint32_t _contextRetained = 0;
	bool _registred = false;
	cocos2d::Map<String, cocos2d::Texture2D *> _textures;
	Map<cocos2d::Texture2D *, std::pair<float, String>> _texturesScore;
	CallbackMap _callbackMap;

	Rc<GLProgramSet> _batchDrawing;
	Rc<GLProgramSet> _rawDrawing;
};

NS_SP_END

#endif /* LIBS_STAPPLER_FEATURES_TEXTURE_SPTEXTURECACHE_H_ */
