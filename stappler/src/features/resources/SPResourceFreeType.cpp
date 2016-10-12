/*
 * SPResourceFreeType.cpp
 *
 *  Created on: 10 июля 2015 г.
 *      Author: sbkarr
 */

#include "SPDefine.h"
#include "SPResource.h"
#include "SPImage.h"
#include "SPString.h"
#include "SPData.h"
#include "SPAsset.h"
#include "SPFilesystem.h"
#include "SPRichTextMultipartParser.h"

#ifdef DEBUG
#define SP_FONT_DEBUG 0

NS_SP_EXTERN_BEGIN
static std::map<int, std::string> s_errs;
static std::string s_unknownError("unknown error");
int sp_FT_Err_Def(int err, const char *str);
NS_SP_EXTERN_END
//#define FT_ERRORDEF( e, v, s ) static int e = sp_FT_Err_Def(v, s);
#endif

#include "ft2build.h"
#include FT_FREETYPE_H
#include FT_GLYPH_H

#ifdef DEBUG
NS_SP_EXTERN_BEGIN
int sp_FT_Err_Def(int err, const char *str) {
	s_errs.insert(std::make_pair(err, str));
	return err;
}
NS_SP_EXTERN_END
#endif

NS_SP_EXT_BEGIN(resource)

using AssetMap = cocos2d::Map<std::string, Asset *>;
using FontMap = std::map<std::string, Font::Data>;

FontSet * readFontSet(FontSet::Config &&cfg, const AssetMap &assets);
FontSet * writeFontSet(FontSet::Config &&cfg, const AssetMap &assets, FontMap &fonts,
		const uint8_t *imageData, uint32_t imageWidth, uint32_t imageHeight);

static const std::string &sp_FT_Err_Desc(int err) {
#ifdef DEBUG
	auto it = s_errs.find(err);
	if (it != s_errs.end()) {
		return it->second;
	} else {
		return s_unknownError;
	}
#else
	static std::string s_unknownError("unknown error");
	return s_unknownError;
#endif
}

#define LAYOUT_PADDING 2.0f

const char16_t * s_glyphs = \
u"\"!#$%&'()*+,-./:;<=>?@[\\]^_`{|}~«»— №…’" \
u"0123456789" u"ABCDEFGHIJKLMNOPQRSTUVWXYZ" u"abcdefghijklmnopqrstuvwxyz" \
u"АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ" u"абвгдеёжзийклмнопрстуфхцчшщъыьэюя" \
u"£¥₠₡₣₤₥₦₧₨₩₫₪€₭₮₯₰₱₲₳₴₵–₽" u"ńéè\u00ADβα";

class FreeTypeLibrary;

static FreeTypeLibrary *s_freeTypeLibraryInstance = nullptr;

struct LayoutChar {
	std::vector<Font::Char *> *_chars = nullptr;
	FT_Face _face = nullptr;

	float _width = 0.0f;
	float _height = 0.0f;
	char16_t _charID = 0;

	LayoutChar() { }
	LayoutChar(std::vector<Font::Char *> *c, FT_Face face, float w, float h, char16_t charId)
	: _chars(c), _face(face), _width(w), _height(h), _charID(charId) { }

	operator bool() {
		return _chars;
	}
};

struct charHeightComparator {
    bool operator() (const Font::Char * const & lhs, const Font::Char * const &rhs) const {
		auto h1 = (int)lhs->height(); auto h2 = (int)rhs->height();
		auto w1 = (int)lhs->width(); auto w2 = (int)rhs->width();
		if (h1 == h2 && w1 == w2) {
			return lhs->charID < rhs->charID;
		} else if (h1 == h2) {
			return lhs->width() > rhs->width();
		} else {
			return h1 > h2;
		}
    }

