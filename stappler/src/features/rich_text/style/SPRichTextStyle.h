/*
 * SPRichTextStyle.h
 *
 *  Created on: 21 апр. 2015 г.
 *      Author: sbkarr
 */

#ifndef LIBS_STAPPLER_FEATURES_RICH_TEXT_SPRICHTEXTSTYLE_H_
#define LIBS_STAPPLER_FEATURES_RICH_TEXT_SPRICHTEXTSTYLE_H_

#include "SPDefine.h"
#include "SPCharReader.h"

#include "base/ccTypes.h"

NS_SP_EXT_BEGIN(rich_text)

using CharGroupId = chars::CharGroupId;
using StringReader = CharReaderUtf8;

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

constexpr uint32_t RTEngineVersion() { return 2; }

namespace style {
	struct Size;

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
	using CssStringFunction = Function<void(CssStringId, const String &)>;
}

using Style = style::ParameterList;
using FontStyle = style::FontStyleParameters;
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
	virtual String getCssString(CssStringId) const = 0;
};

namespace style {
	struct Size {
		enum class Metric : EnumSize {
			Percent,
			Px,
			Em,
			Auto,
			Dpi,
			Dppx,
			Contain, // only for background-size
			Cover, // only for background-size
		};

		float computeValue(float base, float fontSize = 0.0f, bool autoIsZero = false) const;
		inline bool isAuto() const { return metric == Metric::Auto; }

		float value = 0.0f;
		Metric metric = Metric::Auto;

		Size(float value, Metric m) : value(value), metric(m) { }

		Size() = default;
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
		Bold = W700,
		Normal = W400,
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

	namespace FontSize {
		constexpr uint8_t XXSmall = 8;
		constexpr uint8_t XSmall = 10;
		constexpr uint8_t Small = 12;
		constexpr uint8_t Medium = 14;
		constexpr uint8_t Large = 16;
		constexpr uint8_t XLarge = 20;
		constexpr uint8_t XXLarge = 24;
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
		Center,
		Sub,
		Super,
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
		cocos2d::Color3B color;
		uint8_t fontSize;
		uint8_t opacity;
		cocos2d::Color4B color4;
		Size sizeValue;
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

		template<ParameterName Name, class Value>
		void set(const Value &);
	};

	struct FontStyleParameters {
		using FontStyle = style::FontStyle;
		using FontWeight = style::FontWeight;

		FontStyle fontStyle = FontStyle::Normal;
		FontWeight fontWeight = FontWeight::Normal;
		FontStretch fontStretch = FontStretch::Normal;
		FontVariant fontVariant = FontVariant::Normal;
		ListStyleType listStyleType = ListStyleType::None;
		uint8_t fontSize = FontSize::Medium;
		String fontFamily;

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
		Size textIndent;
		Size lineHeight;
		Size listOffset;

		inline bool operator == (const ParagraphLayoutParameters &other) const { return memcmp(this, &other, sizeof(ParagraphLayoutParameters)) == 0; }
		inline bool operator != (const ParagraphLayoutParameters &other) const { return !(*this == other); }
	};

	struct TextLayoutParameters {
		TextTransform textTransform = TextTransform::None;
		TextDecoration textDecoration = TextDecoration::None;
		WhiteSpace whiteSpace = WhiteSpace::Normal;
		Hyphens hyphens = Hyphens::Manual;
		VerticalAlign verticalAlign = VerticalAlign::Baseline;
		cocos2d::Color3B color = cocos2d::Color3B::BLACK;
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

		Size marginTop;
		Size marginRight;
		Size marginBottom;
		Size marginLeft;

		Size paddingTop;
		Size paddingRight;
		Size paddingBottom;
		Size paddingLeft;

		Size width;
		Size height;
		Size minWidth;
		Size minHeight;
		Size maxWidth;
		Size maxHeight;

		inline bool operator == (const BlockModelParameters &other) const {
			return memcmp(this, &other, sizeof(BlockModelParameters)) == 0; }
		inline bool operator != (const BlockModelParameters &other) const { return !(*this == other); }
	};

	struct InlineModelParameters {
		Size marginTop;
		Size marginRight;
		Size marginBottom;
		Size marginLeft;

		Size paddingTop;
		Size paddingRight;
		Size paddingBottom;
		Size paddingLeft;

		Size width;
		Size height;
		Size minWidth;
		Size minHeight;
		Size maxWidth;
		Size maxHeight;

		inline bool operator == (const InlineModelParameters &other) const { return memcmp(this, &other, sizeof(InlineModelParameters)) == 0; }
		inline bool operator != (const InlineModelParameters &other) const { return !(*this == other); }
	};

