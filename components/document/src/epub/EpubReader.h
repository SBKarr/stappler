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

#ifndef EPUB_EPUBREADER_H_
#define EPUB_EPUBREADER_H_

#include "EpubInfo.h"
#include "SLReader.h"

NS_EPUB_BEGIN

class Reader : public layout::Reader {
public:
	using StringReader = StringViewUtf8;

	virtual ~Reader() { }

protected:
	virtual void onPushTag(Tag &) override;
	virtual void onPopTag(Tag &) override;
	virtual void onInlineTag(Tag &) override;
	virtual void onTagContent(Tag &, StringReader &) override;

	bool isCaseAllowed() const;
	bool isNamespaceImplemented(const String &) const;

	//virtual bool isStyleAttribute(const String &tagName, const String &name) const override;
	//virtual void addStyleAttribute(layout::style::Tag &tag, const String &name, const String &value) override;

	struct SwitchData {
		bool parsed = false;
		bool active = false;
	};

	Vector<SwitchData> _switchStatus;
};

NS_EPUB_END

#endif /* LAYOUT_EPUB_SPEPUBREADER_H_ */
