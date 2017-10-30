// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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

#include "SPLayout.h"
#include "SLStyle.h"

NS_LAYOUT_BEGIN

namespace style {

	inline static uint8_t increment(uint8_t current) {
		if (current < 16) {
			return current + 2;
		} else {
			return current + 4;
		}
	}

	inline static uint8_t decrement(uint8_t current) {
		if (current <= 16) {
			return current - 2;
		} else {
			return current - 4;
		}
	}

	static uint8_t modifySize(uint8_t current, FontSizeIncrement inc) {
		switch (inc) {
		case FontSizeIncrement::Larger: current = increment(current); break;
		case FontSizeIncrement::Smaller: current = decrement(current); break;
		case FontSizeIncrement::None: break;
		case FontSizeIncrement::XLarger: current = increment(increment(current)); break;
		case FontSizeIncrement::XSmaller: current = decrement(decrement(current)); break;
		}

		return current;
	}

	FontStyleParameters ParameterList::compileFontStyle(const RendererInterface *renderer) const {
		FontStyleParameters compiled;
		FontSizeIncrement incr = FontSizeIncrement::None;
		for (auto &it : data) {
			if (it.mediaQuery == MediaQueryNone() || renderer->resolveMediaQuery(it.mediaQuery)) {
				switch(it.name) {
				case ParameterName::FontSizeIncrement: incr = it.value.fontSizeIncrement; break;
				case ParameterName::FontStyle: compiled.fontStyle = it.value.fontStyle; break;
				case ParameterName::FontWeight: compiled.fontWeight = it.value.fontWeight; break;
				case ParameterName::FontSize: compiled.fontSize = it.value.fontSize; break;
				case ParameterName::FontStretch: compiled.fontStretch = it.value.fontStretch; break;
				case ParameterName::FontVariant: compiled.fontVariant = it.value.fontVariant; break;
				case ParameterName::ListStyleType: compiled.listStyleType = it.value.listStyleType; break;
				case ParameterName::FontFamily: compiled.fontFamily = renderer->getCssString(it.value.stringId); break;
				default: break;
				}
			}
		}
		if (compiled.fontFamily.empty()) {
			compiled.fontFamily = "default";
		}
		if (incr != FontSizeIncrement::None) {
			compiled.fontSize = modifySize(compiled.fontSize, incr);
		}
		return compiled;
	}
	TextLayoutParameters ParameterList::compileTextLayout(const RendererInterface *renderer) const {
		TextLayoutParameters compiled;
		for (auto &it : data) {
			if (it.mediaQuery == MediaQueryNone() || renderer->resolveMediaQuery(it.mediaQuery)) {
				switch(it.name) {
				case ParameterName::TextTransform: compiled.textTransform = it.value.textTransform; break;
				case ParameterName::TextDecoration: compiled.textDecoration = it.value.textDecoration; break;
				case ParameterName::WhiteSpace: compiled.whiteSpace = it.value.whiteSpace;  break;
				case ParameterName::Hyphens: compiled.hyphens = it.value.hyphens;  break;
				case ParameterName::VerticalAlign: compiled.verticalAlign = it.value.verticalAlign;  break;
				case ParameterName::Color: compiled.color = it.value.color; break;
				case ParameterName::Opacity: compiled.opacity = it.value.opacity; break;
				default: break;
				}
			}
		}
		return compiled;
	}
	ParagraphLayoutParameters ParameterList::compileParagraphLayout(const RendererInterface *renderer) const {
		ParagraphLayoutParameters compiled;
		for (auto &it : data) {
			if (it.mediaQuery == MediaQueryNone() || renderer->resolveMediaQuery(it.mediaQuery)) {
				switch(it.name) {
				case ParameterName::TextAlign: compiled.textAlign = it.value.textAlign; break;
				case ParameterName::TextIndent: compiled.textIndent = it.value.sizeValue; break;
				case ParameterName::LineHeight: compiled.lineHeight = it.value.sizeValue; break;
				case ParameterName::XListStyleOffset: compiled.listOffset = it.value.sizeValue; break;
				default: break;
				}
			}
		}
		return compiled;
	}
	BlockModelParameters ParameterList::compileBlockModel(const RendererInterface *renderer) const {
		BlockModelParameters compiled;
		for (auto &it : data) {
			if (it.mediaQuery == MediaQueryNone() || renderer->resolveMediaQuery(it.mediaQuery)) {
				switch(it.name) {
				case ParameterName::Display: compiled.display = it.value.display; break;
				case ParameterName::Float: compiled.floating = it.value.floating; break;
				case ParameterName::Clear: compiled.clear = it.value.clear; break;
				case ParameterName::MarginTop: compiled.marginTop = it.value.sizeValue; break;
				case ParameterName::MarginRight: compiled.marginRight = it.value.sizeValue; break;
				case ParameterName::MarginBottom: compiled.marginBottom = it.value.sizeValue; break;
				case ParameterName::MarginLeft: compiled.marginLeft = it.value.sizeValue; break;
				case ParameterName::PaddingTop: compiled.paddingTop = it.value.sizeValue; break;
				case ParameterName::PaddingRight: compiled.paddingRight = it.value.sizeValue; break;
				case ParameterName::PaddingBottom: compiled.paddingBottom = it.value.sizeValue; break;
				case ParameterName::PaddingLeft: compiled.paddingLeft = it.value.sizeValue; break;
				case ParameterName::Width: compiled.width = it.value.sizeValue; break;
				case ParameterName::Height: compiled.height = it.value.sizeValue; break;
				case ParameterName::MinWidth: compiled.minWidth = it.value.sizeValue; break;
				case ParameterName::MinHeight: compiled.minHeight = it.value.sizeValue; break;
				case ParameterName::MaxWidth: compiled.maxWidth = it.value.sizeValue; break;
				case ParameterName::MaxHeight: compiled.maxHeight = it.value.sizeValue; break;
				case ParameterName::ListStylePosition: compiled.listStylePosition = it.value.listStylePosition; break;
				case ParameterName::ListStyleType: compiled.listStyleType = it.value.listStyleType; break;
				case ParameterName::PageBreakAfter: compiled.pageBreakAfter = it.value.pageBreak; break;
				case ParameterName::PageBreakBefore: compiled.pageBreakBefore = it.value.pageBreak; break;
				case ParameterName::PageBreakInside: compiled.pageBreakInside = it.value.pageBreak; break;
				default: break;
				}
			}
		}
		return compiled;
	}
	InlineModelParameters ParameterList::compileInlineModel(const RendererInterface *renderer) const {
		InlineModelParameters compiled;
		for (auto &it : data) {
			if (it.mediaQuery == MediaQueryNone() || renderer->resolveMediaQuery(it.mediaQuery)) {
				switch(it.name) {
				case ParameterName::MarginTop: compiled.marginTop = it.value.sizeValue; break;
				case ParameterName::MarginRight: compiled.marginRight = it.value.sizeValue; break;
				case ParameterName::MarginBottom: compiled.marginBottom = it.value.sizeValue; break;
				case ParameterName::MarginLeft: compiled.marginLeft = it.value.sizeValue; break;
				case ParameterName::PaddingTop: compiled.paddingTop = it.value.sizeValue; break;
				case ParameterName::PaddingRight: compiled.paddingRight = it.value.sizeValue; break;
				case ParameterName::PaddingBottom: compiled.paddingBottom = it.value.sizeValue; break;
				case ParameterName::PaddingLeft: compiled.paddingLeft = it.value.sizeValue; break;
				case ParameterName::Width: compiled.width = it.value.sizeValue; break;
				case ParameterName::Height: compiled.height = it.value.sizeValue; break;
				case ParameterName::MinWidth: compiled.minWidth = it.value.sizeValue; break;
				case ParameterName::MinHeight: compiled.minHeight = it.value.sizeValue; break;
				case ParameterName::MaxWidth: compiled.maxWidth = it.value.sizeValue; break;
				case ParameterName::MaxHeight: compiled.maxHeight = it.value.sizeValue; break;
				default: break;
				}
			}
		}
		return compiled;
	}
	BackgroundParameters ParameterList::compileBackground(const RendererInterface *renderer) const {
		BackgroundParameters compiled;
		uint8_t opacity = 255;
		for (auto &it : data) {
			if (it.mediaQuery == MediaQueryNone() || renderer->resolveMediaQuery(it.mediaQuery)) {
				switch(it.name) {
				case ParameterName::Display: compiled.display = it.value.display; break;
				case ParameterName::BackgroundColor: compiled.backgroundColor = it.value.color4; break;
				case ParameterName::Opacity: opacity = it.value.opacity; break;
				case ParameterName::BackgroundRepeat: compiled.backgroundRepeat = it.value.backgroundRepeat; break;
				case ParameterName::BackgroundPositionX: compiled.backgroundPositionX = it.value.sizeValue; break;
				case ParameterName::BackgroundPositionY: compiled.backgroundPositionY = it.value.sizeValue; break;
				case ParameterName::BackgroundSizeWidth: compiled.backgroundSizeWidth = it.value.sizeValue; break;
				case ParameterName::BackgroundSizeHeight: compiled.backgroundSizeHeight = it.value.sizeValue; break;
				case ParameterName::BackgroundImage: compiled.backgroundImage = renderer->getCssString(it.value.stringId); break;
				default: break;
				}
			}
		}

		compiled.backgroundColor.a = (uint8_t)(compiled.backgroundColor.a * opacity / 255.0f);
		return compiled;
	}

