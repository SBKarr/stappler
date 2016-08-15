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
#include "base/CCMap.h"
#include "base/CCVector.h"

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
		std::string name;
		uint16_t version;
		bool dynamic;

		std::vector<FontRequest> requests;
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

#endif
