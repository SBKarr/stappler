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

#ifndef _SPFont_h_
#define _SPFont_h_

#include "SPImage.h"
#include "SPEventHandler.h"
#include "SPDataSubscription.h"
#include "SPDataListener.h"
#include "SPCharGroup.h"
#include "SPRichTextStyle.h"
#include "SPRichTextNode.h"
#include "SPAsset.h"
#include "base/CCMap.h"
#include "base/CCVector.h"

#include <mutex>

NS_SP_EXT_BEGIN(font)

using FontFace = rich_text::style::FontFace;
using FontParameters = rich_text::style::FontStyleParameters;

using FontStyle = rich_text::style::FontStyle;
using FontWeight = rich_text::style::FontWeight;
using FontStretch = rich_text::style::FontStretch;
using ReceiptCallback = Function<Bytes(const Source *, const String &)>;

struct Metrics {
	uint16_t size = 0;
	uint16_t height = 0;
	int16_t ascender = 0;
	int16_t descender = 0;
	int16_t underlinePosition = 0;
	int16_t underlineThickness = 0;
};

struct CharLayout {
	char16_t charID = 0;
	int16_t xOffset = 0;
	int16_t yOffset = 0;
	uint16_t xAdvance = 0;

	operator char16_t() const { return charID; }
};

struct CharSpec { // 8 bytes
	char16_t charID = 0;
	uint16_t pos = 0;
	uint16_t advance = 0;
	enum Display : uint16_t {
		Char,
		Block,
		Hidden
	} display = Char;
};

struct FontCharString {
	void addChar(char16_t);
	void addString(const String &);
	void addString(const WideString &);
	void addString(const char16_t *, size_t);

	Vector<char16_t> chars;
};

struct FontData {
	uint16_t getHeight() const;
	int16_t getAscender() const;
	int16_t getDescender() const;

	CharLayout getChar(char16_t c) const;

	uint16_t xAdvance(char16_t c) const;
	int16_t kerningAmount(char16_t first, char16_t second) const;

	Metrics metrics;
	Vector<CharLayout> chars;
	Map<uint32_t, int16_t> kerning;
};

class FontLayout : public Ref {
public:
	using Data = FontData;

	FontLayout(const Source *, const String &name, const String &family, uint8_t size, const FontFace &, const ReceiptCallback &, float);

	/* addString functions will update layout data from its font face
	 * this call may be very expensive */

	void addString(const String &);
	void addString(const WideString &);
	void addString(const char16_t *, size_t);

	void addString(const FontCharString &);

	void addSortedChars(const Vector<char16_t> &); // should be sorted vector

	Arc<Data> getData();

	const String &getName() const;
	const String &getFamily() const;
	const ReceiptCallback &getCallback() const;
	const FontFace &getFontFace() const;

	float getDensity() const;
	uint8_t getOriginalSize() const;
	uint16_t getSize() const;

	FontParameters getStyle() const;

protected:
	static Metrics requestMetrics(const Source *source, const Vector<String> &, uint16_t, const ReceiptCallback &);
	static Arc<Data> requestLayoutUpgrade(const Source *source, const Vector<String> &, const Arc<Data> &, const Vector<char16_t> &, const ReceiptCallback &);

	void merge(const Vector<char16_t> &);

	float _density;
	String _name;
	String _family;
	uint8_t _size;
	FontFace _face;
	Arc<Data> _data;
	ReceiptCallback _callback = nullptr;
	const Source * _source = nullptr;
	std::mutex _mutex;
};

struct CharTexture {
	char16_t charID = 0;
	uint16_t x = 0;
	uint16_t y = 0;
	uint16_t width = 0;
	uint16_t height = 0;
	uint8_t texture = maxOf<uint8_t>();

	operator char16_t() const { return charID; }
};

class Source : public Ref, public EventHandler {
public:
	// when texture was updated, all labels should recalculate quads arrays
	static EventHeader onTextureUpdated;

	using FontFaceMap = Map<String, Vector<FontFace>>;
	using AssetMap = Map<String, Rc<AssetFile>>;
	using SearchDirs = Vector<String>;

	static size_t getFontFaceScore(const FontParameters &label, const FontFace &file);
	static Bytes acquireFontData(const Source *, const String &, const ReceiptCallback &);

	template <typename ... Args>
	static FontParameters getFontParameters(const String &family, uint8_t size, Args && ... args) {
		FontParameters p;
		p.fontFamily = family;
		p.fontSize = size;
		readParameters(p, std::forward<Args>(args)...);
		return p;
	}

	~Source();

	/* face map and scale is persistent within source,
	 * you should create another source object, if you want another map or scale */
	bool init(FontFaceMap &&, const ReceiptCallback &, float scale = 1.0f, SearchDirs && = SearchDirs(), AssetMap && = AssetMap(), bool scheduled = true);

	void clone(Source *, const Function<void(Source *)> & = nullptr);

	Arc<FontLayout> getLayout(const FontParameters &); // returns persistent ptr, Layout will be created if needed
	Arc<FontLayout> getLayout(const String &); // returns persistent ptr

	bool hasLayout(const FontParameters &);
	bool hasLayout(const String &);

	template <typename ... Args>
	Arc<FontLayout> getLayout(const String &family, uint8_t size, Args && ... args) {
		return getLayout(getFontParameters(family, size, std::forward<Args>(args)...));
	}

	Map<String, Arc<FontLayout>> getLayoutMap();

	float getFontScale() const;
	void update(float dt);

	String getFamilyName(uint32_t id) const;

	void addTextureString(const String &, const String &);
	void addTextureString(const String &, const WideString &);
	void addTextureString(const String &, const char16_t *, size_t);