	OutlineParameters ParameterList::compileOutline(const RendererInterface *renderer) const {
		OutlineParameters compiled;
		for (auto &it : data) {
			if (it.mediaQuery == MediaQueryNone() || renderer->resolveMediaQuery(it.mediaQuery)) {
				switch(it.name) {
				case ParameterName::BorderLeftStyle: compiled.borderLeftStyle = it.value.borderStyle; break;
				case ParameterName::BorderLeftWidth: compiled.borderLeftWidth = it.value.sizeValue; break;
				case ParameterName::BorderLeftColor: compiled.borderLeftColor = it.value.color4; break;
				case ParameterName::BorderTopStyle: compiled.borderTopStyle = it.value.borderStyle; break;
				case ParameterName::BorderTopWidth: compiled.borderTopWidth = it.value.sizeValue; break;
				case ParameterName::BorderTopColor: compiled.borderTopColor = it.value.color4; break;
				case ParameterName::BorderRightStyle: compiled.borderRightStyle = it.value.borderStyle; break;
				case ParameterName::BorderRightWidth: compiled.borderRightWidth = it.value.sizeValue; break;
				case ParameterName::BorderRightColor: compiled.borderRightColor = it.value.color4; break;
				case ParameterName::BorderBottomStyle: compiled.borderBottomStyle = it.value.borderStyle; break;
				case ParameterName::BorderBottomWidth: compiled.borderBottomWidth = it.value.sizeValue; break;
				case ParameterName::BorderBottomColor: compiled.borderBottomColor = it.value.color4; break;
				case ParameterName::OutlineStyle: compiled.outlineStyle = it.value.borderStyle; break;
				case ParameterName::OutlineWidth: compiled.outlineWidth = it.value.sizeValue; break;
				case ParameterName::OutlineColor: compiled.outlineColor = it.value.color4; break;
				default: break;
				}
			}
		}
		return compiled;
	}

