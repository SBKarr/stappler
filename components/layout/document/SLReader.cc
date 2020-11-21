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
#include "SLParser.h"
#include "SLReader.h"
#include "SPFilesystem.h"
#include "SPString.h"

//#define SP_RTREADER_LOG(...) log::format("RTReader", __VA_ARGS__)
#define SP_RTREADER_LOG(...)

NS_LAYOUT_BEGIN

static String getRootXType(const Vector<style::Tag> &stack) {
	for (auto it = stack.rbegin(); it != stack.rend(); it ++) {
		if (!it->xType.empty()) {
			return it->xType;
		}
	}
	return String();
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
					addCssString(name, value);
				}
				ret.style.read(name, value);
			});
		} else if (paramName == "href") {
			auto ref = parser::readHtmlTagParamValue(tag);
			if (!ref.empty() && ref.front() != '#' && ref.find("://") == String::npos && !_page->path.empty()) {
				ret.attributes.emplace(paramName, filepath::reconstructPath(filepath::merge(filepath::root(_page->path), ref)));
			} else if (!ref.empty()) {
				ret.attributes.emplace(paramName, std::move(ref));
			}
		} else if (paramName == "class") {
			paramValue = parser::readHtmlTagParamValue(tag);
			string::split(string::tolower(paramValue), " ", [&ret] (const StringView &r) {
				 ret.classes.emplace_back(r.str());
			});
			ret.attributes.emplace(paramName, paramValue);
		} else if (paramName == "x-refs" && ret.type == Tag::Block) {
			paramValue = parser::readHtmlTagParamValue(tag);
			ret.autoRefs = (paramValue == "auto");
		} else {
			if (tag.is('=')) {
				String val = parser::readHtmlTagParamValue(tag);
				SP_RTREADER_LOG("'%s' attr: '%s' : '%s'", ret.name.c_str(), paramName.c_str(), val.c_str());
				ret.attributes.emplace(paramName, std::move(val));
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
	} else if ((ret.type == Tag::Image || ret.xType == "image" || ret.name == "table") && ret.id.empty()) {
		ret.id = toString("__id__", _pseudoId, "__", encodePathString(_page->path));
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
				if (src.front() != '/' && src.find("://") == String::npos && !_page->path.empty()) {
					it->second = filepath::reconstructPath(filepath::merge(filepath::root(_page->path), it->second));
				}
			}
			_page->assets.push_back(it->second);

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

	if (tag.is('/') || ret.name == "br" || ret.name == "hr" || ret.name == "col" || ret.type == Tag::Image
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
		//_styleStack.push_back(specializeStyle(tag, (_styleStack.empty()?_nodeStack.back()->getStyle():_styleStack.back())));
		if (tag.type == Tag::Block) {
			auto &node = _nodeStack.back()->pushNode(tag.name, tag.id, move(tag.style), std::move(tag.attributes));
			SP_RTREADER_LOG("'%s'", tag.name.c_str());
			_nodeStack.push_back(&node);
		} else if (tag.type == Tag::Markup && tag.name == "body") {
			//auto &body = _nodeStack.front();
			//body->pushStyle(_styleStack.back());
		}
	}
}
void Reader::onPopTag(Tag &tag) {
	if (!_htmlTag || !hasParentTag("head")) {
		if (tag.type == Tag::Block) {
			_nodeStack.pop_back();
			SP_RTREADER_LOG("'/%s'", tag.name.c_str());
		}
		/*if (!_styleStack.empty()) {
			_styleStack.pop_back();
		}*/
	}
}
void Reader::onInlineTag(Tag &tag) {
	if (!_htmlTag || !hasParentTag("head")) {
		SP_RTREADER_LOG("'inline' '%s'", tag.name.c_str());
		if (tag.name == "br") {
			_nodeStack.back()->pushLineBreak();
		} else if (tag.type != Tag::Special) {
			_nodeStack.back()->pushNode(tag.name, tag.id, move(tag.style), std::move(tag.attributes));
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

					Map<String, String> attr;
					attr.emplace("href", ref);
					attr.emplace("class", "auto");
					attr.emplace("x-refs", "auto");

					Node &hrefNode = node->pushNode("a", "", move(refStyle), std::move(attr));
					hrefNode.pushValue(ref);
				}
			});
		}
	}
}

void Reader::onStyleTag(StringReader &s) {
	parser::readStyleTag(*this, s, _page->fonts);
}

void Reader::onStyleObject(const String &selector, const style::StyleVec &vec, const MediaQueryId &query) {
	if ((selector == "*" || selector == "body") && !_nodeStack.empty()) {
		_nodeStack.front()->pushStyle(vec, query);
	}

	auto it = _page->styles.find(selector);
	if (it == _page->styles.end()) {
		it = _page->styles.insert(pair(selector, style::ParameterList())).first;
	}
	it->second.read(vec, query);
	SP_RTREADER_LOG("Added NamedStyle '%s' : %s", selector.c_str(), it->second.css().c_str());
}

static void Reader_resolveFontPaths(const String &root, Map<String, Vector<style::FontFace>> &fonts) {
	if (root.empty()) {
		return;
	}

	for (auto &it : fonts) {
		for (auto &vit : it.second) {
			for (auto &fit : vit.src) {
				String &path = fit.file;
				if (!path.empty() && path.front() != '/' && path.find(":") == String::npos) {
					path = String("document://") + filepath::reconstructPath(filepath::merge(filepath::root(root), path));
					SP_RTREADER_LOG("Resolve font path: '%s' '%s'", it.first.c_str(), path.c_str());
				}
			}
		}
	}
}

bool Reader::readHtml(ContentPage &page, const StringView &str, MetaPairs &meta) {
	SP_RTREADER_LOG("read: %s", page.path.c_str());
	Node &ret = page.root;

	_page = &page;
	_meta = &meta;

	ret.pushStyle(style::getStyleForTag("body"));
	_nodeStack.push_back(&ret);
	_current = _origin = StringReader(str.data(), str.size());

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
	Reader_resolveFontPaths(_page->path, _page->fonts);
	return true;
}

bool Reader::readCss(ContentPage &page, const StringView &str) {
	_page = &page;

	_current = _origin = StringReader(str.data(), str.size());
	parser::readStyleTag(*this, _current, _page->fonts);
	Reader_resolveFontPaths(_page->path, _page->fonts);
	return true;
}

MediaQueryId Reader::addMediaQuery(style::MediaQuery &&mediaQuery) {
	_page->queries.emplace_back(std::move(mediaQuery));
	return (MediaQueryId) _page->queries.size() - 1;
}
void Reader::addCssString(CssStringId id, const String &str) {
	_page->strings.insert(pair(id, str));
}

void Reader::addCssString(const StringView &name, const String &origStr) {
	auto str = parser::resolveCssString(StringView(origStr));
	if (name == "background-image") {
		_page->assets.push_back(str.str());
	}
	SP_RTREADER_LOG("css string %s", str.data());
	CssStringId value = hash::hash32(str.data(), str.size());
	_page->strings.insert(pair(value, str.str()));
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
	if (href.front() == '/' || href.find("://") != String::npos) {
		path = href;
	} else {
		path = filepath::reconstructPath(filepath::merge(filepath::root(_page->path), href));
	}

	_page->assets.push_back(path);
	_page->styleReferences.push_back(path);
}

NS_LAYOUT_END
