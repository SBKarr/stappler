// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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

#include "SPDefine.h"
#include "SPFont.h"
#include "SPResource.h"
#include "SPFilesystem.h"
#include "SPThread.h"
#include "SPThreadManager.h"
#include "SPTextureCache.h"
#include "SPDynamicLabel.h"

#include "ft2build.h"
#include FT_FREETYPE_H
#include FT_GLYPH_H

#define LAYOUT_PADDING uint16_t(1)

NS_SP_EXT_BEGIN(font)

struct FontStruct {
	String file;
	Bytes data;
};

using FontStructMap = Map<String, FontStruct>;
using FontFaceMap = Map<String, FT_Face>;
using FontFacePriority = Vector<Pair<String, uint16_t>>;

struct UVec2 {
	uint16_t x;
	uint16_t y;
};

struct URect {
	uint16_t x;
	uint16_t y;
	uint16_t width;
	uint16_t height;

	UVec2 origin() const { return UVec2{x, y}; }
};

class FontLibrary;
class FontLibraryCache {
public:
	FontLibraryCache(FontLibrary *);
	~FontLibraryCache();

	void clear();
	void update();
	Time getTimer() const;

	Metrics requestMetrics(const Source *, const Vector<String> &srcs, uint16_t size, const ReceiptCallback &cb);
	Arc<FontLayout::Data> requestLayoutUpgrade(const Source *, const Vector<String> &, const Arc<FontLayout::Data> &data, const Vector<char16_t> &chars, const ReceiptCallback &cb);

	bool updateTextureWithSource(uint32_t v, Source *source, const Map<String, Vector<char16_t>> &l, Vector<Rc<cocos2d::Texture2D>> &t);

protected:
	FT_Face openFontFace(const Bytes &data, const String &font, uint16_t fontSize);
	FontStructMap::iterator processFile(Bytes && data, const String &file);
	FontStructMap::iterator openFile(const Source *source, const ReceiptCallback &cb, const String &name, const String &file);
	FT_Face getFace(const Source *source, const ReceiptCallback &cb, const String &file, uint16_t size);

	Metrics getMetrics(FT_Face face, uint16_t);

	void requestCharUpdate(const Source *, const ReceiptCallback &cb, const Vector<String> &, uint16_t size, Vector<FT_Face> &, Vector<CharLayout> &, char16_t);
	bool getKerning(const Vector<FT_Face> &faces, char16_t first, char16_t second, int16_t &value);

	void drawCharOnBitmap(cocos2d::Texture2D *bmp, const URect &, FT_Bitmap *bitmap);

	FontLibrary *library;
	Time timer;
	Set<String> files;
	FontStructMap openedFiles;
	FontFaceMap openedFaces;

	FT_Library FTlibrary = nullptr;
};

using FontTextureLayout = Map<String, Vector<CharTexture>>;

class FontLibrary {
public:
	static FontLibrary *getInstance();
	FontLibrary();
	~FontLibrary();
	void update(float dt);

	Arc<FontLibraryCache> getCache();

	Vector<size_t> getQuadsCount(const FormatSpec *format, const Map<String, Vector<CharTexture>> &layouts, size_t texSize);

	bool writeTextureQuads(uint32_t v, Source *, const FormatSpec *, const Vector<Rc<cocos2d::Texture2D>> &,
			Vector<Rc<DynamicQuadArray>> &, Vector<Vector<bool>> &);

	bool writeTextureRects(uint32_t v, Source *, const FormatSpec *, float scale, Vector<Rect> &);

	bool isSourceRequestValid(Source *, uint32_t);

	void cleanupSource(Source *);
	Arc<FontTextureLayout> getSourceLayout(Source *);
	void setSourceLayout(Source *, FontTextureLayout &&);

protected:
	std::mutex _mutex;
	Time _timer = 0;
	Map<uint64_t, Arc<FontLibraryCache>> _cache;

	std::mutex _sourcesMutex;
	Map<Source *, Arc<FontTextureLayout>> _sources;

	static FontLibrary *s_instance;
	static std::mutex s_mutex;
};

struct FontLayoutData {
	Arc<FontLayout> layout;
	Vector<CharTexture> *chars;
	Vector<FT_Face> faces;
};

class LayoutBitmap : public Bitmap {
public:
	LayoutBitmap(uint32_t w, uint32_t h);
	void draw(const URect &rect, FT_Bitmap *bitmap);
};

struct LayoutNode {
	LayoutNode* _child[2];
	URect _rc;
	CharTexture * _char = nullptr;

	LayoutNode(const URect &rect);
	LayoutNode(const UVec2 &origin, CharTexture *c);
	~LayoutNode();

	bool insert(CharTexture *c);
	size_t nodes() const;
	void finalize(uint8_t tex);
};

FontLibrary *FontLibrary::s_instance = nullptr;
std::mutex FontLibrary::s_mutex;

FontLibraryCache::FontLibraryCache(FontLibrary *lib) : library(lib) {
	FT_Init_FreeType( &FTlibrary );
}

FontLibraryCache::~FontLibraryCache() {
	clear();
	FT_Done_FreeType(FTlibrary);
}

void FontLibraryCache::clear() {
	for (auto &it : openedFaces) {
		if (it.second) {
			FT_Done_Face(it.second);
		}
	}

	files.clear();
	openedFiles.clear();
	openedFaces.clear();
}

void FontLibraryCache::update() {
	timer = Time::now();
}