	struct BackgroundParameters {
		Display display = Display::Default;
		Color4B backgroundColor;
		BackgroundRepeat backgroundRepeat = BackgroundRepeat::NoRepeat;
		Size backgroundPositionX;
		Size backgroundPositionY;
		Size backgroundSizeWidth;
		Size backgroundSizeHeight;

		String backgroundImage;

		inline bool operator == (const BackgroundParameters &other) const {
			return memcmp(this, &other, sizeof(BackgroundParameters) - sizeof(String)) == 0
					&& backgroundImage == other.backgroundImage;
		}
		inline bool operator != (const BackgroundParameters &other) const { return !(*this == other); }
	};

	struct OutlineParameters {
		BorderStyle borderLeftStyle = BorderStyle::None;
		Size borderLeftWidth;
		Color4B borderLeftColor;
		BorderStyle borderTopStyle = BorderStyle::None;
		Size borderTopWidth;
		Color4B borderTopColor;
		BorderStyle borderRightStyle = BorderStyle::None;
		Size borderRightWidth;
		Color4B borderRightColor;
		BorderStyle borderBottomStyle = BorderStyle::None;
		Size borderBottomWidth;
		Color4B borderBottomColor;
		BorderStyle outlineStyle = BorderStyle::None;
		Size outlineWidth;
		Color4B outlineColor;

		inline bool operator == (const OutlineParameters &other) const { return memcmp(this, &other, sizeof(InlineModelParameters)) == 0; }
		inline bool operator != (const OutlineParameters &other) const { return !(*this == other); }
	};

	struct ParameterList {
		static bool isInheritable(ParameterName name);
		static bool isAllowedForMediaQuery(ParameterName name);

		template<ParameterName Name, class Value> void set(const Value &value, MediaQueryId mediaQuery = MediaQueryNone());
		void set(const Parameter &p, bool force = false);

		void read(const StyleVec &, MediaQueryId mediaQuary = MediaQueryNone());

		void merge(const ParameterList &, bool inherit = false);
		void merge(const StyleVec &);

		Vector<Parameter> get(ParameterName) const;
		Vector<Parameter> get(ParameterName, const RendererInterface *) const;

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

		style::StyleVec style;
		style::StyleVec weakStyle;
		style::ParameterList compiledStyle;

		Map<String, String> attributes;

		bool closable = false;
		bool autoRefs = false;

		operator bool() const {return !name.empty();}

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

	struct FontFace {
		Vector<String> src;

		FontStyle fontStyle = FontStyle::Normal;
		FontWeight fontWeight = FontWeight::Normal;
		FontStretch fontStretch = FontStretch::Normal;

		String getConfigName(const String &family, uint8_t size) const;

		FontFace() = default;

		FontFace(String && src, FontStyle style = FontStyle::Normal,
				FontWeight weight = FontWeight::Normal, FontStretch stretch = FontStretch::Normal)
		: src{std::move(src)}, fontStyle(style), fontWeight(weight), fontStretch(stretch) { }

		FontFace(Vector<String> && src, FontStyle style = FontStyle::Normal,
				FontWeight weight = FontWeight::Normal, FontStretch stretch = FontStretch::Normal)
		: src(std::move(src)), fontStyle(style), fontWeight(weight), fontStretch(stretch) { }
	};

	struct CssData {
		Map<String, ParameterList> styles;
		Map<String, Vector<FontFace>> fonts;
	};

	ParameterList getStyleForTag(const String &, Tag::Type type);

	bool readColor(const String &str, cocos2d::Color4B &color4);

	bool readStyleValue(const String &str, Size &value, bool resolutionMetric = false, bool allowEmptyMetric = false);
	String readStringContents(const String &prefix, const String &origStr, CssStringId &value);
	bool readStringValue(const String &prefix, const String &origStr, CssStringId &value);
	bool splitValue(const String &str, String &first, String &second);
	bool readAspectRatioValue(const String &str, float &value);
	bool readStyleMargin(const String &origStr, Size &top, Size &right, Size &bottom, Size &left);

	bool readMediaParameter(Vector<Parameter> &params, const String &name, const String &value, const CssStringFunction &cb);

	String getFontConfigName(const String &, uint8_t, FontStyle, FontWeight, FontStretch, FontVariant, bool caps);
}

NS_SP_EXT_END(rich_text)

#endif /* LIBS_STAPPLER_FEATURES_RICH_TEXT_SPRICHTEXTSTYLE_H_ */
