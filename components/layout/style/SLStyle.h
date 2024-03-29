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

#ifndef LAYOUT_STYLE_SLSTYLE_H_
#define LAYOUT_STYLE_SLSTYLE_H_

#include "SPStringView.h"
#include "SPSpanView.h"
#include "SPLayout.h"

NS_LAYOUT_BEGIN

using StringReader = StringViewUtf8;

using HtmlIdentifier = chars::Compose<char16_t,
		chars::Range<char16_t, u'0', u'9'>,
		chars::Range<char16_t, u'A', u'Z'>,
		chars::Range<char16_t, u'a', u'z'>,
		chars::Chars<char16_t, u'_', u'-', u'!', u'/', u':'>
>;

using CssIdentifier = chars::Compose<char16_t,
		chars::Range<char16_t, u'0', u'9'>,
		chars::Range<char16_t, u'A', u'Z'>,
		chars::Range<char16_t, u'a', u'z'>,
		chars::Chars<char16_t, u'_', u'-', u'!', u'.', u'*', u'#', u'@'>
>;

using CssSelector = chars::Compose<char16_t,
		chars::Range<char16_t, u'0', u'9'>,
		chars::Range<char16_t, u'a', u'z'>,
		chars::Range<char16_t, u'A', u'Z'>,
		chars::Range<char16_t, u'а', u'я'>,
		chars::Range<char16_t, u'А', u'Я'>,
		chars::Chars<char16_t, u'\u0020', u'\u00A0', u'-', u'_', u'!', u'.', u':', u'*', u'+', u'#', u'@', u'ё', u'Ё'>
>;

using SingleQuote = chars::Chars<char16_t, u'\''>;
using DoubleQuote = chars::Chars<char16_t, u'"'>;

using MediaQueryId = uint16_t;
inline MediaQueryId MediaQueryNone() { return maxOf<MediaQueryId>(); }

using CssStringId = uint32_t;
inline CssStringId CssStringNone() { return maxOf<CssStringId>(); }

namespace style {
	struct Metric;

	struct FontStyleParameters;
	struct ParagraphLayoutParameters;
	struct TextLayoutParameters;
	struct BlockModelParameters;
	struct InlineModelParameters;
	struct BackgroundParameters;
	struct OutlineParameters;
	struct ParameterList;
	struct Tag;

	using StyleVec = Vector<Pair<String, String>>;

	using EnumSize = uint8_t;
	using NameSize = uint16_t;

	using CssStyleFunction = Function<void(const String &, const StyleVec &)>;
	using CssStringFunction = Function<void(CssStringId, const StringView &)>;
}

using Style = style::ParameterList;
using FontParams = style::FontStyleParameters;
using ParagraphStyle = style::ParagraphLayoutParameters;
using TextStyle = style::TextLayoutParameters;
using BlockStyle = style::BlockModelParameters;
using InlineStyle = style::InlineModelParameters;
using BackgroundStyle = style::BackgroundParameters;
using OutlineStyle = style::OutlineParameters;

class RendererInterface {
public:
	virtual ~RendererInterface() { }
	virtual bool resolveMediaQuery(MediaQueryId queryId) const = 0;
	virtual StringView getCssString(CssStringId) const = 0;
};

class SimpleRendererInterface : public RendererInterface {
public:
	virtual ~SimpleRendererInterface() { }
	SimpleRendererInterface();
	SimpleRendererInterface(const Vector<bool> *, const Map<CssStringId, String> *);

	virtual bool resolveMediaQuery(MediaQueryId queryId) const;
	virtual StringView getCssString(CssStringId) const;

protected:
	const Vector<bool> *_media = nullptr;
	const Map<CssStringId, String> *_strings = nullptr;
};

namespace style {
	struct Metric {
		enum Units : EnumSize {
			Percent,
			Px,
			Em,
			Rem,
			Auto,
			Dpi,
			Dppx,
			Contain, // only for background-size
			Cover, // only for background-size
			Vw,
			Vh,
			VMin,
			VMax
		};

		float computeValueStrong(float base, const MediaParameters &, float fontSize = nan()) const;
		float computeValueAuto(float base, const MediaParameters &, float fontSize = nan()) const;

