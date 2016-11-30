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
#include "SPRichTextReader.h"
#include "SPRichTextParser.h"
#include "SPString.h"
#include "SPFilesystem.h"

//#define SP_RTREADER_LOG(...) log::format("RTReader", __VA_ARGS__)
#define SP_RTREADER_LOG(...)

NS_SP_EXT_BEGIN(rich_text)

static String getRootXType(const Vector<style::Tag> &stack) {
	for (auto it = stack.rbegin(); it != stack.rend(); it ++) {
		if (!it->xType.empty()) {
			return it->xType;
		}
	}
	return String();
}

Reader::Style Reader::specializeStyle(const Tag &tag, const Style &parentStyle) {
	Style style;
	style.merge(parentStyle, true);
	style.merge(style::getStyleForTag(tag.name, tag.type));
	auto it = _namedStyles.find("*");
	if (it != _namedStyles.end()) {
		style.merge(it->second, true);
	}
	if (!tag.weakStyle.empty()) {
		style.merge(tag.weakStyle);
	}
	style.merge(tag.compiledStyle, false);
	it = _namedStyles.find(tag.name);
	if (it != _namedStyles.end()) {
		style.merge(it->second);
	}
	if (!tag.classes.empty()) {
		for (auto &cl : tag.classes) {
			SP_RTREADER_LOG("search class: %s", cl.c_str());
			it = _namedStyles.find(String(".") + cl);
			if (it != _namedStyles.end()) {
				style.merge(it->second);
			}
			it = _namedStyles.find(tag.name + "." + cl);
			if (it != _namedStyles.end()) {
				style.merge(it->second);
			}
		}
	}
	if (!tag.id.empty()) {
		it = _namedStyles.find(String("#") + tag.id);
		if (it != _namedStyles.end()) {
			style.merge(it->second);
		}
		it = _namedStyles.find(tag.name + "#" + tag.id);
		if (it != _namedStyles.end()) {
			style.merge(it->second);
		}
	}
	if (!tag.style.empty()) {
		style.merge(tag.style);
	}

	SP_RTREADER_LOG("style for '%s' => \n%s", tag.name.c_str(), style.css().c_str());

	return style;
}

static String encodePathString(const String &str) {
	String ret; ret.reserve(str.size());
	for (auto &c : str) {
		auto b = tolower(c);
		if ((b >= 'a' && b <= 'z') || (b >= '0' && b <= '9') || b == '_') {
			ret.push_back(b);
		} else if (b == '-' || b == '/' || b == '\\') {
			ret.push_back('_');
		}
	}
	return ret;
}

