/*
 * SPEpubReader.cpp
 *
 *  Created on: 15 авг. 2016 г.
 *      Author: sbkarr
 */

#include "SPDefine.h"
#include "SPEpubReader.h"
#include "SPEpubDocument.h"
#include "SPRichTextStyleParameters.hpp"

NS_EPUB_BEGIN

void Reader::onPushTag(Tag &t) {
	if (!isCaseAllowed()) {
		if (t.name == "epub:case") {
			if (!_switchStatus.empty() && !_switchStatus.back().parsed) {
				auto nsIt = t.attributes.find("required-namespace");
				if (nsIt != t.attributes.end()) {
					if (isNamespaceImplemented(nsIt->second)) {
						auto &b = _switchStatus.back();
						b.parsed = true;
						b.active = true;
					}
				}
			}
		} else if (t.name == "epub:default") {
			if (!_switchStatus.empty() && !_switchStatus.back().parsed) {
				auto &b = _switchStatus.back();
				b.parsed = true;
				b.active = true;
			}
		}
	} else if (t.name != "epub:switch" && t.name != "epub:case" && t.name != "epub:default") {
		rich_text::Reader::onPushTag(t);
	} else if (t.name == "epub:switch") {
		_switchStatus.push_back(SwitchData{false, false});
	}
}
void Reader::onPopTag(Tag &t) {
	if (t.name == "epub:switch") {
		if (!_switchStatus.empty()) {
			_switchStatus.pop_back();
		}
	} else if (isCaseAllowed()) {
		if (t.name == "epub:case" || t.name == "epub:default") {
			if (!_switchStatus.empty() && _switchStatus.back().active) {
				_switchStatus.back().active = false;
			}
		} else {
			rich_text::Reader::onPopTag(t);
		}
	}
}
void Reader::onInlineTag(Tag &t) {
	if (isCaseAllowed()) {
		rich_text::Reader::onInlineTag(t);
	}
}
void Reader::onTagContent(Tag &t, StringReader &r) {
	if (isCaseAllowed()) {
		if (t.name != "epub:switch" && t.name != "epub:case" && t.name != "epub:default") {
			rich_text::Reader::onTagContent(t, r);
		}
	}
}

bool Reader::isCaseAllowed() const {
	if (_switchStatus.empty() || _switchStatus.back().active) {
		return true;
	}
	return false;
}

bool Reader::isNamespaceImplemented(const String &) const {
	return false;
}

bool Reader::isStyleAttribute(const String &tagName, const String &name) const {
	if (name == "epub:type") {
		return true;
	} else {
		return rich_text::Reader::isStyleAttribute(tagName, name);
	}
}

void Reader::addStyleAttribute(rich_text::style::Tag &tag, const String &name, const String &value) {
	using namespace rich_text::style;

	if (name == "epub:type") {
		if (value == "footnote" || value == "endnote" || value == "footnotes" || value == "endnotes") {
			tag.compiledStyle.set(Parameter::create<ParameterName::Display>(Display::None, MediaQuery::NoTooltipOption));
			tag.compiledStyle.set(Parameter::create<ParameterName::Display>(Display::Default, MediaQuery::IsTooltipOption));
		}
	} else {
		rich_text::Reader::addStyleAttribute(tag, name, value);
	}
}

NS_EPUB_END
