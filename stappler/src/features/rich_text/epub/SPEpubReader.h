/*
 * SPEpubReader.h
 *
 *  Created on: 15 авг. 2016 г.
 *      Author: sbkarr
 */

#ifndef STAPPLER_SRC_FEATURES_RICH_TEXT_EPUB_SPEPUBREADER_H_
#define STAPPLER_SRC_FEATURES_RICH_TEXT_EPUB_SPEPUBREADER_H_

#include "SPRichTextReader.h"
#include "SPEpubInfo.h"

NS_EPUB_BEGIN

class Reader : public rich_text::Reader {
public:
	using StringReader = CharReaderUtf8;

	virtual ~Reader() { }

protected:
	virtual void onPushTag(Tag &) override;
	virtual void onPopTag(Tag &) override;
	virtual void onInlineTag(Tag &) override;
	virtual void onTagContent(Tag &, StringReader &) override;

	bool isCaseAllowed() const;
	bool isNamespaceImplemented(const String &) const;

	virtual bool isStyleAttribute(const String &tagName, const String &name) const override;
	virtual void addStyleAttribute(rich_text::style::Tag &tag, const String &name, const String &value) override;

	struct SwitchData {
		bool parsed = false;
		bool active = false;
	};

	Vector<SwitchData> _switchStatus;
};

NS_EPUB_END

#endif /* STAPPLER_SRC_FEATURES_RICH_TEXT_EPUB_SPEPUBREADER_H_ */
