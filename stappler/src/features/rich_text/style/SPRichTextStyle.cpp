/*
 * SPRichTextStyle.cpp
 *
 *  Created on: 21 апр. 2015 г.
 *      Author: sbkarr
 */

#include "SPDefine.h"
#include "SPRichTextStyleParameters.hpp"
#include "SPRichTextParser.h"
#include "SPString.h"

NS_SP_EXT_BEGIN(rich_text)

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
template<> void Parameter::set<ParameterName::FontSizeNumeric, Size>(const style::Size &v) { value.sizeValue = v; }
template<> void Parameter::set<ParameterName::FontSizeIncrement, FontSizeIncrement>(const FontSizeIncrement &v) { value.fontSizeIncrement = v; }
template<> void Parameter::set<ParameterName::TextTransform, TextTransform>(const TextTransform &v) { value.textTransform = v; }
template<> void Parameter::set<ParameterName::TextDecoration, TextDecoration>(const TextDecoration &v) { value.textDecoration = v; }
template<> void Parameter::set<ParameterName::TextAlign, TextAlign>(const TextAlign &v) { value.textAlign = v; }
template<> void Parameter::set<ParameterName::WhiteSpace, WhiteSpace>(const WhiteSpace &v) { value.whiteSpace = v; }
template<> void Parameter::set<ParameterName::Hyphens, Hyphens>(const Hyphens &v) { value.hyphens = v; }
template<> void Parameter::set<ParameterName::Display, Display>(const Display &v) { value.display = v; }
template<> void Parameter::set<ParameterName::ListStyleType, ListStyleType>(const ListStyleType &v) { value.listStyleType = v; }
template<> void Parameter::set<ParameterName::ListStylePosition, ListStylePosition>(const ListStylePosition &v) { value.listStylePosition = v; }
template<> void Parameter::set<ParameterName::XListStyleOffset, Size>(const Size &v) { value.sizeValue = v; }
template<> void Parameter::set<ParameterName::Float, Float>(const Float &v) { value.floating = v; }
template<> void Parameter::set<ParameterName::Clear, Clear>(const Clear &v) { value.clear = v; }
template<> void Parameter::set<ParameterName::Color, cocos2d::Color3B>(const cocos2d::Color3B &v) { value.color = v; }
template<> void Parameter::set<ParameterName::Opacity, uint8_t>(const uint8_t &v) { value.opacity = v; }
template<> void Parameter::set<ParameterName::TextIndent, Size>(const Size &v) { value.sizeValue = v; }
template<> void Parameter::set<ParameterName::LineHeight, Size>(const Size &v) { value.sizeValue = v; }
template<> void Parameter::set<ParameterName::MarginTop, Size>(const Size &v) { value.sizeValue = v; }
template<> void Parameter::set<ParameterName::MarginRight, Size>(const Size &v) { value.sizeValue = v; }
template<> void Parameter::set<ParameterName::MarginBottom, Size>(const Size &v) { value.sizeValue = v; }
template<> void Parameter::set<ParameterName::MarginLeft, Size>(const Size &v) { value.sizeValue = v; }
template<> void Parameter::set<ParameterName::Width, Size>(const Size &v) { value.sizeValue = v; }
template<> void Parameter::set<ParameterName::Height, Size>(const Size &v) { value.sizeValue = v; }
template<> void Parameter::set<ParameterName::MinWidth, Size>(const Size &v) { value.sizeValue = v; }
template<> void Parameter::set<ParameterName::MinHeight, Size>(const Size &v) { value.sizeValue = v; }
template<> void Parameter::set<ParameterName::MaxWidth, Size>(const Size &v) { value.sizeValue = v; }
template<> void Parameter::set<ParameterName::MaxHeight, Size>(const Size &v) { value.sizeValue = v; }
template<> void Parameter::set<ParameterName::PaddingTop, Size>(const Size &v) { value.sizeValue = v; }
template<> void Parameter::set<ParameterName::PaddingRight, Size>(const Size &v) { value.sizeValue = v; }
template<> void Parameter::set<ParameterName::PaddingBottom, Size>(const Size &v) { value.sizeValue = v; }
template<> void Parameter::set<ParameterName::PaddingLeft, Size>(const Size &v) { value.sizeValue = v; }
template<> void Parameter::set<ParameterName::FontFamily, CssStringId>(const CssStringId &v) { value.stringId = v; }
template<> void Parameter::set<ParameterName::BackgroundColor, cocos2d::Color4B>(const cocos2d::Color4B &v) { value.color4 = v; }
template<> void Parameter::set<ParameterName::BackgroundImage, CssStringId>(const CssStringId &v) { value.stringId = v; }
template<> void Parameter::set<ParameterName::BackgroundPositionX, Size>(const Size &v) { value.sizeValue = v; }
template<> void Parameter::set<ParameterName::BackgroundPositionY, Size>(const Size &v) { value.sizeValue = v; }
template<> void Parameter::set<ParameterName::BackgroundRepeat, BackgroundRepeat>(const BackgroundRepeat &v) { value.backgroundRepeat = v; }
template<> void Parameter::set<ParameterName::BackgroundSizeWidth, Size>(const Size &v) { value.sizeValue = v; }
template<> void Parameter::set<ParameterName::BackgroundSizeHeight, Size>(const Size &v) { value.sizeValue = v; }
template<> void Parameter::set<ParameterName::VerticalAlign, VerticalAlign>(const VerticalAlign &v) { value.verticalAlign = v; }
template<> void Parameter::set<ParameterName::BorderTopStyle, BorderStyle>(const BorderStyle &v) { value.borderStyle = v; }
template<> void Parameter::set<ParameterName::BorderTopWidth, Size>(const Size &v) { value.sizeValue = v; }
template<> void Parameter::set<ParameterName::BorderTopColor, cocos2d::Color4B>(const cocos2d::Color4B &v) { value.color4 = v; }
template<> void Parameter::set<ParameterName::BorderRightStyle, BorderStyle>(const BorderStyle &v) { value.borderStyle = v; }
template<> void Parameter::set<ParameterName::BorderRightWidth, Size>(const Size &v) { value.sizeValue = v; }
template<> void Parameter::set<ParameterName::BorderRightColor, cocos2d::Color4B>(const cocos2d::Color4B &v) { value.color4 = v; }
template<> void Parameter::set<ParameterName::BorderBottomStyle, BorderStyle>(const BorderStyle &v) { value.borderStyle = v; }
template<> void Parameter::set<ParameterName::BorderBottomWidth, Size>(const Size &v) { value.sizeValue = v; }
template<> void Parameter::set<ParameterName::BorderBottomColor, cocos2d::Color4B>(const cocos2d::Color4B &v) { value.color4 = v; }
template<> void Parameter::set<ParameterName::BorderLeftStyle, BorderStyle>(const BorderStyle &v) { value.borderStyle = v; }
template<> void Parameter::set<ParameterName::BorderLeftWidth, Size>(const Size &v) { value.sizeValue = v; }
template<> void Parameter::set<ParameterName::BorderLeftColor, cocos2d::Color4B>(const cocos2d::Color4B &v) { value.color4 = v; }
template<> void Parameter::set<ParameterName::OutlineStyle, BorderStyle>(const BorderStyle &v) { value.borderStyle = v; }
template<> void Parameter::set<ParameterName::OutlineWidth, Size>(const Size &v) { value.sizeValue = v; }
template<> void Parameter::set<ParameterName::OutlineColor, cocos2d::Color4B>(const cocos2d::Color4B &v) { value.color4 = v; }
template<> void Parameter::set<ParameterName::PageBreakAfter, PageBreak>(const PageBreak &v) { value.pageBreak = v; }
template<> void Parameter::set<ParameterName::PageBreakBefore, PageBreak>(const PageBreak &v) { value.pageBreak = v; }
template<> void Parameter::set<ParameterName::PageBreakInside, PageBreak>(const PageBreak &v) { value.pageBreak = v; }

