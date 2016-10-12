/****************************************************************************
 Copyright (c) 2013      Zynga Inc.
 Copyright (c) 2013-2014 Chukong Technologies Inc.

 http://www.cocos2d-x.org

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
 ****************************************************************************/

#ifndef _SPFont_h_
#define _SPFont_h_

#include "SPImage.h"
#include "SPEventHandler.h"
#include "SPDataSubscription.h"
#include "SPDataListener.h"
#include "SPCharGroup.h"
#include "SPRichTextStyle.h"
#include "SPRichTextNode.h"
#include "SPDynamicTexture.h"
#include "base/CCMap.h"
#include "base/CCVector.h"

#include <mutex>

NS_SP_BEGIN

class Font : public cocos2d::Ref {
public:
	struct Char {
		Char(uint16_t ID) : charID(ID) { }

		Char(uint16_t ID, const cocos2d::Rect &rect, int16_t xOff, int16_t yOff, uint16_t xAdv)
		: charID(ID), rect(rect), xOffset(xOff), yOffset(yOff), xAdvance(xAdv) { }

		Char(const Char &) = default;

		inline float x() const { return rect.origin.x; }
		inline float y() const { return rect.origin.y; }
		inline float width() const { return rect.size.width; }
		inline float height() const { return rect.size.height; }

		inline const Font * getFont() const { return font; }

		bool operator== (const Char &other) const { return charID == other.charID; }
		bool operator< (const Char &other) const { return charID < other.charID; }
		bool operator== (char16_t c) const { return charID == c; }
		bool operator< (char16_t c) const { return charID < c; }
		operator bool() const { return charID != 0; }

		char16_t charID = 0;
		cocos2d::Rect rect;
		int16_t xOffset = 0;
		int16_t yOffset = 0;
		uint16_t xAdvance = 0;
		const Font *font = nullptr;
	};

	struct CharBlock {
		uint16_t width;
		uint16_t height;
	};

	struct CharSpec {
		enum Display : uint8_t {
			Char,
			Block,
			Hidden
		} display = Char;

		uint8_t underline = 0;
		int16_t posX = 0;
		int16_t posY = 0;
		cocos2d::Color4B color;

		union {
			const Font::Char *uCharPtr = nullptr;
			CharBlock uBlockPtr;
		};

		inline void set(const Font::Char *c, Display d = Char) {
			uCharPtr = c;
			display = d;
		}

		inline void set(uint16_t w, uint16_t h) {
			uBlockPtr = CharBlock{w, h};
			display = Block;
		}

		inline char16_t charId() const {
			switch (display) {
			case Char:
			case Hidden:
				return uCharPtr->charID;
				break;
			case Block:
				break;
			}
			return 0;
		}

		inline bool drawable() const {
			return display == Char;
		}

		inline uint16_t xAdvance() const {
			switch (display) {
			case Char:
			case Hidden:
				return uCharPtr->xAdvance;
				break;
			case Block:
				return uBlockPtr.width;
				break;
			}
			return 0;
		}
		inline uint16_t xAdvancePosition() const {
			switch (display) {
			case Char:
			case Hidden:
				return posX + uCharPtr->xOffset + (uint16_t)uCharPtr->width();
				break;
			case Block:
				return posX + uBlockPtr.width;
				break;
			}
			return 0;
		}

		inline uint16_t width() const {
			switch (display) {
			case Char:
			case Hidden:
				return (uint16_t)uCharPtr->width();
				break;
			case Block:
				return uBlockPtr.width;
				break;
			}
			return 0;
		}

		inline uint16_t height() const {
			switch (display) {
			case Char:
			case Hidden:
				return (uint16_t)uCharPtr->height();
				break;
			case Block:
				return uBlockPtr.height;
				break;
			}
			return 0;
		}
	};

	struct Metrics {
		uint16_t size;
		uint16_t height;
		int16_t ascender;
		int16_t descender;
		int16_t underlinePosition;
		int16_t underlineThickness;
	};

	using CharSet = std::set<Char>;
	using KerningMap = std::map<uint32_t, int16_t>;

	struct Data {
		Font::Metrics metrics;
		Font::CharSet chars;
		Font::KerningMap kerning;
	};

public:
	Font(const std::string &name, Metrics &&metrics, CharSet &&chars, KerningMap &&kerning, Image *tex);
	virtual ~Font();

	inline const std::string &getName() const { return _name; }