Time FontLibraryCache::getTimer() const {
	return timer;
}

Metrics FontLibraryCache::requestMetrics(const Source *source, const Vector<String> &srcs, uint16_t size, const ReceiptCallback &cb) {
	for (auto &it : srcs) {
		if (auto face = getFace(source, cb, it, size)) {
			return getMetrics(face, size);
		}
	}
	return Metrics();
}

static chars::CharGroupId getCharGroupForChar(char16_t c) {
	using namespace chars;
	if (CharGroup<char16_t, CharGroupId::Numbers>::match(c)) {
		return CharGroupId::Numbers;
	} else if (CharGroup<char16_t, CharGroupId::Latin>::match(c)) {
		return CharGroupId::Latin;
	} else if (CharGroup<char16_t, CharGroupId::Cyrillic>::match(c)) {
		return CharGroupId::Cyrillic;
	} else if (CharGroup<char16_t, CharGroupId::Currency>::match(c)) {
		return CharGroupId::Currency;
	} else if (CharGroup<char16_t, CharGroupId::GreekBasic>::match(c)) {
		return CharGroupId::GreekBasic;
	} else if (CharGroup<char16_t, CharGroupId::Math>::match(c)) {
		return CharGroupId::Math;
	} else if (CharGroup<char16_t, CharGroupId::TextPunctuation>::match(c)) {
		return CharGroupId::TextPunctuation;
	}
	return CharGroupId::None;
}

static void addCharGroup(Vector<char16_t> &vec, chars::CharGroupId id) {
	using namespace chars;
	auto f = [&vec] (char16_t c) {
		auto it = std::lower_bound(vec.begin(), vec.end(), c);
		if (it == vec.end() || *it != c) {
			vec.insert(it, c);
		}
	};

	switch (id) {
	case CharGroupId::Numbers: CharGroup<char16_t, CharGroupId::Numbers>::foreach(f); break;
	case CharGroupId::Latin: CharGroup<char16_t, CharGroupId::Latin>::foreach(f); break;
	case CharGroupId::Cyrillic: CharGroup<char16_t, CharGroupId::Cyrillic>::foreach(f); break;
	case CharGroupId::Currency: CharGroup<char16_t, CharGroupId::Currency>::foreach(f); break;
	case CharGroupId::GreekBasic: CharGroup<char16_t, CharGroupId::GreekBasic>::foreach(f); break;
	case CharGroupId::Math: CharGroup<char16_t, CharGroupId::Math>::foreach(f); break;
	case CharGroupId::TextPunctuation: CharGroup<char16_t, CharGroupId::TextPunctuation>::foreach(f); break;
	default: break;
	}
}

void FontLibraryCache::requestCharUpdate(const Source *source, const ReceiptCallback &cb, const Vector<String> &srcs, uint16_t size,
		Vector<FT_Face> &faces, Vector<CharLayout> &layout, char16_t theChar) {
	for (uint32_t i = 0; i <= srcs.size(); ++ i) {
		FT_Face face = nullptr;
		if (i < faces.size()) {
			face = faces.at(i);
		} else {
			if (i < srcs.size()) {
				faces.push_back(getFace(source, cb, srcs.at(i), size));
			} else {
				faces.push_back(getFace(source, cb, resource::getFallbackFont(), size));
			}
			face = faces.back();
		}

		if (face) {
			auto it = std::lower_bound(layout.begin(), layout.end(), theChar);
			if (it == layout.end() || it->charID != theChar) {
				int glyph_index = FT_Get_Char_Index(face, theChar);
				if (!glyph_index) {
					continue;
				}

				auto err = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT | FT_LOAD_NO_BITMAP);
				if (err != FT_Err_Ok) {
					continue;
				}

				// store result in the passed rectangle
				layout.insert(it, CharLayout{theChar,
					static_cast<int16_t>(face->glyph->metrics.horiBearingX >> 6),
					static_cast<int16_t>(- (face->glyph->metrics.horiBearingY >> 6)),
					static_cast<uint16_t>(face->glyph->metrics.horiAdvance >> 6)});
				return;
			}
		}
	}

	auto it = std::lower_bound(layout.begin(), layout.end(), theChar);
	if (it == layout.end() || *it != theChar) {
		layout.insert(it, CharLayout{theChar, 0, 0, 0});
	}
}

static Pair<FT_Face, int> getFaceForChar(const Vector<FT_Face> &faces, char16_t theChar) {
	for (auto &face : faces) {
		int glyph_index = FT_Get_Char_Index(face, theChar);
		if (glyph_index) {
			return pair(face, glyph_index);
		}
	}
	return pair(nullptr, 0);
}

bool FontLibraryCache::getKerning(const Vector<FT_Face> &faces, char16_t first, char16_t second, int16_t &value) {
	auto firstFace = getFaceForChar(faces, first);
	auto secondFace = getFaceForChar(faces, first);
	if (firstFace.first && firstFace.first == secondFace.first && FT_HAS_KERNING(firstFace.first)) {
		const FT_Face face = firstFace.first;
		const int glyph1 = firstFace.second;
		const int glyph2 = secondFace.second;

		FT_Vector kerning;
		auto err = FT_Get_Kerning( face, glyph1, glyph2, FT_KERNING_DEFAULT, &kerning);
		if (err != FT_Err_Ok) {
			return false;
		}
		value = (int16_t)(kerning.x >> 6);

		if (value != 0) {
			return true;
		}
	}
	return false;
}

