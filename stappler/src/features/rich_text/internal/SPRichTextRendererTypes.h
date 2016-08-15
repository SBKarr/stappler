/*
 * SPRichTextRendererTypes.h
 *
 *  Created on: 02 мая 2015 г.
 *      Author: sbkarr
 */

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
	NoHeightCheck = 2 << 0, // disable re-rendering when viewport height is changed
	RenderById = 3 << 0, // render nodes by html id instead of spine name
	PaginatedLayout = 4 << 0, // render as book pages instead of html linear layout
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

	virtual bool resolveMediaQuery(MediaQueryId queryId) const;
	virtual String getCssString(CssStringId) const;
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