Reader::Tag Reader::onTag(StringReader &tag) {
	Tag ret;
	ret.name = parser::readHtmlTagName(tag);
	if (ret.name.empty()) {
		tag.skipUntil<Chars<'>'>>();
		if (tag.is('>')) {
			tag ++;
		}
		return ret;
	}

	if (ret.name.compare(0, 3, "!--") == 0) { // process comment
		tag.skipUntilString("-->", false);
		ret.name.clear();
		return ret;
	}
	ret.type = style::Tag::getType(ret.name);

	String paramName;
	String paramValue;
	while (ret.type != Tag::Special && !tag.empty() && !tag.is('>') && !tag.is('/')) {
		paramName.clear();
		paramValue.clear();

		paramName = parser::readHtmlTagParamName(tag);
		if (paramName.empty()) {
			continue;
		}

		if (paramName == "id") {
			ret.id = parser::readHtmlTagParamValue(tag);
		} else if (paramName == "x-type") {
			ret.xType = parser::readHtmlTagParamValue(tag);
			ret.attributes.emplace("x-type", ret.xType);
		} else if (paramName == "style") {
			parser::readHtmlStyleValue(tag, [&] (const String &name, const String &value) {
				SP_RTREADER_LOG("%s style: '%s' : '%s'", ret.name.c_str(), name.c_str(), value.c_str());
				if (name == "font-family" || name == "background-image") {
					addCssString(value);
				}
				ret.style.push_back(pair(name, value));
			});
		} else if (paramName == "href") {
			auto ref = parser::readHtmlTagParamValue(tag);
			if (!ref.empty() && ref.front() != '#' && ref.find("://") == String::npos && !_path.empty()) {
				ret.attributes.emplace(paramName, filepath::reconstructPath(filepath::merge(filepath::root(_path), ref)));
			} else if (!ref.empty()) {
				ret.attributes.emplace(paramName, std::move(ref));
			}
		} else if (paramName == "class") {
			paramValue = parser::readHtmlTagParamValue(tag);
			string::split(string::tolower(paramValue), " ", [&ret] (const CharReaderBase &r) {
				 ret.classes.emplace_back(r.str());
			});
		} else if (paramName == "x-refs" && ret.type == Tag::Block) {
			paramValue = parser::readHtmlTagParamValue(tag);
			ret.autoRefs = (paramValue == "auto");
		} else {
			if (tag.is('=')) {
				if (isStyleAttribute(ret.name, paramName)) {
					addStyleAttribute(ret, paramName, parser::readHtmlTagParamValue(tag));
				} else {
					String val = parser::readHtmlTagParamValue(tag);
					SP_RTREADER_LOG("'%s' attr: '%s' : '%s'", ret.name.c_str(), paramName.c_str(), val.c_str());
					ret.attributes.emplace(paramName, std::move(val));
				}
			} else {
				SP_RTREADER_LOG("'%s' attr: '%s'", ret.name.c_str(), paramName.c_str());
				ret.attributes.emplace(paramName, "");
			}
		}
	}

	if (ret.type == Tag::Special) {
		if (ret.name == "meta") {
			String name;
			String content;
			while (!tag.empty() && !tag.is('>') && !tag.is('/')) {
				paramName.clear();
				paramValue.clear();

				paramName = parser::readHtmlTagParamName(tag);
				if (paramName.empty()) {
					continue;
				}

				if (paramName == "name") {
					name = parser::readHtmlTagParamValue(tag);
				} else if (paramName == "content") {
					content = parser::readHtmlTagParamValue(tag);
				}
			}
			if (!name.empty() && !content.empty()) {
				onMeta(std::move(name), std::move(content));
			}
		} else if (ret.name == "link") {
			while (!tag.empty() && !tag.is('>') && !tag.is('/')) {
				paramName.clear();
				paramValue.clear();

				paramName = parser::readHtmlTagParamName(tag);
				if (paramName.empty()) {
					continue;
				}

				if (tag.is('=')) {
					ret.attributes.emplace(paramName, parser::readHtmlTagParamValue(tag));
				} else {
					ret.attributes.emplace(paramName, "");
				}
			}

			auto relIt = ret.attributes.find("rel");
			if (relIt != ret.attributes.end()) {
				if (relIt->second == "stylesheet") {
					auto hrefIt = ret.attributes.find("href");
					if (hrefIt != ret.attributes.end() && !hrefIt->second.empty()) {
						onCss(hrefIt->second);
					}
				}
			}
		}
	} else if ((ret.type == Tag::Image || ret.xType == "image") && ret.id.empty()) {
		ret.id = toString("__id__", _pseudoId, "__", encodePathString(_path));
		_pseudoId ++;
	}

	if (ret.xType == "image") {
		ret.attributes.emplace("href", toString("#", ret.id));
	}
	if (ret.name == "img") {
		auto it = ret.attributes.find("src");
		if (it != ret.attributes.end()) {
			if (!it->second.empty()) {
				auto & src = it->second;
				if (src.front() != '/' && src.find("://") == String::npos && !_path.empty()) {
					it->second = filepath::reconstructPath(filepath::merge(filepath::root(_path), it->second));
				}
			}

			if (ret.xType != "image") {
				auto & src = it->second;
				auto pos = src.rfind("?");
				if (pos != String::npos && pos != src.length() - 1) {
					auto opts = src.substr(pos + 1);
					StringReader s(opts);
					StringReader opt = s.readUntil<Chars<'&', ';'>>();
					while (!opt.empty()) {
						if (opt == "preview") {
							if (getRootXType(_tagStack) != "image") {
								auto target = toString("#", ret.id);
								ret.attributes.emplace("href", toString("#", ret.id));
							}
							break;
						} else {
							s.skipChars<Chars<'&', ';'>>();
							opt = s.readUntil<Chars<'&', ';'>>();
						}
					}
				}
			}
		}
	}

	if (tag.is('/') || ret.name == "br" || ret.name == "hr" || ret.type == Tag::Image
			|| ret.type == Tag::Special) {
		ret.closable = false;
	} else {
		ret.closable = true;
	}

	if (tag.is('/') || ret.type == Tag::Special) {
		tag.skipUntil<Chars<'>'>>();
	}

	if (tag.is('>')) {
		tag ++;
	}

	return ret;
}

