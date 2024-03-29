/**
 Copyright (c) 2022 Roman Katuntsev <sbkarr@stappler.org>

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


#ifndef COMPONENTS_LAYOUT_FONT_SLFONTSOURCE_H_
#define COMPONENTS_LAYOUT_FONT_SLFONTSOURCE_H_

#include "SLFont.h"

#ifndef SP_DISABLE_FONT_LAYOUT

NS_LAYOUT_BEGIN

using FontTextureMap = Map<String, Vector<CharTexture>>;

struct FontTextureInterface {
	Function<size_t(uint16_t, uint16_t)> emplaceTexture;
	Function<bool(size_t, const void *data, uint16_t offsetX, uint16_t offsetY, uint16_t width, uint16_t height)> draw;
};

/* Font layout data
 *
 * this immutable struct stores data about
 * - basic fonts metric
 * - char layout for formatting
 * - kerning data for formatting
 */
struct FontData final : public Ref {
	bool init();
	bool init(const FontData &data);

	uint16_t getHeight() const;
	int16_t getAscender() const;
	int16_t getDescender() const;

	CharLayout getChar(char16_t c) const;

	uint16_t xAdvance(char16_t c) const;
	int16_t kerningAmount(char16_t first, char16_t second) const;

	const Metrics &getMetrics() const;

	Metrics metrics;
	Vector<CharLayout> chars;
	Map<uint32_t, int16_t> kerning;
};

/* FontLayout - Copy-on-write wrapper for FontData
 *
 */
class FontLayout final : public Ref {
public:
	// Callback to acquire metrics from font source and size
	using MetricCallback = Function<Metrics(const FontSource *,
			const Vector<FontFace::FontFaceSource> &, FontSize, const ReceiptCallback &)>;

	using UpdateCallback = Function<Rc<FontData>(const FontSource *,
			const Vector<FontFace::FontFaceSource> &, const Rc<FontData> &, const Vector<char16_t> &, const ReceiptCallback &)>;

	bool init(const FontSource *, const String &name, const StringView &family, FontSize size, const FontFace &, const ReceiptCallback &,
			const MetricCallback &, const UpdateCallback &);
	virtual ~FontLayout();

	/* addString functions will update layout data from its font face
	 * this call may be very expensive */

	bool addString(const String &);
	bool addString(const WideString &);
	bool addString(const char16_t *, size_t);

	bool addString(const FontCharString &);

	bool addSortedChars(const Vector<char16_t> &); // should be sorted vector

	Rc<FontData> getData();

	const String &getName() const;
	const String &getFamily() const;
	const ReceiptCallback &getCallback() const;
	const FontFace &getFontFace() const;

	FontSize getSize() const;

	FontParameters getStyle() const;

protected:
	bool merge(const Vector<char16_t> &);

	String _name;
	String _family;
	FontSize _dsize;
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

	/* Font face map defines association between font name and specific font faces with sources */
	using FontFaceMap = Map<String, Vector<FontFace>>;

	using SearchDirs = Vector<String>;

	/* Heuristical scoring for required font parameters and existed font faces
	 *
	 * This function can be used to find most appropriate FontFace for drawing with parameters */
	static size_t getFontFaceScore(const FontParameters &label, const FontFace &file);

	/* Merge source FontFaceMap into target */
	static void mergeFontFace(FontFaceMap &target, const FontFaceMap &source);

	/* Construct FontParameters struct from name, size and styling enums (FontStyle/FontWeight/FontStretch) */
	template <typename ... Args>
	static FontParameters getFontParameters(const String &family, FontSize size, Args && ... args) {
		FontParameters p;
		p.fontFamily = family;
		p.fontSize = size;
		readParameters(p, std::forward<Args>(args)...);
		return p;
	}

	virtual ~FontSource();

	/* face map and scale are persistent within source,
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

	StringView getFamilyName(uint32_t id) const;

	void addTextureString(const String &, const String &);
	void addTextureString(const String &, const WideString &);
	void addTextureString(const String &, const char16_t *, size_t);

	const Vector<char16_t> & addTextureChars(StringView, const Vector<CharSpec> &);
	const Vector<char16_t> & addTextureChars(StringView, const Vector<CharSpec> &, uint32_t start, uint32_t count);

	Vector<char16_t> &getTextureLayout(StringView);
	const Map<String, Vector<char16_t>> &getTextureLayoutMap() const;

	uint32_t getVersion() const;
	uint32_t getTextureVersion() const;

	bool isTextureRequestValid(uint32_t) const;
	bool isDirty() const;

	const SearchDirs &getSearchDirs() const;

	void preloadChars(const FontParameters &, const Vector<char16_t> &);

	float getDensity() const { return _density; }

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

class FormatterFontSource : public FormatterSourceInterface {
public:
	virtual ~FormatterFontSource() { }

	FormatterFontSource(Rc<FontSource> &&);

	virtual FontLayoutId getLayout(const FontParameters &f, float scale) override;
	virtual void addString(FontLayoutId, const FontCharString &) override;
	virtual uint16_t getFontHeight(FontLayoutId) override;
	virtual int16_t getKerningAmount(FontLayoutId, char16_t first, char16_t second, uint16_t face) const override;
	virtual Metrics getMetrics(FontLayoutId) override;
	virtual CharLayout getChar(FontLayoutId, char16_t, uint16_t &) override;
	virtual StringView getFontName(FontLayoutId) override;

protected:
	struct LayoutData {
		uint16_t id;
		Rc<FontLayout> layout;
		Rc<FontData> data;
	};

	uint16_t _nextId = 0;
	Rc<FontSource> _source;
	std::unordered_map<uint16_t, LayoutData> _layouts;
};

NS_LAYOUT_END

#endif

#endif /* COMPONENTS_LAYOUT_FONT_SLFONTSOURCE_H_ */