void FontLibraryCache::drawCharOnBitmap(cocos2d::Texture2D *bmp, const URect &rect, FT_Bitmap *bitmap) {
	if (bitmap->pixel_mode == FT_PIXEL_MODE_MONO) {

	} else {
		bmp->updateWithData(bitmap->buffer, rect.x, rect.y, bitmap->width, bitmap->rows);
	}
}

void FontLibraryCache_addChar(Vector<char16_t> &charsToUpdate, const Arc<FontLayout::Data> &data, const Vector<char16_t> &chars, char16_t theChar) {
	auto spaceIt = std::lower_bound(chars.begin(), chars.end(), theChar);
	if (spaceIt == chars.end() || *spaceIt != theChar) {
		auto spaceDataIt = std::lower_bound(data->chars.begin(), data->chars.end(), theChar);
		if (spaceDataIt == data->chars.end() || spaceDataIt->charID != theChar) {
			charsToUpdate.push_back(theChar);
		}
	}
}

Arc<FontLayout::Data> FontLibraryCache::requestLayoutUpgrade(const Source *source, const Vector<String> &srcs,
		const Arc<FontLayout::Data> &data, const Vector<char16_t> &chars, const ReceiptCallback &cb) {
	Arc<FontLayout::Data> ret;
	if (!data || data->metrics.size == 0) {
		return data;
	} else {
		ret = Arc<FontLayout::Data>::create(*(data.get()));

		Vector<char16_t> charsToUpdate;

		uint32_t mask = 0;

		FontLibraryCache_addChar(charsToUpdate, data, chars, char16_t(' '));
		FontLibraryCache_addChar(charsToUpdate, data, chars, char16_t('\n'));
		FontLibraryCache_addChar(charsToUpdate, data, chars, char16_t('\r'));

		for (auto &c : chars) {
			auto g = getCharGroupForChar(c);
			if (g != chars::CharGroupId::None) {
				if ((mask & toInt(g)) == 0) {
					mask |= toInt(g);
					addCharGroup(charsToUpdate, g);
				}
			} else {
				auto it = std::lower_bound(charsToUpdate.begin(), charsToUpdate.end(), c);
				if (it == charsToUpdate.end() || *it != c) {
					charsToUpdate.insert(it, c);
				}
			}
		}


		Vector<FT_Face> faces; faces.reserve(srcs.size());
		ret->chars.reserve(ret->chars.size() + charsToUpdate.size());

		// calculate layouts
		for (auto &c : charsToUpdate) {
			requestCharUpdate(source, cb, srcs, ret->metrics.size, faces, ret->chars, c);
		}

		// kerning
		for (auto &c : charsToUpdate) {
			int16_t kernValue = 0;
			for (auto &it : ret->chars) {
				if (getKerning(faces, c, it.charID, kernValue)) {
					ret->kerning.emplace(c << 16 | (it.charID & 0xffff), kernValue);
				}
				if (getKerning(faces, it.charID, c, kernValue)) {
					ret->kerning.emplace(it.charID << 16 | (c & 0xffff), kernValue);
				}
			}
		}

		return ret;
	}
}

FT_Face FontLibraryCache::openFontFace(const Bytes &data, const String &font, uint16_t fontSize) {
	FT_Face face;

	if (data.empty()) {
		return nullptr;
	}

	FT_Error err = FT_Err_Ok;

	err = FT_New_Memory_Face(FTlibrary, data.data(), data.size(), 0, &face );
	if (err != FT_Err_Ok) {
		return nullptr;
	}

	//we want to use unicode
	err = FT_Select_Charmap(face, FT_ENCODING_UNICODE);
	if (err != FT_Err_Ok) {
		FT_Done_Face(face);
		return nullptr;
	}

	// set the requested font size
	err = FT_Set_Pixel_Sizes(face, fontSize, fontSize);
	if (err != FT_Err_Ok) {
		FT_Done_Face(face);
		return nullptr;
	}

	if (face) {
		openedFaces.insert(std::make_pair(font, face));
	}
	return face;
}