		inline bool isAuto() const { return metric == Units::Auto; }

		inline bool isFixed() const {
			switch (metric) {
			case Units::Px:
			case Units::Em:
			case Units::Rem:
			case Units::Vw:
			case Units::Vh:
			case Units::VMin:
			case Units::VMax:
				return true;
				break;
			default:
				break;
			}
			return false;
		}

		float value = 0.0f;
		Units metric = Units::Auto;

		Metric(float value, Units m) : value(value), metric(m) { }

		Metric() = default;
	};

	enum class FontStyle : EnumSize {
		Normal,
		Italic,
		Oblique,
	};

	enum class FontWeight : EnumSize {
		W100,
		W200,
		W300,
		W400,
		W500,
		W600,
		W700,
		W800,
		W900,
		Thin = W100,
		ExtraLight = W200,
		Light = W300,
		Normal = W400,
		Regular = W400,
		Medium = W500,
		SemiBold = W600,
		Bold = W700,
		ExtraBold = W800,
		Heavy = W900,
		Black = W900,
	};

	enum class FontStretch : EnumSize {
		UltraCondensed,
		ExtraCondensed,
		Condensed,
		SemiCondensed,
		Normal,
		SemiExpanded,
		Expanded,
		ExtraExpanded,
		UltraExpanded
	};

	struct FontSize : ValueWrapper<uint16_t, class FontSizeFlag> {
		static const FontSize XXSmall;
		static const FontSize XSmall;
		static const FontSize Small;
		static const FontSize Medium;
		static const FontSize Large;
		static const FontSize XLarge;
		static const FontSize XXLarge;

		using ValueWrapper::ValueWrapper;
	};

	enum class FontVariant : EnumSize {
		Normal,
		SmallCaps,
	};

	enum class FontSizeIncrement : EnumSize {
		None,
		XSmaller,
		Smaller,
		Larger,
		XLarger,
	};

	enum class TextTransform : EnumSize {
		None,
		Uppercase,
		Lowercase,
	};

	enum class TextDecoration : EnumSize {
		None,
		LineThrough,
		Overline,
		Underline,
	};

	enum class TextAlign : EnumSize {
		Left,
		Center,
		Right,
		Justify,
	};

	enum class WhiteSpace : EnumSize {
		Normal,
		Nowrap,
		Pre,
		PreLine,
		PreWrap,
	};

	enum class Hyphens : EnumSize {
		None,
		Manual,
		Auto,
	};

	enum class Display : EnumSize {
		None,
		Default,
		RunIn,
		Inline,
		InlineBlock,
		Block,
		ListItem,
		Table,
		TableCell,
		TableColumn,
		TableCaption,
	};

	enum class Float : EnumSize {
		None,
		Left,
		Right,
	};

	enum class Clear : EnumSize {
		None,
		Left,
		Right,
		Both,
	};

	enum class BackgroundRepeat : EnumSize {
		NoRepeat,
		Repeat,
		RepeatX,
		RepeatY,
	};

	enum class VerticalAlign : EnumSize {
		Baseline,
		Middle,
		Sub,
		Super,
		Top,
		Bottom
	};

	enum class BorderStyle : EnumSize {
		None,
		Solid,
		Dotted,
		Dashed,
	};

	enum class MediaType : EnumSize {
		All,
		Screen,
		Print,
		Speech,
	};

	enum class Orientation : EnumSize {
		Landscape,
		Portrait,
	};

	enum class Pointer : EnumSize {
		Fine,
		Coarse,
		None,
	};

	enum class Hover : EnumSize {
		Hover,
		OnDemand,
		None,
	};

	enum class LightLevel : EnumSize {
		Normal,
		Dim,
		Washed,
	};

	enum class Scripting : EnumSize {
		None,
		InitialOnly,
		Enabled,
	};

	enum class ListStyleType : EnumSize {
		None,
		Circle,
		Disc,
		Square,
		XMdash,
		Decimal,
		DecimalLeadingZero,
		LowerAlpha,
		LowerGreek,
		LowerRoman,
		UpperAlpha,
		UpperRoman
	};

	enum class ListStylePosition : EnumSize {
		Outside,
		Inside,
	};