void ParameterList::set(const Parameter &p, bool force) {
	if (p.mediaQuery != MediaQueryNone() && !isAllowedForMediaQuery(p.name)) {
		return;
	}

	data.push_back(p);
}

ParameterList getStyleForTag(const String &tag, Tag::Type type) {
	ParameterList style;

	if (tag == "p" || tag == "h1" || tag == "h2" || tag == "h3" || tag == "h4" || tag == "h5" || tag == "h6") {
		style.data.push_back(Parameter::create<ParameterName::MarginTop>(Size(10, Size::Metric::Px)));
		style.data.push_back(Parameter::create<ParameterName::MarginBottom>(Size(10, Size::Metric::Px)));
		style.data.push_back(Parameter::create<ParameterName::TextIndent>(Size(20, Size::Metric::Px)));
		style.data.push_back(Parameter::create<ParameterName::Display>(Display::Block));
	}

	if (tag == "span" || tag == "a" || tag == "strong" || tag == "em" || tag == "nobr"
			|| tag == "sub" || tag == "sup" || tag == "inf" || tag == "b"
			|| tag == "i" || tag == "u" || tag == "nobr") {
		style.data.push_back(Parameter::create<ParameterName::Display>(Display::Inline));
	}

	if (tag == "h1") {
		style.set(Parameter::create<ParameterName::MarginTop>(Size(20, Size::Metric::Px)), true);
		style.data.push_back(Parameter::create<ParameterName::FontSize>(FontSize::XXLarge));
	} else if (tag == "h2") {
		style.set(Parameter::create<ParameterName::MarginTop>(Size(20, Size::Metric::Px)), true);
		style.data.push_back(Parameter::create<ParameterName::FontSize>(FontSize::XLarge));
	} else if (tag == "h3") {
		style.set(Parameter::create<ParameterName::MarginTop>(Size(20, Size::Metric::Px)), true);
		style.data.push_back(Parameter::create<ParameterName::FontSize>(FontSize::Large));
	} else if (tag == "hr") {
		style.data.push_back(Parameter::create<ParameterName::MarginTop>(Size(10, Size::Metric::Px)));
		style.data.push_back(Parameter::create<ParameterName::MarginRight>(Size(10, Size::Metric::Px)));
		style.data.push_back(Parameter::create<ParameterName::MarginBottom>(Size(10, Size::Metric::Px)));
		style.data.push_back(Parameter::create<ParameterName::MarginLeft>(Size(10, Size::Metric::Px)));
		style.data.push_back(Parameter::create<ParameterName::Height>(Size(2, Size::Metric::Px)));
		style.data.push_back(Parameter::create<ParameterName::BackgroundColor>(cocos2d::Color4B(0, 0, 0, 127)));
	} else if (tag == "a") {
		style.data.push_back(Parameter::create<ParameterName::TextDecoration>(TextDecoration::Underline));
		style.data.push_back(Parameter::create<ParameterName::Color>(cocos2d::Color3B(0x0d, 0x47, 0xa1)));
	} else if (tag == "b" || tag == "strong") {
		style.data.push_back(Parameter::create<ParameterName::FontWeight>(FontWeight::Bold));
	} else if (tag == "i" || tag == "em") {
		style.data.push_back(Parameter::create<ParameterName::FontStyle>(FontStyle::Italic));
	} else if (tag == "u") {
		style.data.push_back(Parameter::create<ParameterName::TextDecoration>(TextDecoration::Underline));
	} else if (tag == "nobr") {
		style.data.push_back(Parameter::create<ParameterName::WhiteSpace>(WhiteSpace::Nowrap));
	} else if (tag == "pre") {
		style.data.push_back(Parameter::create<ParameterName::WhiteSpace>(WhiteSpace::Pre));
	} else if (tag == "sub" || tag == "inf") {
		style.data.push_back(Parameter::create<ParameterName::VerticalAlign>(VerticalAlign::Sub));
		style.data.push_back(Parameter::create<ParameterName::FontSizeIncrement>(FontSizeIncrement::XSmaller));
	} else if (tag == "sup") {
		style.data.push_back(Parameter::create<ParameterName::VerticalAlign>(VerticalAlign::Super));
		style.data.push_back(Parameter::create<ParameterName::FontSizeIncrement>(FontSizeIncrement::XSmaller));
	} else if (tag == "body") {
		style.data.push_back(Parameter::create<ParameterName::MarginLeft>(Size(16, Size::Metric::Px), MediaQuery::IsScreenLayout));
		style.data.push_back(Parameter::create<ParameterName::MarginRight>(Size(16, Size::Metric::Px), MediaQuery::IsScreenLayout));
	} else if (tag == "br") {
		style.data.push_back(Parameter::create<ParameterName::WhiteSpace>(style::WhiteSpace::PreLine));
		style.data.push_back(Parameter::create<ParameterName::Display>(Display::Inline));
	} else if (tag == "li") {
		style.data.push_back(Parameter::create<ParameterName::Display>(Display::ListItem));
	} else if (tag == "ol") {
		style.data.push_back(Parameter::create<ParameterName::ListStyleType>(ListStyleType::Decimal));
		style.data.push_back(Parameter::create<ParameterName::PaddingLeft>(Size(36, Size::Metric::Px)));
		style.data.push_back(Parameter::create<ParameterName::MarginTop>(Size(10, Size::Metric::Px)));
		style.data.push_back(Parameter::create<ParameterName::MarginBottom>(Size(10, Size::Metric::Px)));
		style.data.push_back(Parameter::create<ParameterName::XListStyleOffset>(Size(16, Size::Metric::Px)));
	} else if (tag == "ul") {
		style.data.push_back(Parameter::create<ParameterName::ListStyleType>(ListStyleType::Disc));
		style.data.push_back(Parameter::create<ParameterName::PaddingLeft>(Size(36, Size::Metric::Px)));
		style.data.push_back(Parameter::create<ParameterName::MarginTop>(Size(10, Size::Metric::Px)));
		style.data.push_back(Parameter::create<ParameterName::MarginBottom>(Size(10, Size::Metric::Px)));
		style.data.push_back(Parameter::create<ParameterName::XListStyleOffset>(Size(16, Size::Metric::Px)));
	} else if (tag == "img") {
		style.data.push_back(Parameter::create<ParameterName::BackgroundSizeWidth>(Size(1.0, Size::Metric::Percent)));
		style.data.push_back(Parameter::create<ParameterName::BackgroundSizeHeight>(Size(1.0, Size::Metric::Percent)));
		style.data.push_back(Parameter::create<ParameterName::PageBreakInside>(PageBreak::Avoid));
		style.data.push_back(Parameter::create<ParameterName::Display>(Display::InlineBlock));
	}

	return style;
}