void Reader::onPushTag(Tag &tag) {
	if (tag.name == "style") {
		onStyleTag(_current);
	} else if (tag.name == "html") {
		_htmlTag = true;
	} else if (!_htmlTag || !hasParentTag("head")) {
		_styleStack.push_back(specializeStyle(tag, (_styleStack.empty()?_nodeStack.back()->getStyle():_styleStack.back())));
		if (tag.type == Tag::Block) {
			auto &node = _nodeStack.back()->pushNode(tag.name, tag.id, _styleStack.back(), std::move(tag.attributes));
			SP_RTREADER_LOG("'%s'", tag.name.c_str());
			_nodeStack.push_back(&node);
		} else if (tag.type == Tag::Markup && tag.name == "body") {
			auto &body = _nodeStack.front();
			body->pushStyle(_styleStack.back());
		}
	}
}
void Reader::onPopTag(Tag &tag) {
	if (!_htmlTag || !hasParentTag("head")) {
		if (tag.type == Tag::Block) {
			_nodeStack.pop_back();
			SP_RTREADER_LOG("'/%s'", tag.name.c_str());
		}
		_styleStack.pop_back();
	}
}
void Reader::onInlineTag(Tag &tag) {
	if (!_htmlTag || !hasParentTag("head")) {
		SP_RTREADER_LOG("'inline' '%s'", tag.name.c_str());
		if (tag.name == "br") {
			_nodeStack.back()->pushLineBreak();
		} else if (tag.type != Tag::Special) {
			_nodeStack.back()->pushNode(tag.name, tag.id, specializeStyle(tag,
					(_styleStack.empty()?_nodeStack.back()->getStyle():_styleStack.back())), std::move(tag.attributes));
		}
	}
}

void Reader::onTagContent(Tag &tag, StringReader &s) {
	if (!_htmlTag || !hasParentTag("head")) {
		if (s.isSpace()) {
			if (!_nodeStack.empty()) {
				Node * b = _nodeStack.back();
				if (b->hasValue()) {
					SP_RTREADER_LOG("'content' '%s' : '\\ws'", tag.name.c_str());
					b->pushValue(" ");
				}
			}
			return;
		}
		//SP_RTREADER_LOG("'content' '%s' : '%s'", tag.name.c_str(), s.str().c_str());
		if (!shouldParseRefs()) {
			_nodeStack.back()->pushValue(s.str());
		} else {
			parser::RefParser(s.str(), [&] (const String &c, const String &ref) {
				Node * node = _nodeStack.back();
				if (!c.empty()) {
					node->pushValue(s.str());
				}
				if (!ref.empty()) {
					Style refStyle;
					refStyle.merge(node->getStyle(), true);
					refStyle.merge(style::getStyleForTag("a", Tag::Block));

					// push css specialized styles
					auto it = _namedStyles.find("a"); if (it != _namedStyles.end()) { refStyle.merge(it->second); }
					it = _namedStyles.find("a:auto"); if (it != _namedStyles.end()) { refStyle.merge(it->second); }
					it = _namedStyles.find("a.auto"); if (it != _namedStyles.end()) { refStyle.merge(it->second); }

					Map<String, String> attr;
					attr.emplace("href", ref);

					Node &hrefNode = node->pushNode("a", "", refStyle, std::move(attr));
					hrefNode.pushValue(ref);
				}
			});
		}
	}
}