	enum class PageBreak : EnumSize {
		Always,
		Auto,
		Avoid,
		Left,
		Right
	};

	enum class Autofit : EnumSize {
		None,
		Width,
		Height,
		Cover,
		Contain,
	};

	enum class BorderCollapse : EnumSize {
		Separate,
		Collapse,
	};

	enum class CaptionSide : EnumSize {
		Top,
		Bottom,
	};

	enum class ParameterName : NameSize {
		/* css-selectors */

		Unknown = 0,
		FontStyle = 1, // enum
		FontWeight, // enum
		FontSize, // enum
		FontVariant, // enum
		FontSizeIncrement, // enum
		FontStretch, // enum
		FontSizeNumeric, // size
		TextTransform, // enum
		TextDecoration, // enum
		TextAlign, // enum
		WhiteSpace, // enum
		Hyphens, // enum
		Display, // enum
		Float, // enum
		Clear, // enum
		Color, // color
		Opacity, // opacity
		TextIndent, // size
		LineHeight, // size
		MarginTop, // size
		MarginRight, // size
		MarginBottom, // size
		MarginLeft, // size
		Width, // size
		Height, // size
		MinWidth, // size
		MinHeight, // size
		MaxWidth, // size
		MaxHeight, // size
		PaddingTop, // size
		PaddingRight, // size
		PaddingBottom, // size
		PaddingLeft, // size
		FontFamily, // string id
		BackgroundColor, // color4
		BackgroundImage, // string id
		BackgroundPositionX, // size
		BackgroundPositionY, // size
		BackgroundRepeat, // enum
		BackgroundSizeWidth, // size
		BackgroundSizeHeight, // size
		VerticalAlign, // enum
		BorderTopStyle, // enum
		BorderTopWidth, // size
		BorderTopColor, // color4
		BorderRightStyle, // enum
		BorderRightWidth, // size
		BorderRightColor, // color4
		BorderBottomStyle, // enum
		BorderBottomWidth, // size
		BorderBottomColor, // color4
		BorderLeftStyle, // enum
		BorderLeftWidth, // size
		BorderLeftColor, // color4
		OutlineStyle, // enum
		OutlineWidth, // size
		OutlineColor, // color4
		ListStyleType, // enum
		ListStylePosition, // enum
		XListStyleOffset, // size
		PageBreakBefore, // enum
		PageBreakAfter, // enum
		PageBreakInside, // enum
		Autofit, // enum
		BorderCollapse, // enum
		CaptionSide, // enum

		/* media - specific */
		MediaType = 256,
		Orientation,
		Pointer,
		Hover,
		LightLevel,
		Scripting,
		AspectRatio,
		MinAspectRatio,
		MaxAspectRatio,
		Resolution,
		MinResolution,
		MaxResolution,
		MediaOption,
	};

	union ParameterValue {
		FontStyle fontStyle;
		FontWeight fontWeight;
		FontSizeIncrement fontSizeIncrement;
		FontStretch fontStretch;
		FontVariant fontVariant;
		TextTransform textTransform;
		TextDecoration textDecoration;
		TextAlign textAlign;
		WhiteSpace whiteSpace;
		Hyphens hyphens;
		Display display;
		Float floating;
		Clear clear;
		MediaType mediaType;
		Orientation orientation;
		Pointer pointer;
		Hover hover;
		LightLevel lightLevel;
		Scripting scripting;
		BackgroundRepeat backgroundRepeat;
		VerticalAlign verticalAlign;
		BorderStyle borderStyle;
		ListStyleType listStyleType;
		ListStylePosition listStylePosition;
		PageBreak pageBreak;
		Autofit autofit;
		BorderCollapse borderCollapse;
		CaptionSide captionSide;
		Color3B color;
		FontSize fontSize;
		uint8_t opacity;
		Color4B color4;
		Metric sizeValue;
		float floatValue;
		CssStringId stringId;

		ParameterValue();
	};

	struct Parameter {
		ParameterName name = ParameterName::Unknown;
		MediaQueryId mediaQuery = MediaQueryNone();
		ParameterValue value;

		template<ParameterName Name, class Value>
		static Parameter create(const Value &value, MediaQueryId mediaQuery = MediaQueryNone());

