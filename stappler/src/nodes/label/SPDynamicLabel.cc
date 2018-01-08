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
#include "SPDynamicLabel.h"
#include "SPString.h"
#include "SPLocale.h"
#include "SLFontFormatter.h"
#include "SPGLProgramSet.h"
#include "SPLayer.h"
#include "SPEventListener.h"
#include "SPTextureCache.h"

#include "renderer/CCGLProgram.h"
#include "renderer/CCGLProgramState.h"
#include "renderer/CCGLProgramCache.h"

NS_SP_BEGIN

DynamicLabel::~DynamicLabel() { }

bool DynamicLabel::init(Source *source, const DescriptionStyle &style, const String &str, float width, Alignment alignment, float density) {
	_isHighPrecision = true;

	if (!LayeredBatchNode::init(density)) {
		return false;
	}

	_source = source;
	_style = style;

	auto el = Rc<EventListener>::create();
	el->onEventWithObject(Source::onTextureUpdated, source, std::bind(&DynamicLabel::onTextureUpdated, this));
	addComponent(el);

	_listener = el;

	setColor(_style.text.color);
	setOpacity(_style.text.opacity);

	setNormalized(true);

	setString(str);
	setWidth(width);
	setAlignment(alignment);

	updateLabel();

    return true;
}

void DynamicLabel::tryUpdateLabel(bool force) {
	if (_labelDirty) {
		if (force) {
			_textures.clear();
		}
		updateLabel();
	}
}

void DynamicLabel::setStyle(const DescriptionStyle &style) {
	_style = style;

    setColor(_style.text.color);
    setOpacity(_style.text.opacity);

	_labelDirty = true;
}

const DynamicLabel::DescriptionStyle &DynamicLabel::getStyle() const {
	return _style;
}

void DynamicLabel::setSource(Source *source) {
	if (source != _source) {
		_listener->clear();
		_source = source;
		_textures.clear();
		_colorMap.clear();
		_format = nullptr;
		_labelDirty = true;
		_listener->onEventWithObject(Source::onTextureUpdated, source, std::bind(&DynamicLabel::onTextureUpdated, this));
	}
}

DynamicLabel::Source *DynamicLabel::getSource() const {
	return _source;
}

void DynamicLabel::updateLabel() {
	if (!_source) {
		return;
	}

	if (_string16.empty()) {
		_format = nullptr;
		_formatDirty = true;
		setContentSize(Size(0.0f, getFontHeight() / _density));
		return;
	}

	auto spec = Rc<layout::FormatSpec>::create(_string16.size(), _compiledStyles.size() + 1);
	updateLabel(spec);

	if (_format) {
		if (_format->chars.empty()) {
			setContentSize(Size(0.0f, getFontHeight() / _density));
		} else {
			setContentSize(Size(_format->width / _density, _format->height / _density));
		}

		_labelDirty = false;
		_colorDirty = false;
		_formatDirty = true;
	}
}

void DynamicLabel::updateLabel(FormatSpec *format) {
	_compiledStyles = compileStyle();
	_format = format;

	_style.text.color = _displayedColor;
	_style.text.opacity = _displayedOpacity;
	_style.text.whiteSpace = layout::style::WhiteSpace::PreWrap;

	auto adjustValue = 255;

	do {
		if (adjustValue == 255) {
			adjustValue = 0;
		} else {
			adjustValue += 1;
		}

		_format->clear();

		layout::Formatter formatter(_source, _format, _density);
		formatter.setWidth((uint16_t)roundf(_width * _density));
		formatter.setTextAlignment(_alignment);
		formatter.setMaxWidth((uint16_t)roundf(_maxWidth * _density));
		formatter.setMaxLines(_maxLines);
		formatter.setOpticalAlignment(_opticalAlignment);
		formatter.setFillerChar(_fillerChar);
		formatter.setEmplaceAllChars(_emplaceAllChars);

		if (_lineHeight != 0.0f) {
			if (_isLineHeightAbsolute) {
				formatter.setLineHeightAbsolute((uint16_t)(_lineHeight * _density));
			} else {
				formatter.setLineHeightRelative(_lineHeight);
			}
		}

		formatter.begin((uint16_t)roundf(_textIndent * _density));

		size_t drawedChars = 0;
		for (auto &it : _compiledStyles) {
			DescriptionStyle params = _style.merge(_source, it.style);
			if (adjustValue > 0) {
				params.font.fontSize -= adjustValue;
			}

			auto start = _string16.c_str() + it.start;
			auto len = it.length;

			if (_localeEnabled && hasLocaleTags(start, len)) {
				WideString str(resolveLocaleTags(start, len));

				start = str.c_str();
				len = str.length();

				if (_maxChars > 0 && drawedChars + len > _maxChars) {
					len = _maxChars - drawedChars;
				}
				if (!formatter.read(params.font, params.text, start, len)) {
					drawedChars += len;
					break;
				}
			} else {
				if (_maxChars > 0 && drawedChars + len > _maxChars) {
					len = _maxChars - drawedChars;
				}
				if (!formatter.read(params.font, params.text, start, len)) {
					drawedChars += len;
					break;
				}
			}

			if (!_format->ranges.empty()) {
				_format->ranges.back().colorDirty = params.colorDirty;
				_format->ranges.back().opacityDirty = params.opacityDirty;
			}
		}
		formatter.finalize();
	} while(_format->overflow && adjustValue < _adjustValue);
}

