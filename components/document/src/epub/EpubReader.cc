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
#include "EpubDocument.h"
#include "EpubReader.h"
#include "SLStyle.h"

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
		layout::Reader::onPushTag(t);
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
			layout::Reader::onPopTag(t);
		}
	}
}
void Reader::onInlineTag(Tag &t) {
	if (isCaseAllowed()) {
		layout::Reader::onInlineTag(t);
	}
}
void Reader::onTagContent(Tag &t, StringReader &r) {
	if (isCaseAllowed()) {
		if (t.name != "epub:switch" && t.name != "epub:case" && t.name != "epub:default") {
			layout::Reader::onTagContent(t, r);
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

NS_EPUB_END