FontStructMap::iterator FontLibraryCache::processFile(Bytes && data, const String &file) {
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

FontStructMap::iterator FontLibraryCache::openFile(const Source *source, const ReceiptCallback &cb, const String &name, const String &file) {
	auto it = openedFiles.find(file);
	if (it != openedFiles.end()) {
		return it;
	} else {
		if (files.find(file) == files.end()) {
			files.insert(file);
			if (cb) {
				auto data = cb(source, file);
				if (!data.empty()) {
					return processFile(std::move(data), file);
				}
			}
			return processFile(filesystem::readFile(file), file);
		}
	}
	return openedFiles.end();
}

FT_Face FontLibraryCache::getFace(const Source *source, const ReceiptCallback &cb, const String &file, uint16_t size) {
	std::string fontname = toString(file, ":", size);
	auto it = openedFaces.find(fontname);
	if (it != openedFaces.end()) {
		return it->second;
	} else {
		auto fileIt = openFile(source, cb, fontname, file);
		if (fileIt == openedFiles.end()) {
			return nullptr;
		} else {
			return openFontFace(fileIt->second.data, fontname, size);
		}
	}
}

Metrics FontLibraryCache::getMetrics(FT_Face face, uint16_t size) {
	Metrics ret;
	ret.size = size;
	ret.height = face->size->metrics.height >> 6;
	ret.ascender = face->size->metrics.ascender >> 6;
	ret.descender = face->size->metrics.descender >> 6;
	ret.underlinePosition = face->underline_position >> 6;
	ret.underlineThickness = face->underline_thickness >> 6;
	return ret;
}



void FontLibrary::cleanupSource(Source *source) {
	_sourcesMutex.lock();
	_sources.erase(source);
	_sourcesMutex.unlock();
}
Arc<FontTextureLayout> FontLibrary::getSourceLayout(Source *source) {
	Arc<FontTextureLayout> ret;
	_sourcesMutex.lock();
	auto it = _sources.find(source);
	if (it != _sources.end()) {
		ret = it->second;
	}
	_sourcesMutex.unlock();
	return ret;
}
void FontLibrary::setSourceLayout(Source *source, FontTextureLayout &&map) {
	Arc<FontTextureLayout> ret;
	_sourcesMutex.lock();
	auto it = _sources.find(source);
	if (it != _sources.end()) {
		it->second = Arc<FontTextureLayout>::create(std::move(map));
	} else {
		_sources.emplace(source, Arc<FontTextureLayout>::create(std::move(map)));
	}
	_sourcesMutex.unlock();
}

Vector<size_t> FontLibrary::getQuadsCount(const FormatSpec *format, const Map<String, Vector<CharTexture>> &layouts, size_t texSize) {
	Vector<size_t> ret; ret.resize(texSize);

	auto count = format->chars.size();
	auto range = format->ranges.begin();
	auto texVecIt = layouts.find(range->layout->getName());
	if (texVecIt == layouts.end()) {
		return Vector<size_t>();
	}
	const Vector<font::CharTexture> *texVec = &texVecIt->second;

	for (uint32_t charIdx = 0; charIdx < count; ++ charIdx) {
		if (charIdx >= range->start + range->count) {
			++ range;
			while (range->count == 0) {
				++ range;
			}
			texVecIt = layouts.find(range->layout->getName());
			if (texVecIt == layouts.end()) {
				return Vector<size_t>();
			}
			texVec = &texVecIt->second;
		}

		const font::CharSpec &c = format->chars[charIdx];
		if (c.display == font::CharSpec::Char) {
			auto texIt = std::lower_bound(texVec->begin(), texVec->end(), c.charID);
			++ (ret[texIt->texture]);
		}
	}

	return ret;
}

bool FontLibrary::writeTextureQuads(uint32_t v, Source *source, const FormatSpec *format, const Vector<Rc<cocos2d::Texture2D>> &texs,
		Vector<Rc<DynamicQuadArray>> &quads, Vector<Vector<bool>> &colorMap) {
	colorMap.resize(texs.size());

	if (!isSourceRequestValid(source, v)) {
		return false;
	}

	auto layoutsRef = getSourceLayout(source);
	auto &layouts = *layoutsRef;

	quads.reserve(texs.size());
	for (size_t i = 0; i < texs.size(); ++ i) {
		quads.push_back(Rc<DynamicQuadArray>::alloc());
	}

	auto sizes = getQuadsCount(format, layouts, texs.size());
	for (size_t i = 0; i < sizes.size(); ++ i) {
		quads[i]->setCapacity(sizes[i]);
		colorMap[i].reserve(sizes[i] * 2);
	}

	auto range = format->ranges.begin();
	auto line = format->lines.begin();
	auto count = format->chars.size();
	Arc<font::FontLayout> layout(range->layout);
	auto texVecIt = layouts.find(layout->getName());
	if (texVecIt == layouts.end()) {
		return false;
	}
	const Vector<font::CharTexture> *texVec = &texVecIt->second;
	Arc<font::FontData> data = layout->getData();
	const font::Metrics *metrics = &data->metrics;
	const Vector<font::CharLayout> *charVec = &data->chars;

	for (uint32_t charIdx = 0; charIdx < count; ++ charIdx) {
		if (charIdx >= range->start + range->count) {
			if (!isSourceRequestValid(source, v)) {
				return false;
			}

			++ range;
			while (range->count == 0) {
				++ range;
			}
			layout = range->layout;
			texVecIt = layouts.find(layout->getName());
			if (texVecIt == layouts.end()) {
				return false;
			}
			texVec = &texVecIt->second;
			data = layout->getData();
			metrics = &data->metrics;
			charVec = &data->chars;
		}

		if (charIdx >= line->start + line->count) {
			++ line;
			while (line->count == 0 && line != format->lines.end()) {
				++ line;
			}
		}

		const font::CharSpec &c = format->chars[charIdx];
		if (c.display == font::CharSpec::Char) {
			auto texIt = std::lower_bound(texVec->begin(), texVec->end(), c.charID);
			auto charIt = std::lower_bound(charVec->begin(), charVec->end(), c.charID);

			if (texIt != texVec->end() && texIt->charID == c.charID && charIt != charVec->end() && charIt->charID == c.charID && texIt->texture != maxOf<uint8_t>()) {
				DynamicQuadArray *quad = quads[texIt->texture];
				cocos2d::Texture2D * tex = texs[texIt->texture];
				auto &cMap = colorMap[texIt->texture];
				cMap.push_back(range->colorDirty);
				cMap.push_back(range->opacityDirty);
				switch (range->align) {
				case font::VerticalAlign::Sub:
					quad->drawChar(*metrics, *charIt, *texIt, c.pos, format->height - line->pos + metrics->descender / 2, range->color, range->underline, tex->getPixelsWide(), tex->getPixelsHigh());
					break;
				case font::VerticalAlign::Super:
					quad->drawChar(*metrics, *charIt, *texIt, c.pos, format->height - line->pos + metrics->ascender / 2, range->color, range->underline, tex->getPixelsWide(), tex->getPixelsHigh());
					break;
				default:
					quad->drawChar(*metrics, *charIt, *texIt, c.pos, format->height - line->pos, range->color, range->underline, tex->getPixelsWide(), tex->getPixelsHigh());
					break;
				}
			}
		}
	}

	return true;
}

bool FontLibrary::writeTextureRects(uint32_t v, Source *source, const FormatSpec *format, float scale, Vector<Rect> &rects) {
	if (!isSourceRequestValid(source, v)) {
		return false;
	}

	auto layoutsRef = getSourceLayout(source);
	auto &layouts = *layoutsRef;

	auto range = format->ranges.begin();
	auto line = format->lines.begin();
	auto count = format->chars.size();
	Arc<font::FontLayout> layout(range->layout);
	auto texVecIt = layouts.find(layout->getName());
	if (texVecIt == layouts.end()) {
		return false;
	}
	const Vector<font::CharTexture> *texVec = &texVecIt->second;
	Arc<font::FontData> data = layout->getData();
	const font::Metrics *metrics = &data->metrics;
	const Vector<font::CharLayout> *charVec = &data->chars;

	for (uint32_t charIdx = 0; charIdx < count; ++ charIdx) {
		if (charIdx >= range->start + range->count) {
			++ range;
			while (range->count == 0) {
				++ range;
			}
			layout = range->layout;
			texVecIt = layouts.find(layout->getName());
			if (texVecIt == layouts.end()) {
				return false;
			}
			texVec = &texVecIt->second;
			data = layout->getData();
			metrics = &data->metrics;
			charVec = &data->chars;
		}

		if (charIdx >= line->start + line->count) {
			++ line;
			while (line->count == 0) {
				++ line;
			}
		}

		const font::CharSpec &c = format->chars[charIdx];
		if (c.display == font::CharSpec::Char) {
			auto texIt = std::lower_bound(texVec->begin(), texVec->end(), c.charID);
			auto charIt = std::lower_bound(charVec->begin(), charVec->end(), c.charID);

			if (texIt != texVec->end() && texIt->charID == c.charID && charIt != charVec->end() && charIt->charID == c.charID && texIt->texture != maxOf<uint8_t>()) {
				const auto posX = c.pos + charIt->xOffset;
				const auto posY = format->height - line->pos - (charIt->yOffset + texIt->height) - metrics->descender;
				switch (range->align) {
				case font::VerticalAlign::Sub:
					rects.emplace_back(posX * scale, (posY + metrics->descender / 2) * scale, texIt->width * scale, texIt->height * scale);
					break;
				case font::VerticalAlign::Super:
					rects.emplace_back(posX * scale, (posY + metrics->ascender / 2) * scale, texIt->width * scale, texIt->height * scale);
					break;
				default:
					rects.emplace_back(posX * scale, posY * scale, texIt->width * scale, texIt->height * scale);
					break;
				}
			}
		}
	}

	return true;
}

bool FontLibrary::isSourceRequestValid(Source *source, uint32_t v) {
	if (!source->isTextureRequestValid(v)) {
		_sources.erase(source);
		log::format("RequestAborted", "version: %u", v);
		return false;
	}
	return true;
}

bool FontLibraryCache::updateTextureWithSource(uint32_t v, Source *source, const Map<String, Vector<char16_t>> &l, Vector<Rc<cocos2d::Texture2D>> &t) {
	if (!library->isSourceRequestValid(source, v)) {
		return false;
	}

	Map<String, Vector<CharTexture>> sourceRef;
	size_t count = 0;
	Vector<FontLayoutData> names; names.reserve(l.size());
	for (auto &it : l) {
		auto lIt = sourceRef.emplace(it.first, Vector<CharTexture>()).first;
		auto &lRef = lIt->second;
		lRef.reserve(it.second.size());
		for (auto &c : it.second) {
			lRef.push_back(CharTexture{c});
		}

		auto layout = source->getLayout(it.first);
		if (!layout) {
			auto params = font::FontParameters::create(it.first);
			layout = source->getLayout(params);
			layout->addSortedChars(it.second);
		}

		names.push_back(FontLayoutData{source->getLayout(it.first), &lRef, Vector<FT_Face>()});
		count += it.second.size();
	}

	Vector<CharTexture *> layoutData; layoutData.reserve(count);

	if (!library->isSourceRequestValid(source, v)) {
		return false;
	}

	float square = 0.0f;
	for (auto &data : names) {
		auto &chars = *(data.chars);
		auto &faces = data.faces;
		auto &cb = data.layout->getCallback();
		auto &srcs = data.layout->getFontFace().src;
		auto size = data.layout->getSize();

		for (auto &theChar : chars) {
			for (uint32_t i = 0; i <= srcs.size(); ++ i) {
				FT_Face face = nullptr;
				if (i < faces.size()) {
					face = faces.at(i);
				} else {
					if (i < srcs.size()) {
						faces.push_back(getFace(source, cb, srcs.at(i), size));
					} else {
						faces.push_back(getFace(source, cb, resource::getFallbackFont(), size));
					}
					face = faces.back();
				}

				if (face) {
					int glyph_index = FT_Get_Char_Index(face, theChar.charID);
					if (!glyph_index) {
						continue;
					}

					auto err = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT | FT_LOAD_NO_BITMAP);
					if (err != FT_Err_Ok) {
						continue;
					}

					theChar.width = (face->glyph->metrics.width >> 6);
					theChar.height = (face->glyph->metrics.height >> 6);

					auto it = std::lower_bound(layoutData.begin(), layoutData.end(), &theChar, [] (const CharTexture * l, const CharTexture * r) -> bool{
						if (l->height == r->height && l->width == r->width) {
							return l->charID < r->charID;
						} else if (l->height == r->height) {
							return l->width > r->width;
						} else {
							return l->height > r->height;
						}
					});
					layoutData.emplace(it, &theChar);
					square += (theChar.width + theChar.height);
					break;
				}
			}
		}
	}

	if (!library->isSourceRequestValid(source, v)) {
		return false;
	}

	bool emplaced = false;
	count = layoutData.size();
	if (square < 3_MiB) {
		bool s = true;
		uint16_t h = 256, w = 256; uint32_t sq2 = h * w;
		for (; sq2 < square; sq2 *= 2, s = !s) {
			if (s) { w *= 2; } else { h *= 2; }
		}

		LayoutNode *l = nullptr;
		while (sq2 < 4_MiB) {
			if (l) { delete l; l = nullptr; }
			l = new LayoutNode(URect{0, 0, w, h});
			for (auto &it : layoutData) {
				if (!l->insert(it)) {
					break;
				}
			}

			auto nodes = l->nodes();
			if (nodes == count) {
				l->finalize(0);
				t.emplace_back(Rc<cocos2d::Texture2D>::create(cocos2d::Texture2D::PixelFormat::A8, w, h));
				t.back()->updateWithData("\xFF", w-1, h-1, 1, 1);
				t.back()->setAliasTexParameters();
				delete l;
				emplaced = true;
				break;
			} else {
				if (s) { w *= 2; } else { h *= 2; }
				sq2 *= 2; s = !s;
			}
		}
	}
	if (!emplaced) {
		uint16_t w = 2048, h = 2048;
		uint8_t bmpIdx = 0;
		LayoutNode *l = new LayoutNode(URect{0, 0, w, h});
		for (auto &it : layoutData) {
			if (!l->insert(it)) {
				l->finalize(bmpIdx);
				t.emplace_back(Rc<cocos2d::Texture2D>::create(cocos2d::Texture2D::PixelFormat::A8, w, h));
				t.back()->updateWithData("\xFF", w-1, h-1, 1, 1);
				t.back()->setAliasTexParameters();
				delete l;

				l = new LayoutNode(URect{0, 0, w, h});
				l->insert(it);
				++ bmpIdx;
			}
		}

		if (l && l->nodes() > 0) {
			l->finalize(bmpIdx);
			t.emplace_back(Rc<cocos2d::Texture2D>::create(cocos2d::Texture2D::PixelFormat::A8, w, h));
			t.back()->updateWithData("\xFF", w-1, h-1, 1, 1);
			t.back()->setAliasTexParameters();
		}
		if (l) {
			delete l;
		}
	}

	for (auto &data : names) {
		auto &chars = *(data.chars);
		auto &faces = data.faces;
		for (auto &theChar : chars) {
			if (!library->isSourceRequestValid(source, v)) {
				return false;
			}
			for (uint32_t i = 0; i < faces.size(); ++ i) {
				FT_Face face = faces.at(i);
				if (face) {
					int glyph_index = FT_Get_Char_Index(face, theChar.charID);
					if (!glyph_index) {
						continue;
					}

					auto err = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT | FT_LOAD_RENDER);
					if (err != FT_Err_Ok) {
						continue;
					}

					//log::format("Texture", "%s: %d '%s'", data.layout->getName().c_str(), theChar.charID, string::toUtf8(theChar.charID).c_str());

					if (face->glyph->bitmap.buffer != nullptr) {
						drawCharOnBitmap(t.at(theChar.texture), URect{theChar.x, theChar.y, theChar.width, theChar.height}, &face->glyph->bitmap);
					} else {
						if (!string::isspace(theChar.charID)) {
							log::format("Font", "error: no bitmap for (%d) '%s'", theChar.charID, string::toUtf8(theChar.charID).c_str());
						}
					}
					break;
				}
			}
		}
	}

	if (!library->isSourceRequestValid(source, v)) {
		return false;
	}

	library->setSourceLayout(source, std::move(sourceRef));
	return true;
}

