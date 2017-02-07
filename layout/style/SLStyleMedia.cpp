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

#include "SLParser.h"
#include "SPLayout.h"
#include "SLStyleParameters.hpp"
#include "SPString.h"

//#define SP_RTMEDIA_LOG(...) logTag("RTMedia", __VA_ARGS__)
#define SP_RTMEDIA_LOG(...)

NS_LAYOUT_BEGIN

namespace style {

template<> void Parameter::set<ParameterName::MediaType, MediaType>(const MediaType &v) { value.mediaType = v; }
template<> void Parameter::set<ParameterName::Orientation, Orientation>(const Orientation &v) { value.orientation = v; }
template<> void Parameter::set<ParameterName::Hover, Hover>(const Hover &v) { value.hover = v; }
template<> void Parameter::set<ParameterName::Pointer, Pointer>(const Pointer &v) { value.pointer = v; }
template<> void Parameter::set<ParameterName::LightLevel, LightLevel>(const LightLevel &v) { value.lightLevel = v; }
template<> void Parameter::set<ParameterName::Scripting, Scripting>(const Scripting &v) { value.scripting = v; }
template<> void Parameter::set<ParameterName::AspectRatio, float>(const float &v) { value.floatValue = v; }
template<> void Parameter::set<ParameterName::MinAspectRatio, float>(const float &v) { value.floatValue = v; }
template<> void Parameter::set<ParameterName::MaxAspectRatio, float>(const float &v) { value.floatValue = v; }
template<> void Parameter::set<ParameterName::Resolution, Metric>(const Metric &v) { value.sizeValue = v; }
template<> void Parameter::set<ParameterName::MinResolution, Metric>(const Metric &v) { value.sizeValue = v; }
template<> void Parameter::set<ParameterName::MaxResolution, Metric>(const Metric &v) { value.sizeValue = v; }
template<> void Parameter::set<ParameterName::MediaOption, CssStringId>(const CssStringId &v) { value.stringId = v; }

bool readMediaParameter(Vector<Parameter> &params, const String &name, const CharReaderBase &value, const CssStringFunction &cb) {
	SP_RTMEDIA_LOG("media: %s -> %s", name.c_str(), value.data());
	if (name == "width") {
		Metric size;
		if (readStyleValue(value, size)) {
			params.push_back(Parameter::create<ParameterName::Width>(size));
		}
	} else if (name == "height") {
		Metric size;
		if (readStyleValue(value, size)) {
			params.push_back(Parameter::create<ParameterName::Height>(size));
		}
	} else if (name == "min-width") {
		Metric size;
		if (readStyleValue(value, size)) {
			params.push_back(Parameter::create<ParameterName::MinWidth>(size));
		}
	} else if (name == "min-height") {
		Metric size;
		if (readStyleValue(value, size)) {
			params.push_back(Parameter::create<ParameterName::MinHeight>(size));
		}
	} else if (name == "max-width") {
		Metric size;
		if (readStyleValue(value, size)) {
			params.push_back(Parameter::create<ParameterName::MaxWidth>(size));
		}
	} else if (name == "max-height") {
		Metric size;
		if (readStyleValue(value, size)) {
			params.push_back(Parameter::create<ParameterName::MaxHeight>(size));
		}
	} else if (name == "orientation") {
		if (value.compare("landscape")) {
			params.push_back(Parameter::create<ParameterName::Orientation>(Orientation::Landscape));
		} else if (value.compare("portrait")) {
			params.push_back(Parameter::create<ParameterName::Orientation>(Orientation::Portrait));
		}
	} else if (name == "pointer") {
		if (value.compare("none")) {
			params.push_back(Parameter::create<ParameterName::Pointer>(Pointer::None));
		} else if (value.compare("fine")) {
			params.push_back(Parameter::create<ParameterName::Pointer>(Pointer::Fine));
		} else if (value.compare("coarse")) {
			params.push_back(Parameter::create<ParameterName::Pointer>(Pointer::Coarse));
		}
	} else if (name == "hover") {
		if (value.compare("none")) {
			params.push_back(Parameter::create<ParameterName::Hover>(Hover::None));
		} else if (value.compare("hover")) {
			params.push_back(Parameter::create<ParameterName::Hover>(Hover::Hover));
		} else if (value.compare("on-demand")) {
			params.push_back(Parameter::create<ParameterName::Hover>(Hover::OnDemand));
		}
	} else if (name == "light-level") {
		if (value.compare("dim")) {
			params.push_back(Parameter::create<ParameterName::LightLevel>(LightLevel::Dim));
		} else if (value.compare("normal")) {
			params.push_back(Parameter::create<ParameterName::LightLevel>(LightLevel::Normal));
		} else if (value.compare("washed")) {
			params.push_back(Parameter::create<ParameterName::LightLevel>(LightLevel::Washed));
		}
	} else if (name == "scripting") {
		if (value.compare("none")) {
			params.push_back(Parameter::create<ParameterName::Scripting>(Scripting::None));
		} else if (value.compare("initial-only")) {
			params.push_back(Parameter::create<ParameterName::Scripting>(Scripting::InitialOnly));
		} else if (value.compare("enabled")) {
			params.push_back(Parameter::create<ParameterName::Scripting>(Scripting::Enabled));
		}
	} else if (name == "aspect-ratio") {
		float ratio = 0.0f;
		if (readAspectRatioValue(value, ratio)) {
			params.push_back(Parameter::create<ParameterName::AspectRatio>(ratio));
		}
	} else if (name == "min-aspect-ratio") {
		float ratio = 0.0f;
		if (readAspectRatioValue(value, ratio)) {
			params.push_back(Parameter::create<ParameterName::MinAspectRatio>(ratio));
		}
	} else if (name == "max-aspect-ratio") {
		float ratio = 0.0f;
		if (readAspectRatioValue(value, ratio)) {
			params.push_back(Parameter::create<ParameterName::MaxAspectRatio>(ratio));
		}
	} else if (name == "resolution") {
		Metric size;
		if (readStyleValue(value, size, true)) {
			params.push_back(Parameter::create<ParameterName::Resolution>(size));
		}
	} else if (name == "min-resolution") {
		Metric size;
		if (readStyleValue(value, size, true)) {
			params.push_back(Parameter::create<ParameterName::MinResolution>(size));
		}
	} else if (name == "max-resolution") {
		Metric size;
		if (readStyleValue(value, size, true)) {
			params.push_back(Parameter::create<ParameterName::MaxResolution>(size));
		}
	} else if (name == "x-option") {
		CssStringId stringId;
		auto str = readStringContents("", value, stringId);
		if (!str.empty()) {
			cb(stringId, str);
			params.push_back(Parameter::create<ParameterName::MediaOption>(stringId));
		}
	}
	return true;
}

Vector<MediaQuery::Query> MediaQuery_readList(StringReader &s, const CssStringFunction &cb) {
	Vector<MediaQuery::Query> query;
	while (!s.empty()) {
		MediaQuery::Query q = parser::readMediaQuery(s, cb);
		if (!q.params.empty()) {
			query.push_back(std::move(q));
		}

		s.skipChars<Group<CharGroupId::WhiteSpace>>();
		if (!s.is(',')) {
			break;
		}
	}
	return query;
}

bool MediaQuery::parse(const String &str, const CssStringFunction &cb) {
	StringReader s(str);

	list = MediaQuery_readList(s, cb);
	return !list.empty();
}

bool MediaQuery::Query::setMediaType(const String &id) {
	if (id == "all") {
		params.push_back(style::Parameter::create<style::ParameterName::MediaType>(style::MediaType::All));
	} else if (id == "screen") {
		params.push_back(style::Parameter::create<style::ParameterName::MediaType>(style::MediaType::Screen));
	} else if (id == "print") {
		params.push_back(style::Parameter::create<style::ParameterName::MediaType>(style::MediaType::Print));
	} else if (id == "speech") {
		params.push_back(style::Parameter::create<style::ParameterName::MediaType>(style::MediaType::Speech));
	} else {
		return false; // invalid mediatype
	}
	return true;
}

void MediaQuery::clear() {
	list.clear();
}

MediaQueryId MediaQuery::IsScreenLayout = 0;
MediaQueryId MediaQuery::IsPrintLayout = 1;
MediaQueryId MediaQuery::NoTooltipOption = 2;
MediaQueryId MediaQuery::IsTooltipOption = 3;

Vector<MediaQuery> MediaQuery::getDefaultQueries(Map<CssStringId, String> & strings) {
	Vector<MediaQuery> ret;

	auto mediaCssStringFn = [&strings] (CssStringId strId, const CharReaderBase &string) {
		strings.insert(pair(strId, string.str()));
	};

	ret.emplace_back(); ret.back().parse("screen", mediaCssStringFn);
	ret.emplace_back(); ret.back().parse("print", mediaCssStringFn);
	ret.emplace_back(); ret.back().parse("not all and (x-option:tooltip)", mediaCssStringFn);
	ret.emplace_back(); ret.back().parse("all and (x-option:tooltip)", mediaCssStringFn);

	return ret;
}

}

NS_LAYOUT_END