	void writeStyle(StringStream &stream, const style::Metric &size) {
		switch (size.metric) {
		case style::Metric::Units::Percent:
			stream << size.value * 100 << "%";
			break;
		case style::Metric::Units::Px:
			stream << size.value << "px";
			break;
		case style::Metric::Units::Em:
			stream << size.value << "em";
			break;
		case style::Metric::Units::Rem:
			stream << size.value << "rem";
			break;
		case style::Metric::Units::Auto:
			stream << "auto";
			break;
		case style::Metric::Units::Dpi:
			stream << size.value << "dpi";
			break;
		case style::Metric::Units::Dppx:
			stream << size.value << "dppx";
			break;
		case style::Metric::Units::Contain:
			stream << "contain";
			break;
		case style::Metric::Units::Cover:
			stream << "cover";
			break;
		case style::Metric::Units::Vw:
			stream << "vw";
			break;
		case style::Metric::Units::Vh:
			stream << "vh";
			break;
		case style::Metric::Units::VMin:
			stream << "vmin";
			break;
		case style::Metric::Units::VMax:
			stream << "vmax";
			break;
		}
	}

	void writeStyle(StringStream &stream, const Color3B &c) {
		stream << "rgb(" << (int)c.r << ", " << (int)c.g << ", " << (int)c.b << ")";
	}

	void writeStyle(StringStream &stream, const Color4B &c) {
		stream << "rgba(" << (int)c.r << ", " << (int)c.g << ", " << (int)c.b << ", " << (float)c.a / 255.0f << ")";
	}

	void writeStyle(StringStream &stream, const uint8_t &o) {
		stream << (float)o / 255.0f;
	}

	void writeStyle(StringStream &stream, const CssStringId &s, const RendererInterface *iface) {
		if (iface) {
			stream << "\"" << iface->getCssString(s) << "\"";
		} else {
			stream << "StringId(" << s << ")";
		}
	}

