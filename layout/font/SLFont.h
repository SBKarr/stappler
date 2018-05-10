/**
Copyright (c) 2016-2018 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef LAYOUT_FONT_SLFONT_H_
#define LAYOUT_FONT_SLFONT_H_

#include "SLStyle.h"
#include "SLNode.h"
#include "SPCharGroup.h"

NS_LAYOUT_BEGIN

using FontFace = style::FontFace;
using FontParameters = style::FontStyleParameters;

using FontStyle = style::FontStyle;
using FontWeight = style::FontWeight;
using FontStretch = style::FontStretch;
using ReceiptCallback = Function<Bytes(const FontSource *, const String &)>;

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
	int16_t pos = 0;
	uint16_t advance = 0;
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

struct FontCharString {
	void addChar(char16_t);
	void addString(const String &);
	void addString(const WideString &);
	void addString(const char16_t *, size_t);

	Vector<char16_t> chars;
};

using FontTextureMap = Map<String, Vector<CharTexture>>;

struct FontTextureInterface {
	Function<size_t(uint16_t, uint16_t)> emplaceTexture;
	Function<bool(size_t, const void *data, uint16_t offsetX, uint16_t offsetY, uint16_t width, uint16_t height)> draw;
};

struct FontData final : public AtomicRef {
	bool init();
	bool init(const FontData &data);

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

class FontLayout final : public AtomicRef {
public:
	using MetricCallback = Function<Metrics(const FontSource *, const Vector<String> &, uint16_t, const ReceiptCallback &)>;
	using UpdateCallback = Function<Rc<FontData>(const FontSource *, const Vector<String> &, const Rc<FontData> &, const Vector<char16_t> &, const ReceiptCallback &)>;

	bool init(const FontSource *, const String &name, const StringView &family, uint8_t size, const FontFace &, const ReceiptCallback &,
			const MetricCallback &, const UpdateCallback &);
	virtual ~FontLayout();

	/* addString functions will update layout data from its font face
	 * this call may be very expensive */

	void addString(const String &);
	void addString(const WideString &);
	void addString(const char16_t *, size_t);

	void addString(const FontCharString &);

	void addSortedChars(const Vector<char16_t> &); // should be sorted vector

	Rc<FontData> getData();

	const String &getName() const;
	const String &getFamily() const;
	const ReceiptCallback &getCallback() const;
	const FontFace &getFontFace() const;

	//uint8_t getOriginalSize() const;
	uint16_t getSize() const;

	FontParameters getStyle() const;

protected:
	void merge(const Vector<char16_t> &);

	//float _density;
	String _name;
	String _family;
	uint16_t _dsize;
	FontFace _face;
	Rc<FontData> _data;
	ReceiptCallback _callback = nullptr;
	MetricCallback _metricCallback = nullptr;
	UpdateCallback _updateCallback = nullptr;
	const FontSource * _source = nullptr;
	Mutex _mutex;
};

class FontSource : public Ref {
public:
	using UpdateCallback = Function<void(uint32_t, const Map<String, Vector<char16_t>> &)>;
	using FontFaceMap = Map<String, Vector<FontFace>>;
	using SearchDirs = Vector<String>;

	static size_t getFontFaceScore(const FontParameters &label, const FontFace &file);
	static void mergeFontFace(FontFaceMap &target, const FontFaceMap &);

	template <typename ... Args>
	static FontParameters getFontParameters(const String &family, uint8_t size, Args && ... args) {
		FontParameters p;
		p.fontFamily = family;
		p.fontSize = size;
		readParameters(p, std::forward<Args>(args)...);
		return p;
	}

	virtual ~FontSource();

	/* face map and scale is persistent within source,
	 * you should create another source object, if you want another map or scale */
	virtual bool init(FontFaceMap &&, const ReceiptCallback &, float scale = 1.0f, SearchDirs && = SearchDirs());

	Rc<FontLayout> getLayout(const FontParameters &, float scale = nan()); // returns persistent ptr, Layout will be created if needed
	Rc<FontLayout> getLayout(const String &); // returns persistent ptr

	bool hasLayout(const FontParameters &);
	bool hasLayout(const String &);

	template <typename ... Args>
	Rc<FontLayout> getLayout(const String &family, uint8_t size, Args && ... args) {
		return getLayout(getFontParameters(family, size, std::forward<Args>(args)...));
	}

	Map<String, Rc<FontLayout>> getLayoutMap();

	float getFontScale() const;
	void update();

	String getFamilyName(uint32_t id) const;

	void addTextureString(const String &, const String &);
	void addTextureString(const String &, const WideString &);
	void addTextureString(const String &, const char16_t *, size_t);

	const Vector<char16_t> & addTextureChars(const String &, const Vector<CharSpec> &);
	const Vector<char16_t> & addTextureChars(const String &, const Vector<CharSpec> &, uint32_t start, uint32_t count);

	Vector<char16_t> &getTextureLayout(const String &);
	const Map<String, Vector<char16_t>> &getTextureLayoutMap() const;

	uint32_t getVersion() const;
	uint32_t getTextureVersion() const;

	bool isTextureRequestValid(uint32_t) const;
	bool isDirty() const;

	const SearchDirs &getSearchDirs() const;

	void preloadChars(const FontParameters &, const Vector<char16_t> &);

protected:
	Rc<FontLayout> getLayout(const FontLayout *); // returns persistent ptr, Layout will be created if needed

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

	FontFace * getFontFace(const StringView &name, const FontParameters &);

	void onResult(uint32_t v);

	FontFaceMap _fontFaces; // persistent face map
	float _fontScale = 1.0f; // persistent scale value
	float _density = 1.0f;
	Map<uint32_t, String> _families;

	Mutex _mutex;
	Map<String, Rc<FontLayout>> _layouts;
	ReceiptCallback _callback = nullptr;
	UpdateCallback _updateCallback = nullptr;
	FontLayout::MetricCallback _metricCallback = nullptr;
	FontLayout::UpdateCallback _layoutCallback = nullptr;

	bool _dirty = false;
	Map<String, Vector<char16_t>> _textureLayouts;

	uint32_t _textureVersion = 0;
	std::atomic<uint32_t> _version;
	SearchDirs _searchDirs;
	bool _scheduled = false;
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

NS_LAYOUT_END

#endif
