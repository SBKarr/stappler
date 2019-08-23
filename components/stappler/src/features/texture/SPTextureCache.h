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

#include "SPBitmap.h"
#include "SPGLProgramSet.h"
#include "SPEventHandler.h"
#include "base/CCMap.h"
#include "renderer/CCTexture2D.h"

NS_SP_BEGIN

class TextureCache : public EventHandler {
public:
	static constexpr float GetDelayTime() { return 1.0f; }

	using BitmapFormat = Bitmap::PixelFormat;
	using TextureFormat = cocos2d::Texture2D::PixelFormat;

	struct TextureIndex {
		String file;
		BitmapFormat fmt = BitmapFormat::Auto;
		float density = screen::density();

		bool operator < (const TextureIndex &) const;
		bool operator > (const TextureIndex &) const;

		bool operator == (const TextureIndex &) const;
		bool operator != (const TextureIndex &) const;
	};

	struct AssetDownloader;

	using Callback = Function<void(cocos2d::Texture2D *)>;
	using CallbackVec = Vector<Callback>;
	using CallbackMap = Map<TextureIndex, CallbackVec>;

	static TextureCache *getInstance();
	static Thread &thread();

	static Rc<cocos2d::Texture2D> uploadTexture(const Bytes &, const Size &hint = Size(), float density = 1.0f);
	static Rc<cocos2d::Texture2D> uploadTexture(const Bitmap &);
	static cocos2d::Texture2D::PixelFormat getPixelFormat(BitmapFormat fmt);

	static String getPathForUrl(const String &);
	static bool isCachedTextureUrl(const String &);

	~TextureCache();

	GLProgramSet *getPrograms() const;

	void update(float dt);

	Rc<cocos2d::Texture2D> renderImage(cocos2d::Texture2D *, TextureFormat fmt, const layout::Image &,
			const Size &contentSize, layout::style::Autofit = layout::style::Autofit::Contain, const Vec2 &autofitPos = Anchor::Middle, float density = 0.0f);

	void renderImageInBackground(const Callback &, cocos2d::Texture2D *, TextureFormat fmt, const layout::Image &,
			const Size &contentSize, layout::style::Autofit = layout::style::Autofit::Contain, const Vec2 &autofitPos = Anchor::Middle, float density = 0.0f);

	void addTexture(const StringView &, const Callback & = nullptr, bool forceReload = false);
	void addTexture(const StringView &, float, const Callback & = nullptr, bool forceReload = false);
	void addTexture(const StringView &, float, BitmapFormat = BitmapFormat::Auto, const Callback & = nullptr, bool forceReload = false);

	void addTexture(Asset *, const Callback & = nullptr, bool forceReload = false);
	void addTexture(Asset *, float, const Callback & = nullptr, bool forceReload = false);
	void addTexture(Asset *, float, BitmapFormat = BitmapFormat::Auto, const Callback & = nullptr, bool forceReload = false);

	bool hasTexture(const StringView &, float = screen::density(), BitmapFormat = BitmapFormat::Auto);

	void uploadBitmap(Bitmap &&, const Function<void(cocos2d::Texture2D *)> &, Ref * = nullptr);
	void uploadBitmap(Vector<Bitmap> &&tex, const Function<void(Vector<Rc<cocos2d::Texture2D>> &&tex)> &, Ref * = nullptr);

	/* add texture, that was manually loaded, into cache, useful to ensure cache reloading */
	void addLoadedTexture(const StringView &, cocos2d::Texture2D *);
	void addLoadedTexture(const StringView &, float, cocos2d::Texture2D *);
	void addLoadedTexture(const StringView &, float, BitmapFormat, cocos2d::Texture2D *);

	void removeLoadedTexture(const StringView &, float = screen::density(), BitmapFormat = BitmapFormat::Auto);

	bool makeCurrentContext();
	void freeCurrentContext();

	void reloadTextures();

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

	void addAssetTexture(Asset *, float, BitmapFormat, const Callback &, bool forceReload);

	void reloadTexture(cocos2d::Texture2D *, const TextureIndex &); // performed in main thread
	Rc<cocos2d::Texture2D> loadTexture(const TextureIndex &); // performed in cache thread

	Rc<cocos2d::Texture2D> doRenderImage(cocos2d::Texture2D *, TextureFormat fmt, const layout::Image &,
			const Size &constntSize, layout::style::Autofit, const Vec2 &autofitPos, float density);

	uint32_t _contextRetained = 0;
	bool _registred = false;
	bool _reloadDirty = false;
	Map<TextureIndex, Rc<cocos2d::Texture2D>> _textures;
	Map<cocos2d::Texture2D *, std::pair<float, TextureIndex>> _texturesScore;
	CallbackMap _callbackMap;

	Rc<GLProgramSet> _mainProgramSet;
	Rc<GLProgramSet> _threadProgramSet;

	Rc<draw::Canvas> _vectorCanvas;
	Rc<draw::Canvas> _threadVectorCanvas;
};

NS_SP_END

#endif /* LIBS_STAPPLER_FEATURES_TEXTURE_SPTEXTURECACHE_H_ */