FontLibrary *FontLibrary::getInstance() {
	s_mutex.lock();
	if (!s_instance) {
		s_instance = new FontLibrary;
	}
	s_mutex.unlock();
	return s_instance;
}

FontLibrary::FontLibrary() {
	Thread::onMainThread([this] {
		auto scheduler = cocos2d::Director::getInstance()->getScheduler();
		scheduler->scheduleUpdate(this, 0, false);
	});
	_timer = Time::now();
}

FontLibrary::~FontLibrary() {
	cocos2d::Director::getInstance()->getScheduler()->unscheduleUpdate(this);
}

void FontLibrary::update(float dt) {
	auto now = Time::now();
	if (now - _timer > TimeInterval::seconds(1)) {
		_timer = Time::now();

		_mutex.lock();

		Vector<uint64_t> expired;

		for (auto &it : _cache) {
			if (now - it.second->getTimer() < TimeInterval::seconds(1)) {
				expired.emplace_back(it.first);
			}
		}

		for (auto &it : expired) {
			_cache.erase(it);
		}

		_mutex.unlock();
	}
}

Arc<FontLibraryCache> FontLibrary::getCache() {
	Arc<FontLibraryCache> ret;
	_mutex.lock();
	auto id = ThreadManager::getInstance()->getNativeThreadId();
	auto it = _cache.find(id);
	if (it != _cache.end()) {
		ret = it->second;
		ret->update();
	} else {
		ret = _cache.emplace(id, Arc<FontLibraryCache>::create(this)).first->second;
	}

	_mutex.unlock();
	return ret;
}