	String ParameterList::css(const RendererInterface *iface) const {
		StringStream stream;
		stream << "{\n";
		for (auto &it : data) {
			stream << "\t";
			switch (it.name) {
			case ParameterName::Unknown:
				stream << "unknown";
				break;
			case ParameterName::FontStyle:
				stream << "font-style: ";
				switch (it.value.fontStyle) {
				case FontStyle::Italic: stream << "italic"; break;
				case FontStyle::Normal: stream << "normal"; break;
				case FontStyle::Oblique: stream << "oblique"; break;
				};
				break; // enum
			case ParameterName::FontWeight:
				stream << "font-weight: ";
				switch (it.value.fontWeight) {
				case FontWeight::Bold: stream << "bold"; break;
				case FontWeight::Normal: stream << "normal"; break;
				case FontWeight::W100: stream << "100"; break;
				case FontWeight::W200: stream << "200"; break;
				case FontWeight::W300: stream << "300"; break;
				case FontWeight::W500: stream << "500"; break;
				case FontWeight::W600: stream << "600"; break;
				case FontWeight::W800: stream << "800"; break;
				case FontWeight::W900: stream << "900"; break;
				};
				break; // enum
			case ParameterName::FontStretch:
				stream << "font-stretch: ";
				switch (it.value.fontStretch) {
				case FontStretch::Normal: stream << "normal"; break;
				case FontStretch::UltraCondensed: stream << "ultra-condensed"; break;
				case FontStretch::ExtraCondensed: stream << "extra-condensed"; break;
				case FontStretch::Condensed: stream << "condensed"; break;
				case FontStretch::SemiCondensed: stream << "semi-condensed"; break;
				case FontStretch::SemiExpanded: stream << "semi-expanded"; break;
				case FontStretch::Expanded: stream << "expanded"; break;
				case FontStretch::ExtraExpanded: stream << "extra-expanded"; break;
				case FontStretch::UltraExpanded: stream << "ultra-expanded"; break;
				};
				break; // enum
			case ParameterName::FontVariant:
				stream << "font-variant: ";
				switch (it.value.fontVariant) {
				case FontVariant::Normal: stream << "normal"; break;
				case FontVariant::SmallCaps: stream << "smapp-caps"; break;
				};
				break; // enum
			case ParameterName::FontSize:
				stream << "font-size: ";
				switch (it.value.fontSize) {
				case FontSize::XXSmall: stream << "xx-small"; break;
				case FontSize::XSmall: stream << "x-small"; break;
				case FontSize::Small: stream << "small"; break;
				case FontSize::Medium: stream << "medium"; break;
				case FontSize::Large: stream << "large"; break;
				case FontSize::XLarge: stream << "x-large"; break;
				case FontSize::XXLarge: stream << "xx-large"; break;
				};
				break; // enum
			case ParameterName::FontSizeNumeric:
				stream << "font-size: ";
				writeStyle(stream, it.value.sizeValue);
				break;
			case ParameterName::FontSizeIncrement:
				stream << "font-size-increment: ";
				switch (it.value.fontSizeIncrement) {
				case FontSizeIncrement::XSmaller: stream << "x-smaller"; break;
				case FontSizeIncrement::Smaller: stream << "smaller"; break;
				case FontSizeIncrement::None: stream << "none"; break;
				case FontSizeIncrement::Larger: stream << "larger"; break;
				case FontSizeIncrement::XLarger: stream << "x-larger"; break;
				};
				break; // enum
			case ParameterName::TextTransform:
				stream << "text-transform: ";
				switch (it.value.textTransform) {
				case TextTransform::Uppercase: stream << "uppercase"; break;
				case TextTransform::Lowercase: stream << "lovercase"; break;
				case TextTransform::None: stream << "none"; break;
				};
				break; // enum
			case ParameterName::TextDecoration:
				stream << "text-decoration: ";
				switch (it.value.textDecoration) {
				case TextDecoration::Underline: stream << "underline"; break;
				case TextDecoration::None: stream << "none"; break;
				};
				break; // enum
			case ParameterName::TextAlign:
				stream << "text-align: ";
				switch (it.value.textAlign) {
				case TextAlign::Center: stream << "center"; break;
				case TextAlign::Left: stream << "left"; break;
				case TextAlign::Right: stream << "right"; break;
				case TextAlign::Justify: stream << "justify"; break;
				};
				break; // enum
			case ParameterName::WhiteSpace:
				stream << "white-space: ";
				switch (it.value.whiteSpace) {
				case WhiteSpace::Normal: stream << "normal"; break;
				case WhiteSpace::Nowrap: stream << "nowrap"; break;
				case WhiteSpace::Pre: stream << "pre"; break;
				case WhiteSpace::PreLine: stream << "pre-line"; break;
				case WhiteSpace::PreWrap: stream << "pre-wrap"; break;
				};
				break; // enum
			case ParameterName::Hyphens:
				stream << "hyphens: ";
				switch (it.value.hyphens) {
				case Hyphens::None: stream << "none"; break;
				case Hyphens::Manual: stream << "manual"; break;
				case Hyphens::Auto: stream << "auto"; break;
				};
				break; // enum
			case ParameterName::Display:
				stream << "display: ";
				switch (it.value.display) {
				case Display::None: stream << "none"; break;
				case Display::Default: stream << "default"; break;
				case Display::RunIn: stream << "run-in"; break;
				case Display::Inline: stream << "inline"; break;
				case Display::InlineBlock: stream << "inline-block"; break;
				case Display::Block: stream << "block"; break;
				case Display::ListItem: stream << "list-item"; break;
				};
				break; // enum
			case ParameterName::Float:
				stream << "float: ";
				switch (it.value.floating) {
				case Float::None: stream << "none"; break;
				case Float::Left: stream << "left"; break;
				case Float::Right: stream << "right"; break;
				};
				break; // enum
			case ParameterName::Clear:
				stream << "clear: ";
				switch (it.value.clear) {
				case Clear::None: stream << "none"; break;
				case Clear::Left: stream << "left"; break;
				case Clear::Right: stream << "right"; break;
				case Clear::Both: stream << "both"; break;
				};
				break; // enum
			case ParameterName::ListStylePosition:
				stream << "list-style-position: ";
				switch (it.value.listStylePosition) {
				case ListStylePosition::Outside: stream << "outside"; break;
				case ListStylePosition::Inside: stream << "inside"; break;
				};
				break; // enum
			case ParameterName::ListStyleType:
				stream << "list-style-type: ";
				switch (it.value.listStyleType) {
				case ListStyleType::None: stream << "none"; break;
				case ListStyleType::Circle: stream << "circle"; break;
				case ListStyleType::Disc: stream << "disc"; break;
				case ListStyleType::Square: stream << "square"; break;
				case ListStyleType::XMdash: stream << "x-mdash"; break;
				case ListStyleType::Decimal: stream << "decimial"; break;
				case ListStyleType::DecimalLeadingZero: stream << "decimial-leading-zero"; break;
				case ListStyleType::LowerAlpha: stream << "lower-alpha"; break;
				case ListStyleType::LowerGreek: stream << "lower-greek"; break;
				case ListStyleType::LowerRoman: stream << "lower-roman"; break;
				case ListStyleType::UpperAlpha: stream << "upper-alpha"; break;
				case ListStyleType::UpperRoman: stream << "upper-roman"; break;
				};
				break; // enum
			case ParameterName::XListStyleOffset:
				stream << "x-list-style-offset: ";
				writeStyle(stream, it.value.sizeValue);
				break; // enum
			case ParameterName::Color:
				stream << "color: ";
				writeStyle(stream, it.value.color);
				break; // color
			case ParameterName::Opacity:
				stream << "opacity: ";
				writeStyle(stream, it.value.opacity);
				break; // opacity
			case ParameterName::TextIndent:
				stream << "text-indent: ";
				writeStyle(stream, it.value.sizeValue);
				break; // size
			case ParameterName::LineHeight:
				stream << "line-height: ";
				writeStyle(stream, it.value.sizeValue);
				break; // size
			case ParameterName::MarginTop:
				stream << "margin-top: ";
				writeStyle(stream, it.value.sizeValue);
				break; // size
			case ParameterName::MarginRight:
				stream << "margin-right: ";
				writeStyle(stream, it.value.sizeValue);
				break; // size
			case ParameterName::MarginBottom:
				stream << "margin-bottom: ";
				writeStyle(stream, it.value.sizeValue);
				break; // size
			case ParameterName::MarginLeft:
				stream << "margin-left: ";
				writeStyle(stream, it.value.sizeValue);
				break; // size
			case ParameterName::Width:
				stream << "width: ";
				writeStyle(stream, it.value.sizeValue);
				break; // size
			case ParameterName::Height:
				stream << "height: ";
				writeStyle(stream, it.value.sizeValue);
				break; // size
			case ParameterName::MinWidth:
				stream << "min-width: ";
				writeStyle(stream, it.value.sizeValue);
				break; // size
			case ParameterName::MinHeight:
				stream << "min-height: ";
				writeStyle(stream, it.value.sizeValue);
				break; // size
			case ParameterName::MaxWidth:
				stream << "max-width: ";
				writeStyle(stream, it.value.sizeValue);
				break; // size
			case ParameterName::MaxHeight:
				stream << "max-height: ";
				writeStyle(stream, it.value.sizeValue);
				break; // size
			case ParameterName::PaddingTop:
				stream << "padding-top: ";
				writeStyle(stream, it.value.sizeValue);
				break; // size
			case ParameterName::PaddingRight:
				stream << "padding-right: ";
				writeStyle(stream, it.value.sizeValue);
				break; // size
			case ParameterName::PaddingBottom:
				stream << "padding-bottom: ";
				writeStyle(stream, it.value.sizeValue);
				break; // size
			case ParameterName::PaddingLeft:
				stream << "padding-left: ";
				writeStyle(stream, it.value.sizeValue);
				break; // size
			case ParameterName::FontFamily:
				stream << "font-family: ";
				writeStyle(stream, it.value.stringId, iface);
				break; // string id
			case ParameterName::BackgroundColor:
				stream << "background-color: ";
				writeStyle(stream, it.value.color4);
				break; // color4
			case ParameterName::BackgroundImage:
				stream << "background-image: ";
				writeStyle(stream, it.value.stringId, iface);
				break; // string id
			case ParameterName::BackgroundPositionX:
				stream << "background-position-x: ";
				writeStyle(stream, it.value.sizeValue);
				break; // size
			case ParameterName::BackgroundPositionY:
				stream << "background-position-y: ";
				writeStyle(stream, it.value.sizeValue);
				break; // size
			case ParameterName::BackgroundRepeat:
				stream << "background-repeat: ";
				switch (it.value.backgroundRepeat) {
				case BackgroundRepeat::NoRepeat: stream << "no-repeat"; break;
				case BackgroundRepeat::Repeat: stream << "repeat"; break;
				case BackgroundRepeat::RepeatX: stream << "repeat-x"; break;
				case BackgroundRepeat::RepeatY: stream << "repeat-y"; break;
				};
				break; // enum
			case ParameterName::BackgroundSizeWidth:
				stream << "background-size-width: ";
				writeStyle(stream, it.value.sizeValue);
				break; // size
			case ParameterName::BackgroundSizeHeight:
				stream << "background-size-height: ";
				writeStyle(stream, it.value.sizeValue);
				break; // size
			case ParameterName::VerticalAlign:
				stream << "vertical-align: ";
				switch (it.value.verticalAlign) {
				case VerticalAlign::Baseline: stream << "baseline"; break;
				case VerticalAlign::Center: stream << "center"; break;
				case VerticalAlign::Sub: stream << "sub"; break;
				case VerticalAlign::Super: stream << "super"; break;
				};
				break; // enum
			case ParameterName::BorderTopStyle:
				stream << "border-top-style: ";
				switch (it.value.borderStyle) {
				case BorderStyle::None: stream << "none"; break;
				case BorderStyle::Solid: stream << "solid"; break;
				case BorderStyle::Dotted: stream << "dotted"; break;
				case BorderStyle::Dashed: stream << "dashed"; break;
				};
				break; // enum
			case ParameterName::BorderTopWidth:
				stream << "border-top-width: ";
				writeStyle(stream, it.value.sizeValue);
				break; // size
			case ParameterName::BorderTopColor:
				stream << "border-top-color: ";
				writeStyle(stream, it.value.color4);
				break; // color4
			case ParameterName::BorderRightStyle:
				stream << "border-right-style: ";
				switch (it.value.borderStyle) {
				case BorderStyle::None: stream << "none"; break;
				case BorderStyle::Solid: stream << "solid"; break;
				case BorderStyle::Dotted: stream << "dotted"; break;
				case BorderStyle::Dashed: stream << "dashed"; break;
				};
				break; // enum
			case ParameterName::BorderRightWidth:
				stream << "border-right-width: ";
				writeStyle(stream, it.value.sizeValue);
				break; // size
			case ParameterName::BorderRightColor:
				stream << "border-right-color: ";
				writeStyle(stream, it.value.color4);
				break; // color4
			case ParameterName::BorderBottomStyle:
				stream << "border-bottom-style: ";
				switch (it.value.borderStyle) {
				case BorderStyle::None: stream << "none"; break;
				case BorderStyle::Solid: stream << "solid"; break;
				case BorderStyle::Dotted: stream << "dotted"; break;
				case BorderStyle::Dashed: stream << "dashed"; break;
				};
				break; // enum
			case ParameterName::BorderBottomWidth:
				stream << "border-bottom-width: ";
				writeStyle(stream, it.value.sizeValue);
				break; // size
			case ParameterName::BorderBottomColor:
				stream << "border-bottom-color: ";
				writeStyle(stream, it.value.color4);
				break; // color4
			case ParameterName::BorderLeftStyle:
				stream << "border-left-style: ";
				switch (it.value.borderStyle) {
				case BorderStyle::None: stream << "none"; break;
				case BorderStyle::Solid: stream << "solid"; break;
				case BorderStyle::Dotted: stream << "dotted"; break;
				case BorderStyle::Dashed: stream << "dashed"; break;
				};
				break; // enum
			case ParameterName::BorderLeftWidth:
				stream << "border-left-width: ";
				writeStyle(stream, it.value.sizeValue);
				break; // size
			case ParameterName::BorderLeftColor:
				stream << "border-left-color: ";
				writeStyle(stream, it.value.color4);
				break; // color4
			case ParameterName::OutlineStyle:
				stream << "outline-style: ";
				switch (it.value.borderStyle) {
				case BorderStyle::None: stream << "none"; break;
				case BorderStyle::Solid: stream << "solid"; break;
				case BorderStyle::Dotted: stream << "dotted"; break;
				case BorderStyle::Dashed: stream << "dashed"; break;
				};
				break; // enum
			case ParameterName::OutlineWidth:
				stream << "outline-width: ";
				writeStyle(stream, it.value.sizeValue);
				break; // size
			case ParameterName::OutlineColor:
				stream << "outline-color: ";
				writeStyle(stream, it.value.color4);
				break; // color4
			case ParameterName::PageBreakAfter:
				stream << "page-break-after: ";
				switch (it.value.pageBreak) {
				case PageBreak::Always: stream << "always"; break;
				case PageBreak::Auto: stream << "auto"; break;
				case PageBreak::Avoid: stream << "avoid"; break;
				case PageBreak::Left: stream << "left"; break;
				case PageBreak::Right: stream << "right"; break;
				};
				break; // enum
			case ParameterName::PageBreakBefore:
				stream << "page-break-before: ";
				switch (it.value.pageBreak) {
				case PageBreak::Always: stream << "always"; break;
				case PageBreak::Auto: stream << "auto"; break;
				case PageBreak::Avoid: stream << "avoid"; break;
				case PageBreak::Left: stream << "left"; break;
				case PageBreak::Right: stream << "right"; break;
				};
				break; // enum
			case ParameterName::PageBreakInside:
				stream << "page-break-inside: ";
				switch (it.value.pageBreak) {
				case PageBreak::Always: stream << "always"; break;
				case PageBreak::Auto: stream << "auto"; break;
				case PageBreak::Avoid: stream << "avoid"; break;
				case PageBreak::Left: stream << "left"; break;
				case PageBreak::Right: stream << "right"; break;
				};
				break; // enum
			case ParameterName::Autofit:
				stream << "x-autofit: ";
				switch (it.value.autofit) {
				case Autofit::None: stream << "none"; break;
				case Autofit::Cover: stream << "cover"; break;
				case Autofit::Contain: stream << "contain"; break;
				case Autofit::Width: stream << "width"; break;
				case Autofit::Height: stream << "height"; break;
				};
				break; // enum

			/* media - specific */
			case ParameterName::MediaType:
				stream << "media-type: ";
				switch (it.value.mediaType) {
				case MediaType::All: stream << "all"; break;
				case MediaType::Screen: stream << "screen"; break;
				case MediaType::Print: stream << "print"; break;
				case MediaType::Speech: stream << "speach"; break;
				};
				break; // enum
			case ParameterName::Orientation:
				stream << "orientation: ";
				switch (it.value.orientation) {
				case Orientation::Landscape: stream << "landscape"; break;
				case Orientation::Portrait: stream << "portrait"; break;
				};
				break; // enum
			case ParameterName::Pointer:
				stream << "pointer: ";
				switch (it.value.pointer) {
				case Pointer::None: stream << "none"; break;
				case Pointer::Fine: stream << "fine"; break;
				case Pointer::Coarse: stream << "coarse"; break;
				};
				break; // enum
			case ParameterName::Hover:
				stream << "hover: ";
				switch (it.value.hover) {
				case Hover::None: stream << "none"; break;
				case Hover::Hover: stream << "hover"; break;
				case Hover::OnDemand: stream << "on-demand"; break;
				};
				break; // enum
			case ParameterName::LightLevel:
				stream << "light-level: ";
				switch (it.value.lightLevel) {
				case LightLevel::Dim: stream << "dim"; break;
				case LightLevel::Washed: stream << "washed"; break;
				case LightLevel::Normal: stream << "normal"; break;
				};
				break; // enum
			case ParameterName::Scripting:
				stream << "scripting: ";
				switch (it.value.scripting) {
				case Scripting::None: stream << "none"; break;
				case Scripting::InitialOnly: stream << "initial-only"; break;
				case Scripting::Enabled: stream << "enabled"; break;
				};
				break; // enum
			case ParameterName::AspectRatio:
				stream << "aspect-ratio: ";
				writeStyle(stream, it.value.floatValue);
				break;
			case ParameterName::MinAspectRatio:
				stream << "min-aspect-ratio: ";
				writeStyle(stream, it.value.floatValue);
				break;
			case ParameterName::MaxAspectRatio:
				stream << "max-aspect-ratio: ";
				writeStyle(stream, it.value.floatValue);
				break;
			case ParameterName::Resolution:
				stream << "resolution";
				writeStyle(stream, it.value.sizeValue);
				break;
			case ParameterName::MinResolution:
				stream << "min-resolution: ";
				writeStyle(stream, it.value.sizeValue);
				break;
			case ParameterName::MaxResolution:
				stream << "max-resolution: ";
				writeStyle(stream, it.value.sizeValue);
				break;
			case ParameterName::MediaOption:
				stream << "media-option: ";
				writeStyle(stream, it.value.stringId, iface);
				break;
			}
			if (it.mediaQuery != MediaQueryNone()) {
				stream << " media(" << it.mediaQuery;
				if (iface && iface->resolveMediaQuery(it.mediaQuery)) {
					stream << ":passed";
				}
				stream << ")";
			}
			stream << ";\n";
		}
		stream << "}\n";
		return stream.str();
	}