    bool operator() (const LayoutChar & lhs, const LayoutChar &rhs) const {
		auto h1 = (int)lhs._height; auto h2 = (int)rhs._height;
		auto w1 = (int)lhs._width; auto w2 = (int)rhs._width;
		if (h1 == h2 && w1 == w2) {
			if (lhs._face == rhs._face) {
				return lhs._charID < rhs._charID;
			} else {
				return lhs._face > rhs._face;
			}
		} else if (h1 == h2) {
			return w1 > w2;
		} else {
			return h1 > h2;
		}
    }

    bool operator() (LayoutChar * const & lhs, LayoutChar * const & rhs) const {
		auto h1 = (int)lhs->_height; auto h2 = (int)rhs->_height;
		auto w1 = (int)lhs->_width; auto w2 = (int)rhs->_width;
		if (h1 == h2 && w1 == w2) {
			if (lhs->_face == rhs->_face) {
				return lhs->_charID < rhs->_charID;
			} else {
				return lhs->_face > rhs->_face;
			}
		} else if (h1 == h2) {
			return w1 > w2;
		} else {
			return h1 > h2;
		}
    }
};

class LayoutBitmap : public Bitmap {
public:
	LayoutBitmap(uint32_t w, uint32_t h) : Bitmap() {
		alloc(w, h, Format::A8);
		_data.back() = 255;
	}

	void draw(const cocos2d::Rect &rect, FT_Bitmap *bitmap) {
		int xOffset = rect.origin.x;
		int yOffset = rect.origin.y;
		auto data = _data.data();
		for (decltype(bitmap->width) i = 0; i < bitmap->width; i++) {
			for (decltype(bitmap->rows) j = 0; j < bitmap->rows; j++) {
				int pos = j * bitmap->width + i;
				int bufferPos = ((yOffset + j) * _width + i + xOffset);
				if (bitmap->pixel_mode == FT_PIXEL_MODE_MONO) {
					data[bufferPos] = (bitmap->buffer[pos / 8] & (1 << pos % 8))?255:0;
				} else if (bitmap->pixel_mode == FT_PIXEL_MODE_GRAY) {
					data[bufferPos] = bitmap->buffer[pos];
				}
			}
		}
	}
};

class LayoutNode {
public:
    LayoutNode* _child[2];
    cocos2d::Rect _rc;
    LayoutChar _char;

	LayoutNode(const cocos2d::Rect &rect) {
		_rc = rect;
		_child[0] = nullptr;
		_child[1] = nullptr;
	}

	LayoutNode(const cocos2d::Vec2 &origin, LayoutChar c, bool rotated) {
		_child[0] = nullptr;
		_child[1] = nullptr;
		_char = c;
		_rc = cocos2d::Rect(origin.x, origin.y, c._width, c._height);
	}

	~LayoutNode() {
		if (_child[0]) delete _child[0];
		if (_child[1]) delete _child[1];
	}

	bool insert(LayoutChar c) {
		if (_child[0] && _child[1]) {
			return _child[0]->insert(c) || _child[1]->insert(c);
		} else {
			if (_char) {
				return false;
			}

			if ((_rc.size.width < c._width  || _rc.size.height < c._height)) {
				return false;
			}

			if (_rc.size.width < c._width  || _rc.size.height < c._height) {
				return false;
			}

			if (_rc.size.width == c._width || _rc.size.height == c._height) {
				if (_rc.size.width == c._width) {
					_child[0] = new LayoutNode(_rc.origin, c, false);
					_child[1] = new LayoutNode(cocos2d::Rect(_rc.origin.x, _rc.origin.y + c._height + LAYOUT_PADDING, _rc.size.width, _rc.size.height - c._height - LAYOUT_PADDING));
				} else if (_rc.size.height == c._height) {
					_child[0] = new LayoutNode(_rc.origin, c, false);
					_child[1] = new LayoutNode(cocos2d::Rect(_rc.origin.x + c._width + LAYOUT_PADDING, _rc.origin.y, _rc.size.width - c._width - LAYOUT_PADDING, _rc.size.height));
				}
				return true;
			}

			//(decide which way to split)
			float dw = _rc.size.width - c._width;
			float dh = _rc.size.height - c._height;

			if (dw > dh) {
				_child[0] = new LayoutNode(cocos2d::Rect(_rc.origin.x, _rc.origin.y, c._width, _rc.size.height));
				_child[1] = new LayoutNode(cocos2d::Rect(_rc.origin.x + c._width + LAYOUT_PADDING, _rc.origin.y, dw - LAYOUT_PADDING, _rc.size.height));
			} else {
				_child[0] = new LayoutNode(cocos2d::Rect(_rc.origin.x, _rc.origin.y, _rc.size.width, c._height));
				_child[1] = new LayoutNode(cocos2d::Rect(_rc.origin.x, _rc.origin.y + c._height + LAYOUT_PADDING, _rc.size.width, dh - LAYOUT_PADDING));
			}

			_child[0]->insert(c);

			return true;
		}
	}