Metrics FontLayout::requestMetrics(const Source *source, const Vector<String> &srcs, uint16_t size, const ReceiptCallback &cb) {
	auto cache = FontLibrary::getInstance()->getCache();
	return cache->requestMetrics(source, srcs, size, cb);
}

Arc<FontLayout::Data> FontLayout::requestLayoutUpgrade(const Source *source, const Vector<String> &srcs, const Arc<Data> &data, const Vector<char16_t> &chars, const ReceiptCallback &cb) {
	auto cache = FontLibrary::getInstance()->getCache();
	return cache->requestLayoutUpgrade(source, srcs, data, chars, cb);
}


void Source::clone(Source *source, const Function<void(Source *)> &cb) {
	if (source->getLayoutMap().empty()) {
		if (cb) {
			cb(this);
		}
	}
	auto lPtr = new Map<String, Arc<FontLayout>>(source->getLayoutMap());
	auto tlPtr = new Map<String, Vector<char16_t>>(source->getTextureLayoutMap());
	auto tPtr = new Vector<Rc<cocos2d::Texture2D>>();
	auto sPtr = new Rc<Source>(source);

	auto &thread = TextureCache::thread();
	thread.perform([this, sPtr, lPtr, tlPtr, tPtr] (Ref *) -> bool {
		Vector<char16_t> vec;;
		for (auto &it : (*lPtr)) {
			Arc<FontLayout> l(it.second);
			auto style = l->getStyle();
			auto data = l->getData();
			vec.clear();
			if (vec.capacity() < data->chars.size()) {
				vec.reserve(data->chars.size());
			}
			for (auto &c : data->chars) {
				vec.emplace_back(c);
			}

			getLayout(style)->addSortedChars(vec);
		}

		auto ret = TextureCache::getInstance()->performWithGL([&] {
			auto cache = FontLibrary::getInstance()->getCache();
			return cache->updateTextureWithSource(_version.load(), this, *tlPtr, *tPtr);
		});
		delete lPtr;
		return ret;
	}, [this, tlPtr, tPtr, sPtr, cb] (Ref *, bool success) {
		if (success) {
			onTextureResult(std::move(*tlPtr), std::move(*tPtr));
			if (cb) {
				cb(this);
			}
		}
		delete tlPtr;
		delete tPtr;
		delete sPtr;
	}, this);
}