	const Vector<char16_t> & addTextureChars(const String &, const Vector<CharSpec> &);
	const Vector<char16_t> & addTextureChars(const String &, const Vector<CharSpec> &, uint32_t start, uint32_t count);

	Vector<char16_t> &getTextureLayout(const String &);
	const Map<String, Vector<char16_t>> &getTextureLayoutMap() const;
	cocos2d::Texture2D *getTexture(uint8_t) const;
	const Vector<Rc<cocos2d::Texture2D>> &getTextures() const;

	uint32_t getVersion() const;

	bool isTextureRequestValid(uint32_t) const;
	bool isDirty() const;

	const SearchDirs &getSearchDirs() const;
	const AssetMap &getAssetMap() const;

	void schedule();
	void unschedule();

	void preloadChars(const FontParameters &, const Vector<char16_t> &);

protected:
	static void readParameter(FontParameters &p, FontStyle style) {
		p.fontStyle = style;
	}
	static void readParameter(FontParameters &p, FontWeight weight) {
		p.fontWeight = weight;
	}
	static void readParameter(FontParameters &p, FontStretch stretch) {
		p.fontStretch = stretch;
	}

	template <typename T, typename ... Args>
	static void readParameters(FontParameters &p, T && t, Args && ... args) {
		readParameter(p, t);
		readParameters(p, std::forward<Args>(args)...);
	}

	template <typename T>
	static void readParameters(FontParameters &p, T && t) {
		readParameter(p, t);
	}

	FontFace * getFontFace(const String &name, const FontParameters &);

	void onTextureResult(Vector<Rc<cocos2d::Texture2D>> &&);
	void onTextureResult(Map<String, Vector<char16_t>> &&, Vector<Rc<cocos2d::Texture2D>> &&);

	void updateTexture(uint32_t, const Map<String, Vector<char16_t>> &);
	void cleanup();

	FontFaceMap _fontFaces; // persistent face map
	float _fontScale = 1.0f; // persistent scale value
	Map<uint32_t, String> _families;

	std::mutex _mutex;
	Map<String, Arc<FontLayout>> _layouts;
	ReceiptCallback _callback = nullptr;

	bool _dirty = false;
	Vector<Rc<cocos2d::Texture2D>> _textures;
	Map<String, Vector<char16_t>> _textureLayouts;

	std::atomic<uint32_t> _version;
	AssetMap _assets;
	SearchDirs _searchDirs;
	bool _scheduled = false;
};

class Controller : public data::Subscription {
public:
	static EventHeader onUpdate;
	static EventHeader onSource;

	using FontFaceMap = Map<String, Vector<FontFace>>;
	using AssetMap = Map<String, Rc<AssetFile>>;
	using SearchDirs = Vector<String>;

	static void mergeFontFace(FontFaceMap &target, const FontFaceMap &);

	~Controller();

	bool init(FontFaceMap && map, Vector<String> && searchDir, const ReceiptCallback & = nullptr);
	bool init(FontFaceMap && map, Vector<String> && searchDir, float scale, const ReceiptCallback & = nullptr);

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

	Source * getSource() const;

	void update(float dt);

protected:
	virtual void updateSource();
	virtual void performSourceUpdate();
	virtual void onSourceUpdated(Source *);

	virtual void onAssets(const std::vector<Asset *> &);
	virtual void onAssetUpdated(Asset *);

	virtual Rc<Source> makeSource(AssetMap &&);

	enum DirtyFlags : uint32_t {
		None = 0,
		DirtyAssets = 1,
		DirtySearchDirs = 2,
		DirtyFontFace = 4,
		DirtyReceiptCallback = 8,
	};

	uint32_t _dirtyFlags = None;

	bool _inUpdate = false;

	bool _dirty = true;
	float _scale = 1.0f;
	FontFaceMap _fontFaces;
	ReceiptCallback _callback = nullptr;
	SearchDirs _searchDir;

	Set<String> _urls;
	Vector<data::Listener<Asset>> _assets;

	Rc<Source> _source;
};

inline bool operator< (const CharTexture &t, const CharTexture &c) { return t.charID < c.charID; }
inline bool operator> (const CharTexture &t, const CharTexture &c) { return t.charID > c.charID; }
inline bool operator<= (const CharTexture &t, const CharTexture &c) { return t.charID <= c.charID; }
inline bool operator>= (const CharTexture &t, const CharTexture &c) { return t.charID >= c.charID; }

inline bool operator< (const CharTexture &t, const char16_t &c) { return t.charID < c; }
inline bool operator> (const CharTexture &t, const char16_t &c) { return t.charID > c; }
inline bool operator<= (const CharTexture &t, const char16_t &c) { return t.charID <= c; }
inline bool operator>= (const CharTexture &t, const char16_t &c) { return t.charID >= c; }

inline bool operator< (const CharLayout &l, const CharLayout &c) { return l.charID < c.charID; }
inline bool operator> (const CharLayout &l, const CharLayout &c) { return l.charID > c.charID; }
inline bool operator<= (const CharLayout &l, const CharLayout &c) { return l.charID <= c.charID; }
inline bool operator>= (const CharLayout &l, const CharLayout &c) { return l.charID >= c.charID; }

inline bool operator< (const CharLayout &l, const char16_t &c) { return l.charID < c; }
inline bool operator> (const CharLayout &l, const char16_t &c) { return l.charID > c; }
inline bool operator<= (const CharLayout &l, const char16_t &c) { return l.charID <= c; }
inline bool operator>= (const CharLayout &l, const char16_t &c) { return l.charID >= c; }

NS_SP_EXT_END(font)

#endif