static bool ParameterList_readListStyleType(ParameterList &list, const String &value, MediaQueryId mediaQuary) {
	if (value == "none") {
		list.set<ParameterName::ListStyleType>(ListStyleType::None, mediaQuary);
		return true;
	} else if (value == "circle") {
		list.set<ParameterName::ListStyleType>(ListStyleType::Circle, mediaQuary);
		return true;
	} else if (value == "disc") {
		list.set<ParameterName::ListStyleType>(ListStyleType::Disc, mediaQuary);
		return true;
	} else if (value == "square") {
		list.set<ParameterName::ListStyleType>(ListStyleType::Square, mediaQuary);
		return true;
	} else if (value == "x-mdash") {
		list.set<ParameterName::ListStyleType>(ListStyleType::XMdash, mediaQuary);
		return true;
	} else if (value == "decimal") {
		list.set<ParameterName::ListStyleType>(ListStyleType::Decimal, mediaQuary);
		return true;
	} else if (value == "decimal-leading-zero") {
		list.set<ParameterName::ListStyleType>(ListStyleType::DecimalLeadingZero, mediaQuary);
		return true;
	} else if (value == "lower-alpha" || value == "lower-latin") {
		list.set<ParameterName::ListStyleType>(ListStyleType::LowerAlpha, mediaQuary);
		return true;
	} else if (value == "lower-greek") {
		list.set<ParameterName::ListStyleType>(ListStyleType::LowerGreek, mediaQuary);
		return true;
	} else if (value == "lower-roman") {
		list.set<ParameterName::ListStyleType>(ListStyleType::LowerRoman, mediaQuary);
		return true;
	} else if (value == "upper-alpha" || value == "upper-latin") {
		list.set<ParameterName::ListStyleType>(ListStyleType::UpperAlpha, mediaQuary);
		return true;
	} else if (value == "upper-roman") {
		list.set<ParameterName::ListStyleType>(ListStyleType::UpperRoman, mediaQuary);
		return true;
	}
	return false;
}

template <style::ParameterName Name>
static bool ParameterList_readBorderStyle(ParameterList &list, const String &value, MediaQueryId mediaQuary) {
	if (value == "none") {
		list.set<Name>(BorderStyle::None, mediaQuary);
		return true;
	} else if (value == "solid") {
		list.set<Name>(BorderStyle::Solid, mediaQuary);
		return true;
	} else if (value == "dotted") {
		list.set<Name>(BorderStyle::Dotted, mediaQuary);
		return true;
	} else if (value == "dashed") {
		list.set<Name>(BorderStyle::Dashed, mediaQuary);
		return true;
	}
	return false;
}

template <style::ParameterName Name>
static bool ParameterList_readBorderColor(ParameterList &list, const String &value, MediaQueryId mediaQuary) {
	if (value == "transparent") {
		list.set<Name>(cocos2d::Color4B(0, 0, 0, 0), mediaQuary);
		return true;
	}

	cocos2d::Color4B color;
	if (readColor(value, color)) {
		list.set<Name>(color, mediaQuary);
		return true;
	}
	return false;
}

template <style::ParameterName Name>
static bool ParameterList_readBorderWidth(ParameterList &list, const String &value, MediaQueryId mediaQuary) {
	if (value == "thin") {
		list.set<Name>(Size(2.0f, Size::Metric::Px), mediaQuary);
		return true;
	} else if (value == "medium") {
		list.set<Name>(Size(4.0f, Size::Metric::Px), mediaQuary);
		return true;
	} else if (value == "thick") {
		list.set<Name>(Size(6.0f, Size::Metric::Px), mediaQuary);
		return true;
	}

	Size v;
	if (readStyleValue(value, v)) {
		list.set<Name>(v, mediaQuary);
		return true;
	}
	return false;
}

