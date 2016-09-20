/*
 * SPHtmlParser.h
 *
 *  Created on: 22 сент. 2016 г.
 *      Author: sbkarr
 */

#ifndef COMMON_HTML_SPHTMLPARSER_H_
#define COMMON_HTML_SPHTMLPARSER_H_

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

struct Tag;

template <typename ReaderType, typename TagType = html::Tag>
void parse(ReaderType &r, const CharReaderUtf8 &s);

InvokerCallTest_MakeInvoker(Html, onBeginTag);
InvokerCallTest_MakeInvoker(Html, onEndTag);
InvokerCallTest_MakeInvoker(Html, onTagAttribute);
InvokerCallTest_MakeInvoker(Html, onPushTag);
InvokerCallTest_MakeInvoker(Html, onPopTag);
InvokerCallTest_MakeInvoker(Html, onInlineTag);
InvokerCallTest_MakeInvoker(Html, onTagContent);

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

public:
	InvokerCallTest_MakeCallMethod(Html, onBeginTag, T);
	InvokerCallTest_MakeCallMethod(Html, onEndTag, T);
	InvokerCallTest_MakeCallMethod(Html, onTagAttribute, T);
	InvokerCallTest_MakeCallMethod(Html, onPushTag, T);
	InvokerCallTest_MakeCallMethod(Html, onPopTag, T);
	InvokerCallTest_MakeCallMethod(Html, onInlineTag, T);
	InvokerCallTest_MakeCallMethod(Html, onTagContent, T);
};

struct Tag : public ReaderClassBase<char16_t> {
	using StringReader = CharReaderUtf8;

	static void readQuotedString(StringReader &s, StringStream &str, char quoted);

	static StringReader readName(StringReader &is);
	static StringReader readAttrName(StringReader &s);
	static StringReader readAttrValue(StringReader &s);

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

template <typename ReaderType, typename TagType = Tag, typename Traits = ParserTraits<ReaderType>>
struct Parser : public ReaderClassBase<char16_t> {
	using StringReader = CharReaderUtf8;
	using Tag = TagType;

	Parser(ReaderType &r) : reader(&r) { }

	inline void cancel() {
		current.clear();
		canceled = true;
	}

	bool parse(const StringReader &r) {
		current = r;
		while (!current.empty()) {
			auto prefix = current.readUntil<Chars<u'<'>>(); // move to next tag
			if (!prefix.empty()) {
				if (!tagStack.empty()) {
					onTagContent(tagStack.back(), prefix);
				} else {
					TagType t{StringReader()};
					onTagContent(t, prefix);
				}
			}

			if (!current.is('<')) {
				break; // next tag not found
			}

			++ current; // drop '<'
			if (current.is('/')) { // close some parsed tag
				++ current; // drop '/'

				auto tag = current.readUntil<Chars<'>'>>();
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
				auto name = TagType::readName(current);
				if (name.empty()) { // found tag without readable name
					current.skipUntil<Chars<'>'>>();
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
					current.skipUntil<Chars<'>'>>();
					if (current.is('>')) {
						++ current;
					}
					continue;
				}

				TagType tag(name);
				onBeginTag(tag);

				StringReader attrName;
				StringReader attrValue;
				while (!current.empty() && !current.is('>') && !current.is('/')) {
					attrName.clear();
					attrValue.clear();

					attrName = TagType::readAttrName(current);
					if (attrName.empty()) {
						continue;
					}

					attrValue = TagType::readAttrValue(current);
					onTagAttribute(tag, attrName, attrValue);
				}

				if (current.is('/')) {
					tag.setClosable(false);
				}

				current.skipUntil<Chars<'>'>>();
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

template <typename ReaderType, typename TagType>
void parse(ReaderType &r, const CharReaderUtf8 &s) {
	html::Parser<ReaderType, TagType> p(r);
	p.parse(s);
}

NS_SP_EXT_END(html)

#endif /* COMMON_HTML_SPHTMLPARSER_H_ */