		Parameter(ParameterName name, MediaQueryId query);
		Parameter(const Parameter &, MediaQueryId query);

		template<ParameterName Name, class Value>
		void set(const Value &);
	};

	struct FontStyleParameters {
		using FontStyle = style::FontStyle;
		using FontWeight = style::FontWeight;

		static FontStyleParameters create(const String &);

		FontStyle fontStyle = FontStyle::Normal;
		FontWeight fontWeight = FontWeight::Normal;
		FontStretch fontStretch = FontStretch::Normal;
		FontVariant fontVariant = FontVariant::Normal;
		ListStyleType listStyleType = ListStyleType::None;
		FontSize fontSize = FontSize::Medium;
		StringView fontFamily;

		String getConfigName(bool caps = false) const;
		FontStyleParameters getSmallCaps() const;

		inline bool operator == (const FontStyleParameters &other) const {
			return fontStyle == other.fontStyle && fontWeight == other.fontWeight
					&& fontSize == other.fontSize && fontStretch == other.fontStretch
					&& fontVariant == other.fontVariant
					&& listStyleType == other.listStyleType && fontFamily == other.fontFamily; }
		inline bool operator != (const FontStyleParameters &other) const { return !(*this == other); }
	};

	struct ParagraphLayoutParameters {
		TextAlign textAlign = TextAlign::Left;
		Metric textIndent;
		Metric lineHeight;
		Metric listOffset;

		inline bool operator == (const ParagraphLayoutParameters &other) const { return memcmp(this, &other, sizeof(ParagraphLayoutParameters)) == 0; }
		inline bool operator != (const ParagraphLayoutParameters &other) const { return !(*this == other); }
	};

	struct TextLayoutParameters {
		TextTransform textTransform = TextTransform::None;
		TextDecoration textDecoration = TextDecoration::None;
		WhiteSpace whiteSpace = WhiteSpace::Normal;
		Hyphens hyphens = Hyphens::Manual;
		VerticalAlign verticalAlign = VerticalAlign::Baseline;
		Color3B color = Color3B::BLACK;
		uint8_t opacity = 222;

		inline bool operator == (const TextLayoutParameters &other) const {
			return memcmp(this, &other, sizeof(TextLayoutParameters)) == 0; }
		inline bool operator != (const TextLayoutParameters &other) const { return !(*this == other); }
	};

	struct BlockModelParameters {
		Display display = Display::Default;
		Float floating = Float::None;
		Clear clear = Clear::None;
		ListStyleType listStyleType = ListStyleType::None;
		ListStylePosition listStylePosition = ListStylePosition::Outside;

		PageBreak pageBreakBefore = PageBreak::Auto;
		PageBreak pageBreakAfter = PageBreak::Auto;
		PageBreak pageBreakInside = PageBreak::Auto;
		BorderCollapse borderCollapse = BorderCollapse::Collapse;
		CaptionSide captionSide = CaptionSide::Top;

		Metric marginTop;
		Metric marginRight;
		Metric marginBottom;
		Metric marginLeft;

		Metric paddingTop;
		Metric paddingRight;
		Metric paddingBottom;
		Metric paddingLeft;

		Metric width;
		Metric height;
		Metric minWidth;
		Metric minHeight;
		Metric maxWidth;
		Metric maxHeight;

		inline bool operator == (const BlockModelParameters &other) const {
			return memcmp(this, &other, sizeof(BlockModelParameters)) == 0; }
		inline bool operator != (const BlockModelParameters &other) const { return !(*this == other); }
	};

	struct InlineModelParameters {
		Metric marginRight;
		Metric marginLeft;
		Metric paddingRight;
		Metric paddingLeft;

		inline bool operator == (const InlineModelParameters &other) const { return memcmp(this, &other, sizeof(InlineModelParameters)) == 0; }
		inline bool operator != (const InlineModelParameters &other) const { return !(*this == other); }
	};

	struct BackgroundParameters {
		Display display = Display::Default;
		Color4B backgroundColor;
		BackgroundRepeat backgroundRepeat = BackgroundRepeat::NoRepeat;
		Metric backgroundPositionX;
		Metric backgroundPositionY;
		Metric backgroundSizeWidth;
		Metric backgroundSizeHeight;

