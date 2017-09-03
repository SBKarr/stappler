// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2016-2017 Roman Katuntsev <sbkarr@stappler.org>

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

#include "SLFontLibrary.h"
#include "SPBitmap.h"
#include "SPFilesystem.h"

#include "ft2build.h"
#include FT_FREETYPE_H
#include FT_GLYPH_H

NS_LAYOUT_BEGIN

constexpr uint16_t LAYOUT_PADDING  = 1;

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

static CharGroupId getCharGroupForChar(char16_t c) {
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

static void addCharGroup(Vector<char16_t> &vec, CharGroupId id) {
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

static void addLayoutChar(Vector<char16_t> &charsToUpdate, const Arc<FontLayout::Data> &data, const Vector<char16_t> &chars, char16_t theChar) {
	auto spaceIt = std::lower_bound(chars.begin(), chars.end(), theChar);
	if (spaceIt == chars.end() || *spaceIt != theChar) {
		auto spaceDataIt = std::lower_bound(data->chars.begin(), data->chars.end(), theChar);
		if (spaceDataIt == data->chars.end() || spaceDataIt->charID != theChar) {
			charsToUpdate.push_back(theChar);
		}
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

static FontStructMap::iterator processFile(FontStructMap & openedFiles, Bytes && data, const String &file) {
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

static Metrics getMetrics(FT_Face face, uint16_t size) {
	Metrics ret;
	ret.size = size;
	ret.height = face->size->metrics.height >> 6;
	ret.ascender = face->size->metrics.ascender >> 6;
	ret.descender = face->size->metrics.descender >> 6;
	ret.underlinePosition = face->underline_position >> 6;
	ret.underlineThickness = face->underline_thickness >> 6;
	return ret;
}

LayoutBitmap::LayoutBitmap(uint32_t w, uint32_t h) : Bitmap() {
	alloc(w, h, PixelFormat::A8);
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
				data[bufferPos] = (bitmap->buffer[pos / 8] & (1 << (pos % 8)))?255:0;
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

FT_Face FreeTypeInterface::openFontFace(const Bytes &data, const String &font, uint16_t fontSize) {
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

FontStructMap::iterator FreeTypeInterface::openFile(const FontSource *source, const ReceiptCallback &cb, const String &name, const String &file) {
	auto it = openedFiles.find(file);
	if (it != openedFiles.end()) {
		return it;
	} else {
		if (files.find(file) == files.end()) {
			files.insert(file);
			if (cb) {
				auto data = cb(source, file);
				if (!data.empty()) {
					return processFile(openedFiles, std::move(data), file);
				}
			}
			return processFile(openedFiles, filesystem::readFile(file), file);
		}
	}
	return openedFiles.end();
}

FT_Face FreeTypeInterface::getFace(const FontSource *source, const ReceiptCallback &cb, const String &file, uint16_t size) {
	String fontname = toString(file, ":", size);
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

void FreeTypeInterface::requestCharUpdate(const FontSource *source, const ReceiptCallback &cb, const Vector<String> &srcs, uint16_t size,
		Vector<FT_Face> &faces, Vector<CharLayout> &layout, char16_t theChar) {
	for (uint32_t i = 0; i <= srcs.size(); ++ i) {
		FT_Face face = nullptr;
		if (i < faces.size()) {
			face = faces.at(i);
		} else {
			if (i < srcs.size()) {
				faces.push_back(getFace(source, cb, srcs.at(i), size));
			} else {
				faces.push_back(getFace(source, cb, fallbackFont, size));
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

bool FreeTypeInterface::getKerning(const Vector<FT_Face> &faces, char16_t first, char16_t second, int16_t &value) {
	auto firstFace = getFaceForChar(faces, first);
	auto secondFace = getFaceForChar(faces, second);
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

FreeTypeInterface::FreeTypeInterface(const String &def) : fallbackFont(def) {
	FT_Init_FreeType( &FTlibrary );
}

FreeTypeInterface::~FreeTypeInterface() {
	clear();
	FT_Done_FreeType(FTlibrary);
}

void FreeTypeInterface::clear() {
	for (auto &it : openedFaces) {
		if (it.second) {
			FT_Done_Face(it.second);
		}
	}

	files.clear();
	openedFiles.clear();
	openedFaces.clear();
}

Metrics FreeTypeInterface::requestMetrics(const FontSource *source, const Vector<String> &srcs, uint16_t size, const ReceiptCallback &cb) {
	for (auto &it : srcs) {
		if (auto face = getFace(source, cb, it, size)) {
			return getMetrics(face, size);
		}
	}
	return Metrics();
}

Arc<FontLayout::Data> FreeTypeInterface::requestLayoutUpgrade(const FontSource *source, const Vector<String> &srcs,
		const Arc<FontLayout::Data> &data, const Vector<char16_t> &chars, const ReceiptCallback &cb) {
	Arc<FontLayout::Data> ret;
	if (!data || data->metrics.size == 0) {
		return data;
	} else {
		ret = Arc<FontLayout::Data>::create(*(data.get()));

		Vector<char16_t> charsToUpdate;

		uint32_t mask = 0;

		addLayoutChar(charsToUpdate, data, chars, char16_t(' '));
		addLayoutChar(charsToUpdate, data, chars, char16_t('\n'));
		addLayoutChar(charsToUpdate, data, chars, char16_t('\r'));

		for (auto &c : chars) {
			auto g = getCharGroupForChar(c);
			if (g != CharGroupId::None) {
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

struct FontLayoutData {
	Arc<FontLayout> layout;
	Vector<CharTexture> *chars;
	Vector<FT_Face> faces;
};

FontTextureMap FreeTypeInterface::updateTextureWithSource(uint32_t v, FontSource *source, const Map<String, Vector<char16_t>> &l, const FontTextureInterface &t) {
	Map<String, Vector<CharTexture>> sourceRef;
	if (source && !source->isTextureRequestValid(v)) {
		return FontTextureMap();
	}

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
			auto params = FontParameters::create(it.first);
			layout = source->getLayout(params);
			layout->addSortedChars(it.second);
		}

		names.push_back(FontLayoutData{source->getLayout(it.first), &lRef, Vector<FT_Face>()});
		count += it.second.size();
	}

	Vector<CharTexture *> layoutData; layoutData.reserve(count);

	if (source && !source->isTextureRequestValid(v)) {
		return FontTextureMap();
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
						faces.push_back(getFace(source, cb, fallbackFont, size));
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

	if (source && !source->isTextureRequestValid(v)) {
		return FontTextureMap();
	}

	Vector<size_t> texIndexes;

	bool emplaced = false;
	count = layoutData.size();
	if (square < 3_MiB) {
		bool s = true;
		uint16_t h = 128, w = 128; uint32_t sq2 = h * w;
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
				texIndexes.emplace_back(t.emplaceTexture(w, h));
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
				texIndexes.emplace_back(t.emplaceTexture(w, h));
				delete l;

				l = new LayoutNode(URect{0, 0, w, h});
				l->insert(it);
				++ bmpIdx;
			}
		}

		if (l && l->nodes() > 0) {
			l->finalize(bmpIdx);
			texIndexes.emplace_back(t.emplaceTexture(w, h));
		}
		if (l) {
			delete l;
		}
	}

	for (auto &data : names) {
		auto &chars = *(data.chars);
		auto &faces = data.faces;
		for (auto &theChar : chars) {
			if (source && !source->isTextureRequestValid(v)) {
				return FontTextureMap();
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
						if (face->glyph->bitmap.pixel_mode == FT_PIXEL_MODE_MONO) {

						} else {
							auto &bmp = face->glyph->bitmap;
							t.draw(texIndexes[theChar.texture], bmp.buffer, theChar.x, theChar.y, bmp.width, bmp.rows);
						}
					} else {
						if (!string::isspace(theChar.charID) && theChar.charID != char16_t(0x0A)) {
							log::format("Font", "error: no bitmap for (%d) '%s'", theChar.charID, string::toUtf8(theChar.charID).c_str());
						}
					}
					break;
				}
			}
		}
	}

	if (source && !source->isTextureRequestValid(v)) {
		return FontTextureMap();
	}

	return sourceRef;
}

NS_LAYOUT_END