	FontStyleParameters FontStyleParameters::create(const String &str) {
		FontStyleParameters ret;

		enum State {
			Family,
			Size,
			Style,
			Weight,
			Stretch,
			Overflow,
		} state = Family;

		string::split(str, ".", [&] (const StringView &ir) {
			StringView r(ir);
			switch (state) {
			case Family:
				ret.fontFamily = r.str();
				state = Size;
				break;
			case Size:
				if (r.is("xxs")) { ret.fontSize = style::FontSize::XXSmall; }
				else if (r.is("xs")) { ret.fontSize = style::FontSize::XSmall; }
				else if (r.is("s")) { ret.fontSize = style::FontSize::Small; }
				else if (r.is("m")) { ret.fontSize = style::FontSize::Medium; }
				else if (r.is("l")) {ret.fontSize = style::FontSize::Large; }
				else if (r.is("xl")) { ret.fontSize = style::FontSize::XLarge; }
				else if (r.is("xxl")) { ret.fontSize = style::FontSize::XXLarge; }
				else { ret.fontSize = uint8_t(r.readInteger()); }
				state = Style;
				break;
			case Style:
				if (r.is("i")) { ret.fontStyle = style::FontStyle::Italic; }
				else if (r.is("o")) { ret.fontStyle = style::FontStyle::Oblique; }
				else if (r.is("n")) { ret.fontStyle = style::FontStyle::Normal; }
				state = Weight;
				break;
			case Weight:
				if (r.is("n")) { ret.fontWeight = style::FontWeight::Normal; }
				else if (r.is("b")) { ret.fontWeight = style::FontWeight::Bold; }
				else if (r.is("100")) { ret.fontWeight = style::FontWeight::W100; }
				else if (r.is("200")) { ret.fontWeight = style::FontWeight::W200; }
				else if (r.is("300")) { ret.fontWeight = style::FontWeight::W300; }
				else if (r.is("400")) { ret.fontWeight = style::FontWeight::W400; }
				else if (r.is("500")) { ret.fontWeight = style::FontWeight::W500; }
				else if (r.is("600")) { ret.fontWeight = style::FontWeight::W600; }
				else if (r.is("700")) { ret.fontWeight = style::FontWeight::W700; }
				else if (r.is("800")) { ret.fontWeight = style::FontWeight::W800; }
				else if (r.is("900")) { ret.fontWeight = style::FontWeight::W900; }
				state = Stretch;
				break;
			case Stretch:
				if (r.is("n")) { ret.fontStretch = style::FontStretch::Normal; }
				else if (r.is("ucd")) { ret.fontStretch = style::FontStretch::UltraCondensed; }
				else if (r.is("ecd")) { ret.fontStretch = style::FontStretch::ExtraCondensed; }
				else if (r.is("cd")) { ret.fontStretch = style::FontStretch::Condensed; }
				else if (r.is("scd")) { ret.fontStretch = style::FontStretch::SemiCondensed; }
				else if (r.is("sex")) { ret.fontStretch = style::FontStretch::SemiExpanded; }
				else if (r.is("ex")) { ret.fontStretch = style::FontStretch::Expanded; }
				else if (r.is("eex")) { ret.fontStretch = style::FontStretch::ExtraExpanded; }
				else if (r.is("uex")) { ret.fontStretch = style::FontStretch::UltraExpanded; }
				state = Overflow;
				break;
			default: break;
			}
		});
		return ret;
	}

