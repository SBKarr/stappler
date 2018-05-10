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

#include "SPLayout.h"
#include "SLDocument.h"
#include "SLRendererTypes.h"
#include "SPString.h"

NS_LAYOUT_BEGIN

float MediaParameters::getDefaultFontSize() const {
	return style::FontSize::Medium * fontScale;
}

void MediaParameters::addOption(const String &str) {
	auto value = string::hash32(str);
	_options.insert(pair(value, str));
}
void MediaParameters::removeOption(const String &str) {
	auto value = string::hash32(str);
	auto it = _options.find(value);
	if (it != _options.end()) {
		_options.erase(it);
	}
}

bool MediaParameters::hasOption(const String &str) const {
	return hasOption(string::hash32(str));
}
bool MediaParameters::hasOption(CssStringId value) const {
	auto it = _options.find(value);
	if (it != _options.end()) {
		return true;
	} else {
		return false;
	}
}

Vector<bool> MediaParameters::resolveMediaQueries(const Vector<style::MediaQuery> &vec) const {
	Vector<bool> ret;
	for (auto &it : vec) {
		bool success = false;
		for (auto &query : it.list) {
			bool querySuccess = true;
			for (auto &param : query.params) {
				bool paramSuccess = false;
				switch(param.name) {
				case style::ParameterName::MediaType:
					if (param.value.mediaType == style::MediaType::All
							|| param.value.mediaType == mediaType) {
						paramSuccess = true;
					}
					break;
				case style::ParameterName::Orientation: paramSuccess = param.value.orientation == orientation; break;
				case style::ParameterName::Pointer: paramSuccess = param.value.pointer == pointer; break;
				case style::ParameterName::Hover: paramSuccess = param.value.hover == hover; break;
				case style::ParameterName::LightLevel: paramSuccess = param.value.lightLevel == lightLevel; break;
				case style::ParameterName::Scripting: paramSuccess = param.value.scripting == scripting; break;
				case style::ParameterName::Width:
					paramSuccess = (param.value.sizeValue.metric == style::Metric::Units::Px
							&& surfaceSize.width == param.value.sizeValue.value)
							|| (param.value.sizeValue.metric == style::Metric::Units::Percent
									&& param.value.sizeValue.value == 1.0f);
					break;
				case style::ParameterName::Height:
					paramSuccess = (param.value.sizeValue.metric == style::Metric::Units::Px
							&& surfaceSize.height == param.value.sizeValue.value)
							|| (param.value.sizeValue.metric == style::Metric::Units::Percent
									&& param.value.sizeValue.value == 1.0f);
					break;
				case style::ParameterName::MinWidth:
					paramSuccess = (param.value.sizeValue.metric == style::Metric::Units::Px
							&& surfaceSize.width >= param.value.sizeValue.value)
							|| (param.value.sizeValue.metric == style::Metric::Units::Percent
									&& param.value.sizeValue.value >= 1.0f);
					break;
				case style::ParameterName::MinHeight:
					paramSuccess = (param.value.sizeValue.metric == style::Metric::Units::Px
							&& surfaceSize.height >= param.value.sizeValue.value)
							|| (param.value.sizeValue.metric == style::Metric::Units::Percent
									&& param.value.sizeValue.value >= 1.0f);
					break;
				case style::ParameterName::MaxWidth:
					paramSuccess = (param.value.sizeValue.metric == style::Metric::Units::Px
							&& surfaceSize.width <= param.value.sizeValue.value)
							|| (param.value.sizeValue.metric == style::Metric::Units::Percent
									&& param.value.sizeValue.value <= 1.0f);
					break;
				case style::ParameterName::MaxHeight:
					paramSuccess = (param.value.sizeValue.metric == style::Metric::Units::Px
							&& surfaceSize.height <= param.value.sizeValue.value)
							|| (param.value.sizeValue.metric == style::Metric::Units::Percent
									&& param.value.sizeValue.value <= 1.0f);
					break;
				case style::ParameterName::AspectRatio:
					paramSuccess = (surfaceSize.width / surfaceSize.height) == param.value.floatValue;
					break;
				case style::ParameterName::MinAspectRatio:
					paramSuccess = (surfaceSize.width / surfaceSize.height) >= param.value.floatValue;
					break;
				case style::ParameterName::MaxAspectRatio:
					paramSuccess = (surfaceSize.width / surfaceSize.height) <= param.value.floatValue;
					break;
				case style::ParameterName::Resolution:
					paramSuccess = (param.value.sizeValue.metric == style::Metric::Units::Dpi && dpi == param.value.sizeValue.value)
						|| (param.value.sizeValue.metric == style::Metric::Units::Dppx && density == param.value.sizeValue.value);
					break;
				case style::ParameterName::MinResolution:
					paramSuccess = (param.value.sizeValue.metric == style::Metric::Units::Dpi && dpi >= param.value.sizeValue.value)
						|| (param.value.sizeValue.metric == style::Metric::Units::Dppx && density >= param.value.sizeValue.value);
					break;
				case style::ParameterName::MaxResolution:
					paramSuccess = (param.value.sizeValue.metric == style::Metric::Units::Dpi && dpi <= param.value.sizeValue.value)
						|| (param.value.sizeValue.metric == style::Metric::Units::Dppx && density <= param.value.sizeValue.value);
					break;
				case style::ParameterName::MediaOption:
					paramSuccess = hasOption(param.value.stringId);
					break;
				default: break;
				}
				if (!paramSuccess) {
					querySuccess = false;
					break;
				}
			}
			if (query.negative) {
				querySuccess = !querySuccess;
			}

			if (querySuccess) {
				success = true;
				break;
			}
		}
		ret.push_back(success);
	}
	return ret;
}

bool MediaParameters::shouldRenderImages() const {
	return (flags & (RenderFlag::Mask)RenderFlag::NoImages) == 0;
}

NS_LAYOUT_END