void Reader::onStyleTag(StringReader &s) {
	parser::readStyleTag(*this, s, _fonts);
}

void Reader::onStyleObject(const String &selector, const style::StyleVec &vec, const MediaQueryId &query) {
	if ((selector == "*" || selector == "body") && !_nodeStack.empty()) {
		_nodeStack.front()->pushStyle(vec, query);
	}

	auto it = _namedStyles.find(selector);
	if (it == _namedStyles.end()) {
		it = _namedStyles.insert(pair(selector, style::ParameterList())).first;
	}
	it->second.read(vec, query);
	SP_RTREADER_LOG("Added NamedStyle '%s' : %s", selector.c_str(), it->second.css().c_str());
}

static void Reader_resolveFontPaths(const String &root, HtmlPage::FontMap &fonts) {
	for (auto &it : fonts) {
		for (auto &vit : it.second) {
			for (auto &fit : vit.src) {
				String &path = fit;
				if (path.front() != '/' && path.find(":") == String::npos) {
					path = String("document://") + filepath::reconstructPath(filepath::merge(filepath::root(root), path));
					SP_RTREADER_LOG("Resolve font path: '%s' '%s'", it.first.c_str(), path.c_str());
				}
			}
		}
	}
}

bool Reader::readHtml(HtmlPage &page, const CharReaderBase &str, CssStrings &strings, MediaQueries &queries, MetaPairs &meta, CssMap &cssMap) {
	SP_RTREADER_LOG("read: %s", page.path.c_str());
	Node &ret = page.root;

	_path = page.path;
	_cssStrings = &strings;
	_mediaQueries = &queries;
	_meta = &meta;
	_css = &cssMap;

	ret.pushStyle(style::getStyleForTag("body", style::Tag::Markup));
	_nodeStack.push_back(&ret);
	_origin = StringReader(str.data(), str.size());
	_current = StringReader(str.data(), str.size());

	while (true) {
		auto prefix = _current.readUntil<Chars<'<'>>();
		if (!prefix.empty()) {
			if (!_tagStack.empty()) {
				onTagContent(_tagStack.back(), prefix);
			} else {
				Tag t;
				onTagContent(t, prefix);
			}
		}

		if (!_current.is('<')) {
			break;
		}

		_current ++; // drop '<'
		if (_current.is('/')) {
			_current ++; // drop '/'

			auto tag = _current.readUntil<Chars<'>'>>().str();
			if (!tag.empty() && _current.is('>') && !_tagStack.empty()) {
				auto it = _tagStack.end();
				do {
					it --;
					if (tag == it->name) {
						// close all tag after <tag>
						auto nit = _tagStack.end();
						do {
							nit --;
							onPopTag(*nit);
							_tagStack.pop_back();
						} while (nit != it);
						break;
					}
				} while(it != _tagStack.begin());

				if (_htmlTag && _tagStack.empty()) {
					break;
				}
			} else if (_current.empty()) {
				break;
			}
			_current ++; // drop '>'
		} else {
			Tag t = onTag(_current);
			if (t) {
				if (t.closable) {
					onPushTag(t);
					_tagStack.push_back(std::move(t));
				} else {
					onInlineTag(t);
				}
			}

			if (_current.empty()) {
				break;
			}
		}
	}

	if (!_tagStack.empty()) {
		auto nit = _tagStack.end();
		do {
			nit --;
			onPopTag(*nit);
			_tagStack.pop_back();
		} while (nit != _tagStack.begin());
	}

	SP_RTREADER_LOG("root style: %s", ret.getStyle().css().c_str());

	if (!ret) {
		return false;
	}
	Reader_resolveFontPaths(_path, _fonts);
	page.fonts = std::move(_fonts);
	return true;
}