	String FontStyleParameters::getConfigName(bool caps) const {
		return getFontConfigName(fontFamily, fontSize, fontStyle, fontWeight, fontStretch, fontVariant, caps);
	}

	FontStyleParameters FontStyleParameters::getSmallCaps() const {
		FontStyleParameters ret = *this;
		ret.fontSize -= ret.fontSize / 5;
		return ret;
	}

	BackgroundParameters::BackgroundParameters(Autofit a) {
		backgroundPositionX.metric = Metric::Units::Auto;
		backgroundPositionY.metric = Metric::Units::Auto;
		switch (a) {
		case Autofit::Contain:
			backgroundSizeWidth.metric = Metric::Units::Contain;
			backgroundSizeHeight.metric = Metric::Units::Contain;
			break;
		case Autofit::Cover:
			backgroundSizeWidth.metric = Metric::Units::Cover;
			backgroundSizeHeight.metric = Metric::Units::Cover;
			break;
		case Autofit::Width:
			backgroundSizeWidth.metric = Metric::Units::Percent;
			backgroundSizeWidth.value = 1.0f;
			backgroundSizeHeight.metric = Metric::Units::Auto;
			break;
		case Autofit::Height:
			backgroundSizeWidth.metric = Metric::Units::Auto;
			backgroundSizeHeight.metric = Metric::Units::Percent;
			backgroundSizeHeight.value = 1.0f;
			break;
		case Autofit::None:
			backgroundSizeWidth.metric = Metric::Units::Percent;
			backgroundSizeWidth.value = 1.0f;
			backgroundSizeHeight.metric = Metric::Units::Percent;
			backgroundSizeHeight.value = 1.0f;
			break;
		}
	}