void DynamicLabel::visit(cocos2d::Renderer *r, const Mat4& t, uint32_t f, ZPath &zPath) {
	if (!_visible) {
		return;
	}
	if (_labelDirty) {
		updateLabel();
	}
	if (_formatDirty) {
		updateQuads(f);
	}
	if (_colorDirty) {
		updateColorQuads();
	}
	LayeredBatchNode::visit(r, t, f, zPath);
}

void DynamicLabel::updateColor() {
	if (_format) {
		for (auto &it : _format->ranges) {
			if (!it.colorDirty) {
				it.color.r = _displayedColor.r;
				it.color.g = _displayedColor.g;
				it.color.b = _displayedColor.b;
			}
			if (!it.opacityDirty) {
				it.color.a = _displayedOpacity;
			}
		}
	}
	_colorDirty = true;
}

void DynamicLabel::updateColorQuads() {
	if (!_textures.empty() && !_colorMap.empty()) {
		for (size_t i = 0; i < _textures.size(); ++ i) {
			DynamicQuadArray * quads = _textures[i].quads;
			auto &cMap = _colorMap[i];
			if (quads->size() * 2 == cMap.size()) {
				for (size_t j = 0; j < quads->size(); ++j) {
					if (!cMap[j * 2]) { quads->setColor3B(j, _displayedColor); }
					if (!cMap[j * 2 + 1]) { quads->setOpacity(j, _displayedOpacity); }
				}
			}
		}
	}
	_colorDirty = false;
}

void DynamicLabel::setDensity(float density) {
	if (density != _density) {
		LayeredBatchNode::setDensity(density);
		_labelDirty = true;
	}
}

void DynamicLabel::setStandalone(bool value) {
	if (_standalone != value) {
		_standalone = value;
		_standaloneTextures.clear();
		_standaloneMap.clear();
		_standaloneChars.clear();
		_formatDirty = true;
	}
}
bool DynamicLabel::isStandalone() const {
	return _standalone;
}

void DynamicLabel::setAdjustValue(uint8_t val) {
	if (_adjustValue != val) {
		_adjustValue = val;
		_labelDirty = true;
	}
}
uint8_t DynamicLabel::getAdjustValue() const {
	return _adjustValue;
}

bool DynamicLabel::isOverflow() const {
	if (_format) {
		return _format->overflow;
	}
	return false;
}

size_t DynamicLabel::getCharsCount() const {
	return _format?_format->chars.size():0;
}
size_t DynamicLabel::getLinesCount() const {
	return _format?_format->lines.size():0;
}
DynamicLabel::LineSpec DynamicLabel::getLine(uint32_t num) const {
	if (_format) {
		if (num < _format->lines.size()) {
			return _format->lines[num];
		}
	}
	return LineSpec();
}

uint16_t DynamicLabel::getFontHeight() const {
	return const_cast<Source *>(_source.get())->getLayout(_style.font)->getData()->metrics.height;
}