	int nodes() {
		if (_char) {
			return 1;
		} else if (_child[0] && _child[1]) {
			return _child[0]->nodes() + _child[1]->nodes();
		} else {
			return 0;
		}
	}

	void drawChar(LayoutBitmap &bmp) {
		if (_char) {
			auto face = _char._face;
			auto charId = _char._charID;

			if (_char._width > 0 && _char._height > 0) {
				auto err = FT_Load_Char(face, (FT_ULong)charId, FT_LOAD_DEFAULT | FT_LOAD_RENDER);
				if (err != FT_Err_Ok) {
					log::format("Font", "load error: %d %s for '%s'", err, sp_FT_Err_Desc(err).c_str(), string::toUtf8(charId).c_str());
					return;
				}

				for (auto &it : *(_char._chars)) {
					it->rect.origin.x = _rc.origin.x;
					it->rect.origin.y = _rc.origin.y;
				}
				if (face->glyph->bitmap.buffer != nullptr) {
					bmp.draw(_rc, &face->glyph->bitmap);
				} else {
					if (!string::isspace(charId)) {
						log::format("Font", "error: no bitmap for (%d) '%s'", charId, string::toUtf8(charId).c_str());
					}
				}
			} else {
				for (auto &it : *(_char._chars)) {
					it->rect.origin.x = _rc.origin.x;
					it->rect.origin.y = _rc.origin.y;
				}
			}
		} else if (_child[0] && _child[1]) {
			_child[0]->drawChar(bmp);
			_child[1]->drawChar(bmp);
		}
	};
};

struct FontStruct {
	String file;
	Bytes data;
};

using FontStructMap = Map<String, FontStruct>;
using FontFaceMap = Map<String, FT_Face>;
using FontFacePriority = std::vector<std::pair<String, uint16_t>>;
using FontCharPair = std::pair<Font::Char, FT_Face>;
using FontCharsVec = std::vector<FontCharPair>;
using FontCharsMap = Map<String, std::pair<FT_Face, FontCharsVec>>;

struct FontSetCache {
	Set<String> files;
	FontStructMap openedFiles;
	FontFaceMap openedFaces;

	FontSet::Config const & config;
	AssetMap const & assets;

	FT_Library FTlibrary;