void Source::updateTexture(uint32_t v, const Map<String, Vector<char16_t>> &l) {
	auto &thread = TextureCache::thread();
	if (thread.isOnThisThread()) {
		Vector<Rc<cocos2d::Texture2D>> tPtr;
		TextureCache::getInstance()->performWithGL([&] {
			auto cache = FontLibrary::getInstance()->getCache();
			if (cache->updateTextureWithSource(v, this, l, tPtr)) {
				onTextureResult(std::move(tPtr));
			}
		});
	} else {
		auto lPtr = new Map<String, Vector<char16_t>>(l);
		auto tPtr = new Vector<Rc<cocos2d::Texture2D>>();
		thread.perform([this, lPtr, tPtr, v] (Ref *) -> bool {
			auto ret = TextureCache::getInstance()->performWithGL([&] {
				auto cache = FontLibrary::getInstance()->getCache();
				return cache->updateTextureWithSource(v, this, *lPtr, *tPtr);
			});
			delete lPtr;
			return ret;
		}, [this, tPtr] (Ref *, bool success) {
			if (success) {
				onTextureResult(std::move(*tPtr));
			}
			delete tPtr;
		}, this);
	}
}

void Source::cleanup() {
	Source *s = this;
	auto &thread = TextureCache::thread();
	thread.perform([s] (Ref *) -> bool {
		auto cache = FontLibrary::getInstance();
		cache->cleanupSource(s);
		return true;
	});
}

LayoutBitmap::LayoutBitmap(uint32_t w, uint32_t h) : Bitmap() {
	alloc(w, h, Format::A8);
	_data.back() = 255;
}

