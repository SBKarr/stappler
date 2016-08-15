/*
 * MaterialFonts.cpp
 *
 *  Created on: 12 нояб. 2014 г.
 *      Author: sbkarr
 */

#include "Material.h"
#include "MaterialFont.h"
#include "MaterialResourceManager.h"

#include "SPFont.h"

NS_MD_BEGIN

const Font *Font::systemFont(Type type) {
	return ResourceManager::getInstance()->getSystemFont(type);
}

Font::Font (Type type, const std::string &name, uint16_t size, uint8_t opacity, bool caps)
: _type(type), _opacity(opacity), _caps(caps), _name(name), _size(size) { }

Font::Font (Type type, const std::string &name, stappler::Font *font, uint8_t opacity, bool caps)
: _type(type), _opacity(opacity), _caps(caps), _name(name), _size(font->getSize()), _font(font) { }

void Font::setFont(stappler::Font *font) {
	_font = font;
	if (_font) {
		_size = _font->getSize();
	}
}

std::vector<stappler::FontRequest> FontSet::getRequest(float density) {
	return {
		stappler::FontRequest("Headline", "common/fonts/Roboto-Regular.woff", (uint16_t)floorf(24 * density)),
		stappler::FontRequest("Title", "common/fonts/Roboto-Medium.woff", (uint16_t)floorf(20 * density)),
		stappler::FontRequest("Subhead", "common/fonts/Roboto-Regular.woff", (uint16_t)floorf(16 * density)),
		stappler::FontRequest("Body_1", "common/fonts/Roboto-Medium.woff", (uint16_t)floorf(14 * density)),
		stappler::FontRequest("Body_2", "common/fonts/Roboto-Regular.woff", (uint16_t)floorf(14 * density)),
		stappler::FontRequest("Caption", "common/fonts/Roboto-Regular.woff", (uint16_t)floorf(12 * density))
	};
}

FontSet::FontSet() { }

bool FontSet::init(stappler::FontSet *set) {
	_name = set->getName();
	_version = set->getVersion();
	auto &systemFonts = set->getAllFonts();
	for (auto &it : systemFonts) {
		if (it.first == "Headline") {
			_fonts[Font::Type::System_Headline] = new Font(Font::Type::System_Headline, "Headline", it.second, 222);
		} else if (it.first == "Title") {
			_fonts[Font::Type::System_Title] = new Font(Font::Type::System_Title, "Title", it.second, 222);
		} else if (it.first == "Subhead") {
			_fonts[Font::Type::System_Subhead] = new Font(Font::Type::System_Subhead, "Subhead", it.second, 222);
		} else if (it.first == "Body_1") {
			_fonts[Font::Type::System_Body_1] = new Font(Font::Type::System_Body_1, "Body_1", it.second, 222);
			_fonts[Font::Type::System_Button] = new Font(Font::Type::System_Button, "Button", it.second, 222, true);
		} else if (it.first == "Body_2") {
			_fonts[Font::Type::System_Body_2] = new Font(Font::Type::System_Body_2, "Body_2", it.second, 222);
		} else if (it.first == "Caption") {
			_fonts[Font::Type::System_Caption] = new Font(Font::Type::System_Caption, "Caption", it.second, 138);
		}
	}
	return true;
}

const material::Font * FontSet::getFont(Font::Type type) const {
	auto it = _fonts.find(type);
	if (it != _fonts.end()) {
		return it->second;
	}
	return nullptr;
}

NS_MD_END