style::CssData Reader::readCss(const String &path, const CharReaderBase &str, CssStrings &strings, MediaQueries &queries) {
	_path = path;
	_cssStrings = &strings;
	_mediaQueries = &queries;

	_origin = StringReader(str.data(), str.size());
	_current = StringReader(str.data(), str.size());
	parser::readStyleTag(*this, _current, _fonts);
	Reader_resolveFontPaths(_path, _fonts);
	return style::CssData{std::move(_namedStyles), std::move(_fonts)};
}

MediaQueryId Reader::addMediaQuery(style::MediaQuery &&mediaQuery) {
	_mediaQueries->emplace_back(std::move(mediaQuery));
	return (MediaQueryId) _mediaQueries->size() - 1;
}
void Reader::addCssString(CssStringId id, const String &str) {
	_cssStrings->insert(pair(id, str));
}

void Reader::addCssString(const String &origStr) {
	auto str = parser::resolveCssString(origStr);
	SP_RTREADER_LOG("css string %s", str.c_str());
	CssStringId value = string::hash32(str);
	_cssStrings->insert(pair(value, str));
}

bool Reader::hasParentTag(const String &str) {
	for (auto &it : _tagStack) {
		if (it.name == str) {
			return true;
		}
	}
	return false;
}

bool Reader::shouldParseRefs() {
	for (auto &it : _tagStack) {
		if (it.autoRefs) {
			return true;
		}
	}
	return false;
}

void Reader::onMeta(String && name, String && content) {
	_meta->emplace_back(pair(std::move(name), std::move(content)));
}

void Reader::onCss(const String & href) {
	String path;
	if (href.front() == '/') {
		path = href;
	} else {
		path = filepath::reconstructPath(filepath::merge(filepath::root(_path), href));
	}

	auto cssIt = _css->find(path);
	if (cssIt != _css->end()) {
		for (auto &it : cssIt->second.styles) {
			auto nit = _namedStyles.find(it.first);
			if (nit == _namedStyles.end()) {
				_namedStyles.emplace(it.first, it.second);
			} else {
				nit->second.merge(it.second);
			}
		}
	}
}

bool Reader::isStyleAttribute(const String &tag, const String &name) const {
	if (name == "align" || name == "width" || name == "height") {
		return true;
	}
	if (tag == "li" || tag == "ul" || tag == "ol") {
		return name == "type";
	}
	return false;
}

void Reader::addStyleAttribute(Tag &tag, const String &name, const String &value) {
	if (name == "align") {
		tag.weakStyle.push_back(pair("text-align", value));
		tag.weakStyle.push_back(pair("text-indent", "0px"));
	} else if (name == "width") {
		tag.weakStyle.push_back(pair("width", value + "px"));
	} else if (name == "height") {
		tag.weakStyle.push_back(pair("height", value + "px"));
	} else if (name == "type") {
		if (value == "disc" || value == "circle" || value == "square") {
			tag.weakStyle.push_back(pair("list-style-type", value));
		} else if (value == "A") {
			tag.weakStyle.push_back(pair("list-style-type", "upper-alpha"));
		} else if (value == "a") {
			tag.weakStyle.push_back(pair("list-style-type", "lower-alpha"));
		} else if (value == "I") {
			tag.weakStyle.push_back(pair("list-style-type", "upper-roman"));
		} else if (value == "i") {
			tag.weakStyle.push_back(pair("list-style-type", "lower-roman"));
		} else if (value == "1") {
			tag.weakStyle.push_back(pair("list-style-type", "decimal"));
		}
	}
}

NS_SP_EXT_END(rich_text)