void LayoutBitmap::draw(const URect &rect, FT_Bitmap *bitmap) {
	auto xOffset = rect.x;
	auto yOffset = rect.y;
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

LayoutNode::LayoutNode(const URect &rect) {
	_rc = rect;
	_child[0] = nullptr;
	_child[1] = nullptr;
}

LayoutNode::LayoutNode(const UVec2 &origin, CharTexture *c) {
	_child[0] = nullptr;
	_child[1] = nullptr;
	_char = c;
	_rc = URect{origin.x, origin.y, c->width, c->height};
}

LayoutNode::~LayoutNode() {
	if (_child[0]) delete _child[0];
	if (_child[1]) delete _child[1];
}

bool LayoutNode::insert(CharTexture *c) {
	if (_child[0] && _child[1]) {
		return _child[0]->insert(c) || _child[1]->insert(c);
	} else {
		if (_char) {
			return false;
		}

		if (_rc.width < c->width  || _rc.height < c->height) {
			return false;
		}

		if (_rc.width == c->width || _rc.height == c->height) {
			if (_rc.height == c->height) {
				_child[0] = new LayoutNode(_rc.origin(), c);
				_child[1] = new LayoutNode(URect{uint16_t(_rc.x + c->width + LAYOUT_PADDING), _rc.y,
					(_rc.width > c->width + LAYOUT_PADDING)?uint16_t(_rc.width - c->width - LAYOUT_PADDING):uint16_t(0), _rc.height});
			} else if (_rc.width == c->width) {
				_child[0] = new LayoutNode(_rc.origin(), c);
				_child[1] = new LayoutNode(URect{_rc.x, uint16_t(_rc.y + c->height + LAYOUT_PADDING),
					_rc.width, (_rc.height > c->height + LAYOUT_PADDING)?uint16_t(_rc.height - c->height - LAYOUT_PADDING):uint16_t(0)});
			}
			return true;
		}

		//(decide which way to split)
		int16_t dw = _rc.width - c->width;
		int16_t dh = _rc.height - c->height;

		if (dw > dh) {
			_child[0] = new LayoutNode(URect{_rc.x, _rc.y, c->width, _rc.height});
			_child[1] = new LayoutNode(URect{uint16_t(_rc.x + c->width + LAYOUT_PADDING), _rc.y, uint16_t(dw - LAYOUT_PADDING), _rc.height});
		} else {
			_child[0] = new LayoutNode(URect{_rc.x, _rc.y, _rc.width, c->height});
			_child[1] = new LayoutNode(URect{_rc.x, uint16_t(_rc.y + c->height + LAYOUT_PADDING), _rc.width, uint16_t(dh - LAYOUT_PADDING)});
		}

		_child[0]->insert(c);

		return true;
	}
}

size_t LayoutNode::nodes() const {
	if (_char) {
		return 1;
	} else if (_child[0] && _child[1]) {
		return _child[0]->nodes() + _child[1]->nodes();
	} else {
		return 0;
	}
}

void LayoutNode::finalize(uint8_t tex) {
	if (_char) {
		_char->x = _rc.x;
		_char->y = _rc.y;
		_char->texture = tex;
	} else {
		if (_child[0]) { _child[0]->finalize(tex); }
		if (_child[1]) { _child[1]->finalize(tex); }
	}
}

NS_SP_EXT_END(font)

NS_SP_BEGIN

void DynamicLabel::updateQuadsBackground(Source *source, FormatSpec *format) {
	auto v = source->getVersion();
	auto time = Time::now();
	auto sourceRef = new Rc<Source>(source);
	auto formatRef = new Rc<FormatSpec>(format);
	auto tPtr = new Vector<Rc<cocos2d::Texture2D>>(source->getTextures());
	auto qPtr = new Vector<Rc<DynamicQuadArray>>();
	auto cPtr = new Vector<Vector<bool>>();

	auto &thread = TextureCache::thread();
	thread.perform([this, sourceRef, formatRef, tPtr, qPtr, cPtr, v] (Ref *) -> bool {
		auto cache = font::FontLibrary::getInstance();
		return cache->writeTextureQuads(v, *sourceRef, *formatRef, *tPtr, *qPtr, *cPtr);
	}, [this, time, sourceRef, formatRef, tPtr, qPtr, cPtr] (Ref *, bool success) {
		if (success) {
			onQuads(time, *tPtr, std::move(*qPtr), std::move(*cPtr));
		}
		delete sourceRef;
		delete formatRef;
		delete tPtr;
		delete qPtr;
		delete cPtr;
	}, this);
}

void DynamicLabel::updateQuadsForeground(Source *source, const FormatSpec *format) {
	auto v = source->getVersion();
	auto time = Time::now();

	Vector<Rc<cocos2d::Texture2D>> tPtr(source->getTextures());
	Vector<Rc<DynamicQuadArray>> qPtr;
	Vector<Vector<bool>> cPtr;

	auto cache = font::FontLibrary::getInstance();
	if (cache->writeTextureQuads(v, source, format, tPtr, qPtr, cPtr)) {
		onQuads(time, tPtr, std::move(qPtr), std::move(cPtr));
	}
}

void DynamicLabel::makeLabelQuads(Source *source, const FormatSpec *format,
		const Function<void(const TextureVec &newTex, QuadVec &&newQuads, ColorMapVec &&cMap)> &cb) {
	auto v = source->getVersion();

	auto &tPtr = source->getTextures();
	Vector<Rc<DynamicQuadArray>> qPtr;
	Vector<Vector<bool>> cPtr;

	auto cache = font::FontLibrary::getInstance();
	if (cache->writeTextureQuads(v, source, format, tPtr, qPtr, cPtr)) {
		cb(tPtr, std::move(qPtr), std::move(cPtr));
	}
}

void DynamicLabel::makeLabelRects(Source *source, const FormatSpec *format, float scale, const Function<void(const Vector<Rect> &)> &cb) {
	auto v = source->getVersion();
	Vector<Rect> rects; rects.reserve(format->chars.size());

	auto cache = font::FontLibrary::getInstance();
	if (cache->writeTextureRects(v, source, format, scale, rects)) {
		cb(rects);
	}
}

NS_SP_END
