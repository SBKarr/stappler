/**
Copyright (c) 2016-2019 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef COMMON_UTILS_NETWORK_SPHTMLPARSER_H_
#define COMMON_UTILS_NETWORK_SPHTMLPARSER_H_

#include "SPStringView.h"
#include "SPString.h"

NS_SP_EXT_BEGIN(html)

/* Reader sample:
struct Reader {
	using Parser = html::Parser<Reader>;
	using Tag = Parser::Tag;
	using StringReader = Parser::StringReader;

	inline void onBeginTag(Parser &p, Tag &tag) {
		log::text("onBeginTag", tag.name);
	}

	inline void onEndTag(Parser &p, Tag &tag, bool isClosable) {
		log::text("onEndTag", tag.name);
	}

	inline void onTagAttribute(Parser &p, Tag &tag, StringReader &name, StringReader &value) {
		log::vtext("onTagAttribute", tag.name, ": ", name, " = ", value);
	}

	inline void onPushTag(Parser &p, Tag &tag) {
		log::text("onPushTag", tag.name);
	}

	inline void onPopTag(Parser &p, Tag &tag) {
		log::text("onPopTag", tag.name);
	}

	inline void onInlineTag(Parser &p, Tag &tag) {
		log::text("onInlineTag", tag.name);
	}

	inline void onTagContent(Parser &p, Tag &tag, StringReader &s) {
		log::vtext("onTagContent", tag.name, ": ", s);
	}
};
*/

template <typename StringReader>
struct Tag;

template <typename ReaderType, typename StringReader = StringViewUtf8,
	typename TagType = typename std::conditional<
		std::is_same<typename ReaderType::Tag, html::Tag<StringReader>>::value,
		html::Tag<StringReader>,
		typename ReaderType::Tag
	>::type>
void parse(ReaderType &r, const StringReader &s, bool rootOnly = true, bool lowercase = true);

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
	InvokerCallTest_MakeCallTest(shouldLowercaseTokens, success, failure);
	InvokerCallTest_MakeCallTest(shouldParseTag, success, failure);
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

	void setHasContent(bool v) { content = v; }
	bool hasContent() const { return content; }

	StringReader name;
	bool closable = true;
	bool content = false;
};

template <typename ReaderType, typename __StringReader = StringViewUtf8,
		typename TagType = typename html::Tag<__StringReader>,
		typename Traits = ParserTraits<ReaderType>>
struct Parser {
	using StringReader = __StringReader;
	using OrigCharType = typename StringReader::CharType;
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

	Parser(ReaderType &r) : reader(&r) {
		if constexpr (Traits::shouldLowercaseTokens) {
			lowercase = reader->shouldLowercaseTokens(*this);
		}
	}

	inline void cancel() {
		current.clear();
		canceled = true;
	}

