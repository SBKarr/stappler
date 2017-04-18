/**
Copyright (c) 2017 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef STAPPLER_SRC_FEATURES_RESOURCES_SPFONT_H_
#define STAPPLER_SRC_FEATURES_RESOURCES_SPFONT_H_

#include "SPEventHeader.h"
#include "SLFontFormatter.h"
#include "SLFontLibrary.h"

#include "SPDataListener.h"
#include "SPAsset.h"

NS_SP_EXT_BEGIN(font)

using ReceiptCallback = layout::ReceiptCallback;
using Metrics = layout::Metrics;
using CharLayout = layout::CharLayout;
using CharTexture = layout::CharTexture;
using FontCharString = layout::FontCharString;
using FontData = layout::FontData;
using FontLayout = layout::FontLayout;
using FontFace = layout::FontFace;

using CharSpec = layout::CharSpec;
using FormatSpec = layout::FormatSpec;
using LineSpec = layout::LineSpec;
using HyphenMap = layout::HyphenMap;
using FontParameters = layout::FontParameters;
using FontStyle = layout::FontStyle;
using FontWeight = layout::FontWeight;
using FontStretch = layout::FontStretch;
using TextTransform = layout::TextTransform;

class FontSource : public layout::FontSource, public EventHandler {
public:
	// when texture was updated, all labels should recalculate quads arrays
	static EventHeader onTextureUpdated;

	using AssetMap = Map<String, Rc<AssetFile>>;
	using FontTextureMap = layout::FontTextureMap;
	using FontTextureInterface = layout::FontTextureInterface;

	static Bytes acquireFontData(const layout::FontSource *, const String &, const ReceiptCallback &);

	static Metrics requestMetrics(const layout::FontSource *source, const Vector<String> &, uint16_t, const ReceiptCallback &);
	static Arc<FontData> requestLayoutUpgrade(const layout::FontSource *source, const Vector<String> &, const Arc<FontData> &, const Vector<char16_t> &, const ReceiptCallback &);

	virtual ~FontSource();

	virtual bool init(FontFaceMap &&, const ReceiptCallback &, float scale = 1.0f, SearchDirs && = SearchDirs(), AssetMap && = AssetMap(), bool scheduled = true);

	void clone(FontSource *, const Function<void(FontSource *)> & = nullptr);

	cocos2d::Texture2D *getTexture(uint8_t) const;
	const Vector<Rc<cocos2d::Texture2D>> &getTextures() const;
	const AssetMap &getAssetMap() const;

	void update(float dt);
	void schedule();
	void unschedule();

	void cleanup();

	FontTextureMap updateTextures(const Map<String, Vector<char16_t>> &l, Vector<Rc<cocos2d::Texture2D>> &);

protected:
	void updateTexture(uint32_t, const Map<String, Vector<char16_t>> &);
	void onTextureResult(Vector<Rc<cocos2d::Texture2D>> &&, uint32_t);
	void onTextureResult(Map<String, Vector<char16_t>> &&map, Vector<Rc<cocos2d::Texture2D>> &&tex, uint32_t v);

	Vector<Rc<cocos2d::Texture2D>> _textures;
	AssetMap _assets;
};

class FontController : public data::Subscription {
public:
	static EventHeader onUpdate;
	static EventHeader onSource;

	using FontFaceMap = Map<String, Vector<FontFace>>;
	using AssetMap = Map<String, Rc<AssetFile>>;
	using SearchDirs = Vector<String>;

	~FontController();

	bool init(FontFaceMap && map, Vector<String> && searchDir, const ReceiptCallback & = nullptr, bool scheduled = true);
	bool init(FontFaceMap && map, Vector<String> && searchDir, float scale, const ReceiptCallback & = nullptr, bool scheduled = true);

	void setSearchDirs(Vector<String> &&);
	void addSearchDir(const Vector<String> &);
	void addSearchDir(const String &);
	const Vector<String> &getSearchDir() const;

	void setFontFaceMap(FontFaceMap && map);
	void addFontFaceMap(const FontFaceMap & map);
	void addFontFace(const String &, FontFace &&);
	const FontFaceMap &getFontFaceMap() const;

	void setFontScale(float);
	float getFontScale() const;

	void setReceiptCallback(const ReceiptCallback &);
	const ReceiptCallback & getReceiptCallback() const;

	bool empty() const;

	FontSource * getSource() const;

	void update(float dt);

protected:
	virtual void updateSource();
	virtual void performSourceUpdate();
	virtual void onSourceUpdated(FontSource *);

	virtual void onAssets(const Vector<Rc<Asset>> &);
	virtual void onAssetUpdated(Asset *);

	virtual Rc<FontSource> makeSource(AssetMap &&, bool schedule = true);

	enum DirtyFlags : uint32_t {
		None = 0,
		DirtyAssets = 1,
		DirtySearchDirs = 2,
		DirtyFontFace = 4,
		DirtyReceiptCallback = 8,
	};

	uint32_t _dirtyFlags = None;

	bool _inUpdate = false;
	bool _scheduled = true;

	bool _dirty = true;
	float _scale = 1.0f;
	FontFaceMap _fontFaces;
	ReceiptCallback _callback = nullptr;
	SearchDirs _searchDir;

	Set<String> _urls;
	Vector<data::Listener<Asset>> _assets;

	Rc<FontSource> _source;
};

NS_SP_EXT_END(font)

#endif /* STAPPLER_SRC_FEATURES_RESOURCES_SPFONT_H_ */
