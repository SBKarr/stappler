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

#include "SPLayout.h"
//#include "SLParser.h"
#include "SLStyle.h"
#include "SLRendererTypes.h"
#include "SPString.h"

NS_LAYOUT_BEGIN

SimpleRendererInterface::SimpleRendererInterface() { }
SimpleRendererInterface::SimpleRendererInterface(const Vector<bool> *media, const Map<CssStringId, String> *strings)
: _media(media), _strings(strings) { }

bool SimpleRendererInterface::resolveMediaQuery(MediaQueryId queryId) const {
	return _media->at(queryId);
}

StringView SimpleRendererInterface::getCssString(CssStringId id) const {
	return _strings->at(id);
}

namespace style {

static bool _readStyleValue(StringView &r, Metric &value, bool resolutionMetric = false, bool allowEmptyMetric = false) {
	r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
	if (!resolutionMetric && r.starts_with("auto")) {
		r += 4;
		value.metric = Metric::Units::Auto;
		value.value = 0.0f;
		return true;
	}

	auto fRes = r.readFloat();
	if (!fRes.valid()) {
		return false;
	}

	auto fvalue = fRes.get();
	if (fvalue == 0.0f) {
		value.value = fvalue;
		value.metric = Metric::Units::Px;
		return true;
	}

	r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();

	auto str = r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();

	if (!resolutionMetric) {
		if (str.is('%')) {
			++ str;
			value.value = fvalue / 100.0f;
			value.metric = Metric::Units::Percent;
			return true;
		} else if (str == "em") {
			str += 2;
			value.value = fvalue;
			value.metric = Metric::Units::Em;
			return true;
		} else if (str == "rem") {
			str += 3;
			value.value = fvalue;
			value.metric = Metric::Units::Rem;
			return true;
		} else if (str == "px") {
			str += 2;
			value.value = fvalue;
			value.metric = Metric::Units::Px;
			return true;
		} else if (str == "pt") {
			str += 2;
			value.value = fvalue * 4.0f / 3.0f;
			value.metric = Metric::Units::Px;
			return true;
		} else if (str == "pc") {
			str += 2;
			value.value = fvalue * 15.0f;
			value.metric = Metric::Units::Px;
			return true;
		} else if (str == "mm") {
			str += 2;
			value.value = fvalue * 3.543307f;
			value.metric = Metric::Units::Px;
			return true;
		} else if (str == "cm") {
			str += 2;
			value.value = fvalue * 35.43307f;
			value.metric = Metric::Units::Px;
			return true;
		} else if (str == "in") {
			str += 2;
			value.value = fvalue * 90.0f;
			value.metric = Metric::Units::Px;
			return true;
		} else if (str == "vw") {
			str += 2;
			value.value = fvalue;
			value.metric = Metric::Units::Vw;
			return true;
		} else if (str == "vh") {
			str += 2;
			value.value = fvalue;
			value.metric = Metric::Units::Vh;
			return true;
		} else if (str == "vmin") {
			str += 4;
			value.value = fvalue;
			value.metric = Metric::Units::VMin;
			return true;
		} else if (str == "vmax") {
			str += 4;
			value.value = fvalue;
			value.metric = Metric::Units::VMax;
			return true;
		}
	} else {
		if (str == "dpi") {
			str += 3;
			value.value = fvalue;
			value.metric = Metric::Units::Dpi;
			return true;
		} else if (str == "dpcm") {
			str += 4;
			value.value = fvalue / 2.54f;
			value.metric = Metric::Units::Dpi;
			return true;
		} else if (str == "dppx") {
			str += 4;
			value.value = fvalue;
			value.metric = Metric::Units::Dppx;
			return true;
		}
	}

	if (allowEmptyMetric) {
		value.value = fvalue;
		return true;
	}

	return false;
}

bool readStyleValue(const StringView &istr, Metric &value, bool resolutionMetric, bool allowEmptyMetric) {
	StringView str(istr);
	return _readStyleValue(str, value, resolutionMetric, allowEmptyMetric);
}

StringView readStringContents(const String &prefix, const StringView &origStr, CssStringId &value) {
	StringView str = parser::resolveCssString(origStr);
	value = hash::hash32(str.data(), str.size());
	return str;
}

bool readStringValue(const String &prefix, const StringView &origStr, CssStringId &value) {
	auto str = readStringContents(prefix, origStr, value);
	return !str.empty();
}

bool splitValue(const StringView &istr, StringView &first, StringView &second) {
	StringView str(istr);
	str.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();

	auto f = str.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
	str.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
	auto s = str.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();

	if (!f.empty() && !s.empty()) {
		first = f;
		second = s;
		return true;
	}

	return false;
}

bool readAspectRatioValue(const StringView &istr, float &value) {
	StringView str(istr);
	float first, second;

	if (!str.readFloat().grab(first)) {
		return false;
	}

	if (str.empty()) {
		value = first;
		return true;
	}

	str.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
	if (str.is('/')) {
		++ str;
		str.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();

		if (!str.readFloat().grab(second)) {
			return false;
		} else {
			value = first / second;
			return true;
		}
	}

	return false;
}

bool readStyleMargin(const StringView &origStr, Metric &top, Metric &right, Metric &bottom, Metric &left) {
	StringView str(origStr);
	Metric values[4];
	int count = 0;

	while (!str.empty()) {
		if (str.empty()) {
			return false;
		}
		if (!_readStyleValue(str, values[count])) {
			return false;
		}
		str.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		count ++;
		if (count == 4) {
			break;
		}
	}

	if (count == 0) {
		return false;
	} else if (count == 1) {
		top = right = bottom = left = values[0];
	} else if (count == 2) {
		top = bottom = values[0];
		right = left = values[1];
	} else if (count == 3) {
		top = values[0];
		right = left = values[1];
		bottom = values[2];
	} else {
		top = values[0];
		right = values[1];
		bottom = values[2];
		left = values[3];
	}

	return true;
}

float Metric::computeValueStrong(float base, const MediaParameters &media, float fontSize) const {
	switch (metric) {
	case Units::Auto: return nan(); break;
	case Units::Px: return value; break;
	case Units::Em: return (!isnan(fontSize)) ? fontSize * value : media.getDefaultFontSize() * value; break;
	case Units::Rem: return media.getDefaultFontSize() * value; break;
	case Units::Percent: return (!std::isnan(base)?(base * value):nan()); break;
	case Units::Cover: return base; break;
	case Units::Contain: return base; break;
	case Units::Vw: return value * media.surfaceSize.width * 0.01; break;
	case Units::Vh: return value * media.surfaceSize.height * 0.01; break;
	case Units::VMin: return value * std::min(media.surfaceSize.width, media.surfaceSize.height) * 0.01; break;
	case Units::VMax: return value * std::max(media.surfaceSize.width, media.surfaceSize.height) * 0.01; break;
	default: return 0.0f; break;
	}
	return 0.0f;
}

float Metric::computeValueAuto(float base, const MediaParameters &media, float fontSize) const {
	switch (metric) {
	case Units::Auto: return 0.0f; break;
	default: return computeValueStrong(base, media, fontSize); break;
	}
	return 0.0f;
}

ParameterValue::ParameterValue() {
	memset((void *)this, 0, sizeof(ParameterValue));
}

Parameter::Parameter(ParameterName name, MediaQueryId query)
: name(name), mediaQuery(query) { }


Parameter::Parameter(const Parameter &p, MediaQueryId query)
: name(p.name), mediaQuery(query), value(p.value) { }

bool ParameterList::isInheritable(ParameterName name) {
	if (name == ParameterName::MarginTop
			|| name == ParameterName::MarginRight
			|| name == ParameterName::MarginBottom
			|| name == ParameterName::MarginLeft
			|| name == ParameterName::PaddingTop
			|| name == ParameterName::PaddingRight
			|| name == ParameterName::PaddingBottom
			|| name == ParameterName::PaddingLeft
			|| name == ParameterName::Width
			|| name == ParameterName::Height
			|| name == ParameterName::MinWidth
			|| name == ParameterName::MinHeight
			|| name == ParameterName::MaxWidth
			|| name == ParameterName::MaxHeight
			|| name == ParameterName::Display
			|| name == ParameterName::Float
			|| name == ParameterName::Clear
			|| name == ParameterName::BackgroundColor
			|| name == ParameterName::OutlineStyle
			|| name == ParameterName::BorderTopStyle
			|| name == ParameterName::BorderRightStyle
			|| name == ParameterName::BorderBottomStyle
			|| name == ParameterName::BorderLeftStyle
			|| name == ParameterName::BorderTopWidth
			|| name == ParameterName::BorderRightWidth
			|| name == ParameterName::BorderBottomWidth
			|| name == ParameterName::BorderLeftWidth
			|| name == ParameterName::BorderTopColor
			|| name == ParameterName::BorderRightColor
			|| name == ParameterName::BorderBottomColor
			|| name == ParameterName::BorderLeftColor
			|| name == ParameterName::PageBreakBefore
			|| name == ParameterName::PageBreakAfter
			|| name == ParameterName::PageBreakInside) {
		return false;
	}
	return true;
}

bool ParameterList::isAllowedForMediaQuery(ParameterName name) {
	/*if (name == ParameterName::FontFamily
			|| name == ParameterName::FontSize
			|| name == ParameterName::FontStyle
			|| name == ParameterName::FontWeight) {
		return false;
	}*/
	return true;
}

}

NS_LAYOUT_END