	String FontFace::getConfigName(const String &family, uint8_t size) const {
		return getFontConfigName(family, size, fontStyle, fontWeight, fontStretch, FontVariant::Normal, false);
	}

	FontStyleParameters FontFace::getStyle(const String &family, uint8_t size) const {
		FontStyleParameters ret;
		ret.fontFamily = family;
		ret.fontSize = size;
		ret.fontStretch = fontStretch;
		ret.fontStyle = fontStyle;
		ret.fontWeight = fontWeight;
		return ret;
	}

	String getFontConfigName(const String &fontFamily, uint8_t fontSize, FontStyle fontStyle, FontWeight fontWeight,
			FontStretch fontStretch, FontVariant fontVariant, bool caps) {
		auto size = fontSize;
		String name = fontFamily;
		if (caps && fontVariant == style::FontVariant::SmallCaps) {
			size -= size / 5;
		}
		if (size == style::FontSize::XXSmall) {
			name += ".xxs";
		} else if (size == style::FontSize::XSmall) {
			name += ".xs";
		} else if (size == style::FontSize::Small) {
			name += ".s";
		} else if (size == style::FontSize::Medium) {
			name += ".m";
		} else if (size == style::FontSize::Large) {
			name += ".l";
		} else if (size == style::FontSize::XLarge) {
			name += ".xl";
		} else if (size == style::FontSize::XXLarge) {
			name += ".xxl";
		} else {
			name += "." + toString(size);
		}

		switch (fontStyle) {
		case style::FontStyle::Normal: name += ".n"; break;
		case style::FontStyle::Italic: name += ".i"; break;
		case style::FontStyle::Oblique: name += ".o"; break;
		}

		switch (fontWeight) {
		case style::FontWeight::Normal: name += ".n"; break;
		case style::FontWeight::Bold: name += ".b"; break;
		case style::FontWeight::W100: name += ".100"; break;
		case style::FontWeight::W200: name += ".200"; break;
		case style::FontWeight::W300: name += ".300"; break;
		case style::FontWeight::W500: name += ".500"; break;
		case style::FontWeight::W600: name += ".600"; break;
		case style::FontWeight::W800: name += ".800"; break;
		case style::FontWeight::W900: name += ".900"; break;
		}

		switch (fontStretch) {
		case style::FontStretch::Normal: name += ".n"; break;
		case style::FontStretch::UltraCondensed: name += ".ucd"; break;
		case style::FontStretch::ExtraCondensed: name += ".ecd"; break;
		case style::FontStretch::Condensed: name += ".cd"; break;
		case style::FontStretch::SemiCondensed: name += ".scd"; break;
		case style::FontStretch::SemiExpanded: name += ".sex"; break;
		case style::FontStretch::Expanded: name += ".ex"; break;
		case style::FontStretch::ExtraExpanded: name += ".eex"; break;
		case style::FontStretch::UltraExpanded: name += ".uex"; break;
		}

		return name;
	}

}

NS_LAYOUT_END