		StringView backgroundImage;

		BackgroundParameters() = default;
		BackgroundParameters(Autofit);

		inline bool operator == (const BackgroundParameters &other) const {
			return memcmp(this, &other, sizeof(BackgroundParameters) - sizeof(StringView)) == 0
					&& backgroundImage == other.backgroundImage;
		}
		inline bool operator != (const BackgroundParameters &other) const { return !(*this == other); }
	};

	struct OutlineParameters {
		struct Params {
			BorderStyle style = BorderStyle::None;
			Metric width;
			Color4B color;
		};

		Params left;
		Params top;
		Params right;
		Params bottom;
		Params outline;

		inline bool operator == (const OutlineParameters &other) const { return memcmp(this, &other, sizeof(OutlineParameters)) == 0; }
		inline bool operator != (const OutlineParameters &other) const { return !(*this == other); }
	};

	struct ParameterList {
		static bool isInheritable(ParameterName name);
		static bool isAllowedForMediaQuery(ParameterName name);

		template<ParameterName Name, class Value> void set(const Value &value, MediaQueryId mediaQuery = MediaQueryNone());
		void set(const Parameter &p, bool force = false);

		void read(const StyleVec &, MediaQueryId mediaQuary = MediaQueryNone());
		void read(const StringView &, const StringView &, MediaQueryId mediaQuary = MediaQueryNone());

		void merge(const ParameterList &, bool inherit = false);
		void merge(const ParameterList &, const Vector<bool> &, bool inherit = false);
		void merge(const StyleVec &);

		Vector<Parameter> get(ParameterName) const;
		Vector<Parameter> get(ParameterName, const RendererInterface *) const;
		Parameter get(ParameterName, const Vector<bool> &) const;

		FontStyleParameters compileFontStyle(const RendererInterface *) const;
		TextLayoutParameters compileTextLayout(const RendererInterface *) const;
		ParagraphLayoutParameters compileParagraphLayout(const RendererInterface *) const;
		BlockModelParameters compileBlockModel(const RendererInterface *) const;
		InlineModelParameters compileInlineModel(const RendererInterface *) const;
		BackgroundParameters compileBackground(const RendererInterface *) const;
		OutlineParameters compileOutline(const RendererInterface *) const;

		bool operator == (const ParameterList &other);
		bool operator != (const ParameterList &other);

		String css(const RendererInterface * = nullptr) const;

		Vector<Parameter> data;
	};

	struct Tag {
		enum Type {
			Style,
			Block,
			Image,
			Special,
			Markup,
		};

		String name; // tag name
		String id; // <tag id="">
		String xType;
		Vector<String> classes; // <tag class="class1 class2 ...etc">

		Type type = Block;

		ParameterList style;

		Map<String, String> attributes;

		bool closable = false;
		bool autoRefs = false;

		operator bool() const { return !name.empty(); }

		static Tag::Type getType(const String &tagName);
	};

	struct MediaQuery {
		struct Query {
			bool negative = false;
			Vector<Parameter> params;

			bool setMediaType(const String &);
		};

		static MediaQueryId IsScreenLayout;
		static MediaQueryId IsPrintLayout;
		static MediaQueryId NoTooltipOption;
		static MediaQueryId IsTooltipOption;

		Vector<Query> list;

		void clear();
		bool parse(const String &, const CssStringFunction &cb);

		inline operator bool() const { return !list.empty(); }

		static Vector<MediaQuery> getDefaultQueries(Map<CssStringId, String> &);
	};

	/** Single font face definition
	 *
	 * Font face is combination of:
	 * - font style (Normal/Italic)
	 * - font weight (Normal/Bold/100-900)
	 * - font stretch (Condensed/Normal/Extended)
	 *
	 * Font face defined with sources stack
	 * - if char face was not found in front source, it can be found in next source
	 * - search continues until char face is found or all sources is processed
	 * - if char face was not found, engine-wide fallback font can be used
	 */
	struct FontFace {
		struct FontFaceSource {
			String file;
			BytesView bytes;