	bool parse(const StringReader &r, bool rootOnly) {
		current = r;
		while (!current.empty()) {
			auto prefix = current.template readUntil<LtChar>(); // move to next tag
			if (!prefix.empty()) {
				if (!tagStack.empty()) {
					tagStack.back().setHasContent(true);
					onTagContent(tagStack.back(), prefix);
				} else {
					StringReader r;
					Tag t(r);
					t.setHasContent(true);
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
						if (lowercase) {
							if constexpr (sizeof(OrigCharType) == 2) {
								string::tolower_buf((char16_t *)tag.data(), tag.size());
							} else {
								string::tolower_buf((char *)tag.data(), tag.size());
							}
						}
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

					if (rootOnly && tagStack.empty()) {
						if (current.is('>')) {
							++ current; // drop '>'
						}
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

				if constexpr (sizeof(OrigCharType) == 2) {
					if (name.prefix(u"!--", u"!--"_len)) { // process comment
						current.skipUntilString(u"-->", false);
						continue;
					}

					if (name.is(u'?')) { // found processing-instruction
						current.skipUntilString(u"?>", false);
						continue;
					}
				} else {
					if (name.prefix("!--", "!--"_len)) { // process comment
						current.skipUntilString("-->", false);
						continue;
					}

					if (name.is('?')) { // found processing-instruction
						current.skipUntilString("?>", false);
						continue;
					}
				}

				if (name.is('!')) {
					StringReader cdata;
					if constexpr (sizeof(OrigCharType) == 2) {
						if (current.starts_with(u"CDATA[")) {
							cdata = current.readUntilString(u"]]>");
							cdata += "CDATA["_len;
							current += "]]>"_len;
						}
					} else {
						if (current.starts_with("CDATA[")) {
							cdata = current.readUntilString("]]>");
							cdata += "CDATA["_len;
							current += "]]>"_len;
						}
					}

					if (!cdata.empty()) {
						if (!tagStack.empty()) {
							tagStack.back().setHasContent(true);
							onTagContent(tagStack.back(), cdata);
						} else {
							StringReader r;
							Tag t(r);
							t.setHasContent(true);
							onTagContent(t, cdata);
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

				onEndTag(tag, !tag.isClosable());
				if (tag.isClosable()) {
					onPushTag(tag);
					tagStack.emplace_back(std::move(tag));
					if (!shouldParseTag(tag)) {
						auto start = current;
						while (!current.empty()) {
							current.template skipUntil<Chars<'<'>>();
							if (current.is('<')) {
								auto tmp = current.sub(1);
								if (tmp.is('/')) {
									++ tmp;
									if (tmp.starts_with(tag.name)) {
										tmp += tag.name.size();
										tmp.template skipChars<Group<GroupId::WhiteSpace>>();
										if (tmp.is('>')) {
											StringReader content(start.data(), current.data() - start.data());
											if (!content.empty()) {
												onTagContent(tag, content);
											}
											onPopTag(tag);
											tagStack.pop_back();

											++ tmp;
											current = tmp;
											break;
										}
									}
								}
								++ current;
							}
						}
					}
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
		if constexpr (Traits::onReadTagName) {
			StringReader ret(str);
			reader->onReadTagName(*this, ret);
			return ret;
		} else {
			return Tag_readName(str, !lowercase);
		}
	}

	inline StringReader onReadAttributeName(StringReader &str) {
		if constexpr (Traits::onReadAttributeName) {
			StringReader ret(str);
			reader->onReadAttributeName(*this, ret);
			return ret;
		} else {
			return Tag_readAttrName(str, !lowercase);
		}
	}

	inline StringReader onReadAttributeValue(StringReader &str) {
		if constexpr (Traits::onReadAttributeValue) {
			StringReader ret(str);
			reader->onReadAttributeValue(*this, ret);
			return ret;
		} else {
			return Tag_readAttrValue(str, !lowercase);
		}
	}

	inline void onBeginTag(TagType &tag) {
		if constexpr (Traits::onBeginTag) { reader->onBeginTag(*this, tag); }
	}
	inline void onEndTag(TagType &tag, bool isClosed) {
		if constexpr (Traits::onEndTag) { reader->onEndTag(*this, tag, isClosed); }
	}
	inline void onTagAttribute(TagType &tag, StringReader &name, StringReader &value) {
		if constexpr (Traits::onTagAttribute) { reader->onTagAttribute(*this, tag, name, value); }
	}
	inline void onPushTag(TagType &tag) {
		if constexpr (Traits::onPushTag) { reader->onPushTag(*this, tag); }
	}
	inline void onPopTag(TagType &tag) {
		if constexpr (Traits::onPopTag) { reader->onPopTag(*this, tag); }
	}
	inline void onInlineTag(TagType &tag) {
		if constexpr (Traits::onInlineTag) { reader->onInlineTag(*this, tag); }
	}
	inline void onTagContent(TagType &tag, StringReader &s) {
		if constexpr (Traits::onTagContent) { reader->onTagContent(*this, tag, s); }
	}
	inline bool shouldParseTag(TagType &tag) {
		if constexpr (Traits::shouldParseTag) { return reader->shouldParseTag(*this, tag); }
		return true;
	}

	bool lowercase = true;
	bool canceled = false;
	ReaderType *reader;
	StringReader current;
	Vector<TagType> tagStack;
};

template <typename ReaderType, typename StringReader, typename TagType>
void parse(ReaderType &r, const StringReader &s, bool rootOnly, bool lowercase) {
	html::Parser<ReaderType, StringReader, TagType> p(r);
	p.lowercase = lowercase;
	p.parse(s, rootOnly);
}

NS_SP_EXT_END(html)

#endif /* COMMON_UTILS_NETWORK_SPHTMLPARSER_H_ */
