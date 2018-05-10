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

#include "SLParser.h"
#include "SPLayout.h"
#include "SLStyle.h"
#include "SPString.h"

NS_LAYOUT_BEGIN

namespace style {

Tag::Type Tag::getType(const String &tagName) {
	if (tagName == "img") {
		return style::Tag::Image;
	} else if (tagName == "style") {
		return style::Tag::Style;
	} else if (tagName[0] == '!' || tagName[0] == '-' || tagName == "xml" || tagName == "meta" || tagName == "link") {
		return style::Tag::Special;
	} else if (tagName == "html" || tagName == "head" || tagName == "body") {
		return style::Tag::Markup;
	} else {
		return style::Tag::Block;
	}
}


template<> void Parameter::set<ParameterName::FontStyle, FontStyle>(const FontStyle &v) { value.fontStyle = v; }
template<> void Parameter::set<ParameterName::FontWeight, FontWeight>(const FontWeight &v) { value.fontWeight = v; }
template<> void Parameter::set<ParameterName::FontSize, uint8_t>(const uint8_t &v) { value.fontSize = v; }
template<> void Parameter::set<ParameterName::FontStretch, FontStretch>(const FontStretch &v) { value.fontStretch = v; }
template<> void Parameter::set<ParameterName::FontVariant, FontVariant>(const FontVariant &v) { value.fontVariant = v; }
template<> void Parameter::set<ParameterName::FontSizeNumeric, Metric>(const style::Metric &v) { value.sizeValue = v; }
template<> void Parameter::set<ParameterName::FontSizeIncrement, FontSizeIncrement>(const FontSizeIncrement &v) { value.fontSizeIncrement = v; }
template<> void Parameter::set<ParameterName::TextTransform, TextTransform>(const TextTransform &v) { value.textTransform = v; }
template<> void Parameter::set<ParameterName::TextDecoration, TextDecoration>(const TextDecoration &v) { value.textDecoration = v; }
template<> void Parameter::set<ParameterName::TextAlign, TextAlign>(const TextAlign &v) { value.textAlign = v; }
template<> void Parameter::set<ParameterName::WhiteSpace, WhiteSpace>(const WhiteSpace &v) { value.whiteSpace = v; }
template<> void Parameter::set<ParameterName::Hyphens, Hyphens>(const Hyphens &v) { value.hyphens = v; }
template<> void Parameter::set<ParameterName::Display, Display>(const Display &v) { value.display = v; }
template<> void Parameter::set<ParameterName::ListStyleType, ListStyleType>(const ListStyleType &v) { value.listStyleType = v; }
template<> void Parameter::set<ParameterName::ListStylePosition, ListStylePosition>(const ListStylePosition &v) { value.listStylePosition = v; }
template<> void Parameter::set<ParameterName::XListStyleOffset, Metric>(const Metric &v) { value.sizeValue = v; }
template<> void Parameter::set<ParameterName::Float, Float>(const Float &v) { value.floating = v; }
template<> void Parameter::set<ParameterName::Clear, Clear>(const Clear &v) { value.clear = v; }
template<> void Parameter::set<ParameterName::Color, Color3B>(const Color3B &v) { value.color = v; }
template<> void Parameter::set<ParameterName::Color, Color>(const Color &v) { value.color = v; }
template<> void Parameter::set<ParameterName::Opacity, uint8_t>(const uint8_t &v) { value.opacity = v; }
template<> void Parameter::set<ParameterName::TextIndent, Metric>(const Metric &v) { value.sizeValue = v; }
template<> void Parameter::set<ParameterName::LineHeight, Metric>(const Metric &v) { value.sizeValue = v; }
template<> void Parameter::set<ParameterName::MarginTop, Metric>(const Metric &v) { value.sizeValue = v; }
template<> void Parameter::set<ParameterName::MarginRight, Metric>(const Metric &v) { value.sizeValue = v; }
template<> void Parameter::set<ParameterName::MarginBottom, Metric>(const Metric &v) { value.sizeValue = v; }
template<> void Parameter::set<ParameterName::MarginLeft, Metric>(const Metric &v) { value.sizeValue = v; }
template<> void Parameter::set<ParameterName::Width, Metric>(const Metric &v) { value.sizeValue = v; }
template<> void Parameter::set<ParameterName::Height, Metric>(const Metric &v) { value.sizeValue = v; }
template<> void Parameter::set<ParameterName::MinWidth, Metric>(const Metric &v) { value.sizeValue = v; }
template<> void Parameter::set<ParameterName::MinHeight, Metric>(const Metric &v) { value.sizeValue = v; }
template<> void Parameter::set<ParameterName::MaxWidth, Metric>(const Metric &v) { value.sizeValue = v; }
template<> void Parameter::set<ParameterName::MaxHeight, Metric>(const Metric &v) { value.sizeValue = v; }
template<> void Parameter::set<ParameterName::PaddingTop, Metric>(const Metric &v) { value.sizeValue = v; }
template<> void Parameter::set<ParameterName::PaddingRight, Metric>(const Metric &v) { value.sizeValue = v; }
template<> void Parameter::set<ParameterName::PaddingBottom, Metric>(const Metric &v) { value.sizeValue = v; }
template<> void Parameter::set<ParameterName::PaddingLeft, Metric>(const Metric &v) { value.sizeValue = v; }
template<> void Parameter::set<ParameterName::FontFamily, CssStringId>(const CssStringId &v) { value.stringId = v; }
template<> void Parameter::set<ParameterName::BackgroundColor, Color4B>(const Color4B &v) { value.color4 = v; }
template<> void Parameter::set<ParameterName::BackgroundImage, CssStringId>(const CssStringId &v) { value.stringId = v; }
template<> void Parameter::set<ParameterName::BackgroundPositionX, Metric>(const Metric &v) { value.sizeValue = v; }
template<> void Parameter::set<ParameterName::BackgroundPositionY, Metric>(const Metric &v) { value.sizeValue = v; }
template<> void Parameter::set<ParameterName::BackgroundRepeat, BackgroundRepeat>(const BackgroundRepeat &v) { value.backgroundRepeat = v; }
template<> void Parameter::set<ParameterName::BackgroundSizeWidth, Metric>(const Metric &v) { value.sizeValue = v; }
template<> void Parameter::set<ParameterName::BackgroundSizeHeight, Metric>(const Metric &v) { value.sizeValue = v; }
template<> void Parameter::set<ParameterName::VerticalAlign, VerticalAlign>(const VerticalAlign &v) { value.verticalAlign = v; }
template<> void Parameter::set<ParameterName::BorderTopStyle, BorderStyle>(const BorderStyle &v) { value.borderStyle = v; }
template<> void Parameter::set<ParameterName::BorderTopWidth, Metric>(const Metric &v) { value.sizeValue = v; }
template<> void Parameter::set<ParameterName::BorderTopColor, Color4B>(const Color4B &v) { value.color4 = v; }
template<> void Parameter::set<ParameterName::BorderRightStyle, BorderStyle>(const BorderStyle &v) { value.borderStyle = v; }
template<> void Parameter::set<ParameterName::BorderRightWidth, Metric>(const Metric &v) { value.sizeValue = v; }
template<> void Parameter::set<ParameterName::BorderRightColor, Color4B>(const Color4B &v) { value.color4 = v; }
template<> void Parameter::set<ParameterName::BorderBottomStyle, BorderStyle>(const BorderStyle &v) { value.borderStyle = v; }
template<> void Parameter::set<ParameterName::BorderBottomWidth, Metric>(const Metric &v) { value.sizeValue = v; }
template<> void Parameter::set<ParameterName::BorderBottomColor, Color4B>(const Color4B &v) { value.color4 = v; }
template<> void Parameter::set<ParameterName::BorderLeftStyle, BorderStyle>(const BorderStyle &v) { value.borderStyle = v; }
template<> void Parameter::set<ParameterName::BorderLeftWidth, Metric>(const Metric &v) { value.sizeValue = v; }
template<> void Parameter::set<ParameterName::BorderLeftColor, Color4B>(const Color4B &v) { value.color4 = v; }
template<> void Parameter::set<ParameterName::OutlineStyle, BorderStyle>(const BorderStyle &v) { value.borderStyle = v; }
template<> void Parameter::set<ParameterName::OutlineWidth, Metric>(const Metric &v) { value.sizeValue = v; }
template<> void Parameter::set<ParameterName::OutlineColor, Color4B>(const Color4B &v) { value.color4 = v; }
template<> void Parameter::set<ParameterName::PageBreakAfter, PageBreak>(const PageBreak &v) { value.pageBreak = v; }
template<> void Parameter::set<ParameterName::PageBreakBefore, PageBreak>(const PageBreak &v) { value.pageBreak = v; }
template<> void Parameter::set<ParameterName::PageBreakInside, PageBreak>(const PageBreak &v) { value.pageBreak = v; }
template<> void Parameter::set<ParameterName::Autofit, Autofit>(const Autofit &v) { value.autofit = v; }
template<> void Parameter::set<ParameterName::BorderCollapse, BorderCollapse>(const BorderCollapse &v) { value.borderCollapse = v; }
template<> void Parameter::set<ParameterName::CaptionSide, CaptionSide>(const CaptionSide &v) { value.captionSide = v; }

void ParameterList::set(const Parameter &p, bool force) {
	if (p.mediaQuery != MediaQueryNone() && !isAllowedForMediaQuery(p.name)) {
		return;
	}

	if (force && p.mediaQuery == MediaQueryNone()) {
		for (auto &it : data) {
			if (it.name == p.name) {
				it.value = p.value;
				return;
			}
		}
	}

	data.push_back(p);
}

ParameterList getStyleForTag(const StringView &tag, const StringView &parent) {
	ParameterList style;

	if (tag == "div") {
		style.data.push_back(Parameter::create<ParameterName::Display>(Display::Block));
	}

	if (tag == "p" || tag == "h1" || tag == "h2" || tag == "h3" || tag == "h4" || tag == "h5" || tag == "h6") {
		if (parent != "li" && parent != "blockquote") {
			style.data.push_back(Parameter::create<ParameterName::MarginTop>(Metric(0.5f, Metric::Units::Em)));
			style.data.push_back(Parameter::create<ParameterName::MarginBottom>(Metric(0.5f, Metric::Units::Em)));
			if (parent != "dd" && parent != "figcaption") {
				style.data.push_back(Parameter::create<ParameterName::TextIndent>(Metric(1.5f, Metric::Units::Rem)));
			}
		}
		style.data.push_back(Parameter::create<ParameterName::LineHeight>(Metric(1.15f, Metric::Units::Em)));
		style.data.push_back(Parameter::create<ParameterName::Display>(Display::Block));
	}

	if (tag == "span" || tag == "strong" || tag == "em" || tag == "nobr"
			|| tag == "sub" || tag == "sup" || tag == "inf" || tag == "b"
			|| tag == "i" || tag == "u" || tag == "code") {
		style.data.push_back(Parameter::create<ParameterName::Display>(Display::Inline));
	}

	if (tag == "h1") {
		style.set(Parameter::create<ParameterName::MarginTop>(Metric(0.8f, Metric::Units::Em)), true);
		style.data.push_back(Parameter::create<ParameterName::FontSize>(uint8_t(32)));
		style.data.push_back(Parameter::create<ParameterName::FontWeight>(FontWeight::W200));
		style.data.push_back(Parameter::create<ParameterName::Opacity>(uint8_t(222)));

	} else if (tag == "h2") {
		style.set(Parameter::create<ParameterName::MarginTop>(Metric(0.8f, Metric::Units::Em)), true);
		style.data.push_back(Parameter::create<ParameterName::FontSize>(uint8_t(28)));
		style.data.push_back(Parameter::create<ParameterName::FontWeight>(FontWeight::W400));
		style.data.push_back(Parameter::create<ParameterName::Opacity>(uint8_t(222)));

	} else if (tag == "h3") {
		style.set(Parameter::create<ParameterName::MarginTop>(Metric(0.8f, Metric::Units::Em)), true);
		style.data.push_back(Parameter::create<ParameterName::FontSize>(FontSize::XXLarge));
		style.data.push_back(Parameter::create<ParameterName::FontWeight>(FontWeight::W400));
		style.data.push_back(Parameter::create<ParameterName::Opacity>(uint8_t(200)));

	} else if (tag == "h4") {
		style.set(Parameter::create<ParameterName::MarginTop>(Metric(0.8f, Metric::Units::Em)), true);
		style.data.push_back(Parameter::create<ParameterName::FontSize>(FontSize::XLarge));
		style.data.push_back(Parameter::create<ParameterName::FontWeight>(FontWeight::W500));
		style.data.push_back(Parameter::create<ParameterName::Opacity>(uint8_t(188)));

	} else if (tag == "h5") {
		style.set(Parameter::create<ParameterName::MarginTop>(Metric(0.8f, Metric::Units::Em)), true);
		style.data.push_back(Parameter::create<ParameterName::FontSize>(uint8_t(18)));
		style.data.push_back(Parameter::create<ParameterName::FontWeight>(FontWeight::W400));
		style.data.push_back(Parameter::create<ParameterName::Opacity>(uint8_t(222)));

	} else if (tag == "h6") {
		style.set(Parameter::create<ParameterName::MarginTop>(Metric(0.8f, Metric::Units::Em)), true);
		style.data.push_back(Parameter::create<ParameterName::FontSize>(FontSize::Large));
		style.data.push_back(Parameter::create<ParameterName::FontWeight>(FontWeight::W500));
		style.data.push_back(Parameter::create<ParameterName::Opacity>(uint8_t(216)));

	} else if (tag == "p") {
		style.data.push_back(Parameter::create<ParameterName::TextAlign>(TextAlign::Justify));
		style.data.push_back(Parameter::create<ParameterName::Hyphens>(Hyphens::Auto));

	} else if (tag == "hr") {
		style.data.push_back(Parameter::create<ParameterName::MarginTop>(Metric(0.5f, Metric::Units::Em)));
		style.data.push_back(Parameter::create<ParameterName::MarginRight>(Metric(0.5f, Metric::Units::Em)));
		style.data.push_back(Parameter::create<ParameterName::MarginBottom>(Metric(0.5f, Metric::Units::Em)));
		style.data.push_back(Parameter::create<ParameterName::MarginLeft>(Metric(0.5f, Metric::Units::Em)));
		style.data.push_back(Parameter::create<ParameterName::Height>(Metric(2, Metric::Units::Px)));
		style.data.push_back(Parameter::create<ParameterName::BackgroundColor>(Color4B(0, 0, 0, 127)));

	} else if (tag == "a") {
		style.data.push_back(Parameter::create<ParameterName::TextDecoration>(TextDecoration::Underline));
		style.data.push_back(Parameter::create<ParameterName::Color>(Color3B(0x0d, 0x47, 0xa1)));

	} else if (tag == "b" || tag == "strong") {
		style.data.push_back(Parameter::create<ParameterName::FontWeight>(FontWeight::Bold));

	} else if (tag == "s" || tag == "strike") {
		style.data.push_back(Parameter::create<ParameterName::TextDecoration>(TextDecoration::LineThrough));

	} else if (tag == "i" || tag == "em") {
		style.data.push_back(Parameter::create<ParameterName::FontStyle>(FontStyle::Italic));

	} else if (tag == "u") {
		style.data.push_back(Parameter::create<ParameterName::TextDecoration>(TextDecoration::Underline));

	} else if (tag == "nobr") {
		style.data.push_back(Parameter::create<ParameterName::WhiteSpace>(WhiteSpace::Nowrap));

	} else if (tag == "pre") {
		style.data.push_back(Parameter::create<ParameterName::MarginTop>(Metric(0.5f, Metric::Units::Em)));
		style.data.push_back(Parameter::create<ParameterName::MarginBottom>(Metric(0.5f, Metric::Units::Em)));
		style.data.push_back(Parameter::create<ParameterName::WhiteSpace>(WhiteSpace::Pre));
		style.data.push_back(Parameter::create<ParameterName::Display>(Display::Block));
		style.data.push_back(Parameter::create<ParameterName::BackgroundColor>(Color4B(228, 228, 228, 255)));

		style.data.push_back(Parameter::create<ParameterName::PaddingTop>(Metric(0.5f, Metric::Units::Em)));
		style.data.push_back(Parameter::create<ParameterName::PaddingLeft>(Metric(0.5f, Metric::Units::Em)));
		style.data.push_back(Parameter::create<ParameterName::PaddingBottom>(Metric(0.5f, Metric::Units::Em)));
		style.data.push_back(Parameter::create<ParameterName::PaddingRight>(Metric(0.5f, Metric::Units::Em)));

	} else if (tag == "code") {
		style.data.push_back(Parameter::create<ParameterName::FontFamily>(CssStringId("monospace"_hash)));
		style.data.push_back(Parameter::create<ParameterName::BackgroundColor>(Color4B(228, 228, 228, 255)));

	} else if (tag == "sub" || tag == "inf") {
		style.data.push_back(Parameter::create<ParameterName::VerticalAlign>(VerticalAlign::Sub));
		style.data.push_back(Parameter::create<ParameterName::FontSizeIncrement>(FontSizeIncrement::XSmaller));

	} else if (tag == "sup") {
		style.data.push_back(Parameter::create<ParameterName::VerticalAlign>(VerticalAlign::Super));
		style.data.push_back(Parameter::create<ParameterName::FontSizeIncrement>(FontSizeIncrement::XSmaller));

	} else if (tag == "body") {
		style.data.push_back(Parameter::create<ParameterName::MarginLeft>(Metric(0.8f, Metric::Units::Rem), MediaQuery::IsScreenLayout));
		style.data.push_back(Parameter::create<ParameterName::MarginRight>(Metric(0.8f, Metric::Units::Rem), MediaQuery::IsScreenLayout));

	} else if (tag == "br") {
		style.data.push_back(Parameter::create<ParameterName::WhiteSpace>(style::WhiteSpace::PreLine));
		style.data.push_back(Parameter::create<ParameterName::Display>(Display::Inline));

	} else if (tag == "li") {
		style.data.push_back(Parameter::create<ParameterName::Display>(Display::ListItem));
		style.data.push_back(Parameter::create<ParameterName::LineHeight>(Metric(1.2f, Metric::Units::Em)));

	} else if (tag == "ol") {
		style.data.push_back(Parameter::create<ParameterName::Display>(Display::Block));
		style.data.push_back(Parameter::create<ParameterName::ListStyleType>(ListStyleType::Decimal));
		style.data.push_back(Parameter::create<ParameterName::PaddingLeft>(Metric(2.5f, Metric::Units::Em)));
		style.data.push_back(Parameter::create<ParameterName::MarginTop>(Metric(0.25f, Metric::Units::Em)));
		style.data.push_back(Parameter::create<ParameterName::MarginBottom>(Metric(0.25f, Metric::Units::Em)));
		style.data.push_back(Parameter::create<ParameterName::XListStyleOffset>(Metric(0.6f, Metric::Units::Rem)));

	} else if (tag == "ul") {
		style.data.push_back(Parameter::create<ParameterName::Display>(Display::Block));
		if (parent == "li") {
			style.data.push_back(Parameter::create<ParameterName::ListStyleType>(ListStyleType::Circle));
		} else {
			style.data.push_back(Parameter::create<ParameterName::ListStyleType>(ListStyleType::Disc));
		}
		style.data.push_back(Parameter::create<ParameterName::PaddingLeft>(Metric(2.5f, Metric::Units::Em)));
		style.data.push_back(Parameter::create<ParameterName::MarginTop>(Metric(0.25f, Metric::Units::Em)));
		style.data.push_back(Parameter::create<ParameterName::MarginBottom>(Metric(0.25f, Metric::Units::Em)));
		style.data.push_back(Parameter::create<ParameterName::XListStyleOffset>(Metric(0.6f, Metric::Units::Rem)));

	} else if (tag == "img") {
		style.data.push_back(Parameter::create<ParameterName::BackgroundSizeWidth>(Metric(1.0, Metric::Units::Contain)));
		style.data.push_back(Parameter::create<ParameterName::BackgroundSizeHeight>(Metric(1.0, Metric::Units::Contain)));
		style.data.push_back(Parameter::create<ParameterName::PageBreakInside>(PageBreak::Avoid));
		style.data.push_back(Parameter::create<ParameterName::Display>(Display::InlineBlock));

	} else if (tag == "table") {
		style.data.push_back(Parameter::create<ParameterName::Display>(Display::Table));

	} else if (tag == "blockquote") {
		style.data.push_back(Parameter::create<ParameterName::Display>(Display::Block));
		style.data.push_back(Parameter::create<ParameterName::MarginTop>(Metric(0.5f, Metric::Units::Em)));
		style.data.push_back(Parameter::create<ParameterName::MarginBottom>(Metric(0.5f, Metric::Units::Em)));

		if (parent == "blockquote") {
			style.data.push_back(Parameter::create<ParameterName::PaddingLeft>(Metric(0.8f, Metric::Units::Rem)));
		} else {
			style.data.push_back(Parameter::create<ParameterName::MarginLeft>(Metric(1.5f, Metric::Units::Rem)));
			style.data.push_back(Parameter::create<ParameterName::MarginRight>(Metric(1.5f, Metric::Units::Rem)));
			style.data.push_back(Parameter::create<ParameterName::PaddingLeft>(Metric(1.0f, Metric::Units::Rem)));
		}

		style.data.push_back(Parameter::create<ParameterName::PaddingTop>(Metric(0.1f, Metric::Units::Rem)));
		style.data.push_back(Parameter::create<ParameterName::PaddingBottom>(Metric(0.3f, Metric::Units::Rem)));

		style.data.push_back(Parameter::create<ParameterName::BorderLeftColor>(Color4B(0, 0, 0, 64)));
		style.data.push_back(Parameter::create<ParameterName::BorderLeftWidth>(Metric(3, Metric::Units::Px)));
		style.data.push_back(Parameter::create<ParameterName::BorderLeftStyle>(BorderStyle::Solid));

	} else if (tag == "dl") {
		style.data.push_back(Parameter::create<ParameterName::Display>(Display::Block));
		style.data.push_back(Parameter::create<ParameterName::MarginTop>(Metric(1.0f, Metric::Units::Em)));
		style.data.push_back(Parameter::create<ParameterName::MarginBottom>(Metric(1.0f, Metric::Units::Em)));
		style.data.push_back(Parameter::create<ParameterName::MarginLeft>(Metric(1.5f, Metric::Units::Rem)));

	} else if (tag == "dt") {
		style.data.push_back(Parameter::create<ParameterName::Display>(Display::Block));
		style.data.push_back(Parameter::create<ParameterName::FontWeight>(FontWeight::W700));

	} else if (tag == "dd") {
		style.data.push_back(Parameter::create<ParameterName::Display>(Display::Block));
		style.data.push_back(Parameter::create<ParameterName::PaddingLeft>(Metric(1.0f, Metric::Units::Rem)));
		style.data.push_back(Parameter::create<ParameterName::MarginTop>(Metric(0.5f, Metric::Units::Em)));
		style.data.push_back(Parameter::create<ParameterName::MarginBottom>(Metric(0.5f, Metric::Units::Em)));

		style.data.push_back(Parameter::create<ParameterName::BorderLeftColor>(Color4B(0, 0, 0, 64)));
		style.data.push_back(Parameter::create<ParameterName::BorderLeftWidth>(Metric(2, Metric::Units::Px)));
		style.data.push_back(Parameter::create<ParameterName::BorderLeftStyle>(BorderStyle::Solid));

	} else if (tag == "figure") {
		style.data.push_back(Parameter::create<ParameterName::Display>(Display::Block));
		style.data.push_back(Parameter::create<ParameterName::MarginTop>(Metric(1.0f, Metric::Units::Em)));
		style.data.push_back(Parameter::create<ParameterName::MarginBottom>(Metric(0.5f, Metric::Units::Em)));

	} else if (tag == "figcaption") {
		style.data.push_back(Parameter::create<ParameterName::Display>(Display::Block));
		style.data.push_back(Parameter::create<ParameterName::MarginTop>(Metric(0.5f, Metric::Units::Em)));
		style.data.push_back(Parameter::create<ParameterName::MarginBottom>(Metric(0.5f, Metric::Units::Em)));
		style.data.push_back(Parameter::create<ParameterName::FontSize>(FontSize::Small));
		style.data.push_back(Parameter::create<ParameterName::FontWeight>(FontWeight::W500));
		style.data.push_back(Parameter::create<ParameterName::TextAlign>(TextAlign::Center));

	} else if (tag == "caption") {
		style.data.push_back(Parameter::create<ParameterName::TextAlign>(TextAlign::Center));

		style.data.push_back(Parameter::create<ParameterName::PaddingTop>(Metric(0.3f, Metric::Units::Rem)));
		style.data.push_back(Parameter::create<ParameterName::PaddingBottom>(Metric(0.3f, Metric::Units::Rem)));
		style.data.push_back(Parameter::create<ParameterName::PaddingLeft>(Metric(0.3f, Metric::Units::Rem)));
		style.data.push_back(Parameter::create<ParameterName::PaddingRight>(Metric(0.3f, Metric::Units::Rem)));
	}

	return style;
}

static bool ParameterList_readListStyleType(ParameterList &list, const StringView &value, MediaQueryId mediaQuary) {
	if (value.compare("none")) {
		list.set<ParameterName::ListStyleType>(ListStyleType::None, mediaQuary);
		return true;
	} else if (value.compare("circle")) {
		list.set<ParameterName::ListStyleType>(ListStyleType::Circle, mediaQuary);
		return true;
	} else if (value.compare("disc")) {
		list.set<ParameterName::ListStyleType>(ListStyleType::Disc, mediaQuary);
		return true;
	} else if (value.compare("square")) {
		list.set<ParameterName::ListStyleType>(ListStyleType::Square, mediaQuary);
		return true;
	} else if (value.compare("x-mdash")) {
		list.set<ParameterName::ListStyleType>(ListStyleType::XMdash, mediaQuary);
		return true;
	} else if (value.compare("decimal")) {
		list.set<ParameterName::ListStyleType>(ListStyleType::Decimal, mediaQuary);
		return true;
	} else if (value.compare("decimal-leading-zero")) {
		list.set<ParameterName::ListStyleType>(ListStyleType::DecimalLeadingZero, mediaQuary);
		return true;
	} else if (value.compare("lower-alpha") || value.compare("lower-latin")) {
		list.set<ParameterName::ListStyleType>(ListStyleType::LowerAlpha, mediaQuary);
		return true;
	} else if (value.compare("lower-greek")) {
		list.set<ParameterName::ListStyleType>(ListStyleType::LowerGreek, mediaQuary);
		return true;
	} else if (value.compare("lower-roman")) {
		list.set<ParameterName::ListStyleType>(ListStyleType::LowerRoman, mediaQuary);
		return true;
	} else if (value.compare("upper-alpha") || value.compare("upper-latin")) {
		list.set<ParameterName::ListStyleType>(ListStyleType::UpperAlpha, mediaQuary);
		return true;
	} else if (value.compare("upper-roman")) {
		list.set<ParameterName::ListStyleType>(ListStyleType::UpperRoman, mediaQuary);
		return true;
	}
	return false;
}

template <style::ParameterName Name>
static bool ParameterList_readBorderStyle(ParameterList &list, const StringView &value, MediaQueryId mediaQuary) {
	if (value.compare("none")) {
		list.set<Name>(BorderStyle::None, mediaQuary);
		return true;
	} else if (value.compare("solid")) {
		list.set<Name>(BorderStyle::Solid, mediaQuary);
		return true;
	} else if (value.compare("dotted")) {
		list.set<Name>(BorderStyle::Dotted, mediaQuary);
		return true;
	} else if (value.compare("dashed")) {
		list.set<Name>(BorderStyle::Dashed, mediaQuary);
		return true;
	}
	return false;
}

template <style::ParameterName Name>
static bool ParameterList_readBorderColor(ParameterList &list, const StringView &value, MediaQueryId mediaQuary) {
	if (value.compare("transparent")) {
		list.set<Name>(Color4B(0, 0, 0, 0), mediaQuary);
		return true;
	}

	Color4B color;
	if (readColor(value, color)) {
		list.set<Name>(color, mediaQuary);
		return true;
	}
	return false;
}

template <style::ParameterName Name>
static bool ParameterList_readBorderWidth(ParameterList &list, const StringView &value, MediaQueryId mediaQuary) {
	if (value.compare("thin")) {
		list.set<Name>(Metric(2.0f, Metric::Units::Px), mediaQuary);
		return true;
	} else if (value.compare("medium")) {
		list.set<Name>(Metric(4.0f, Metric::Units::Px), mediaQuary);
		return true;
	} else if (value.compare("thick")) {
		list.set<Name>(Metric(6.0f, Metric::Units::Px), mediaQuary);
		return true;
	}

	Metric v;
	if (readStyleValue(value, v)) {
		list.set<Name>(v, mediaQuary);
		return true;
	}
	return false;
}

template <style::ParameterName Style, style::ParameterName Color, style::ParameterName Width>
static void ParameterList_readBorder(ParameterList &list, const StringView &value, MediaQueryId mediaQuary) {
	value.split<StringView::CharGroup<CharGroupId::WhiteSpace>>([&] (const StringView &str) {
		if (!ParameterList_readBorderStyle<Style>(list, str, mediaQuary)) {
			if (!ParameterList_readBorderColor<Color>(list, str, mediaQuary)) {
				ParameterList_readBorderWidth<Width>(list, str, mediaQuary);
			}
		}
	});
}

template <typename T, typename Getter>
static void ParameterList_readQuadValue(const StringView &value, T &top, T &right, T &bottom, T &left, const Getter &g) {
	int count = 0;
	value.split<StringView::CharGroup<CharGroupId::WhiteSpace>>([&] (const StringView &r) {
		count ++;
		if (count == 1) {
			top = right = bottom = left = g(r);
		} else if (count == 2) {
			right = left = g(r);
		} else if (count == 3) {
			bottom = g(r);
		} else if (count == 4) {
			left = g(r);
		}
	});
}

void ParameterList::read(const StringView &name, const StringView &value, MediaQueryId mediaQuary) {
	if (name == "font-weight") {
		if (value.compare("bold")) {
			set<ParameterName::FontWeight>(FontWeight::Bold, mediaQuary);
		} else if (value.compare("normal")) {
			set<ParameterName::FontWeight>(FontWeight::Normal, mediaQuary);
		} else if (value.compare("100")) {
			set<ParameterName::FontWeight>(FontWeight::W100, mediaQuary);
		} else if (value.compare("200")) {
			set<ParameterName::FontWeight>(FontWeight::W200, mediaQuary);
		} else if (value.compare("300")) {
			set<ParameterName::FontWeight>(FontWeight::W300, mediaQuary);
		} else if (value.compare("400")) {
			set<ParameterName::FontWeight>(FontWeight::W400, mediaQuary);
		} else if (value.compare("500")) {
			set<ParameterName::FontWeight>(FontWeight::W500, mediaQuary);
		} else if (value.compare("600")) {
			set<ParameterName::FontWeight>(FontWeight::W600, mediaQuary);
		} else if (value.compare("700")) {
			set<ParameterName::FontWeight>(FontWeight::W700, mediaQuary);
		} else if (value.compare("800")) {
			set<ParameterName::FontWeight>(FontWeight::W800, mediaQuary);
		} else if (value.compare("900")) {
			set<ParameterName::FontWeight>(FontWeight::W900, mediaQuary);
		}
	} else if (name == "font-stretch") {
		if (value.compare("normal")) {
			set<ParameterName::FontStretch>(FontStretch::Normal, mediaQuary);
		} else if (value.compare("ultra-condensed")) {
			set<ParameterName::FontStretch>(FontStretch::UltraCondensed, mediaQuary);
		} else if (value.compare("extra-condensed")) {
			set<ParameterName::FontStretch>(FontStretch::ExtraCondensed, mediaQuary);
		} else if (value.compare("condensed")) {
			set<ParameterName::FontStretch>(FontStretch::Condensed, mediaQuary);
		} else if (value.compare("semi-condensed")) {
			set<ParameterName::FontStretch>(FontStretch::SemiCondensed, mediaQuary);
		} else if (value.compare("semi-expanded")) {
			set<ParameterName::FontStretch>(FontStretch::SemiExpanded, mediaQuary);
		} else if (value.compare("expanded")) {
			set<ParameterName::FontStretch>(FontStretch::Expanded, mediaQuary);
		} else if (value.compare("extra-expanded")) {
			set<ParameterName::FontStretch>(FontStretch::ExtraExpanded, mediaQuary);
		} else if (value.compare("ultra-expanded")) {
			set<ParameterName::FontStretch>(FontStretch::UltraExpanded, mediaQuary);
		}
	} else if (name == "font-style") {
		if (value.compare("italic")) {
			set<ParameterName::FontStyle>(FontStyle::Italic, mediaQuary);
		} else if (value.compare("normal")) {
			set<ParameterName::FontStyle>(FontStyle::Normal, mediaQuary);
		} else if (value.compare("oblique")) {
			set<ParameterName::FontStyle>(FontStyle::Oblique, mediaQuary);
		}
	} else if (name == "font-size") {
		if (value.compare("xx-small")) {
			set<ParameterName::FontSize>(FontSize::XXSmall, mediaQuary);
		} else if (value.compare("x-small")) {
			set<ParameterName::FontSize>(FontSize::XSmall, mediaQuary);
		} else if (value.compare("small")) {
			set<ParameterName::FontSize>(FontSize::Small, mediaQuary);
		} else if (value.compare("medium")) {
			set<ParameterName::FontSize>(FontSize::Medium, mediaQuary);
		} else if (value.compare("large")) {
			set<ParameterName::FontSize>(FontSize::Large, mediaQuary);
		} else if (value.compare("x-large")) {
			set<ParameterName::FontSize>(FontSize::XLarge, mediaQuary);
		} else if (value.compare("xx-large")) {
			set<ParameterName::FontSize>(FontSize::XXLarge, mediaQuary);
		} else if (value.compare("larger")) {
			set<ParameterName::FontSizeIncrement>(FontSizeIncrement::Larger, mediaQuary);
		} else if (value.compare("x-larger")) {
			set<ParameterName::FontSizeIncrement>(FontSizeIncrement::XLarger, mediaQuary);
		} else if (value.compare("smaller")) {
			set<ParameterName::FontSizeIncrement>(FontSizeIncrement::Smaller, mediaQuary);
		} else if (value.compare("x-smaller")) {
			set<ParameterName::FontSizeIncrement>(FontSizeIncrement::XSmaller, mediaQuary);
		} else {
			style::Metric fontSize;
			if (readStyleValue(value, fontSize)) {
				if (fontSize.metric == style::Metric::Units::Px) {
					set<ParameterName::FontSize>((uint8_t)fontSize.value, mediaQuary);
				} else if (fontSize.metric == style::Metric::Units::Em) {
					set<ParameterName::FontSize>((uint8_t)(FontSize::Medium * fontSize.value), mediaQuary);
				}
			}
		}
	} else if (name == "font-variant") {
		if (value.compare("small-caps")) {
			set<ParameterName::FontVariant>(FontVariant::SmallCaps, mediaQuary);
		} else if (value.compare("normal")) {
			set<ParameterName::FontVariant>(FontVariant::Normal, mediaQuary);
		}
	} else if (name == "text-decoration") {
		if (value.compare("underline")) {
			set<ParameterName::TextDecoration>(TextDecoration::Underline, mediaQuary);
		} else if (value.compare("line-through")) {
			set<ParameterName::TextDecoration>(TextDecoration::LineThrough, mediaQuary);
		} else if (value.compare("overline")) {
			set<ParameterName::TextDecoration>(TextDecoration::Overline, mediaQuary);
		} else if (value.compare("none")) {
			set<ParameterName::TextDecoration>(TextDecoration::None, mediaQuary);
		}
	} else if (name == "text-transform") {
		if (value.compare("uppercase")) {
			set<ParameterName::TextTransform>(TextTransform::Uppercase, mediaQuary);
		} else if (value.compare("lowercase")) {
			set<ParameterName::TextTransform>(TextTransform::Lowercase, mediaQuary);
		} else if (value.compare("none")) {
			set<ParameterName::TextTransform>(TextTransform::None, mediaQuary);
		}
	} else if (name == "text-align") {
		if (value.compare("left")) {
			set<ParameterName::TextAlign>(TextAlign::Left, mediaQuary);
		} else if (value.compare("right")) {
			set<ParameterName::TextAlign>(TextAlign::Right, mediaQuary);
		} else if (value.compare("center")) {
			set<ParameterName::TextAlign>(TextAlign::Center, mediaQuary);
		} else if (value.compare("justify")) {
			set<ParameterName::TextAlign>(TextAlign::Justify, mediaQuary);
		}
	} else if (name == "white-space") {
		if (value.compare("normal")) {
			set<ParameterName::WhiteSpace>(WhiteSpace::Normal, mediaQuary);
		} else if (value.compare("nowrap")) {
			set<ParameterName::WhiteSpace>(WhiteSpace::Nowrap, mediaQuary);
		} else if (value.compare("pre")) {
			set<ParameterName::WhiteSpace>(WhiteSpace::Pre, mediaQuary);
		} else if (value.compare("pre-line")) {
			set<ParameterName::WhiteSpace>(WhiteSpace::PreLine, mediaQuary);
		} else if (value.compare("pre-wrap")) {
			set<ParameterName::WhiteSpace>(WhiteSpace::PreWrap, mediaQuary);
		}
	} else if (name == "hyphens" || name == "-epub-hyphens") {
		if (value.compare("none")) {
			set<ParameterName::Hyphens>(Hyphens::None, mediaQuary);
		} else if (value.compare("manual")) {
			set<ParameterName::Hyphens>(Hyphens::Manual, mediaQuary);
		} else if (value.compare("auto")) {
			set<ParameterName::Hyphens>(Hyphens::Auto, mediaQuary);
		}
	} else if (name == "display") {
		if (value.compare("none")) {
			set<ParameterName::Display>(Display::None, mediaQuary);
		} else if (value.compare("run-in")) {
			set<ParameterName::Display>(Display::RunIn, mediaQuary);
		} else if (value.compare("list-item")) {
			set<ParameterName::Display>(Display::ListItem, mediaQuary);
		} else if (value.compare("inline")) {
			set<ParameterName::Display>(Display::Inline, mediaQuary);
		} else if (value.compare("inline-block")) {
			set<ParameterName::Display>(Display::InlineBlock, mediaQuary);
		} else if (value.compare("block")) {
			set<ParameterName::Display>(Display::Block, mediaQuary);
		}
	} else if (name == "list-style-type") {
		ParameterList_readListStyleType(*this, value, mediaQuary);
	} else if (name == "list-style-position") {
		if (value.compare("inside")) {
			set<ParameterName::ListStylePosition>(ListStylePosition::Inside, mediaQuary);
		} else if (value.compare("outside")) {
			set<ParameterName::ListStylePosition>(ListStylePosition::Outside, mediaQuary);
		}
	} else if (name == "list-style") {
		value.split<StringView::CharGroup<CharGroupId::WhiteSpace>>([&] (const StringView &r) {
			if (!ParameterList_readListStyleType(*this, r, mediaQuary)) {
				if (r.compare("inside")) {
					set<ParameterName::ListStylePosition>(ListStylePosition::Inside, mediaQuary);
				} else if (r.compare("outside")) {
					set<ParameterName::ListStylePosition>(ListStylePosition::Outside, mediaQuary);
				}
			}
		});
	} else if (name == "x-list-style-offset") {
		Metric data;
		if (readStyleValue(value, data)) {
			set<ParameterName::XListStyleOffset>(data, mediaQuary);
		}
	} else if (name == "float") {
		if (value.compare("none")) {
			set<ParameterName::Float>(Float::None, mediaQuary);
		} else if (value.compare("left")) {
			set<ParameterName::Float>(Float::Left, mediaQuary);
		} else if (value.compare("right")) {
			set<ParameterName::Float>(Float::Right, mediaQuary);
		}
	} else if (name == "clear") {
		if (value.compare("none")) {
			set<ParameterName::Clear>(Clear::None, mediaQuary);
		} else if (value.compare("left")) {
			set<ParameterName::Clear>(Clear::Left, mediaQuary);
		} else if (value.compare("right")) {
			set<ParameterName::Clear>(Clear::Right, mediaQuary);
		} else if (value.compare("both")) {
			set<ParameterName::Clear>(Clear::Both, mediaQuary);
		}
	} else if (name == "opacity") {
		StringView(value).readFloat().unwrap([&] (float data) {
			set<ParameterName::Opacity>((uint8_t)(math::clamp(data, 0.0f, 1.0f) * 255.0f), mediaQuary);
		});
	} else if (name == "color") {
		Color4B color;
		if (readColor(value, color)) {
			set<ParameterName::Color>(Color3B(color.r, color.g, color.b), mediaQuary);
			if (color.a != 255) {
				set<ParameterName::Opacity>(color.a, mediaQuary);
			}
		}
	} else if (name == "text-indent") {
		Metric data;
		if (readStyleValue(value, data)) {
			set<ParameterName::TextIndent>(data, mediaQuary);
		}
	} else if (name == "line-height") {
		Metric data;
		if (readStyleValue(value, data, false, true)) {
			set<ParameterName::LineHeight>(data, mediaQuary);
		}
	} else if (name == "margin") {
		Metric top, right, bottom, left;
		if (readStyleMargin(value, top, right, bottom, left)) {
			set<ParameterName::MarginTop>(top, mediaQuary);
			set<ParameterName::MarginRight>(right, mediaQuary);
			set<ParameterName::MarginBottom>(bottom, mediaQuary);
			set<ParameterName::MarginLeft>(left, mediaQuary);
		}
	} else if (name == "margin-top") {
		Metric data;
		if (readStyleValue(value, data)) {
			set<ParameterName::MarginTop>(data, mediaQuary);
		}
	} else if (name == "margin-right") {
		Metric data;
		if (readStyleValue(value, data)) {
			set<ParameterName::MarginRight>(data, mediaQuary);
		}
	} else if (name == "margin-bottom") {
		Metric data;
		if (readStyleValue(value, data)) {
			set<ParameterName::MarginBottom>(data, mediaQuary);
		}
	} else if (name == "margin-left") {
		Metric data;
		if (readStyleValue(value, data)) {
			set<ParameterName::MarginLeft>(data, mediaQuary);
		}
	} else if (name == "width") {
		Metric data;
		if (readStyleValue(value, data)) {
			set<ParameterName::Width>(data, mediaQuary);
		}
	} else if (name == "height") {
		Metric data;
		if (readStyleValue(value, data)) {
			set<ParameterName::Height>(data, mediaQuary);
		}
	} else if (name == "min-width") {
		Metric data;
		if (readStyleValue(value, data)) {
			set<ParameterName::MinWidth>(data, mediaQuary);
		}
	} else if (name == "min-height") {
		Metric data;
		if (readStyleValue(value, data)) {
			set<ParameterName::MinHeight>(data, mediaQuary);
		}
	} else if (name == "max-width") {
		Metric data;
		if (readStyleValue(value, data)) {
			set<ParameterName::MaxWidth>(data, mediaQuary);
		}
	} else if (name == "max-height") {
		Metric data;
		if (readStyleValue(value, data)) {
			set<ParameterName::MaxHeight>(data, mediaQuary);
		}
	} else if (name == "padding") {
		Metric top, right, bottom, left;
		if (readStyleMargin(value, top, right, bottom, left)) {
			set<ParameterName::PaddingTop>(top, mediaQuary);
			set<ParameterName::PaddingRight>(right, mediaQuary);
			set<ParameterName::PaddingBottom>(bottom, mediaQuary);
			set<ParameterName::PaddingLeft>(left, mediaQuary);
		}
	} else if (name == "padding-top") {
		Metric data;
		if (readStyleValue(value, data)) {
			set<ParameterName::PaddingTop>(data, mediaQuary);
		}
	} else if (name == "padding-right") {
		Metric data;
		if (readStyleValue(value, data)) {
			set<ParameterName::PaddingRight>(data, mediaQuary);
		}
	} else if (name == "padding-bottom") {
		Metric data;
		if (readStyleValue(value, data)) {
			set<ParameterName::PaddingBottom>(data, mediaQuary);
		}
	} else if (name == "padding-left") {
		Metric data;
		if (readStyleValue(value, data)) {
			set<ParameterName::PaddingLeft>(data, mediaQuary);
		}
	} else if (name == "font-family") {
		if (value.compare("default") || value.compare("serif")) {
			set<ParameterName::FontFamily>(CssStringNone(), mediaQuary);
		} else {
			CssStringId data;
			if (readStringValue("", value, data)) {
				set<ParameterName::FontFamily>(data, mediaQuary);
			}
		}
	} else if (name == "background-color") {
		if (value.compare("transparent")) {
			set<ParameterName::BackgroundColor>(Color4B(0, 0, 0, 0), mediaQuary);
		} else {
			Color4B color;
			if (readColor(value, color)) {
				set<ParameterName::BackgroundColor>(color, mediaQuary);
			}
		}
	} else if (name == "background-image") {
		if (value.compare("none")) {
			set<ParameterName::BackgroundImage>(CssStringNone(), mediaQuary);
		} else {
			CssStringId data;
			if (readStringValue("url", value, data)) {
				set<ParameterName::BackgroundImage>(data, mediaQuary);
			}
		}
	} else if (name == "background-position") {
		StringView first, second;
		Metric x, y;
		bool validX = false, validY = false, swapValues = false;
		if (splitValue(value, first, second)) {
			bool parseError = false;
			bool firstWasCenter = false;
			if (first.compare("center")) {
				x.value = 0.5f; x.metric = Metric::Units::Percent; validX = true; firstWasCenter = true;
			} else if (first.compare("left")) {
				x.value = 0.0f; x.metric = Metric::Units::Percent; validX = true;
			} else if (first.compare("right")) {
				x.value = 1.0f; x.metric = Metric::Units::Percent; validX = true;
			} else if (first.compare("top")) {
				x.value = 0.0f; x.metric = Metric::Units::Percent; validX = true; swapValues = true;
			} else if (first.compare("bottom")) {
				x.value = 1.0f; x.metric = Metric::Units::Percent; validX = true; swapValues = true;
			}

			if (second.compare("center")) {
				y.value = 0.5f; y.metric = Metric::Units::Percent; validY = true;
			} else if (second.compare("left")) {
				if (swapValues || firstWasCenter) {
					y.value = 0.0f; y.metric = Metric::Units::Percent; validY = true; swapValues = true;
				} else {
					parseError = true;
				}
			} else if (second.compare("right")) {
				if (swapValues || firstWasCenter) {
					y.value = 1.0f; y.metric = Metric::Units::Percent; validY = true; swapValues = true;
				} else {
					parseError = true;
				}
			} else if (second.compare("top")) {
				if (!swapValues) {
					y.value = 0.0f; y.metric = Metric::Units::Percent; validY = true; swapValues = true;
				} else {
					parseError = true;
				}
			} else if (second.compare("bottom")) {
				if (!swapValues) {
					y.value = 1.0f; y.metric = Metric::Units::Percent; validY = true; swapValues = true;
				} else {
					parseError = true;
				}
			}

			if (!parseError && !validX) {
				if (readStyleValue(first, x)) {
					validX = true;
				}
			}

			if (!parseError && !validY) {
				if (readStyleValue(second, y)) {
					validY = true;
				}
			}
		} else {
			if (value.compare("center")) {
				x.value = 0.5f; x.metric = Metric::Units::Percent; validX = true;
				y.value = 0.5f; y.metric = Metric::Units::Percent; validY = true;
			} else if (value.compare("top")) {
				x.value = 0.5f; x.metric = Metric::Units::Percent; validX = true;
				y.value = 0.0f; y.metric = Metric::Units::Percent; validY = true;
			} else if (value.compare("right")) {
				x.value = 1.0f; x.metric = Metric::Units::Percent; validX = true;
				y.value = 0.5f; y.metric = Metric::Units::Percent; validY = true;
			} else if (value.compare("bottom")) {
				x.value = 0.5f; x.metric = Metric::Units::Percent; validX = true;
				y.value = 1.0f; y.metric = Metric::Units::Percent; validY = true;
			} else if (value.compare("left")) {
				x.value = 0.0f; x.metric = Metric::Units::Percent; validX = true;
				y.value = 0.5f; y.metric = Metric::Units::Percent; validY = true;
			}
		}
		if (validX && validY) {
			if (!swapValues) {
				set<ParameterName::BackgroundPositionX>(x, mediaQuary);
				set<ParameterName::BackgroundPositionY>(y, mediaQuary);
			} else {
				set<ParameterName::BackgroundPositionX>(y, mediaQuary);
				set<ParameterName::BackgroundPositionY>(x, mediaQuary);
			}

		}
	} else if (name == "background-repeat") {
		if (value.compare("no-repeat")) {
			set<ParameterName::BackgroundRepeat>(BackgroundRepeat::NoRepeat, mediaQuary);
		} else if (value.compare("repeat")) {
			set<ParameterName::BackgroundRepeat>(BackgroundRepeat::Repeat, mediaQuary);
		} else if (value.compare("repeat-x")) {
			set<ParameterName::BackgroundRepeat>(BackgroundRepeat::RepeatX, mediaQuary);
		} else if (value.compare("repeat-y")) {
			set<ParameterName::BackgroundRepeat>(BackgroundRepeat::RepeatY, mediaQuary);
		}
	} else if (name == "background-size") {
		StringView first, second;
		Metric width, height;
		bool validWidth = false, validHeight = false;
		if (value.compare("contain")) {
			width.metric = Metric::Units::Contain; validWidth = true;
			height.metric = Metric::Units::Contain; validHeight = true;
		} else if (value.compare("cover")) {
			width.metric = Metric::Units::Cover; validWidth = true;
			height.metric = Metric::Units::Cover; validHeight = true;
		} else if (splitValue(value, first, second)) {
			if (first.compare("contain")) {
				width.metric = Metric::Units::Contain; validWidth = true;
			} else if (first.compare("cover")) {
				width.metric = Metric::Units::Cover; validWidth = true;
			} else if (readStyleValue(first, width)) {
				validWidth = true;
			}

			if (second.compare("contain")) {
				height.metric = Metric::Units::Contain; validWidth = true;
			} else if (second.compare("cover")) {
				height.metric = Metric::Units::Cover; validWidth = true;
			} else if (readStyleValue(second, height)) {
				validHeight = true;
			}
		} else if (readStyleValue(value, width)) {
			height.metric = Metric::Units::Auto; validWidth = true; validHeight = true;
		}

		if (validWidth && validHeight) {
			set<ParameterName::BackgroundSizeWidth>(width, mediaQuary);
			set<ParameterName::BackgroundSizeHeight>(height, mediaQuary);
		}
	} else if (name == "vertical-align") {
		if (value.compare("baseline")) {
			set<ParameterName::VerticalAlign>(VerticalAlign::Baseline, mediaQuary);
		} else if (value.compare("sub")) {
			set<ParameterName::VerticalAlign>(VerticalAlign::Sub, mediaQuary);
		} else if (value.compare("super")) {
			set<ParameterName::VerticalAlign>(VerticalAlign::Super, mediaQuary);
		} else if (value.compare("middle")) {
			set<ParameterName::VerticalAlign>(VerticalAlign::Middle, mediaQuary);
		} else if (value.compare("top")) {
			set<ParameterName::VerticalAlign>(VerticalAlign::Top, mediaQuary);
		} else if (value.compare("bottom")) {
			set<ParameterName::VerticalAlign>(VerticalAlign::Bottom, mediaQuary);
		}
	} else if (name == "outline") {
		ParameterList_readBorder<ParameterName::OutlineStyle, ParameterName::OutlineColor, ParameterName::OutlineWidth>(*this, value, mediaQuary);
	} else if (name == "outline-style") {
		ParameterList_readBorderStyle<ParameterName::OutlineStyle>(*this, value, mediaQuary);
	} else if (name == "outline-color") {
		ParameterList_readBorderColor<ParameterName::OutlineColor>(*this, value, mediaQuary);
	} else if (name == "outline-width") {
		ParameterList_readBorderWidth<ParameterName::OutlineWidth>(*this, value, mediaQuary);
	} else if (name == "border-top") {
		ParameterList_readBorder<ParameterName::BorderTopStyle, ParameterName::BorderTopColor, ParameterName::BorderTopWidth>(*this, value, mediaQuary);
	} else if (name == "border-top-style") {
		ParameterList_readBorderStyle<ParameterName::BorderTopStyle>(*this, value, mediaQuary);
	} else if (name == "border-top-color") {
		ParameterList_readBorderColor<ParameterName::BorderTopColor>(*this, value, mediaQuary);
	} else if (name == "border-top-width") {
		ParameterList_readBorderWidth<ParameterName::BorderTopWidth>(*this, value, mediaQuary);
	} else if (name == "border-right") {
		ParameterList_readBorder<ParameterName::BorderRightStyle, ParameterName::BorderRightColor, ParameterName::BorderRightWidth>(*this, value, mediaQuary);
	} else if (name == "border-right-style") {
		ParameterList_readBorderStyle<ParameterName::BorderRightStyle>(*this, value, mediaQuary);
	} else if (name == "border-right-color") {
		ParameterList_readBorderColor<ParameterName::BorderRightColor>(*this, value, mediaQuary);
	} else if (name == "border-right-width") {
		ParameterList_readBorderWidth<ParameterName::BorderRightWidth>(*this, value, mediaQuary);
	} else if (name == "border-bottom") {
		ParameterList_readBorder<ParameterName::BorderBottomStyle, ParameterName::BorderBottomColor, ParameterName::BorderBottomWidth>(*this, value, mediaQuary);
	} else if (name == "border-bottom-style") {
		ParameterList_readBorderStyle<ParameterName::BorderBottomStyle>(*this, value, mediaQuary);
	} else if (name == "border-bottom-color") {
		ParameterList_readBorderColor<ParameterName::BorderBottomColor>(*this, value, mediaQuary);
	} else if (name == "border-bottom-width") {
		ParameterList_readBorderWidth<ParameterName::BorderBottomWidth>(*this, value, mediaQuary);
	} else if (name == "border-left") {
		ParameterList_readBorder<ParameterName::BorderLeftStyle, ParameterName::BorderLeftColor, ParameterName::BorderLeftWidth>(*this, value, mediaQuary);
	} else if (name == "border-left-style") {
		ParameterList_readBorderStyle<ParameterName::BorderLeftStyle>(*this, value, mediaQuary);
	} else if (name == "border-left-color") {
		ParameterList_readBorderColor<ParameterName::BorderLeftColor>(*this, value, mediaQuary);
	} else if (name == "border-left-width") {
		ParameterList_readBorderWidth<ParameterName::BorderLeftWidth>(*this, value, mediaQuary);
	} else if (name == "border-style" && !value.empty()) {
		BorderStyle top, right, bottom, left;
		ParameterList_readQuadValue(value, top, right, bottom, left, [&] (const StringView &v) -> BorderStyle {
			if (v.compare("solid")) {
				return BorderStyle::Solid;
			} else if (v.compare("dotted")) {
				return BorderStyle::Dotted;
			} else if (v.compare("dashed")) {
				return BorderStyle::Dashed;
			}
			return BorderStyle::None;
		});
		set<ParameterName::BorderTopStyle>(top, mediaQuary);
		set<ParameterName::BorderRightStyle>(right, mediaQuary);
		set<ParameterName::BorderBottomStyle>(bottom, mediaQuary);
		set<ParameterName::BorderLeftStyle>(left, mediaQuary);
	} else if (name == "border-color" && !value.empty()) {
		Color4B top, right, bottom, left;
		ParameterList_readQuadValue(value, top, right, bottom, left, [&] (const StringView &v) -> Color4B {
			if (v.compare("transparent")) {
				return Color4B(0, 0, 0, 0);
			} else {
				Color4B color(0, 0, 0, 0);
				readColor(v, color);
				return color;
			}
		});
		set<ParameterName::BorderTopColor>(top, mediaQuary);
		set<ParameterName::BorderRightColor>(right, mediaQuary);
		set<ParameterName::BorderBottomColor>(bottom, mediaQuary);
		set<ParameterName::BorderLeftColor>(left, mediaQuary);
	} else if (name == "border-width" && !value.empty()) {
		Metric top, right, bottom, left;
		ParameterList_readQuadValue(value, top, right, bottom, left, [&] (const StringView &v) -> Metric {
			if (v.compare("thin")) {
				return Metric(2.0f, Metric::Units::Px);
			} else if (v.compare("medium")) {
				return Metric(4.0f, Metric::Units::Px);
			} else if (v.compare("thick")) {
				return Metric(6.0f, Metric::Units::Px);
			}

			Metric m(0.0f, Metric::Units::Px);
			readStyleValue(v, m);
			return m;
		});
		set<ParameterName::BorderTopWidth>(top, mediaQuary);
		set<ParameterName::BorderRightWidth>(right, mediaQuary);
		set<ParameterName::BorderBottomWidth>(bottom, mediaQuary);
		set<ParameterName::BorderLeftWidth>(left, mediaQuary);
	} else if (name == "border" && !value.empty()) {
		BorderStyle style = BorderStyle::None;
		Metric width(0.0f, Metric::Units::Px);
		Color4B color(0, 0, 0, 0);
		value.split<StringView::CharGroup<CharGroupId::WhiteSpace>>([&] (const StringView &r) {
			if (r.compare("solid")) {
				style = BorderStyle::Solid;
			} else if (r.compare("dotted")) {
				style = BorderStyle::Dotted;
			} else if (r.compare("dashed")) {
				style = BorderStyle::Dashed;
			} else if (r.compare("none")) {
				style = BorderStyle::None;
			} else if (r.compare("transparent")) {
				color = Color4B(0, 0, 0, 0);
			} else if (r.compare("thin")) {
				width = Metric(2.0f, Metric::Units::Px);
			} else if (r.compare("medium")) {
				width = Metric(4.0f, Metric::Units::Px);
			} else if (r.compare("thick")) {
				width = Metric(6.0f, Metric::Units::Px);
			} else if (!readColor(r, color)) {
				readStyleValue(r, width);
			}
		});
		set<ParameterName::BorderTopStyle>(style, mediaQuary);
		set<ParameterName::BorderRightStyle>(style, mediaQuary);
		set<ParameterName::BorderBottomStyle>(style, mediaQuary);
		set<ParameterName::BorderLeftStyle>(style, mediaQuary);
		set<ParameterName::BorderTopColor>(color, mediaQuary);
		set<ParameterName::BorderRightColor>(color, mediaQuary);
		set<ParameterName::BorderBottomColor>(color, mediaQuary);
		set<ParameterName::BorderLeftColor>(color, mediaQuary);
		set<ParameterName::BorderTopWidth>(width, mediaQuary);
		set<ParameterName::BorderRightWidth>(width, mediaQuary);
		set<ParameterName::BorderBottomWidth>(width, mediaQuary);
		set<ParameterName::BorderLeftWidth>(width, mediaQuary);
	} else if (name == "border-collapse" && !value.empty()) {
		if (value.compare("collapse")) {
			set<ParameterName::BorderCollapse>(BorderCollapse::Collapse, mediaQuary);
		} else if (value.compare("separate")) {
			set<ParameterName::BorderCollapse>(BorderCollapse::Separate, mediaQuary);
		}
	} else if (name == "caption-side" && !value.empty()) {
		if (value.compare("top")) {
			set<ParameterName::CaptionSide>(CaptionSide::Top, mediaQuary);
		} else if (value.compare("bottom")) {
			set<ParameterName::CaptionSide>(CaptionSide::Bottom, mediaQuary);
		}
	} else if (name == "page-break-after") {
		if (value.compare("always")) {
			set<ParameterName::PageBreakAfter>(PageBreak::Always, mediaQuary);
		} else if (value.compare("auto")) {
			set<ParameterName::PageBreakAfter>(PageBreak::Auto, mediaQuary);
		} else if (value.compare("avoid")) {
			set<ParameterName::PageBreakAfter>(PageBreak::Avoid, mediaQuary);
		} else if (value.compare("left")) {
			set<ParameterName::PageBreakAfter>(PageBreak::Left, mediaQuary);
		} else if (value.compare("right")) {
			set<ParameterName::PageBreakAfter>(PageBreak::Right, mediaQuary);
		}
	} else if (name == "page-break-before") {
		if (value.compare("always")) {
			set<ParameterName::PageBreakBefore>(PageBreak::Always, mediaQuary);
		} else if (value.compare("auto")) {
			set<ParameterName::PageBreakBefore>(PageBreak::Auto, mediaQuary);
		} else if (value.compare("avoid")) {
			set<ParameterName::PageBreakBefore>(PageBreak::Avoid, mediaQuary);
		} else if (value.compare("left")) {
			set<ParameterName::PageBreakBefore>(PageBreak::Left, mediaQuary);
		} else if (value.compare("right")) {
			set<ParameterName::PageBreakBefore>(PageBreak::Right, mediaQuary);
		}
	} else if (name == "page-break-inside") {
		if (value.compare("auto")) {
			set<ParameterName::PageBreakInside>(PageBreak::Auto, mediaQuary);
		} else if (value.compare("avoid")) {
			set<ParameterName::PageBreakInside>(PageBreak::Avoid, mediaQuary);
		}
	}
}

void ParameterList::read(const StyleVec &vec, MediaQueryId mediaQuary) {
	for (auto &it : vec) {
		read(it.first, StringView(it.second), mediaQuary);
	}
}

void ParameterList::merge(const ParameterList &style, bool inherit) {
	for (auto &it : style.data) {
		if (!inherit || ParameterList::isInheritable(it.name)) {
			set(it, true);
		}
	}
}

void ParameterList::merge(const ParameterList &style, const Vector<bool> &media, bool inherit) {
	for (auto &it : style.data) {
		if ((!inherit || ParameterList::isInheritable(it.name))
				&& (it.mediaQuery == MediaQueryNone() || media.at(it.mediaQuery))) {
			set(Parameter(it, MediaQueryNone()), true);
		}
	}
}

void ParameterList::merge(const StyleVec &vec) {
	ParameterList list;
	list.read(vec);
	merge(list);
}

Vector<Parameter> ParameterList::get(ParameterName name) const {
	Vector<Parameter> ret;
	for (auto &it : data) {
		if (it.name == name) {
			ret.push_back(it);
		}
	}
	return ret;
}

Vector<Parameter> ParameterList::get(ParameterName name, const RendererInterface *iface) const {
	Vector<Parameter> ret; ret.reserve(1);
	for (auto &it : data) {
		if (it.name == name && (it.mediaQuery == MediaQueryNone() || iface->resolveMediaQuery(it.mediaQuery))) {
			if (ret.empty()) {
				ret.push_back(it);
			} else {
				ret[0] = it;
			}
		}
	}
	return ret;
}

} // namespace style

NS_LAYOUT_END
