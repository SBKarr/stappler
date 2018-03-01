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

#ifndef COMMON_UTILS_SPHTMLPARSER_H_
#define COMMON_UTILS_SPHTMLPARSER_H_

#include "SPCharReader.h"
#include "SPString.h"

NS_SP_EXT_BEGIN(html)

/* Reader sample:
struct Reader {
	using Parser = html::Parser<Reader>;
	using Tag = Parser::Tag;
	using StringReader = Parser::StringReader;

	inline void onBeginTag(Parser &p, Tag &tag) {
		log::format("onBeginTag", "%s", tag.name.str().c_str());
	}

	inline void onEndTag(Parser &p, Tag &tag) {
		log::format("onEndTag", "%s", tag.name.str().c_str());
	}

	inline void onTagAttribute(Parser &p, Tag &tag, StringReader &name, StringReader &value) {
		log::format("onTagAttribute", "%s: %s = %s", tag.name.str().c_str(), name.str().c_str(), value.str().c_str());
	}

	inline void onPushTag(Parser &p, Tag &tag) {
		log::format("onPushTag", "%s", tag.name.str().c_str());
	}

	inline void onPopTag(Parser &p, Tag &tag) {
		log::format("onPopTag", "%s", tag.name.str().c_str());
	}

	inline void onInlineTag(Parser &p, Tag &tag) {
		log::format("onInlineTag", "%s", tag.name.str().c_str());
	}

	inline void onTagContent(Parser &p, Tag &tag, StringReader &s) {
		log::format("onTagContent", "%s: %s", tag.name.str().c_str(), s.str().c_str());
	}
};
 */

template <typename StringReader>
struct Tag;

template <typename ReaderType, typename StringReader = StringViewUtf8, typename TagType = html::Tag<StringReader>>
void parse(ReaderType &r, const StringReader &s);

InvokerCallTest_MakeInvoker(Html, onBeginTag);
InvokerCallTest_MakeInvoker(Html, onEndTag);
InvokerCallTest_MakeInvoker(Html, onTagAttribute);
InvokerCallTest_MakeInvoker(Html, onPushTag);
InvokerCallTest_MakeInvoker(Html, onPopTag);
InvokerCallTest_MakeInvoker(Html, onInlineTag);
InvokerCallTest_MakeInvoker(Html, onTagContent);
InvokerCallTest_MakeInvoker(Html, onReadTagName);
InvokerCallTest_MakeInvoker(Html, onReadAttributeName);
InvokerCallTest_MakeInvoker(Html, onReadAttributeValue);

template <typename T>
struct ParserTraits {
	using success = char;
	using failure = long;

	InvokerCallTest_MakeCallTest(onBeginTag, success, failure);
	InvokerCallTest_MakeCallTest(onEndTag, success, failure);
	InvokerCallTest_MakeCallTest(onTagAttribute, success, failure);
	InvokerCallTest_MakeCallTest(onPushTag, success, failure);
	InvokerCallTest_MakeCallTest(onPopTag, success, failure);
	InvokerCallTest_MakeCallTest(onInlineTag, success, failure);
	InvokerCallTest_MakeCallTest(onTagContent, success, failure);
	InvokerCallTest_MakeCallTest(onReadTagName, success, failure);
	InvokerCallTest_MakeCallTest(onReadAttributeName, success, failure);
	InvokerCallTest_MakeCallTest(onReadAttributeValue, success, failure);

public:
	InvokerCallTest_MakeCallMethod(Html, onBeginTag, T);
	InvokerCallTest_MakeCallMethod(Html, onEndTag, T);
	InvokerCallTest_MakeCallMethod(Html, onTagAttribute, T);
	InvokerCallTest_MakeCallMethod(Html, onPushTag, T);
	InvokerCallTest_MakeCallMethod(Html, onPopTag, T);
	InvokerCallTest_MakeCallMethod(Html, onInlineTag, T);
	InvokerCallTest_MakeCallMethod(Html, onTagContent, T);
	InvokerCallTest_MakeCallMethod(Html, onReadTagName, T);
	InvokerCallTest_MakeCallMethod(Html, onReadAttributeName, T);
	InvokerCallTest_MakeCallMethod(Html, onReadAttributeValue, T);
};

template <typename StringReader>
auto Tag_readName(StringReader &is, bool keepClean = false) -> StringReader;

template <typename StringReader>
auto Tag_readAttrName(StringReader &s, bool keepClean = false) -> StringReader;

template <typename StringReader>
auto Tag_readAttrValue(StringReader &s, bool keepClean = false) -> StringReader;

template <typename __StringReader>
struct Tag : public ReaderClassBase<char16_t> {
	using StringReader = __StringReader;

	Tag(const StringReader &name) : name(name) {
		if (name.is('!')) {
			closable = false;
		}
	}

	const StringReader &getName() const { return name; }

	void setClosable(bool v) { closable = v; }
	bool isClosable() const { return closable; }

	StringReader name;
	bool closable = true;
};

template <typename ReaderType, typename __StringReader = StringViewUtf8, typename TagType = Tag<__StringReader>, typename Traits = ParserTraits<ReaderType>>
struct Parser {
	using StringReader = __StringReader;
	using CharType = typename StringReader::MatchCharType;
	using Tag = TagType;

	template <CharType ... Args>
	using Chars = chars::Chars<CharType, Args...>;

	template <CharType First, CharType Last>
	using Range = chars::Chars<CharType, First, Last>;

	using GroupId = CharGroupId;

	template <GroupId G>
	using Group = chars::CharGroup<CharType, G>;