template <style::ParameterName Style, style::ParameterName Color, style::ParameterName Width>
static void ParameterList_readBorder(ParameterList &list, const String &value, MediaQueryId mediaQuary) {
	string::split(value, " ", [&] (const CharReaderBase &r) {
		auto str = r.str();
		if (!ParameterList_readBorderStyle<Style>(list, str, mediaQuary)) {
			if (!ParameterList_readBorderColor<Color>(list, str, mediaQuary)) {
				ParameterList_readBorderWidth<Width>(list, str, mediaQuary);
			}
		}
	});
}

template <typename T, typename Getter>
void ParameterList_readQuadValue(const String &value, T &top, T &right, T &bottom, T &left, const Getter &g) {
	int count = 0;
	string::split(value, " ", [&] (const CharReaderBase &r) {
		count ++;
		if (count == 1) {
			top = right = bottom = left = g(r.str());
		} else if (count == 2) {
			right = left = g(r.str());
		} else if (count == 3) {
			bottom = g(r.str());
		} else if (count == 4) {
			left = g(r.str());
		}
	});
}

void ParameterList::read(const StyleVec &vec, MediaQueryId mediaQuary) {
	for (auto &it : vec) {
		if (it.first == "font-weight") {
			if (it.second == "bold") {
				set<ParameterName::FontWeight>(FontWeight::Bold, mediaQuary);
			} else if (it.second == "normal") {
				set<ParameterName::FontWeight>(FontWeight::Normal, mediaQuary);
			} else if (it.second == "100") {
				set<ParameterName::FontWeight>(FontWeight::W100, mediaQuary);
			} else if (it.second == "200") {
				set<ParameterName::FontWeight>(FontWeight::W200, mediaQuary);
			} else if (it.second == "300") {
				set<ParameterName::FontWeight>(FontWeight::W300, mediaQuary);
			} else if (it.second == "400") {
				set<ParameterName::FontWeight>(FontWeight::W400, mediaQuary);
			} else if (it.second == "500") {
				set<ParameterName::FontWeight>(FontWeight::W500, mediaQuary);
			} else if (it.second == "600") {
				set<ParameterName::FontWeight>(FontWeight::W600, mediaQuary);
			} else if (it.second == "700") {
				set<ParameterName::FontWeight>(FontWeight::W700, mediaQuary);
			} else if (it.second == "800") {
				set<ParameterName::FontWeight>(FontWeight::W800, mediaQuary);
			} else if (it.second == "900") {
				set<ParameterName::FontWeight>(FontWeight::W900, mediaQuary);
			}
		} else if (it.first == "font-stretch") {
			if (it.second == "normal") {
				set<ParameterName::FontStretch>(FontStretch::Normal, mediaQuary);
			} else if (it.second == "ultra-condensed") {
				set<ParameterName::FontStretch>(FontStretch::UltraCondensed, mediaQuary);
			} else if (it.second == "extra-condensed") {
				set<ParameterName::FontStretch>(FontStretch::ExtraCondensed, mediaQuary);
			} else if (it.second == "condensed") {
				set<ParameterName::FontStretch>(FontStretch::Condensed, mediaQuary);
			} else if (it.second == "semi-condensed") {
				set<ParameterName::FontStretch>(FontStretch::SemiCondensed, mediaQuary);
			} else if (it.second == "semi-expanded") {
				set<ParameterName::FontStretch>(FontStretch::SemiExpanded, mediaQuary);
			} else if (it.second == "expanded") {
				set<ParameterName::FontStretch>(FontStretch::Expanded, mediaQuary);
			} else if (it.second == "extra-expanded") {
				set<ParameterName::FontStretch>(FontStretch::ExtraExpanded, mediaQuary);
			} else if (it.second == "ultra-expanded") {
				set<ParameterName::FontStretch>(FontStretch::UltraExpanded, mediaQuary);
			}
		} else if (it.first == "font-style") {
			if (it.second == "italic") {
				set<ParameterName::FontStyle>(FontStyle::Italic, mediaQuary);
			} else if (it.second == "normal") {
				set<ParameterName::FontStyle>(FontStyle::Normal, mediaQuary);
			} else if (it.second == "oblique") {
				set<ParameterName::FontStyle>(FontStyle::Oblique, mediaQuary);
			}
		} else if (it.first == "font-size") {
			if (it.second == "xx-small") {
				set<ParameterName::FontSize>(FontSize::XXSmall, mediaQuary);
			} else if (it.second == "x-small") {
				set<ParameterName::FontSize>(FontSize::XSmall, mediaQuary);
			} else if (it.second == "small") {
				set<ParameterName::FontSize>(FontSize::Small, mediaQuary);
			} else if (it.second == "medium") {
				set<ParameterName::FontSize>(FontSize::Medium, mediaQuary);
			} else if (it.second == "large") {
				set<ParameterName::FontSize>(FontSize::Large, mediaQuary);
			} else if (it.second == "x-large") {
				set<ParameterName::FontSize>(FontSize::XLarge, mediaQuary);
			} else if (it.second == "xx-large") {
				set<ParameterName::FontSize>(FontSize::XXLarge, mediaQuary);
			} else if (it.second == "larger") {
				set<ParameterName::FontSizeIncrement>(FontSizeIncrement::Larger, mediaQuary);
			} else if (it.second == "x-larger") {
				set<ParameterName::FontSizeIncrement>(FontSizeIncrement::XLarger, mediaQuary);
			} else if (it.second == "smaller") {
				set<ParameterName::FontSizeIncrement>(FontSizeIncrement::Smaller, mediaQuary);
			} else if (it.second == "x-smaller") {
				set<ParameterName::FontSizeIncrement>(FontSizeIncrement::XSmaller, mediaQuary);
			} else {
				style::Size fontSize;
				if (readStyleValue(it.second, fontSize)) {
					if (fontSize.metric == style::Size::Metric::Px) {
						set<ParameterName::FontSize>((uint8_t)fontSize.value, mediaQuary);
					} else if (fontSize.metric == style::Size::Metric::Em) {
						set<ParameterName::FontSize>((uint8_t)(FontSize::Medium * fontSize.value), mediaQuary);
					}
				}
			}
		} else if (it.first == "font-variant") {
			if (it.second == "small-caps") {
				set<ParameterName::FontVariant>(FontVariant::SmallCaps, mediaQuary);
			} else if (it.second == "normal") {
				set<ParameterName::FontVariant>(FontVariant::Normal, mediaQuary);
			}
		} else if (it.first == "text-decoration") {
			if (it.second == "underline") {
				set<ParameterName::TextDecoration>(TextDecoration::Underline, mediaQuary);
			} else if (it.second == "none") {
				set<ParameterName::TextDecoration>(TextDecoration::None, mediaQuary);
			}
		} else if (it.first == "text-transform") {
			if (it.second == "uppercase") {
				set<ParameterName::TextTransform>(TextTransform::Uppercase, mediaQuary);
			} else if (it.second == "lowercase") {
				set<ParameterName::TextTransform>(TextTransform::Lowercase, mediaQuary);
			} else if (it.second == "none") {
				set<ParameterName::TextTransform>(TextTransform::None, mediaQuary);
			}
		} else if (it.first == "text-align") {
			if (it.second == "left") {
				set<ParameterName::TextAlign>(TextAlign::Left, mediaQuary);
			} else if (it.second == "right") {
				set<ParameterName::TextAlign>(TextAlign::Right, mediaQuary);
			} else if (it.second == "center") {
				set<ParameterName::TextAlign>(TextAlign::Center, mediaQuary);
			} else if (it.second == "justify") {
				set<ParameterName::TextAlign>(TextAlign::Justify, mediaQuary);
			}
		} else if (it.first == "white-space") {
			if (it.second == "normal") {
				set<ParameterName::WhiteSpace>(WhiteSpace::Normal, mediaQuary);
			} else if (it.second == "nowrap") {
				set<ParameterName::WhiteSpace>(WhiteSpace::Nowrap, mediaQuary);
			} else if (it.second == "pre") {
				set<ParameterName::WhiteSpace>(WhiteSpace::Pre, mediaQuary);
			} else if (it.second == "pre-line") {
				set<ParameterName::WhiteSpace>(WhiteSpace::PreLine, mediaQuary);
			} else if (it.second == "pre-wrap") {
				set<ParameterName::WhiteSpace>(WhiteSpace::PreWrap, mediaQuary);
			}
		} else if (it.first == "hyphens" || it.first == "-epub-hyphens") {
			if (it.second == "none") {
				set<ParameterName::Hyphens>(Hyphens::None, mediaQuary);
			} else if (it.second == "manual") {
				set<ParameterName::Hyphens>(Hyphens::Manual, mediaQuary);
			} else if (it.second == "auto") {
				set<ParameterName::Hyphens>(Hyphens::Auto, mediaQuary);
			}
		} else if (it.first == "display") {
			if (it.second == "none") {
				set<ParameterName::Display>(Display::None, mediaQuary);
			} else if (it.second == "run-in") {
				set<ParameterName::Display>(Display::RunIn, mediaQuary);
			} else if (it.second == "list-item") {
				set<ParameterName::Display>(Display::ListItem, mediaQuary);
			} else if (it.second == "inline") {
				set<ParameterName::Display>(Display::Inline, mediaQuary);
			} else if (it.second == "inline-block") {
				set<ParameterName::Display>(Display::InlineBlock, mediaQuary);
			} else if (it.second == "block") {
				set<ParameterName::Display>(Display::Block, mediaQuary);
			}
		} else if (it.first == "list-style-type") {
			ParameterList_readListStyleType(*this, it.second, mediaQuary);
		} else if (it.first == "list-style-position") {
			if (it.second == "inside") {
				set<ParameterName::ListStylePosition>(ListStylePosition::Inside, mediaQuary);
			} else if (it.second == "outside") {
				set<ParameterName::ListStylePosition>(ListStylePosition::Outside, mediaQuary);
			}
		} else if (it.first == "list-style") {
			string::split(it.second, " ", [&] (const CharReaderBase &r) {
				if (!ParameterList_readListStyleType(*this, it.second, mediaQuary)) {
					if (it.second == "inside") {
						set<ParameterName::ListStylePosition>(ListStylePosition::Inside, mediaQuary);
					} else if (it.second == "outside") {
						set<ParameterName::ListStylePosition>(ListStylePosition::Outside, mediaQuary);
					}
				}
			});
		} else if (it.first == "x-list-style-offset") {
			Size value;
			if (readStyleValue(it.second, value)) {
				set<ParameterName::XListStyleOffset>(value, mediaQuary);
			}
		} else if (it.first == "float") {
			if (it.second == "none") {
				set<ParameterName::Float>(Float::None, mediaQuary);
			} else if (it.second == "left") {
				set<ParameterName::Float>(Float::Left, mediaQuary);
			} else if (it.second == "right") {
				set<ParameterName::Float>(Float::Right, mediaQuary);
			}
		} else if (it.first == "clear") {
			if (it.second == "none") {
				set<ParameterName::Clear>(Clear::None, mediaQuary);
			} else if (it.second == "left") {
				set<ParameterName::Clear>(Clear::Left, mediaQuary);
			} else if (it.second == "right") {
				set<ParameterName::Clear>(Clear::Right, mediaQuary);
			} else if (it.second == "both") {
				set<ParameterName::Clear>(Clear::Both, mediaQuary);
			}
		} else if (it.first == "opacity") {
			float value = stappler::stof(it.second);
			if (value < 0.0f) {
				value = 0.0f;
			} else if (value > 1.0f) {
				value = 1.0f;
			}
			set<ParameterName::Opacity>((uint8_t)(value * 255.0f), mediaQuary);
		} else if (it.first == "color") {
			cocos2d::Color4B color;
			if (readColor(it.second, color)) {
				set<ParameterName::Color>(cocos2d::Color3B(color.r, color.g, color.b), mediaQuary);
				if (color.a != 255) {
					set<ParameterName::Opacity>(color.a, mediaQuary);
				}
			}
		} else if (it.first == "text-indent") {
			Size value;
			if (readStyleValue(it.second, value)) {
				set<ParameterName::TextIndent>(value, mediaQuary);
			}
		} else if (it.first == "line-height") {
			Size value;
			if (readStyleValue(it.second, value, false, true)) {
				set<ParameterName::LineHeight>(value, mediaQuary);
			}
		} else if (it.first == "margin") {
			Size top, right, bottom, left;
			if (readStyleMargin(it.second, top, right, bottom, left)) {
				set<ParameterName::MarginTop>(top, mediaQuary);
				set<ParameterName::MarginRight>(right, mediaQuary);
				set<ParameterName::MarginBottom>(bottom, mediaQuary);
				set<ParameterName::MarginLeft>(left, mediaQuary);
			}
		} else if (it.first == "margin-top") {
			Size value;
			if (readStyleValue(it.second, value)) {
				set<ParameterName::MarginTop>(value, mediaQuary);
			}
		} else if (it.first == "margin-right") {
			Size value;
			if (readStyleValue(it.second, value)) {
				set<ParameterName::MarginRight>(value, mediaQuary);
			}
		} else if (it.first == "margin-bottom") {
			Size value;
			if (readStyleValue(it.second, value)) {
				set<ParameterName::MarginBottom>(value, mediaQuary);
			}
		} else if (it.first == "margin-left") {
			Size value;
			if (readStyleValue(it.second, value)) {
				set<ParameterName::MarginLeft>(value, mediaQuary);
			}
		} else if (it.first == "width") {
			Size value;
			if (readStyleValue(it.second, value)) {
				set<ParameterName::Width>(value, mediaQuary);
			}
		} else if (it.first == "height") {
			Size value;
			if (readStyleValue(it.second, value)) {
				set<ParameterName::Height>(value, mediaQuary);
			}
		} else if (it.first == "min-width") {
			Size value;
			if (readStyleValue(it.second, value)) {
				set<ParameterName::MinWidth>(value, mediaQuary);
			}
		} else if (it.first == "min-height") {
			Size value;
			if (readStyleValue(it.second, value)) {
				set<ParameterName::MinHeight>(value, mediaQuary);
			}
		} else if (it.first == "max-width") {
			Size value;
			if (readStyleValue(it.second, value)) {
				set<ParameterName::MaxWidth>(value, mediaQuary);
			}
		} else if (it.first == "max-height") {
			Size value;
			if (readStyleValue(it.second, value)) {
				set<ParameterName::MaxHeight>(value, mediaQuary);
			}
		} else if (it.first == "padding") {
			Size top, right, bottom, left;
			if (readStyleMargin(it.second, top, right, bottom, left)) {
				set<ParameterName::PaddingTop>(top, mediaQuary);
				set<ParameterName::PaddingRight>(right, mediaQuary);
				set<ParameterName::PaddingBottom>(bottom, mediaQuary);
				set<ParameterName::PaddingLeft>(left, mediaQuary);
			}
		} else if (it.first == "padding-top") {
			Size value;
			if (readStyleValue(it.second, value)) {
				set<ParameterName::PaddingTop>(value, mediaQuary);
			}
		} else if (it.first == "padding-right") {
			Size value;
			if (readStyleValue(it.second, value)) {
				set<ParameterName::PaddingRight>(value, mediaQuary);
			}
		} else if (it.first == "padding-bottom") {
			Size value;
			if (readStyleValue(it.second, value)) {
				set<ParameterName::PaddingBottom>(value, mediaQuary);
			}
		} else if (it.first == "padding-left") {
			Size value;
			if (readStyleValue(it.second, value)) {
				set<ParameterName::PaddingLeft>(value, mediaQuary);
			}
		} else if (it.first == "font-family") {
			if (it.second == "default" || it.second == "serif") {
				set<ParameterName::FontFamily>(CssStringNone(), mediaQuary);
			} else {
				CssStringId value;
				if (readStringValue("", it.second, value)) {
					set<ParameterName::FontFamily>(value, mediaQuary);
				}
			}
		} else if (it.first == "background-color") {
			if (it.second == "transparent") {
				set<ParameterName::BackgroundColor>(cocos2d::Color4B(0, 0, 0, 0), mediaQuary);
			} else {
				cocos2d::Color4B color;
				if (readColor(it.second, color)) {
					set<ParameterName::BackgroundColor>(color, mediaQuary);
				}
			}
		} else if (it.first == "background-image") {
			if (it.second == "none") {
				set<ParameterName::BackgroundImage>(CssStringNone(), mediaQuary);
			} else {
				CssStringId value;
				if (readStringValue("url", it.second, value)) {
					set<ParameterName::BackgroundImage>(value, mediaQuary);
				}
			}
		} else if (it.first == "background-position") {
			String first, second;
			Size x, y;
			bool validX = false, validY = false, swapValues = false;
			if (splitValue(it.second, first, second)) {
				bool parseError = false;
				bool firstWasCenter = false;
				if (first == "center") {
					x.value = 0.5f; x.metric = Size::Metric::Percent; validX = true; firstWasCenter = true;
				} else if (first == "left") {
					x.value = 0.0f; x.metric = Size::Metric::Percent; validX = true;
				} else if (first == "right") {
					x.value = 1.0f; x.metric = Size::Metric::Percent; validX = true;
				} else if (first == "top") {
					x.value = 0.0f; x.metric = Size::Metric::Percent; validX = true; swapValues = true;
				} else if (first == "bottom") {
					x.value = 1.0f; x.metric = Size::Metric::Percent; validX = true; swapValues = true;
				}

				if (second == "center") {
					y.value = 0.5f; y.metric = Size::Metric::Percent; validY = true;
				} else if (second == "left") {
					if (swapValues || firstWasCenter) {
						y.value = 0.0f; y.metric = Size::Metric::Percent; validY = true; swapValues = true;
					} else {
						parseError = true;
					}
				} else if (second == "right") {
					if (swapValues || firstWasCenter) {
						y.value = 1.0f; y.metric = Size::Metric::Percent; validY = true; swapValues = true;
					} else {
						parseError = true;
					}
				} else if (second == "top") {
					if (!swapValues) {
						y.value = 0.0f; y.metric = Size::Metric::Percent; validY = true; swapValues = true;
					} else {
						parseError = true;
					}
				} else if (second == "bottom") {
					if (!swapValues) {
						y.value = 1.0f; y.metric = Size::Metric::Percent; validY = true; swapValues = true;
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
				if (it.second == "center") {
					x.value = 0.5f; x.metric = Size::Metric::Percent; validX = true;
					y.value = 0.5f; y.metric = Size::Metric::Percent; validY = true;
				} else if (it.second == "top") {
					x.value = 0.5f; x.metric = Size::Metric::Percent; validX = true;
					y.value = 0.0f; y.metric = Size::Metric::Percent; validY = true;
				} else if (it.second == "right") {
					x.value = 1.0f; x.metric = Size::Metric::Percent; validX = true;
					y.value = 0.5f; y.metric = Size::Metric::Percent; validY = true;
				} else if (it.second == "bottom") {
					x.value = 0.5f; x.metric = Size::Metric::Percent; validX = true;
					y.value = 1.0f; y.metric = Size::Metric::Percent; validY = true;
				} else if (it.second == "left") {
					x.value = 0.0f; x.metric = Size::Metric::Percent; validX = true;
					y.value = 0.5f; y.metric = Size::Metric::Percent; validY = true;
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
		} else if (it.first == "background-repeat") {
			if (it.second == "no-repeat") {
				set<ParameterName::BackgroundRepeat>(BackgroundRepeat::NoRepeat, mediaQuary);
			} else if (it.second == "repeat") {
				set<ParameterName::BackgroundRepeat>(BackgroundRepeat::Repeat, mediaQuary);
			} else if (it.second == "repeat-x") {
				set<ParameterName::BackgroundRepeat>(BackgroundRepeat::RepeatX, mediaQuary);
			} else if (it.second == "repeat-y") {
				set<ParameterName::BackgroundRepeat>(BackgroundRepeat::RepeatY, mediaQuary);
			}
		} else if (it.first == "background-size") {
			String first, second;
			Size width, height;
			bool validWidth = false, validHeight = false;
			if (it.second == "contain") {
				width.metric = Size::Metric::Contain; validWidth = true;
				height.metric = Size::Metric::Contain; validHeight = true;
			} else if (it.second == "cover") {
				width.metric = Size::Metric::Cover; validWidth = true;
				height.metric = Size::Metric::Cover; validHeight = true;
			} else if (splitValue(it.second, first, second)) {
				if (first == "contain") {
					width.metric = Size::Metric::Contain; validWidth = true;
				} else if (first == "cover") {
					width.metric = Size::Metric::Cover; validWidth = true;
				} else if (readStyleValue(first, width)) {
					validWidth = true;
				}

				if (second == "contain") {
					height.metric = Size::Metric::Contain; validWidth = true;
				} else if (second == "cover") {
					height.metric = Size::Metric::Cover; validWidth = true;
				} else if (readStyleValue(second, height)) {
					validHeight = true;
				}
			} else if (readStyleValue(it.second, width)) {
				height.metric = Size::Metric::Auto; validWidth = true; validHeight = true;
			}

			if (validWidth && validHeight) {
				set<ParameterName::BackgroundSizeWidth>(width, mediaQuary);
				set<ParameterName::BackgroundSizeHeight>(height, mediaQuary);
			}
		} else if (it.first == "vertical-align") {
			if (it.second == "baseline") {
				set<ParameterName::VerticalAlign>(VerticalAlign::Baseline, mediaQuary);
			} else if (it.second == "sub") {
				set<ParameterName::VerticalAlign>(VerticalAlign::Sub, mediaQuary);
			} else if (it.second == "super") {
				set<ParameterName::VerticalAlign>(VerticalAlign::Super, mediaQuary);
			}
		} else if (it.first == "outline") {
			ParameterList_readBorder<ParameterName::OutlineStyle, ParameterName::OutlineColor, ParameterName::OutlineWidth>(*this, it.second, mediaQuary);
		} else if (it.first == "outline-style") {
			ParameterList_readBorderStyle<ParameterName::OutlineStyle>(*this, it.second, mediaQuary);
		} else if (it.first == "outline-color") {
			ParameterList_readBorderColor<ParameterName::OutlineColor>(*this, it.second, mediaQuary);
		} else if (it.first == "outline-width") {
			ParameterList_readBorderWidth<ParameterName::OutlineWidth>(*this, it.second, mediaQuary);
		} else if (it.first == "border-top") {
			ParameterList_readBorder<ParameterName::BorderTopStyle, ParameterName::BorderTopColor, ParameterName::BorderTopWidth>(*this, it.second, mediaQuary);
		} else if (it.first == "border-top-style") {
			ParameterList_readBorderStyle<ParameterName::BorderTopStyle>(*this, it.second, mediaQuary);
		} else if (it.first == "border-top-color") {
			ParameterList_readBorderColor<ParameterName::BorderTopColor>(*this, it.second, mediaQuary);
		} else if (it.first == "border-top-width") {
			ParameterList_readBorderWidth<ParameterName::BorderTopWidth>(*this, it.second, mediaQuary);
		} else if (it.first == "border-right") {
			ParameterList_readBorder<ParameterName::BorderRightStyle, ParameterName::BorderRightColor, ParameterName::BorderRightWidth>(*this, it.second, mediaQuary);
		} else if (it.first == "border-right-style") {
			ParameterList_readBorderStyle<ParameterName::BorderRightStyle>(*this, it.second, mediaQuary);
		} else if (it.first == "border-right-color") {
			ParameterList_readBorderColor<ParameterName::BorderRightColor>(*this, it.second, mediaQuary);
		} else if (it.first == "border-right-width") {
			ParameterList_readBorderWidth<ParameterName::BorderRightWidth>(*this, it.second, mediaQuary);
		} else if (it.first == "border-bottom") {
			ParameterList_readBorder<ParameterName::BorderBottomStyle, ParameterName::BorderBottomColor, ParameterName::BorderBottomWidth>(*this, it.second, mediaQuary);
		} else if (it.first == "border-bottom-style") {
			ParameterList_readBorderStyle<ParameterName::BorderBottomStyle>(*this, it.second, mediaQuary);
		} else if (it.first == "border-bottom-color") {
			ParameterList_readBorderColor<ParameterName::BorderBottomColor>(*this, it.second, mediaQuary);
		} else if (it.first == "border-bottom-width") {
			ParameterList_readBorderWidth<ParameterName::BorderBottomWidth>(*this, it.second, mediaQuary);
		} else if (it.first == "border-left") {
			ParameterList_readBorder<ParameterName::BorderLeftStyle, ParameterName::BorderLeftColor, ParameterName::BorderLeftWidth>(*this, it.second, mediaQuary);
		} else if (it.first == "border-left-style") {
			ParameterList_readBorderStyle<ParameterName::BorderLeftStyle>(*this, it.second, mediaQuary);
		} else if (it.first == "border-left-color") {
			ParameterList_readBorderColor<ParameterName::BorderLeftColor>(*this, it.second, mediaQuary);
		} else if (it.first == "border-left-width") {
			ParameterList_readBorderWidth<ParameterName::BorderLeftWidth>(*this, it.second, mediaQuary);
		} else if (it.first == "border-style" && !it.second.empty()) {
			BorderStyle top, right, bottom, left;
			ParameterList_readQuadValue(it.second, top, right, bottom, left, [&] (const String &value) -> BorderStyle {
				if (value == "solid") {
					return BorderStyle::Solid;
				} else if (value == "dotted") {
					return BorderStyle::Dotted;
				} else if (value == "dashed") {
					return BorderStyle::Dashed;
				}
				return BorderStyle::None;
			});
			set<ParameterName::BorderTopStyle>(top, mediaQuary);
			set<ParameterName::BorderRightStyle>(right, mediaQuary);
			set<ParameterName::BorderBottomStyle>(bottom, mediaQuary);
			set<ParameterName::BorderLeftStyle>(left, mediaQuary);
		} else if (it.first == "border-color" && !it.second.empty()) {
			cocos2d::Color4B top, right, bottom, left;
			ParameterList_readQuadValue(it.second, top, right, bottom, left, [&] (const String &value) -> cocos2d::Color4B {
				if (value == "transparent") {
					return cocos2d::Color4B(0, 0, 0, 0);
				} else {
					cocos2d::Color4B color(0, 0, 0, 0);
					readColor(value, color);
					return color;
				}
			});
			set<ParameterName::BorderTopColor>(top, mediaQuary);
			set<ParameterName::BorderRightColor>(right, mediaQuary);
			set<ParameterName::BorderBottomColor>(bottom, mediaQuary);
			set<ParameterName::BorderLeftColor>(left, mediaQuary);
		} else if (it.first == "border-width" && !it.second.empty()) {
			Size top, right, bottom, left;
			ParameterList_readQuadValue(it.second, top, right, bottom, left, [&] (const String &value) -> Size {
				if (value == "thin") {
					return Size(2.0f, Size::Metric::Px);
				} else if (value == "medium") {
					return Size(4.0f, Size::Metric::Px);
				} else if (value == "thick") {
					return Size(6.0f, Size::Metric::Px);
				}

				Size v(0.0f, Size::Metric::Px);
				readStyleValue(value, v);
				return v;
			});
			set<ParameterName::BorderTopWidth>(top, mediaQuary);
			set<ParameterName::BorderRightWidth>(right, mediaQuary);
			set<ParameterName::BorderBottomWidth>(bottom, mediaQuary);
			set<ParameterName::BorderLeftWidth>(left, mediaQuary);
		} else if (it.first == "border" && !it.second.empty()) {
			BorderStyle style = BorderStyle::None;
			Size width(0.0f, Size::Metric::Px);
			cocos2d::Color4B color(0, 0, 0, 0);
			string::split(it.second, " ", [&] (const CharReaderBase &r) {
				if (r == "solid") {
					style = BorderStyle::Solid;
				} else if (r == "dotted") {
					style = BorderStyle::Dotted;
				} else if (r == "dashed") {
					style = BorderStyle::Dashed;
				} else if (r == "none") {
					style = BorderStyle::None;
				} else if (r == "transparent") {
					color = cocos2d::Color4B(0, 0, 0, 0);
				} else if (r == "thin") {
					width = Size(2.0f, Size::Metric::Px);
				} else if (r == "medium") {
					width = Size(4.0f, Size::Metric::Px);
				} else if (r == "thick") {
					width = Size(6.0f, Size::Metric::Px);
				} else if (!readColor(r.str(), color)) {
					readStyleValue(r.str(), width);
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
		} else if (it.first == "page-break-after") {
			if (it.second == "always") {
				set<ParameterName::PageBreakAfter>(PageBreak::Always, mediaQuary);
			} else if (it.second == "auto") {
				set<ParameterName::PageBreakAfter>(PageBreak::Auto, mediaQuary);
			} else if (it.second == "avoid") {
				set<ParameterName::PageBreakAfter>(PageBreak::Avoid, mediaQuary);
			} else if (it.second == "left") {
				set<ParameterName::PageBreakAfter>(PageBreak::Left, mediaQuary);
			} else if (it.second == "right") {
				set<ParameterName::PageBreakAfter>(PageBreak::Right, mediaQuary);
			}
		} else if (it.first == "page-break-before") {
			if (it.second == "always") {
				set<ParameterName::PageBreakBefore>(PageBreak::Always, mediaQuary);
			} else if (it.second == "auto") {
				set<ParameterName::PageBreakBefore>(PageBreak::Auto, mediaQuary);
			} else if (it.second == "avoid") {
				set<ParameterName::PageBreakBefore>(PageBreak::Avoid, mediaQuary);
			} else if (it.second == "left") {
				set<ParameterName::PageBreakBefore>(PageBreak::Left, mediaQuary);
			} else if (it.second == "right") {
				set<ParameterName::PageBreakBefore>(PageBreak::Right, mediaQuary);
			}
		} else if (it.first == "page-break-inside") {
			if (it.second == "auto") {
				set<ParameterName::PageBreakInside>(PageBreak::Auto, mediaQuary);
			} else if (it.second == "avoid") {
				set<ParameterName::PageBreakInside>(PageBreak::Avoid, mediaQuary);
			}
		}
	}
}

void ParameterList::merge(const ParameterList &style, bool inherit) {
	for (auto &it : style.data) {
		if (!inherit || ParameterList::isInheritable(it.name)) {
			set(it, true);
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

NS_SP_EXT_END(rich_text)
