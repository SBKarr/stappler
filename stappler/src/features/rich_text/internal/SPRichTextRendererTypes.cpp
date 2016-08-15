/*
 * SPRichTextRendererTypes.cpp
 *
 *  Created on: 02 мая 2015 г.
 *      Author: sbkarr
 */

#include "SPDefine.h"
#include "SPRichTextRendererTypes.h"
#include "SPString.h"
#include "SPRichTextDocument.h"

NS_SP_EXT_BEGIN(rich_text)

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
					paramSuccess = (param.value.sizeValue.metric == style::Size::Metric::Px
							&& surfaceSize.width == param.value.sizeValue.value)
							|| (param.value.sizeValue.metric == style::Size::Metric::Percent
									&& param.value.sizeValue.value == 1.0f);
					break;
				case style::ParameterName::Height:
					paramSuccess = (param.value.sizeValue.metric == style::Size::Metric::Px
							&& surfaceSize.height == param.value.sizeValue.value)
							|| (param.value.sizeValue.metric == style::Size::Metric::Percent
									&& param.value.sizeValue.value == 1.0f);
					break;
				case style::ParameterName::MinWidth:
					paramSuccess = (param.value.sizeValue.metric == style::Size::Metric::Px
							&& surfaceSize.width >= param.value.sizeValue.value)
							|| (param.value.sizeValue.metric == style::Size::Metric::Percent
									&& param.value.sizeValue.value >= 1.0f);
					break;
				case style::ParameterName::MinHeight:
					paramSuccess = (param.value.sizeValue.metric == style::Size::Metric::Px
							&& surfaceSize.height >= param.value.sizeValue.value)
							|| (param.value.sizeValue.metric == style::Size::Metric::Percent
									&& param.value.sizeValue.value >= 1.0f);
					break;
				case style::ParameterName::MaxWidth:
					paramSuccess = (param.value.sizeValue.metric == style::Size::Metric::Px
							&& surfaceSize.width <= param.value.sizeValue.value)
							|| (param.value.sizeValue.metric == style::Size::Metric::Percent
									&& param.value.sizeValue.value <= 1.0f);
					break;
				case style::ParameterName::MaxHeight:
					paramSuccess = (param.value.sizeValue.metric == style::Size::Metric::Px
							&& surfaceSize.height <= param.value.sizeValue.value)
							|| (param.value.sizeValue.metric == style::Size::Metric::Percent
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
					paramSuccess = (param.value.sizeValue.metric == style::Size::Metric::Dpi && dpi == param.value.sizeValue.value)
						|| (param.value.sizeValue.metric == style::Size::Metric::Dppx && density == param.value.sizeValue.value);
					break;
				case style::ParameterName::MinResolution:
					paramSuccess = (param.value.sizeValue.metric == style::Size::Metric::Dpi && dpi >= param.value.sizeValue.value)
						|| (param.value.sizeValue.metric == style::Size::Metric::Dppx && density >= param.value.sizeValue.value);
					break;
				case style::ParameterName::MaxResolution:
					paramSuccess = (param.value.sizeValue.metric == style::Size::Metric::Dpi && dpi <= param.value.sizeValue.value)
						|| (param.value.sizeValue.metric == style::Size::Metric::Dppx && density <= param.value.sizeValue.value);
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

MediaResolver::MediaResolver()
: _document(nullptr) { }

MediaResolver::MediaResolver(Document *doc, const MediaParameters &params, const Vector<String> &opts)
: _document(doc) {
	if (opts.empty()) {
		_media = params.resolveMediaQueries(_document->getMediaQueries());
	} else {
		auto p = params;
		for (auto &it : opts) {
			p.addOption(it);
		}

		_media = p.resolveMediaQueries(_document->getMediaQueries());
	}
}

bool MediaResolver::resolveMediaQuery(MediaQueryId queryId) const {
	if (queryId < _media.size()) {
		return _media[queryId];
	}
	return false;
}

String MediaResolver::getCssString(CssStringId id) const {
	if (!_document) {
		return "";
	}

	auto &map = _document->getCssStrings();
	auto it = map.find(id);
	if (it != map.end()) {
		return it->second;
	}
	return "";
}

NS_SP_EXT_END(rich_text)