	using LtChar = Chars<'<'>;

	Parser(ReaderType &r) : reader(&r) { }

	inline void cancel() {
		current.clear();
		canceled = true;
	}

	bool parse(const StringReader &r) {
		current = r;
		while (!current.empty()) {
			auto prefix = current.template readUntil<LtChar>(); // move to next tag
			if (!prefix.empty()) {
				if (!tagStack.empty()) {
					onTagContent(tagStack.back(), prefix);
				} else {
					StringReader r;
					Tag t(r);
					onTagContent(t, prefix);
				}
			}

			if (!current.is('<')) {
				break; // next tag not found
			}

			++ current; // drop '<'
			if (current.is('/')) { // close some parsed tag
				++ current; // drop '/'

				auto tag = current.template readUntil<Chars<'>'>>();
				if (!tag.empty() && current.is('>') && !tagStack.empty()) {
					auto it = tagStack.end();
					do {
						-- it;
						auto &name = it->getName();
						string::tolower_buf((char *)tag.data(), tag.size());
						if (tag.size() == name.size() && tag.compare(name.data(), name.size())) {
							// close all tag after <tag>
							auto nit = tagStack.end();
							do {
								-- nit;
								onPopTag(*nit);
								tagStack.pop_back();
							} while (nit != it);
							break;
						}
					} while(it != tagStack.begin());

					if (tagStack.empty()) {
						break;
					}
				} else if (current.empty()) {
					break; // fail to parse tag
				}
				++ current; // drop '>'
			} else {
				auto name = onReadTagName(current);
				if (name.empty()) { // found tag without readable name
					current.template skipUntil<Chars<'>'>>();
					if (current.is('>')) {
						current ++;
					}
					continue;
				}

				if (name.prefix("!--", "!--"_len)) { // process comment
					current.skipUntilString("-->", false);
					continue;
				}

				if (name.is('?')) { // found processing-instruction
					current.skipUntilString("?>", false);
					continue;
				}

				if (name.is('!')) {
					if (current.is("CDATA[")) {
						auto data = current.readUntilString("]]>");
						data += "CDATA["_len;
						current += "]]>"_len;

						if (!tagStack.empty()) {
							onTagContent(tagStack.back(), data);
						} else {
							StringReader r;
							Tag t(r);
							onTagContent(t, data);
						}
						continue;
					} else {
						current.template skipUntil<Chars<'>'>>();
						if (current.is('>')) {
							++ current;
						}
						continue;
					}
				}

				TagType tag(name);
				onBeginTag(tag);

				StringReader attrName;
				StringReader attrValue;
				while (!current.empty() && !current.is('>') && !current.is('/')) {
					attrName.clear();
					attrValue.clear();

					attrName = onReadAttributeName(current);
					if (attrName.empty()) {
						continue;
					}

					attrValue = onReadAttributeValue(current);
					onTagAttribute(tag, attrName, attrValue);
				}

				if (current.is('/')) {
					tag.setClosable(false);
				}

				current.template skipUntil<Chars<'>'>>();
				if (current.is('>')) {
					++ current;
				}

				onEndTag(tag);
				if (tag.isClosable()) {
					onPushTag(tag);
					tagStack.emplace_back(std::move(tag));
				} else {
					onInlineTag(tag);
				}
			}
		}

		if (!tagStack.empty()) {
			auto nit = tagStack.end();
			do {
				nit --;
				onPopTag(*nit);
				tagStack.pop_back();
			} while (nit != tagStack.begin());
		}

		return !canceled;
	}

	inline StringReader onReadTagName(StringReader &str) {
		if (Traits::hasMethod_onReadTagName) {
			StringReader ret(str);
			Traits::onReadTagName(*reader, *this, ret);
			return ret;
		} else {
			return Tag_readName(str);
		}
	}

	inline StringReader onReadAttributeName(StringReader &str) {
		if (Traits::hasMethod_onReadAttributeName) {
			StringReader ret(str);
			Traits::onReadAttributeName(*reader, *this, ret);
			return ret;
		} else {
			return Tag_readAttrName(str);
		}
	}

	inline StringReader onReadAttributeValue(StringReader &str) {
		if (Traits::hasMethod_onReadAttributeValue) {
			StringReader ret(str);
			Traits::onReadAttributeValue(*reader, *this, ret);
			return ret;
		} else {
			return Tag_readAttrValue(str);
		}
	}

	inline void onBeginTag(TagType &tag) { Traits::onBeginTag(*reader, *this, tag); }
	inline void onEndTag(TagType &tag) { Traits::onEndTag(*reader, *this, tag); }
	inline void onTagAttribute(TagType &tag, StringReader &name, StringReader &value) { Traits::onTagAttribute(*reader, *this, tag, name, value); }
	inline void onPushTag(TagType &tag) { Traits::onPushTag(*reader, *this, tag); }
	inline void onPopTag(TagType &tag) { Traits::onPopTag(*reader, *this, tag); }
	inline void onInlineTag(TagType &tag) { Traits::onInlineTag(*reader, *this, tag); }
	inline void onTagContent(TagType &tag, StringReader &s) { Traits::onTagContent(*reader, *this, tag, s); }

	bool canceled = false;
	ReaderType *reader;
	StringReader current;
	Vector<TagType> tagStack;
};

template <typename ReaderType, typename StringReader, typename TagType>
void parse(ReaderType &r, const StringReader &s) {
	html::Parser<ReaderType, StringReader, TagType> p(r);
	p.parse(s);
}

NS_SP_EXT_END(html)

#endif /* COMMON_UTILS_SPHTMLPARSER_H_ */