			FontFaceSource() { }
			FontFaceSource(String &&str) : file(move(str)) { }
			FontFaceSource(const String &str) : file(str) { }
			FontFaceSource(const BytesView &bytes) : bytes(bytes) { }
		};

		Vector<FontFaceSource> src;

		FontStyle fontStyle = FontStyle::Normal;
		FontWeight fontWeight = FontWeight::Normal;
		FontStretch fontStretch = FontStretch::Normal;

		String getConfigName(const StringView &family, FontSize size) const;

		FontStyleParameters getStyle(const StringView &family, FontSize size) const;

		FontFace() = default;

		FontFace(String && src, FontStyle style = FontStyle::Normal,
				FontWeight weight = FontWeight::Normal, FontStretch stretch = FontStretch::Normal)
		: src{FontFaceSource(std::move(src))}, fontStyle(style), fontWeight(weight), fontStretch(stretch) { }

		FontFace(FontFaceSource && src, FontStyle style = FontStyle::Normal,
				FontWeight weight = FontWeight::Normal, FontStretch stretch = FontStretch::Normal)
		: src{std::move(src)}, fontStyle(style), fontWeight(weight), fontStretch(stretch) { }

		FontFace(Vector<FontFaceSource> && src, FontStyle style = FontStyle::Normal,
				FontWeight weight = FontWeight::Normal, FontStretch stretch = FontStretch::Normal)
		: src(std::move(src)), fontStyle(style), fontWeight(weight), fontStretch(stretch) { }
	};

	ParameterList getStyleForTag(const StringView &, const StringView &parent = StringView());

	bool readColor(const StringView &str, Color4B &color4);
	bool readColor(const StringView &str, Color3B &color);

	bool readStyleValue(const StringView &str, Metric &value, bool resolutionMetric = false, bool allowEmptyMetric = false);
	StringView readStringContents(const String &prefix, const StringView &origStr, CssStringId &value);
	bool readStringValue(const String &prefix, const StringView &origStr, CssStringId &value);
	bool splitValue(const StringView &str, StringView &first, StringView &second);
	bool readAspectRatioValue(const StringView &str, float &value);
	bool readStyleMargin(const StringView &origStr, Metric &top, Metric &right, Metric &bottom, Metric &left);

	bool readMediaParameter(Vector<Parameter> &params, const String &name, const StringView &value, const CssStringFunction &cb);

	String getFontConfigName(const StringView &, FontSize, FontStyle, FontWeight, FontStretch, FontVariant, bool caps);

	template<ParameterName Name, class Value> Parameter Parameter::create(const Value &v, MediaQueryId query) {
		static_assert(Name != ParameterName::FontSize || !std::is_same_v<Value, uint8_t>, "uint8_t as FontSize is deprecated");
		Parameter p(Name, query);
		p.set<Name>(v);
		return p;
	}

	template<ParameterName Name, class Value> void ParameterList::set(const Value &value, MediaQueryId mediaQuery) {
		if (mediaQuery != MediaQueryNone() && !isAllowedForMediaQuery(Name)) {
			return;
		}
		for (auto &it : data) {
			if (it.name == Name && it.mediaQuery == mediaQuery) {
				it.set<Name>(value);
				return;
			}
		}

		data.push_back(Parameter::create<Name>(value, mediaQuery));
	}

	constexpr FontSize FontSize::XXSmall = FontSize(uint16_t(8));
	constexpr FontSize FontSize::XSmall = FontSize(uint16_t(10));
	constexpr FontSize FontSize::Small = FontSize(uint16_t(12));
	constexpr FontSize FontSize::Medium = FontSize(uint16_t(14));
	constexpr FontSize FontSize::Large = FontSize(uint16_t(16));
	constexpr FontSize FontSize::XLarge = FontSize(uint16_t(20));
	constexpr FontSize FontSize::XXLarge = FontSize(uint16_t(24));
}

NS_LAYOUT_END

namespace std {

template <>
struct hash<stappler::layout::style::FontSize> {
	hash() { }

	size_t operator() (const stappler::layout::style::FontSize &value) const noexcept {
		return hash<uint16_t>{}(value.get());
	}
};

}

#endif /* LAYOUT_STYLE_SLSTYLE_H_ */