	FT_Face openFontFace(const std::vector<uint8_t> &data, const std::string &font, uint16_t fontSize) {
		FT_Face face;

		if (data.empty()) {
			log::format("Font", "fail to load data from %s", font.c_str());
			return nullptr;
		}

		FT_Error err = FT_Err_Ok;

		err = FT_New_Memory_Face(FTlibrary, data.data(), data.size(), 0, &face );
		if (err != FT_Err_Ok) {
			log::format("Font", "fail to load font face: %d (\"%s\")", err, sp_FT_Err_Desc(err).c_str());
			return nullptr;
		}

		//we want to use unicode
		err = FT_Select_Charmap(face, FT_ENCODING_UNICODE);
		if (err != FT_Err_Ok) {
			FT_Done_Face(face);
			log::format("Font", "fail to select charmap: %d (\"%s\")", err, sp_FT_Err_Desc(err).c_str());
			return nullptr;
		}

		// set the requested font size
		err = FT_Set_Pixel_Sizes(face, fontSize, fontSize);
		if (err != FT_Err_Ok) {
			FT_Done_Face(face);
			log::format("Font", "fail to set font size: %d (\"%s\")", err, sp_FT_Err_Desc(err).c_str());
			return nullptr;
		}

		if (face) {
			openedFaces.insert(std::make_pair(font, face));
		}
		return face;
	}

	FontStructMap::iterator processFile(Bytes && data, const std::string &file, Asset *asset) {
		if (data.size() < 12) {
			return openedFiles.end();
		}
		auto it = openedFiles.find(file);
		if (it == openedFiles.end()) {
			return openedFiles.insert(std::make_pair(file, FontStruct{file, std::move(data)})).first;
		} else {
			return it;
		}
	}

	FontStructMap::iterator openFile(const std::string &name, const std::string &file) {
		auto it = openedFiles.find(file);
		if (it != openedFiles.end()) {
			return it;
		} else {
			if (resource::isReceiptUrl(file)) {
				auto assetIt = assets.find(file);
				if (assetIt == assets.end()) {
					return openedFiles.end();
				} else {
					auto a = assetIt->second;
					auto path = a->getFilePath();
					if (files.find(path) == files.end()) {
						files.insert(path);
						return processFile(filesystem::readFile(path), file, a);
					}
				}
			} else if (files.find(file) == files.end()) {
				if (config.receiptCallback) {
					Bytes res = config.receiptCallback(file);
					if (!res.empty()) {
						files.insert(file);
						return processFile(std::move(res), file, nullptr);
					}
				}
				files.insert(file);
				return processFile(filesystem::readFile(file), file, nullptr);
			}
		}
		return openedFiles.end();
	}

	FT_Face getFace(const std::string &file, uint16_t size) {
		std::string fontname = toString(file, ":", size);
		auto it = openedFaces.find(fontname);
		if (it != openedFaces.end()) {
			return it->second;
		} else {
			auto fileIt = openFile(fontname, file);
			if (fileIt == openedFiles.end()) {
				return nullptr;
			} else {
				return openFontFace(fileIt->second.data, fontname, size);
			}
		}
	}

	FontSetCache(const FontSet::Config &cfg, const AssetMap &assets, FT_Library lib)
	: config(cfg), assets(assets), FTlibrary(lib) { }

	~FontSetCache() {
		for (auto &it : openedFaces) {
			if (it.second) {
				FT_Done_Face(it.second);
			}
		}
	}
};

class FreeTypeLibrary {
public:
	static std::u16string getCharGroupString(FontRequest::CharGroup mask) {
		return u" " + getCharGroup(mask);
	}

	static FreeTypeLibrary *getInstance() {
		if (!s_freeTypeLibraryInstance) {
			s_freeTypeLibraryInstance = new FreeTypeLibrary();
		}
		return s_freeTypeLibraryInstance;
	}

	FreeTypeLibrary() {
		FT_Init_FreeType( &_FTlibrary );
		_glyphs = getCharGroupString(
				FontRequest::CharGroup::Numbers
				| FontRequest::CharGroup::PunctuationAdvanced
				| FontRequest::CharGroup::Latin
				| FontRequest::CharGroup::Cyrillic
				| FontRequest::CharGroup::Currency);
		_glyphsCount = _glyphs.length();

		_cachePath = filesystem::writablePath("fonts_cache");
		filesystem::mkdir(_cachePath);

		data::Value d = data::readFile(_cachePath + "/.version");
		if (!d.isDictionary() || d.getInteger("version") != 1) {
			filesystem::remove(_cachePath, true, true);
			filesystem::mkdir(_cachePath);
			data::Value n;
			n.setInteger(1, "version");
			data::save(n, _cachePath + "/.version", data::EncodeFormat::Json);
		}
#if (SP_FONT_DEBUG)
		FT_Int major = 0, minor = 0, patch = 0;
		FT_Library_Version(_FTlibrary, &major, &minor, &patch);
		log("Freetype %d.%d.%d is running", major, minor, patch);
#endif
	}