	inline uint16_t getSize() const { return _metrics.size; }
	inline uint16_t getHeight() const { return _metrics.height; }
	inline int16_t getAscender() const { return _metrics.ascender; }
	inline int16_t getDescender() const { return _metrics.descender; }
	inline int16_t getUnderlinePosition() const { return _metrics.underlinePosition; }
	inline int16_t getUnderlineThickness() const { return _metrics.underlineThickness; }
	inline Image *getImage() const { return _image; }
	inline FontSet *getFontSet() const { return _fontSet; }
	inline const Char *getChar(char16_t c) const {
		if (c == 160) { c = ' '; } // other &nbsp; handling failed on android
		if (c == 0x2011) { c = '-'; }
		auto it = _chars.find(c);
		if (it != _chars.end()) {
			return &(*it);
		}
		return nullptr;
	}

	cocos2d::Texture2D *getTexture() const;
	int16_t kerningAmount(uint16_t first, uint16_t second) const;

private:
	friend class FontSet;
	void setFontSet(FontSet *set);

    std::string _name;
	Metrics _metrics;
	CharSet _chars;
	KerningMap _kerning;

	Rc<Image>_image = nullptr;
	FontSet *_fontSet = nullptr;
};

/* Шрифтовый запрос описывает необходимый приложению шрифт.
 * В запросе описывается название, размер и список требуемых символов (в виде списка или с помощью символьной группы)
 * Для создания шрифта используется список источников,
 * Построитель карты просматривает источники в обратом порядке, пока не найдёт шрифт с нужным символом.
 * Если символа нет во всех указанных шрифтах, будет использован шрифт по умолчанию (fallback font)
 */
struct FontRequest {
	using Receipt = std::vector<std::string>;
	using CharGroup = stappler::chars::CharGroupId;

	std::string name;
	uint16_t size;
	std::u16string chars;
	Receipt receipt;
	std::vector<std::string> aliases;
	CharGroup charGroupMask;

	FontRequest(const std::string &name, uint16_t size, const std::u16string &chars);

	FontRequest(const std::string &name, const std::string &file, uint16_t size);
	FontRequest(const std::string &name, const std::string &file, uint16_t size, const std::u16string &chars);
	FontRequest(const std::string &name, const std::string &file, uint16_t size, CharGroup charGroupMask);
	FontRequest(const std::string &name, Receipt &&file, uint16_t size, const std::u16string &chars);
};

/** FontSet хранит шрифтовые карты, связанные с одной и той же текстурой.
 * Таким образом, все шрифты из FontSet можно рисовать за один вызов GL.
 *
 * FontSet неизменяем во время работы за исключением псевдонимов шрифтов.
 * Создать новый FontSet можно только асинхронным вызовом ResourceLibrary.
 */
class FontSet : public cocos2d::Ref {
public:
	using FontMap = cocos2d::Map<std::string, Font *>;
	using Callback = std::function<void(FontSet *)>;
	using ReceiptCallback = std::function<Bytes(const String &)>;
	struct Config {
		String name;
		uint16_t version;
		bool dynamic;

		Vector<FontRequest> requests;
		ReceiptCallback receiptCallback;

		Config(const std::vector<FontRequest> &vec, const ReceiptCallback & = nullptr);
		Config(std::vector<FontRequest> &&vec, const ReceiptCallback & = nullptr);

		Config(const std::string &, uint16_t, std::vector<FontRequest> &&vec, const ReceiptCallback & = nullptr);
		Config(const std::string &, uint16_t, const std::vector<FontRequest> &vec, const ReceiptCallback & = nullptr);
	};
public:
	static void generate(Config &&, const Callback &callback);

	FontSet(Config &&, cocos2d::Map<std::string, Font *> &&fonts, Image *image);
	~FontSet();

	Font *getFont(const std::string &) const;
	std::string getFontNameForAlias(const std::string &alias) const;

	const FontMap &getAllFonts() const { return _fonts; }
	const std::string &getName() const { return _config.name; }
	uint16_t getVersion() const { return _config.version; }
	Image *getImage() const { return _image; }

protected:
	friend class ResourceLibrary;

	Config _config;

	Rc<Image>_image = nullptr;
	FontMap _fonts;

	std::map<std::string, std::string> _aliases;
};

// Источник создаёт шрифтовые карты на основании запроса и реквизита
// Если запрос содержит веб-адреса, источник контролирует процесс загрузки и обновления данных из сети
// Шаги создания
// - 1) создаётся объект, запрашиваются ассеты
// - 2) после получения всех ассетов вызывается создание карты с использованием уже имеющихся файлов и ассетов,
// - 3) вызывается загрузка и обновление отсутствующих ассетов
// - 4) после завершения всех загрузок вызывается создание обновлённой карты
class FontSource : public data::Subscription, public EventHandler {
public:
	using ReceiptCallback = FontSet::ReceiptCallback;

	virtual bool init(const ReceiptCallback & = nullptr, bool wait = false);
	virtual bool init(const std::string &name, uint16_t, const std::vector<FontRequest> &, const ReceiptCallback & = nullptr, bool wait = false);
	virtual bool init(const std::string &name, uint16_t, std::vector<FontRequest> &&, const ReceiptCallback & = nullptr, bool wait = false);
	virtual bool init(const std::vector<FontRequest> &, const ReceiptCallback & = nullptr, bool wait = false);
	virtual bool init(std::vector<FontRequest> &&, const ReceiptCallback & = nullptr, bool wait = false);

