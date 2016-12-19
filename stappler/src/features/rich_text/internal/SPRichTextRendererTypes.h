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

#ifndef LIBS_STAPPLER_FEATURES_RICH_TEXT_SPRICHTEXTRENDERERTYPES_H_
#define LIBS_STAPPLER_FEATURES_RICH_TEXT_SPRICHTEXTRENDERERTYPES_H_

#include "SPRichTextStyle.h"
#include "SPFont.h"
#include "SPRichTextDocument.h"

NS_SP_EXT_BEGIN(rich_text)

namespace RenderFlag {
using Mask = uint32_t;
enum Flag : Mask {
	NoImages = 1 << 0, // do not render images
	NoHeightCheck = 1 << 1, // disable re-rendering when viewport height is changed
	RenderById = 1 << 2, // render nodes by html id instead of spine name
	PaginatedLayout = 1 << 3, // render as book pages instead of html linear layout
	SplitPages = 1 << 4, // render as book pages instead of html linear layout
};
}

struct MediaParameters {
	Size surfaceSize;

	int dpi = 92;
	float density = 1.0f;
	float fontScale = 1.0f;

	style::MediaType mediaType = style::MediaType::Screen;
	style::Orientation orientation = style::Orientation::Landscape;
	style::Pointer pointer = style::Pointer::Coarse;
	style::Hover hover = style::Hover::None;
	style::LightLevel lightLevel = style::LightLevel::Normal;
	style::Scripting scripting = style::Scripting::None;

	RenderFlag::Mask flags = 0;

	Map<CssStringId, String> _options;

	Margin pageMargin;

	void addOption(const String &);
	void removeOption(const String &);
	bool hasOption(const String &) const;
	bool hasOption(CssStringId) const;

	Vector<bool> resolveMediaQueries(const Vector<style::MediaQuery> &) const;
};

class MediaResolver : public RendererInterface {
public:
	MediaResolver();
	MediaResolver(Document *, const MediaParameters &, const Vector<String> &opts);

	operator bool () const { return _document != nullptr; }

	virtual bool resolveMediaQuery(MediaQueryId queryId) const override;
	virtual String getCssString(CssStringId) const override;
protected:
	Vector<bool> _media;
	Rc<Document> _document;
};

struct FontFileSpec {
	String normal;
	String italic;
	String bold;
	String bold_italic;
};

NS_SP_EXT_END(rich_text)

#endif /* LIBS_STAPPLER_FEATURES_RICH_TEXT_SPRICHTEXTRENDERERTYPES_H_ */