	~FreeTypeLibrary()	{
		FT_Done_FreeType(_FTlibrary);
	}

	void setFallbackFont(const std::string &str) {
		_fallbackFont = str;
	}
	const std::string &getFallbackFont() {
		return _fallbackFont;
	}

	std::string cachePath(const std::string multifontName, const std::string &name) {
		return _cachePath + "/" + multifontName + "." + name;
	}

	bool getKerning(const FontCharPair &first, const FontCharPair &second, int16_t &value) {
		if (first.second == second.second && FT_HAS_KERNING(first.second)) {
			FT_Face face = first.second;
			int glyph1 = FT_Get_Char_Index(face, first.first.charID);
			int glyph2 = FT_Get_Char_Index(face, second.first.charID);

			FT_Vector kerning;
			auto err = FT_Get_Kerning( face, glyph1, glyph2, FT_KERNING_DEFAULT, &kerning);
			if (err != FT_Err_Ok) {
				return false;
			}
			value = (int16_t)(kerning.x >> 6);

			if (value != 0) {
#if (SP_FONT_DEBUG)
				log("kerning pair: %s %s : %d", string::utf16_to_utf8(first.first.charID).c_str(),
						string::utf16_to_utf8(second.first.charID).c_str(), (int16_t)(kerning.x >> 6));
#endif
				return true;
			}
		}
		return false;
	}

	FT_Face getChar(FontRequest &req, FontSetCache &cache, std::vector<FT_Face> &faceVec, char16_t theChar, cocos2d::Size &size, int16_t &xOffset, int16_t &yOffset, int16_t &xAdvance) const {
		int rsize = (int)req.receipt.size();
		for (int i = rsize; i >= 0; i--) {
			FT_Face face = nullptr;
			if ((int)faceVec.size() > (rsize - i)) {
				// Используем ранее загруженный шрифт
				face = faceVec[rsize - i];
			} else {
				// Или пытаемся загрузить новый
				if (i > 0) {
					// из рецепта...
					auto &receipt = req.receipt[i - 1];
					face = cache.getFace(receipt, req.size);
				} else if (!_fallbackFont.empty()) {
					// ... или шрифт по умолчанию
					face = cache.getFace(_fallbackFont, req.size);
#if (SP_FONT_DEBUG)
					log("font fallback for %s (%d) in %s", string::utf16_to_utf8(theChar).c_str(), theChar, req.name.c_str());
#endif
				}
				faceVec.push_back(face);
			}

			if (!face) {
				continue;
			}

			int glyph_index = FT_Get_Char_Index(face, theChar);
			if (!glyph_index) {
	#if (SP_FONT_DEBUG)
				log("Font: fail to load glyph: no such glyph: %d '%s'", theChar, string::utf16_to_utf8(theChar).c_str());
	#endif
				continue;
			}

			auto err = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
			if (err != FT_Err_Ok) {
	#if (SP_FONT_DEBUG)
				log("Font: fail to load glyph: %d (\"%s\")", err, sp_FT_Err_Desc(err).c_str());
	#endif
				continue;
			}

			// store result in the passed rectangle
			xOffset = face->glyph->metrics.horiBearingX >> 6;
			yOffset = - (face->glyph->metrics.horiBearingY >> 6);
			size.width = (face->glyph->metrics.width >> 6);
			size.height = (face->glyph->metrics.height >> 6);
			xAdvance = (static_cast<int16_t>(face->glyph->metrics.horiAdvance >> 6));
			return face;
		}

		return nullptr;
	}