	virtual ~FontSource();

	void setRequest(const std::vector<FontRequest> &);
	void setRequest(std::vector<FontRequest> &&);

	FontSet *getFontSet() const;

	bool isInUpdate() const { return _inUpdate; }

protected:
	FontSource();

	virtual void onAssets(const std::vector<Asset *> &);
	virtual void onAssetUpdated(Asset *);
	virtual void onFontSet(FontSet *);
	virtual void updateFontSet();
	virtual void updateRequest();

	std::string _name;
	uint16_t _version = 0;
	Rc<FontSet>_fontSet = nullptr;
	ReceiptCallback _receiptCallback = nullptr;

	std::set<std::string> _urls;
	std::vector<FontRequest> _requests;
	std::vector<data::Listener<Asset>> _assets;
	std::map<Asset *, std::pair<std::string, uint64_t>> _runningAssets;

	bool _inUpdate = false;
	bool _waitForAllAssets = false;
};

NS_SP_END

NS_SP_EXT_BEGIN(font)

using FontFace = rich_text::style::FontFace;
using FontParameters = rich_text::style::FontStyleParameters;

using FontStyle = rich_text::style::FontStyle;
using FontWeight = rich_text::style::FontWeight;
using FontStretch = rich_text::style::FontStretch;
using ReceiptCallback = Function<Bytes(const String &)>;

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

	FontLayout(const String &name, const String &family, uint8_t size, const FontFace &, const ReceiptCallback &, float);

	/* addString functions will update layout data from its font face
	 * this call may be very expensive */

	void addString(const String &);
	void addString(const WideString &);
	void addString(const char16_t *, size_t);

	void addString(const FontCharString &);

	Arc<Data> getData() const;

	const String &getName() const;
	const ReceiptCallback &getCallback() const;
	const FontFace &getFontFace() const;

	uint16_t getSize() const;

protected:
	static Metrics requestMetrics(const Vector<String> &, uint16_t, const ReceiptCallback &);
	static Arc<Data> requestLayoutUpgrade(const Vector<String> &, const Arc<Data> &, const Vector<char16_t> &, const ReceiptCallback &);

	void merge(const Vector<char16_t> &);

	float _density;
	String _name;
	String _family;
	uint8_t _size;
	FontFace _face;
	Arc<Data> _data;
	ReceiptCallback _callback = nullptr;
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
	// when layout was updated, you should recalculate all labels positions and sizes
	// maybe, it's simplier just rebuild your layout?
	static EventHeader onLayoutUpdated;

	// when texture was updated, all labels should recalculate quads arrays
	static EventHeader onTextureUpdated;

	using FontFaceMap = Map<String, Vector<FontFace>>;

	static size_t getFontFaceScore(const FontParameters &label, const FontFace &file);

	~Source();

	/* face map and scale is persistent within source,
	 * you should create another source object, if you want another map or scale */
	bool init(FontFaceMap &&, const ReceiptCallback &, float scale = 1.0f); // reinit is not allowed

	Arc<FontLayout> getLayout(const FontParameters &); // returns persistent ptr, Layout will be created if needed
	Arc<FontLayout> getLayout(const String &); // returns persistent ptr

	template <typename ... Args>
	Arc<FontLayout> getLayout(const String &family, uint8_t size, Args && ... args) {
		FontParameters p;
		p.fontFamily = family;
		p.fontSize = size;
		readParameters(p, std::forward<Args>(args)...);
		return getLayout(p);
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
	cocos2d::Texture2D *getTexture(uint8_t) const;
	const Vector<Rc<cocos2d::Texture2D>> &getTextures() const;

	uint32_t getVersion() const;

	bool isTextureRequestValid(uint32_t) const;
	bool isDirty() const;

protected:
	void readParameter(FontParameters &p, FontStyle style) {
		p.fontStyle = style;
	}
	void readParameter(FontParameters &p, FontWeight weight) {
		p.fontWeight = weight;
	}
	void readParameter(FontParameters &p, FontStretch stretch) {
		p.fontStretch = stretch;
	}

	template <typename T, typename ... Args>
	void readParameters(FontParameters &p, T && t, Args && ... args) {
		readParameter(p, t);
		readParameters(p, std::forward<Args>(args)...);
	}

	template <typename T>
	void readParameters(FontParameters &p, T && t) {
		readParameter(p, t);
	}

	FontFace * getFontFace(const String &name, const FontParameters &);

	void onTextureResult(Vector<Rc<cocos2d::Texture2D>> &&);

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