void DynamicLabel::updateQuads(uint32_t f) {
	if (!_source) {
		return;
	}

	if (!_format || _format->chars.size() == 0) {
		_textures.clear();
		return;
	}

	if (!_standalone) {
		for (auto &it : _format->ranges) {
			_source->addTextureChars(it.layout->getName(), _format->chars, it.start, it.count);
		}

		if (!_source->isDirty() && !_source->getTextures().empty()) {
			updateQuadsForeground(_source, _format);
		}
	} else {
		bool sourceDirty = false;
		for (auto &it : _format->ranges) {
			auto find_it = _standaloneChars.find(it.layout->getName());
			if (find_it == _standaloneChars.end()) {
				find_it = _standaloneChars.emplace(it.layout->getName(), Vector<char16_t>()).first;
			}

			auto &vec = find_it->second;
			for (uint32_t i = it.start; i < it.count; ++ i) {
				const char16_t &c = _format->chars[i].charID;
				auto char_it = std::lower_bound(vec.begin(), vec.end(), c);
				if (char_it == vec.end() || *char_it != c) {
					vec.emplace(char_it, c);
					sourceDirty = true;
				}
			}
		}

		if (sourceDirty) {
			_standaloneTextures.clear();
			_standaloneMap = _source->updateTextures(_standaloneChars, _standaloneTextures);
		}

		if (!_standaloneTextures.empty()) {
			updateQuadsStandalone(_source, _format);
		}
	}
}

void DynamicLabel::onTextureUpdated() {
	if (!_standalone) {
		_formatDirty = true;
	}
	//_textures.clear();
}

void DynamicLabel::onLayoutUpdated() {
	_labelDirty = false;
}

void DynamicLabel::onQuads(const Time &t, const Vector<Rc<cocos2d::Texture2D>> &newTex,
		Vector<Rc<DynamicQuadArray>> &&newQuads, Vector<Vector<bool>> &&cMap) {

	//log::format("Label", "onQuads %p %lu %s", this, _updateCount, _string8.c_str());

	_formatDirty = false;
	++ _updateCount;

	if (t < _quadRequestTime) {
		return;
	}

	if (newQuads.size() != newTex.size()) {
		return;
	}

	bool replaceTextures = false;
	if (newTex.size() != _textures.size()) {
		replaceTextures = true;
	} else {
		for (size_t i = 0; i < newTex.size(); ++ i) {
			if (newTex[i]->getName() != _textures[i].texture->getName()) {
				replaceTextures = true;
			}
		}
	}

	if (replaceTextures) {
		setTextures(newTex, std::move(newQuads));
	} else {
		for (size_t i = 0; i < newQuads.size(); ++i) {
			_textures[i].quads = std::move(newQuads[i]);
			auto a = _textures[i].atlas;
			if (a) {
				a->clear();
				a->addQuadArray(_textures[i].quads);
			}
		}
	}

	_colorMap = std::move(cMap);

	updateColorQuads();
}

Vec2 DynamicLabel::getCursorPosition(uint32_t charIndex, bool front) const {
	if (_format) {
		if (charIndex < _format->chars.size()) {
			auto &c = _format->chars[charIndex];
			auto line = _format->getLine(charIndex);
			if (line) {
				return Vec2( (front ? c.pos : c.pos + c.advance) / _density, _contentSize.height - line->pos / _density);
			}
		} else if (charIndex >= _format->chars.size() && charIndex != 0) {
			auto &c = _format->chars.back();
			auto &l = _format->lines.back();
			if (c.charID == char16_t(0x0A)) {
				return getCursorOrigin();
			} else {
				return Vec2( (c.pos + c.advance) / _density, _contentSize.height - l.pos / _density);
			}
		}
	}

	return Vec2::ZERO;
}

Vec2 DynamicLabel::getCursorOrigin() const {
	switch (_alignment) {
	case Alignment::Left:
	case Alignment::Justify:
		return Vec2( 0.0f / _density, _contentSize.height - _format->height / _density);
		break;
	case Alignment::Center:
		return Vec2( _contentSize.width * 0.5f / _density, _contentSize.height - _format->height / _density);
		break;
	case Alignment::Right:
		return Vec2( _contentSize.width / _density, _contentSize.height - _format->height / _density);
		break;
	}
	return Vec2::ZERO;
}

Pair<uint32_t, bool> DynamicLabel::getCharIndex(const Vec2 &pos) const {
	auto ret = _format->getChar(pos.x * _density, _format->height - pos.y * _density, FormatSpec::Best);
	if (ret.first == maxOf<uint32_t>()) {
		return pair(maxOf<uint32_t>(), false);
	} else if (ret.second == FormatSpec::Prefix) {
		return pair(ret.first, false);
	} else {
		return pair(ret.first, true);
	}
}

float DynamicLabel::getMaxLineX() const {
	if (_format) {
		return _format->maxLineX / _density;
	}
	return 0.0f;
}

NS_SP_END