	FontMap prepareFontMap(const FontSet::Config &cfg, FontCharsMap &charsMap) {
		FontMap map;

		for (auto &it : cfg.requests) {
			auto charsIt = charsMap.find(it.name);
			if (charsIt == charsMap.end()) {
				continue;
			}

			auto face = charsIt->second.first;
			auto &chars = charsIt->second.second;

			Font::Data data;
			data.metrics.size = it.size;
			data.metrics.height = face->size->metrics.height >> 6;
			data.metrics.ascender = face->size->metrics.ascender >> 6;
			data.metrics.descender = face->size->metrics.descender >> 6;
			data.metrics.underlinePosition = face->underline_position >> 6;
			data.metrics.underlineThickness = face->underline_thickness >> 6;

			for (auto &c : chars) {
				data.chars.insert(c.first);
			}

			auto count = chars.size();
			for (size_t i = 0; i < count; i++) {
				for (size_t j = 0; j < count; j++) {
					int16_t kernValue;
					if (getKerning(chars[i], chars[j], kernValue)) {
						data.kerning.insert(std::make_pair(
								chars[i].first.charID << 16 | (chars[j].first.charID & 0xffff), kernValue));
					}
				}
			}

			map.insert(std::make_pair(it.name, std::move(data)));
		}

		return map;
	}

	FontSet * writeDynamicFont(FontSet::Config &&cfg, const AssetMap &assets, FontCharsMap &charsMap, Image *image) {
		auto map = prepareFontMap(cfg, charsMap);
		cocos2d::Map<std::string, Font *> fontMap;
		for (auto &it : map) {
			auto font = new Font(it.first, std::move(it.second.metrics), std::move(it.second.chars), std::move(it.second.kerning), image);
			fontMap.insert(it.first, font);
			font->release();
		}

		return new FontSet(std::move(cfg), std::move(fontMap), image);
	}

	FontSet * writeFont(FontSet::Config &&cfg, const AssetMap &assets, FontCharsMap &charsMap, LayoutBitmap &bmp) {
		auto map = prepareFontMap(cfg, charsMap);
		return writeFontSet(std::move(cfg), assets, map, bmp.data().data(), bmp.width(), bmp.height());
	}

