/*
 * SPRichTextStyleValues.cpp
 *
 *  Created on: 28 июля 2015 г.
 *      Author: sbkarr
 */

#include "SPDefine.h"
#include "SPRichTextStyle.h"
#include "SPRichTextParser.h"
#include "SPString.h"

NS_SP_EXT_BEGIN(rich_text)

namespace style {

bool readStyleValue(const String &str, Size &value, bool resolutionMetric, bool allowEmptyMetric) {
	if (!resolutionMetric && str.compare(0, 4, "auto") == 0) {
		value.metric = Size::Metric::Auto;
		value.value = 0.0f;
		return true;
	}

	size_t idx = 0;
	float fvalue = stappler::stof(str, &idx);
	if (idx == 0) {
		return false;
	}

	if (fvalue == 0.0f) {
		value.value = fvalue;
		value.metric = Size::Metric::Px;
		return true;
	}

	while(isspace(str[idx]) && idx < str.length()) {
		idx++;
	}

	if (!resolutionMetric) {
		if (str.length() > idx && str[idx] == '%') {
			value.value = fvalue / 100.0f;
			value.metric = Size::Metric::Percent;
			return true;
		}

		if (str.length() > idx + 1 && str[idx] == 'e' && str[idx + 1] == 'm') {
			value.value = fvalue;
			value.metric = Size::Metric::Em;
			return true;
		}

		if (str.length() > idx + 1 && str[idx] == 'p' && str[idx + 1] == 'x') {
			value.value = fvalue;
			value.metric = Size::Metric::Px;
			return true;
		}

		if (str.length() > idx + 1 && str[idx] == 'p' && str[idx + 1] == 't') {
			value.value = fvalue * 4.0f / 3.0f;
			value.metric = Size::Metric::Px;
			return true;
		}
	} else {
		if (str.length() > idx + 2 && str[idx] == 'd' && str[idx + 1] == 'p' && str[idx + 2] == 'i') {
			value.value = fvalue;
			value.metric = Size::Metric::Dpi;
			return true;
		}

		if (str.length() > idx + 3 && str[idx] == 'd' && str[idx + 1] == 'p' && str[idx + 2] == 'c' && str[idx + 3] == 'm') {
			value.value = fvalue / 2.54f;
			value.metric = Size::Metric::Dpi;
			return true;
		}

		if (str.length() > idx + 3 && str[idx] == 'd' && str[idx + 1] == 'p' && str[idx + 2] == 'p' && str[idx + 3] == 'x') {
			value.value = fvalue;
			value.metric = Size::Metric::Dppx;
			return true;
		}
	}

	if (allowEmptyMetric) {
		value.value = fvalue;
		return true;
	}

	return false;
}

String readStringContents(const String &prefix, const String &origStr, CssStringId &value) {
	String str = parser::resolveCssString(origStr);
	value = string::hash32(str);
	return str;
}

bool readStringValue(const String &prefix, const String &origStr, CssStringId &value) {
	auto str = readStringContents(prefix, origStr, value);
	return !str.empty();
}

bool splitValue(const String &str, String &first, String &second) {
	size_t i = 0;
	size_t j = 0;
	while (i < str.length() && str[i] != ' ') {
		i++;
	}

	if (i < str.length()) {
		j = i;
		while (j < str.length() && str[j] == ' ') {
			j++;
		}
	}

	if (i < str.length() && j < str.length()) {
		first = str.substr(0, i); second = str.substr(j);
		return true;
	}

	return false;
}

bool readAspectRatioValue(const String &str, float &value) {
	size_t idx = 0;
	float first, second;

	first = stappler::stof(str, &idx);
	if (idx == 0) {
		return false;
	} else if (idx == str.length()) {
		value = first;
		return true;
	}

	if (str[idx] == '/') {
		auto substr = str.substr(idx+1);
		second = stappler::stof(substr, &idx);
		if (idx == 0) {
			return false;
		} else {
			value = first / second;
			return true;
		}
	}

	return false;
}

bool readStyleMargin(const String &origStr, Size &top, Size &right, Size &bottom, Size &left) {
	size_t len = origStr.length();
	size_t pos = 0;
	size_t field = 0;

	Size values[4];
	int count = 0;

	while (len > pos + field) {
		while(!isspace(origStr[pos + field] & 0xFF) && len > pos + field) {
			field ++;
		}

		if (len >= pos + field) {
			auto substr = origStr.substr(pos, field);
			if (substr == "auto") {
				values[count].value = 0.0f;
				values[count].metric = Size::Metric::Auto;
			} else if (!readStyleValue(substr, values[count])) {
				return false;
			}
			count ++;

			if (count == 4) {
				break;
			}
		}

		while(isspace(origStr[pos + field] & 0xFF) && len > pos + field) {
			field ++;
		}

		pos = pos + field;
		field = 0;
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

float Size::computeValue(float base, float fontSize, bool autoIsZero) const {
	switch (metric) {
	case Metric::Auto: return (autoIsZero)?0:nan(); break;
	case Metric::Px: return value; break;
	case Metric::Em: return fontSize * value; break;
	case Metric::Percent: return (!isnanf(base)?(base * value):nan()); break;
	case Metric::Cover: return base; break;
	case Metric::Contain: return base; break;
	default: return 0.0f; break;
	}
	return 0.0f;
}


ParameterValue::ParameterValue() {
	memset(this, 0, sizeof(ParameterValue));
}

Parameter::Parameter(ParameterName name, MediaQueryId query)
: name(name), mediaQuery(query) { }

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

NS_SP_EXT_END(rich_text)