	FontSet * createMultiFont(FontSet::Config &&cfg, const AssetMap &assets) {
		for (FontRequest &it : cfg.requests) {
			if (it.charGroupMask != 0) {
				it.chars += getCharGroupString(it.charGroupMask);
			}
		}

		FontSetCache cache(cfg, assets, _FTlibrary);
		FontCharsMap charSets;

		for (FontRequest &it : cfg.requests) {
			const std::u16string *chars = nullptr;
			if (it.chars.empty()) {
				chars = &_glyphs;
			} else {
				chars = &it.chars;
			}

			std::map<FT_Face, size_t> faces;

			std::vector<FT_Face> faceVec;
			faceVec.reserve(it.receipt.size() + 1);

			FontCharsVec charSet;
			for (char16_t c : (*chars)) {
				int16_t xAdvance, xOffset, yOffset; cocos2d::Rect rect;
				if (auto face = getChar(it, cache, faceVec, c, rect.size, xOffset, yOffset, xAdvance)) {
					charSet.push_back(std::make_pair(Font::Char(c, rect, xOffset, yOffset, xAdvance), face));
					auto it = faces.find(face);
					if (it == faces.end()) {
						faces.insert(std::make_pair(face, 1));
					} else {
						it->second = it->second + 1;
					}
				} else {
#if (SP_FONT_DEBUG)
					log("Font: char '%s' %u is not defined in font", string::utf16_to_utf8(c).c_str(), c);
#endif
				}
			}
			if (!charSet.empty()) {
				FT_Face topFace = nullptr;
				size_t count = 0;
				for (auto &it : faces) {
					if (count < it.second) {
						topFace = it.first;
						count = it.second;
					}
				}

				charSets.insert(std::make_pair(it.name, std::make_pair(topFace, std::move(charSet))));
			}
		}

		struct FontCharId {
			char16_t charId;
			FT_Face face;

			inline bool operator < (const FontCharId &r) const {
				return (face == r.face) ? (charId < r.charId) : (face < r.face);
			}
		};

		float square = 0;
		std::set<LayoutChar, charHeightComparator> heightSet;
		std::map<FontCharId, std::vector<Font::Char *> *> charGroup;

		for (auto setIt = charSets.begin(); setIt != charSets.end(); setIt ++) {
			for (auto it = setIt->second.second.begin(); it != setIt->second.second.end(); it ++) {
				auto &c = it->first;
				auto &face = it->second;
				auto cgIt = charGroup.find(FontCharId{c.charID, face});
				if (cgIt == charGroup.end()) {
					cgIt = charGroup.insert(std::make_pair(FontCharId{c.charID, face}, new std::vector<Font::Char *>({&c}))).first;
					heightSet.insert(LayoutChar(cgIt->second, face, c.rect.size.width, c.rect.size.height, c.charID));
				} else {
					cgIt->second->push_back(&(it->first));
				}

				square += it->first.width() * it->first.height();
			}
		}

		size_t count = heightSet.size();

		int sq2 = 1; int i = 0; int h = 1; int w = 1;
		for (bool s = true; sq2 < square; i++, sq2 *= 2, s = !s) {
			if (s) { w *= 2; } else { h *= 2; }
		}

		h -= (int)floorf((sq2 - square * ((w > 512)?1.2f:1.35)) / w);
		int incr = h / 16;

		LayoutNode *l = nullptr;
		size_t nodes = 0;
		while (!l || nodes < count) {
			if (l) { delete l; }

			h += incr;
			l = new LayoutNode(cocos2d::Rect(0, 0, w, h));
			for (auto it = heightSet.begin(); it != heightSet.end(); it++) {
				l->insert(*it);
			}
			nodes = l->nodes();
		}

		LayoutBitmap bmp(w, h);
		l->drawChar(bmp);
		delete l;

		for (auto &it : charGroup) {
			delete (it.second);
		}

		FontSet *ret = nullptr;
		if (!cfg.dynamic) {
			ret = writeFont(std::move(cfg), assets, charSets, bmp);
		} else {
			auto image = Image::createWithBitmap(std::move(bmp), true);
			ret = writeDynamicFont(std::move(cfg), assets, charSets, image);
			image->release();
		}

		return ret;
	}

	FontSet * getMultiFont(FontSet::Config &&cfg, const cocos2d::Map<std::string, Asset *> &assets) {
		if (!cfg.dynamic) {
			auto ret = resource::readFontSet(std::move(cfg), assets);
			if (!ret) {
				return createMultiFont(std::move(cfg), assets);
			}
			return ret;
		} else {
			return createMultiFont(std::move(cfg), assets);
		}
	}

private:
	FT_Library _FTlibrary;
	std::u16string _glyphs;
	size_t _glyphsCount;

	std::string _cachePath;
	std::string _fallbackFont;
};


FontSet *generateFromConfig(FontSet::Config &&cfg, const cocos2d::Map<std::string, Asset *> &assets) {
	auto freeTypeLib = FreeTypeLibrary::getInstance();
	return freeTypeLib->getMultiFont(std::move(cfg), assets);
}

void setFallbackFont(const std::string &str) {
	auto freeTypeLib = FreeTypeLibrary::getInstance();
	freeTypeLib->setFallbackFont(str);
}

const std::string &getFallbackFont() {
	auto freeTypeLib = FreeTypeLibrary::getInstance();
	return freeTypeLib->getFallbackFont();
}

NS_SP_EXT_END(resource)
