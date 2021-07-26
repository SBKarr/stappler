
#ifndef UTILS_SRC_ODSTYLE_H_
#define UTILS_SRC_ODSTYLE_H_

#include "OpenDocument.h"
#include "SPLayout.h"

namespace opendocument::style {

using EnumSize = uint8_t;
using NameSize = uint16_t;

//using StringId = stappler::ValueWrapper<uint32_t, class StringIdTag>;
struct StringId {
	uint32_t value;

	bool operator <(const StringId r) const { return value < r.value; }
};

using Color = stappler::layout::Color;
using Color3B = stappler::layout::Color3B;
using Color4B = stappler::layout::Color4B;

using FontStyle = stappler::layout::style::FontStyle;
using FontVariant = stappler::layout::style::FontVariant;
using FontWeight = stappler::layout::style::FontWeight;
using TextTransform = stappler::layout::style::TextTransform;
using FontFamilyGeneric = Font::Generic;
using FontPitch = Font::Pitch;

enum class VerticalAlign : EnumSize {
	Top,
	Middle,
	Bottom,
	Auto,
	Baseline,
	Automatic
};

enum class TextAlign : EnumSize {
	Start,
	End,
	Left,
	Center,
	Right,
	Justify,
};

enum class TextAlignLast : EnumSize {
	Start,
	Center,
	Justify,
};

enum class TableAlign : EnumSize {
	Left,
	Center,
	Right,
	Margins,
};

enum class TextAlignSource : EnumSize {
	Fix,
	ValueType
};

enum class BorderModel : EnumSize {
	Collapsing,
	Separating,
};

enum class TextPositionAlign : EnumSize {
	Baseline,
	Sub,
	Super,
};

enum class LineMode : EnumSize {
	Continuous,
	SkipWhiteSpace
};

enum class LineStyle : EnumSize {
	None,
	Dash,
	DotDash,
	DotDotDash,
	Dotted,
	LongDash,
	Solid,
	Wave
};

enum class LineType : EnumSize {
	Double,
	Single
};

enum class BreakMode : EnumSize {
	Auto,
	Column,
	Page
};

enum class KeepMode : EnumSize {
	Auto,
	Always
};

enum class LineBreak : EnumSize {
	Normal,
	Strict
};

enum class PunctuationWrap : EnumSize {
	Simple,
	Hanging
};

enum class VerticalPos : EnumSize {
	Top,
	Middle,
	Bottom,
	FromTop,
	Below
};

enum class VerticalRel : EnumSize {
	Page,
	PageContent,
	Frame,
	FrameContent,
	Paragraph,
	ParagraphContent,
	Char,
	Line,
	Baseline,
	Text
};

enum class HorizontalPos : EnumSize {
	Left,
	Center,
	Right,
	FromLeft,
	Inside,
	Outside,
	FromInside
};

enum class HorizontalRel : EnumSize {
	Page,
	PageContent,
	PageStartMargin,
	PageEndMargin,
	Frame,
	FrameContent,
	FrameStartMargin,
	FrameEndMargin,
	Paragraph,
	ParagraphContent,
	ParagraphStartMargin,
	ParagraphEndMargin,
	Char
};

enum class ListLevelPositionMode : EnumSize {
	LabelAlignment,
	LabelWidthAndPosition
};

enum class LabelFollowedBy : EnumSize {
	Listtab,
	Space,
	Nothing
};

enum class ColorMode : EnumSize {
	Greyscale,
	Mono,
	Watermark,
	Standard
};

enum class DrawAspect : EnumSize {
	Content,
	Thumbnail,
	Icon,
	PrintView
};

enum class DrawFill : EnumSize {
	None,
	Solid,
	Bitmap,
	Gradient,
	Hatch
};

enum class FillImageRefPoint : EnumSize {
	TopLeft,
	Top,
	TopRight,
	Left,
	Center,
	Right,
	BottomLeft,
	Bottom,
	BottomRight
};

enum class DrawStroke : EnumSize {
	None,
	Dash,
	Solid
};

enum class StrokeLinejoin : EnumSize {
	Miter,
	Round,
	Bevel,
	Middle,
	None
};

enum class StrokeLinecap : EnumSize {
	Butt,
	Square,
	Round
};

enum class DrawUnit : EnumSize {
	Automatic,
	Mm,
	Cm,
	M,
	Km,
	Pt,
	Pc,
	Inch,
	Ft,
	Mi
};

enum class OverflowBehavior : EnumSize {
	Clip, // clip
	AutoCreateNewFrame, // auto-create-new-frame
};

enum class Protect : EnumSize {
	None,
	Content = 1,
	Position = 2,
	Size = 4
};

SP_DEFINE_ENUM_AS_MASK(Protect);

enum class Mirror : EnumSize {
	None,
	Vertical,
	Horizontal,
	HorizontalOnOdd, // horizontal-on-odd
	HHorizontalOnEven, // horizontal-on-even
	VerticalHorizontal,
	VerticalHorizontalOnOdd, // horizontal-on-odd
	VerticalHHorizontalOnEven // horizontal-on-even
};

enum class Repeat : EnumSize {
	NoRepeat,
	Repeat,
	Stretch
};

enum class Wrap : EnumSize {
	None,
	Left,
	Right,
	Parallel,
	Dynamic,
	RunThrough,
	Biggest
};

enum class WrapContourMode : EnumSize {
	Full,
	Outside,
};

enum class FillRule : EnumSize {
	Nonzero,
	Evenodd
};

enum class AnchorType : EnumSize {
	Page,
	Frame,
	Paragraph,
	Char,
	AsChar
};

enum class RunThrough : EnumSize {
	Background,
	Foreground
};

enum class FootnotesPosition : EnumSize {
	Text,
	Page,
	Section,
	Document
};

enum class StartNumberingAt : EnumSize {
	Document,
	Chapter,
	Page
};

struct TextPosition {
	float value = 1.0f; // percents
	TextPositionAlign align = TextPositionAlign::Baseline;
};

struct Metric {
	enum Units {
		None,
		Percent,
		Float,
		Integer,
		Custom,
		Cm,
		Mm,
		In,
		Pt,
		Pc,
		Px,
		Em,
	};

	enum CastomValue : uint32_t {
		Auto,
		Normal,
		Bold,
		Thin,
		Medium,
		Thick,

		Scale,
		ScaleMin,
	};

	union {
		int32_t signedValue = 0;
		float floatValue;
		CastomValue customValue;
	};

	Units metric = Units::None;

	Metric(float value, Units m = Units::Percent) : floatValue(value), metric(m) { }
	Metric(int32_t value) : floatValue(value), metric(Units::Integer) { }
	Metric(CastomValue c) : customValue(c), metric(Units::Custom) { }

	Metric() = default;

	bool isValid() const {
		return metric != Units::None;
	}

	bool operator == (const Metric &m) const {
		return memcmp(this, &m, sizeof(Metric)) == 0;
	}

	Metric operator-() const {
		Metric ret = *this;
		switch (ret.metric) {
		case None: break;
		case Custom: break;
		case Integer: ret.signedValue = -ret.signedValue; break;
		default: ret.floatValue = -ret.floatValue; break;
		}
		return ret;
	}

	Metric operator*=(float val) const {
		Metric ret = *this;
		switch (ret.metric) {
		case None: break;
		case Custom: break;
		case Integer: ret.signedValue = int32_t(ret.signedValue * val); break;
		default: ret.floatValue *= val; break;
		}
		return ret;
	}

	String toString() const;
};

struct Padding {
	Metric top;
	Metric right;
	Metric bottom;
	Metric left;

	bool isSingle() const;
};

using Margin = Padding;

struct Border {
	Metric width;
	LineStyle style = LineStyle::None;
	Color3B color;
};

enum class Name : NameSize {
	Unknown = 0,

	__BeginTextParameters,
	TextBackgroundColor, // Color3B
	TextColor, // Color3B
	TextFontFamily, // String
	TextFontSize, // Metric
	TextFontStyle, // FontStyle enum
	TextFontVariant, // FontVariant enum
	TextFontWeight, // FontWeight enum
	TextHyphenate, // bool
	TextHyphenationPushCharCount, // uint32_t
	TextHyphenationRemainCharCount, // uint32_t
	TextLetterSpacing, // Metric
	TextTransform, // enum TextTransform
	TextFontFamilyGeneric, // enum FontFamilyGeneric
	TextFontName, // String
	TextFontPitch, // enum FontPitch
	TextFontStyleName, // String
	TextPosition, // TextPosition
	TextUnderlineColor, // Color3B
	TextUnderlineMode, // enum DecorationLineMode
	TextUnderlineStyle, // enum DecorationLineStyle
	TextUnderlineType, // enum DecorationLineType
	TextUnderlineWidth, // Metric
	TextScale, // float
	TextLanguage, // StringView
	TextCountry, // StringView
	__EndTextParameters,

	__BeginParagraphParameters,
	ParagraphBackgroundColor, // Color3B
	ParagraphBorder,
	ParagraphBorderBottom,
	ParagraphBorderLeft,
	ParagraphBorderRight,
	ParagraphBorderTop,
	ParagraphBorderTopStyle, // enum DecorationLineStyle
	ParagraphBorderTopWidth, // size
	ParagraphBorderTopColor, // color4
	ParagraphBorderRightStyle, // enum DecorationLineStyle
	ParagraphBorderRightWidth, // size
	ParagraphBorderRightColor, // color4
	ParagraphBorderBottomStyle, // enum DecorationLineStyle
	ParagraphBorderBottomWidth, // size
	ParagraphBorderBottomColor, // color4
	ParagraphBorderLeftStyle, // enum DecorationLineStyle
	ParagraphBorderLeftWidth, // size
	ParagraphBorderLeftColor, // color4
	ParagraphBreakAfter, // enum BreakMode
	ParagraphBreakBefore, // enum BreakMode
	ParagraphKeepTogether, // enum KeepMode
	ParagraphKeepWithNext, // enum KeepMode
	ParagraphLineHeight, // Metric
	ParagraphMargin,
	ParagraphMarginTop, // size
	ParagraphMarginRight, // size
	ParagraphMarginBottom, // size
	ParagraphMarginLeft, // size
	ParagraphPadding,
	ParagraphPaddingTop, // size
	ParagraphPaddingRight, // size
	ParagraphPaddingBottom, // size
	ParagraphPaddingLeft, // size
	ParagraphTextAlign, // enum TextAlign
	ParagraphTextAlignLast, // enum TextAlignLast
	ParagraphTextIndent, // Metric
	ParagraphBackgroundTransparency, // float value
	ParagraphAutoTextIndent, // bool
	ParagraphFontIndependentLineSpacing, // bool
	ParagraphJoinBorder, // bool
	ParagraphJustifySingleWord, // bool
	ParagraphLineBreak, // enum LineBreak
	ParagraphLineHeightAtLeast, // Metric
	ParagraphLineSpacing, // Metric
	ParagraphPageNumber, // uint32_t
	ParagraphPunctuationWrap, // enum PunctuationWrap
	ParagraphRegisterTrue, // bool
	ParagraphTabStopDistance, // Metric
	ParagraphVerticalAlign, // enum VerticalAlign
	ParagraphLineNumber, // uint32_t
	ParagraphNumberLines, // bool
	ParagraphHyphenationLadderCount, // uint32_t or maxOf<uint32_t> as no-limit
	ParagraphOrphans, // uint32_t
	ParagraphWidows, // uint32_t
	__EndParagraphParameters,

	__BeginPageParameters,
	PageBackgroundColor, // Color3B
	PageBorder,
	PageBorderBottom,
	PageBorderLeft,
	PageBorderRight,
	PageBorderTop,
	PageBorderTopStyle, // enum DecorationLineStyle
	PageBorderTopWidth, // size
	PageBorderTopColor, // color4
	PageBorderRightStyle, // enum DecorationLineStyle
	PageBorderRightWidth, // size
	PageBorderRightColor, // color4
	PageBorderBottomStyle, // enum DecorationLineStyle
	PageBorderBottomWidth, // size
	PageBorderBottomColor, // color4
	PageBorderLeftStyle, // enum DecorationLineStyle
	PageBorderLeftWidth, // size
	PageBorderLeftColor, // color4
	PageMargin,
	PageMarginTop, // size
	PageMarginRight, // size
	PageMarginBottom, // size
	PageMarginLeft, // size
	PagePadding,
	PagePaddingTop, // size
	PagePaddingRight, // size
	PagePaddingBottom, // size
	PagePaddingLeft, // size
	PageHeight, // Metric
	PageWidth, // Metric
	PageFirstPageNumber, // uint32_t
	PageFootnoteMaxHeight, // Metric
	PageHeaderMinHeight, // size
	PageHeaderHeight, // size
	PageHeaderMargin,
	PageHeaderMarginTop, // size
	PageHeaderMarginRight, // size
	PageHeaderMarginBottom, // size
	PageHeaderMarginLeft, // size
	PageHeaderPadding,
	PageHeaderPaddingTop, // size
	PageHeaderPaddingRight, // size
	PageHeaderPaddingBottom, // size
	PageHeaderPaddingLeft, // size
	PageFooterMinHeight, // size
	PageFooterHeight, // size
	PageFooterMargin,
	PageFooterMarginTop, // size
	PageFooterMarginRight, // size
	PageFooterMarginBottom, // size
	PageFooterMarginLeft, // size
	PageFooterPadding,
	PageFooterPaddingTop, // size
	PageFooterPaddingRight, // size
	PageFooterPaddingBottom, // size
	PageFooterPaddingLeft, // size
	__EndPageParameters,

	__BeginTableParameters,
	TableBackgroundColor, // Color3B
	TableBreakAfter, // enum BreakMode
	TableBreakBefore, // enum BreakMode
	TableKeepWithNext, // enum KeepMode
	TableMargin,
	TableMarginTop, // Metric
	TableMarginRight, // Metric
	TableMarginBottom, // Metric
	TableMarginLeft, // Metric
	TablePageNumber, // uint32_t
	TableMayBreakBetweenRows, // bool
	TableRelWidth, // Metric
	TableWidth, // Metric
	TableAlign, // enum TableAlign
	TableBorderModel, // enum BorderModel
	__EndTableParameters,

	__BeginTableColumnParameters,
	TableColumnBreakAfter, // enum BreakMode
	TableColumnBreakBefore, // enum BreakMode
	TableColumnRelWidth, // uint32_t
	TableColumnWidth, // Metric
	TableColumnUseOptimalWidth, // bool
	__EndTableColumnParameters,

	__BeginTableRowParameters,
	TableRowBackgroundColor, // Color3B
	TableRowBreakAfter, // enum BreakMode
	TableRowBreakBefore, // enum BreakMode
	TableRowKeepTogether, // enum KeepMode
	TableRowMinHeight, // Metric
	TableRowHeight, // Metric
	TableRowUseOptimalHeight, // bool
	__EndTableRowParameters,

	__BeginTableCellParameters,
	TableCellBackgroundColor, // Color3B
	TableCellBorder,
	TableCellBorderBottom,
	TableCellBorderLeft,
	TableCellBorderRight,
	TableCellBorderTop,
	TableCellBorderTopStyle, // enum DecorationLineStyle
	TableCellBorderTopWidth, // size
	TableCellBorderTopColor, // color4
	TableCellBorderRightStyle, // enum DecorationLineStyle
	TableCellBorderRightWidth, // size
	TableCellBorderRightColor, // color4
	TableCellBorderBottomStyle, // enum DecorationLineStyle
	TableCellBorderBottomWidth, // size
	TableCellBorderBottomColor, // color4
	TableCellBorderLeftStyle, // enum DecorationLineStyle
	TableCellBorderLeftWidth, // size
	TableCellBorderLeftColor, // color4
	TableCellPadding,
	TableCellPaddingTop, // size
	TableCellPaddingRight, // size
	TableCellPaddingBottom, // size
	TableCellPaddingLeft, // size
	TableCellDecimalPlaces, // uint32_t
	TableCellPrintContent, // bool
	TableCellRepeatContent, // bool
	TableCellShrinkToFit, // bool
	TableCellTextAlignSource, // enum TextAlignSource
	TableCellVerticalAlign, // enum VerticalAlign
	__EndTableCellParameters,

	__BeginListParameters,
	ListConsecutiveNumbering, // bool
	__EndListParameters,

	__BeginListStyleParameters,
	ListStyleNumPrefix, // string
	ListStyleNumSuffix, // string
	ListStyleNumFormat, // string
	ListStyleNumLetterSync, // bool
	ListStyleBulletChar, // string
	ListStyleBulletRelativeSize, // float
	ListStyleDisplayLevels, // uint32_t
	ListStyleStartValue, // uint32_t
	__EndListStyleParameters,

	__BeginListLevelParameters,
	ListLevelHeight, // Metric
	ListLevelTextAlign, // enum class TextAlign
	ListLevelWidth, // Metric
	ListLevelFontName, // StringView
	ListLevelVerticalPos, // enum VerticalPos
	ListLevelVerticalRel, // enum VerticalRel
	ListLevelY, // Metric
	ListLevelPositionMode, // enum ListLevelPositionMode
	ListLevelMinLabelDistance, // Metric
	ListLevelMinLabelWidth, // Metric
	ListLevelSpaceBefore, // Metric
	__EndListLevelParameters,

	__BeginListLevelLabelParameters,
	ListLevelLabelMarginLeft, // Metric
	ListLevelLabelTextIndent, // Metric
	ListLevelLabelFollowedBy, // enum LabelFollowedBy
	ListLevelLabelTabStopPosition, // Metric
	__EndListLevelLabelParameters,

	__BeginDrawParameters,
	DrawAutoGrowHeight, // bool
	DrawAutoGrowWidth, // bool
	DrawRed, // float
	DrawGreen, // float
	DrawBlue, // float
	DrawColorInversion, // bool
	DrawColorMode, // enum ColorMode
	DrawContrast, // float
	DrawDecimalPlaces, // uint32_t
	DrawAspect, // enum DrawAspect
	DrawFill, // enum DrawFill
	DrawFillColor, // Color3B
	DrawFillGradientName, // StringView
	DrawFillHatchName, // StringView
	DrawFillHatchSolid, // bool
	DrawFillImageHeight, // float
	DrawFillImageWidth, // float
	DrawFillImageName, // StringView
	DrawFillImageRefPoint, // enum FillImageRefPoint
	DrawFillImageRefPointX, // float
	DrawFillImageRefPointY, // float
	DrawFitToContour, // bool
	DrawFitToSize, // bool
	DrawFrameDisplayBorder, // bool
	DrawFrameDisplayScrollbar, // bool
	DrawFrameMarginHorizontal, // uint32_t (as px)
	DrawFrameMarginVertical, // uint32_t (as px)
	DrawGamma, // float
	DrawGradientStepCount, // uint32_t
	DrawImageOpacity, // float
	DrawLuminance, // float
	DrawMarkerEnd, // StringView
	DrawMarkerEndCenter, // bool
	DrawMarkerEndWidth, // Metric
	DrawMarkerStart, // StringView
	DrawMarkerStartCenter, // bool
	DrawMarkerStartWidth, // Metric
	DrawOpacity, // float
	DrawOpacityName, // StringView
	DrawParallel, // bool
	DrawSecondaryFillColor, // Color3B
	DrawStroke, // enum DrawStroke
	DrawStrokeDash, // StringView
	DrawStrokeDashNames, // StringView
	DrawStrokeLinejoin, // enum StrokeLinejoin
	DrawStrokeLinecap, // enum StrokeLinecap  svg:stroke-linecap
	DrawSymbolColor, // Color3B
	DrawVisibleAreaHeight, // Metric
	DrawVisibleAreaLeft, // Metric
	DrawVisibleAreaTop, // Metric
	DrawVisibleAreaWidth, // Metric
	DrawUnit, // enum DrawUnit
	DrawBackgroundColor, /// Color3B
	DrawBorder,
	DrawBorderBottom,
	DrawBorderLeft,
	DrawBorderRight,
	DrawBorderTop,
	DrawBorderTopStyle, // enum DecorationLineStyle
	DrawBorderTopWidth, // size
	DrawBorderTopColor, // color4
	DrawBorderRightStyle, // enum DecorationLineStyle
	DrawBorderRightWidth, // size
	DrawBorderRightColor, // color4
	DrawBorderBottomStyle, // enum DecorationLineStyle
	DrawBorderBottomWidth, // size
	DrawBorderBottomColor, // color4
	DrawBorderLeftStyle, // enum DecorationLineStyle
	DrawBorderLeftWidth, // size
	DrawBorderLeftColor, // color4
	DrawMargin,
	DrawMarginTop, // Metric
	DrawMarginRight, // Metric
	DrawMarginBottom, // Metric
	DrawMarginLeft, // Metric
	DrawPadding,
	DrawPaddingTop, // size
	DrawPaddingRight, // size
	DrawPaddingBottom, // size
	DrawPaddingLeft, // size
	DrawMinHeight, // Metric
	DrawMaxHeight, // Metric
	DrawMinWidth, // Metric
	DrawMaxWidth, // Metric
	DrawBackgroundTransparency, // float style:background-transparency
	DrawEditable, // bool style:editable
	DrawFlowWithText, // bool style:flow-with-text
	DrawHorizontalPos, // enum HorizontalPos  style:horizontal-pos 20.290,
	DrawHorizontalRel, // enum HorizontalRel  style:horizontal-rel 20.291,
	DrawMirror, // enum Mirror  style:mirror
	DrawNumberWrappedParagraphs, // uint32_t or maxOf<uint32_t> as no-limit  style:number-wrapped-paragraphs
	DrawPrintContent, // bool style:print-content
	DrawOverflowBehavior, // enum OverflowBehavior  style:overflow-behavior
	DrawProtect, // enum Protect  style:protect
	DrawRelHeight, // Metric with percent, scale or scale-min  style:rel-height 20.331,
	DrawRelWidth, // Metric with percent, scale or scale-min  style:rel-width 20.332.1
	DrawRepeat, // enum Repeat  style:repeat
	DrawRunThrough, // enum RunThrough  style:run-through
	DrawShrinkToFit, // bool  style:shrink-to-fit
	DrawVerticalPos, // enum VerticalPos  style:vertical-pos 20.290,
	DrawVerticalRel, // enum VerticalRel  style:vertical-rel 20.291,
	DrawWrap, // enum Wrap  style:wrap
	DrawWrapContour, // bool  style:wrap-contour 20.391
	DrawWrapContourMode, // enum WrapContourMode  style:wrap-contour-mode
	DrawWrapDynamicThreshold, // Metric  style:wrap-dynamic-threshold
	DrawWidth, // Metric  svg:width
	DrawHeight, // Metric  svg:height
	DrawX, // Metric  svg:x
	DrawY, // Metric  svg:y
	DrawStrokeWidth, // Metric  svg:stroke-width
	DrawStrokeOpacity, // float  svg:stroke-opacity
	DrawStrokeColor, // Color3B  svg:stroke-color
	DrawFillRule, // enum FillRule  svg:fill-rule
	DrawAnchorPageNumber, // uint32_t  text:anchor-page-number
	DrawAnchorType, // enum AnchorType  text:anchor-type
	DrawClip, // StringView  fo:clip
	__EndDrawParameters,

	__BeginNoteParameters,
	NoteNumFormat, // StringView  style:num-format
	NoteNumLetterSync, // bool  style:num-letter-sync
	NoteNumPrefix, // StringView  style:num-prefix
	NoteNumSuffix, // StringView  style:num-suffix
	NoteStartValue, // uint32_t  text:start-value
	NoteFootnotesPosition, // enum FootnotesPosition  text:footnotes-position
	NoteStartNumberingAt, // enum StartNumberingAt  text:start-numbering-at
	__EndNoteParameters,
};

/* dr3d:ambient-color 20.67, dr3d:backface-culling 20.69, dr3d:back-scale 20.68, dr3d:close-back 20.70, dr3d:close-front 20.71,
 * dr3d:depth 20.72, dr3d:diffuse-color 20.73, dr3d:edge-rounding 20.74, dr3d:edge-rounding-mode 20.75, dr3d:emissive-color 20.76,
 * dr3d:end-angle 20.77, dr3d:horizontal-segments 20.78, dr3d:lighting-mode 20.79, dr3d:normals-direction 20.80, dr3d:normals-kind 20.81,
 * dr3d:shadow 20.82, dr3d:shininess 20.83, dr3d:specular-color 20.84, dr3d:texture-filter 20.85, dr3d:texture-generation-mode-x 20.88,
 * dr3d:texture-generation-mode-y 20.89, dr3d:texture-kind 20.86, dr3d:texture-mode 20.87, dr3d:vertical-segments 20.90,
 *
 * draw:caption-angle 20.95, draw:caption-angle-type 20.96, draw:caption-escape 20.97, draw:caption-escape-direction 20.98,
 * draw:caption-fit-line-length 20.99, draw:caption-gap 20.100, draw:caption-line-length 20.101, draw:caption-type 20.102,
 * draw:end-guide 20.108, draw:end-line-spacing-horizontal 20.109, draw:end-line-spacing-vertical, draw:guide-distance 20.131,
 * draw:guide-overhang 20.132,  draw:line-distance 20.134, draw:measure-align 20.142, draw:measure-vertical-align 20.143,
 * draw:ole-draw-aspect 20.144, draw:placing 20.148, draw:shadow 20.151, draw:shadow-color 20.152, draw:shadow-offset-x 20.153,
 * draw:shadow-offset-y 20.154, draw:shadow-opacity 20.155, draw:show-unit 20.156, draw:start-guide 20.157, draw:start-line-spacing-horizontal 20.158,
 * draw:start-line-spacing-vertical 20.159, draw:textarea-horizontal-align 20.166, draw:textarea-vertical-align 20.167,
 * draw:tile-repeat-offset 20.168, draw:unit 20.173, draw:wrap-influence-on-position 20.174,
 *
 * fo:clip 20.179, fo:wrap-option 20.223,
 *
 * style:run-through 20.343, style:shadow 20.349, style:writing-mode 20.394.2,
 */

union Value {
	static constexpr uint32_t Normal = stappler::maxOf<uint32_t>();

	FontStyle fontStyle;
	FontVariant fontVariant;
	FontWeight fontWeight;
	TextTransform textTransform;
	FontFamilyGeneric fontFamilyGeneric;
	FontPitch fontPitch;
	LineMode lineMode;
	LineStyle lineStyle;
	LineType lineType;
	BreakMode breakMode;
	KeepMode keepMode;
	TextAlign textAlign;
	TextAlignLast textAlignLast;
	LineBreak lineBreak;
	PunctuationWrap punctuationWrap;
	VerticalAlign verticalAlign;
	TableAlign tableAlign;
	BorderModel borderModel;
	TextAlignSource textAlignSource;
	VerticalPos verticalPos;
	VerticalRel verticalRel;
	HorizontalPos horizontalPos;
	HorizontalRel horizontalRel;
	ListLevelPositionMode listLevelMode;
	LabelFollowedBy labelFollowedBy;
	ColorMode colorMode;
	DrawAspect drawAspect;
	DrawFill drawFill;
	FillImageRefPoint fillImageRefPoint;
	DrawStroke drawStroke;
	StrokeLinejoin strokeLinejoin;
	StrokeLinecap strokeLinecap;
	DrawUnit drawUnit;
	OverflowBehavior overflowBehavior;
	Protect protect;
	Mirror mirror;
	Repeat repeat;
	Wrap wrap;
	WrapContourMode wrapContourMode;
	FillRule fillRule;
	AnchorType anchorType;
	RunThrough runThrough;
	FootnotesPosition footnotesPosition;
	StartNumberingAt startNumberingAt;

	Color3B color;
	float floatValue;
	uint32_t unsignedValue;
	int32_t signedValue;
	bool boolValue;
	StringId stringId;
	Metric metric;
	TextPosition textPosition;

	Value();
};

class Style : public AllocBase {
public:
	enum Type {
		Automatic,
		Common,
		Default,
	};

	enum Family {
		Undefined,
		Chart,
		DrawingPage,
		Graphic,
		Paragraph,
		Presentation,
		Ruby,
		Table,
		TableCell,
		TableColumn,
		TableRow,
		Text,
		PageLayout,
		List,
		Note,
	};

	struct Parameter {
		Name name = Name::Unknown;
		Value value;

		Parameter() = default;
		Parameter(Name name);
		Parameter(const Parameter &) = default;
		Parameter(Parameter &&) = default;

		Parameter &set(const FontStyle &val) { value.fontStyle = val; return *this; }
		Parameter &set(const FontVariant &val) { value.fontVariant = val; return *this; }
		Parameter &set(const FontWeight &val) { value.fontWeight = val; return *this; }
		Parameter &set(const TextTransform &val) { value.textTransform = val; return *this; }
		Parameter &set(const FontFamilyGeneric &val) { value.fontFamilyGeneric = val; return *this; }
		Parameter &set(const FontPitch &val) { value.fontPitch = val; return *this; }
		Parameter &set(const LineMode &val) { value.lineMode = val; return *this; }
		Parameter &set(const LineStyle &val) { value.lineStyle = val; return *this; }
		Parameter &set(const LineType &val) { value.lineType = val; return *this; }
		Parameter &set(const BreakMode &val) { value.breakMode = val; return *this; }
		Parameter &set(const KeepMode &val) { value.keepMode = val; return *this; }
		Parameter &set(const TextAlign &val) { value.textAlign = val; return *this; }
		Parameter &set(const TextAlignLast &val) { value.textAlignLast = val; return *this; }
		Parameter &set(const LineBreak &val) { value.lineBreak = val; return *this; }
		Parameter &set(const PunctuationWrap &val) { value.punctuationWrap = val; return *this; }
		Parameter &set(const VerticalAlign &val) { value.verticalAlign = val; return *this; }
		Parameter &set(const TableAlign &val) { value.tableAlign = val; return *this; }
		Parameter &set(const BorderModel &val) { value.borderModel = val; return *this; }
		Parameter &set(const VerticalPos &val) { value.verticalPos = val; return *this; }
		Parameter &set(const VerticalRel &val) { value.verticalRel = val; return *this; }
		Parameter &set(const HorizontalPos &val) { value.horizontalPos = val; return *this; }
		Parameter &set(const HorizontalRel &val) { value.horizontalRel = val; return *this; }
		Parameter &set(const ListLevelPositionMode &val) { value.listLevelMode = val; return *this; }
		Parameter &set(const LabelFollowedBy &val) { value.labelFollowedBy = val; return *this; }

		Parameter &set(const ColorMode &val) { value.colorMode = val; return *this; }
		Parameter &set(const DrawAspect &val) { value.drawAspect = val; return *this; }
		Parameter &set(const DrawFill &val) { value.drawFill = val; return *this; }
		Parameter &set(const FillImageRefPoint &val) { value.fillImageRefPoint = val; return *this; }
		Parameter &set(const DrawStroke &val) { value.drawStroke = val; return *this; }
		Parameter &set(const StrokeLinejoin &val) { value.strokeLinejoin = val; return *this; }
		Parameter &set(const StrokeLinecap &val) { value.strokeLinecap = val; return *this; }
		Parameter &set(const DrawUnit &val) { value.drawUnit = val; return *this; }
		Parameter &set(const OverflowBehavior &val) { value.overflowBehavior = val; return *this; }
		Parameter &set(const Protect &val) { value.protect = val; return *this; }
		Parameter &set(const Mirror &val) { value.mirror = val; return *this; }
		Parameter &set(const Repeat &val) { value.repeat = val; return *this; }
		Parameter &set(const Wrap &val) { value.wrap = val; return *this; }
		Parameter &set(const WrapContourMode &val) { value.wrapContourMode = val; return *this; }
		Parameter &set(const FillRule &val) { value.fillRule = val; return *this; }
		Parameter &set(const AnchorType &val) { value.anchorType = val; return *this; }
		Parameter &set(const RunThrough &val) { value.runThrough = val; return *this; }
		Parameter &set(const FootnotesPosition &val) { value.footnotesPosition = val; return *this; }
		Parameter &set(const StartNumberingAt &val) { value.startNumberingAt = val; return *this; }

		Parameter &set(const TextAlignSource &val) { value.textAlignSource = val; return *this; }
		Parameter &set(const Color3B &val) { value.color = val; return *this; }
		Parameter &set(const float &val) { value.floatValue = val; return *this; }
		Parameter &set(const uint32_t &val) { value.unsignedValue = val; return *this; }
		Parameter &set(const int32_t &val) { value.signedValue = val; return *this; }
		Parameter &set(const bool &val) { value.boolValue = val; return *this; }
		Parameter &set(const StringId &val) { value.stringId = val; return *this; }
		Parameter &set(const Metric &val) { value.metric = val; return *this; }
		Parameter &set(const TextPosition &val) { value.textPosition = val; return *this; }
	};

	class ListLevel : public AllocBase {
	public:
		enum Type {
			Bullet,
			Number,
		};

		template<Name Name, class V>
		ListLevel & set(const V &);

	protected:
		friend class Style;

		ListLevel(Type, Style *, uint32_t l, Function<void(const StringView &)> *);

		Type type = Bullet;
		Style *textStyle = nullptr;
		uint32_t level = 1;
		Vector<Parameter> text;
		Vector<Parameter> listStyle;
		Vector<Parameter> listLevel;
		Vector<Parameter> listLevelLabel;
		Function<void(const StringView &)> *emplaceString;
	};

	template<Name Name, class V>
	Style & set(const V &);

	Style(Type, Family);
	Style(Type, Family, const StringView &, const Style *, Function<void(const StringView &)> &&empl, const StringView &);

	ListLevel &addListLevel(ListLevel::Type, Style * = nullptr);

	void write(const WriteCallback &, const Callback<StringView(StringId)> &, bool pretty) const;

	void writeDefaultStyle(const WriteCallback &, const Callback<StringView(StringId)> &, bool pretty) const;
	void writeCustomStyle(const WriteCallback &, const Callback<StringView(StringId)> &, bool pretty) const;

	Style & setDefaultOutlineLevel(uint32_t);
	Style & setMasterPage(const MasterPage *);

	Type getType() const { return type; }
	Family getFamily() const { return family; }
	StringView getName() const { return name; }
	const Style *getParent() const { return parent; }

	bool empty() const;

protected:
	void writePageProperties(const WriteCallback &cb, const Callback<StringView(StringId)> &stringIdCb, bool pretty) const;
	void writeListProperties(const WriteCallback &cb, const Callback<StringView(StringId)> &stringIdCb, bool pretty, const ListLevel &) const;
	void writeStyleProperties(const WriteCallback &cb, const Callback<StringView(StringId)> &stringIdCb, bool pretty, Family = Family::Undefined) const;
	void writeNoteProperties(const WriteCallback &cb, const Callback<StringView(StringId)> &stringIdCb, bool pretty) const;

	Type type = Type::Common;
	Family family;
	String name;
	const Style * parent = nullptr;
	const MasterPage *master = nullptr;
	String styleClass;
	uint32_t defaultOutline = 0;
	bool consecutiveNumbering = false;

	Vector<Parameter> text;
	Vector<Parameter> paragraph;
	Vector<Parameter> pageLayout;
	Vector<Parameter> table;
	Vector<Parameter> tableCell;
	Vector<Parameter> tableColumn;
	Vector<Parameter> tableRow;
	Vector<Parameter> graphic;
	Vector<Parameter> note;
	Vector<ListLevel *> list;

	Function<void(const StringView &)> emplaceString;
};

}

namespace opendocument {

const WriteCallback &operator<<(const WriteCallback &cb, const style::Color3B &str);
const WriteCallback &operator<<(const WriteCallback &cb, const style::FontStyle &str);
const WriteCallback &operator<<(const WriteCallback &cb, const style::FontVariant &str);
const WriteCallback &operator<<(const WriteCallback &cb, const style::FontWeight &str);
const WriteCallback &operator<<(const WriteCallback &cb, const style::TextTransform &str);
const WriteCallback &operator<<(const WriteCallback &cb, const style::FontFamilyGeneric &str);
const WriteCallback &operator<<(const WriteCallback &cb, const style::FontPitch &str);
const WriteCallback &operator<<(const WriteCallback &cb, const style::LineMode &str);
const WriteCallback &operator<<(const WriteCallback &cb, const style::LineStyle &str);
const WriteCallback &operator<<(const WriteCallback &cb, const style::LineType &str);
const WriteCallback &operator<<(const WriteCallback &cb, const style::BreakMode &str);
const WriteCallback &operator<<(const WriteCallback &cb, const style::KeepMode &str);
const WriteCallback &operator<<(const WriteCallback &cb, const style::TextAlign &str);
const WriteCallback &operator<<(const WriteCallback &cb, const style::TextAlignLast &str);
const WriteCallback &operator<<(const WriteCallback &cb, const style::LineBreak &str);
const WriteCallback &operator<<(const WriteCallback &cb, const style::PunctuationWrap &str);
const WriteCallback &operator<<(const WriteCallback &cb, const style::VerticalAlign &str);
const WriteCallback &operator<<(const WriteCallback &cb, const style::TableAlign &str);
const WriteCallback &operator<<(const WriteCallback &cb, const style::BorderModel &str);
const WriteCallback &operator<<(const WriteCallback &cb, const style::TextAlignSource &str);
const WriteCallback &operator<<(const WriteCallback &cb, const style::VerticalPos &str);
const WriteCallback &operator<<(const WriteCallback &cb, const style::VerticalRel &str);
const WriteCallback &operator<<(const WriteCallback &cb, const style::HorizontalPos &str);
const WriteCallback &operator<<(const WriteCallback &cb, const style::HorizontalRel &str);
const WriteCallback &operator<<(const WriteCallback &cb, const style::ListLevelPositionMode &str);
const WriteCallback &operator<<(const WriteCallback &cb, const style::LabelFollowedBy &str);

const WriteCallback &operator<<(const WriteCallback &cb, const style::ColorMode &str);
const WriteCallback &operator<<(const WriteCallback &cb, const style::DrawAspect &str);
const WriteCallback &operator<<(const WriteCallback &cb, const style::DrawFill &str);
const WriteCallback &operator<<(const WriteCallback &cb, const style::FillImageRefPoint &str);
const WriteCallback &operator<<(const WriteCallback &cb, const style::DrawStroke &str);
const WriteCallback &operator<<(const WriteCallback &cb, const style::StrokeLinejoin &str);
const WriteCallback &operator<<(const WriteCallback &cb, const style::StrokeLinecap &str);
const WriteCallback &operator<<(const WriteCallback &cb, const style::DrawUnit &str);
const WriteCallback &operator<<(const WriteCallback &cb, const style::OverflowBehavior &str);
const WriteCallback &operator<<(const WriteCallback &cb, const style::Protect &str);
const WriteCallback &operator<<(const WriteCallback &cb, const style::Mirror &str);
const WriteCallback &operator<<(const WriteCallback &cb, const style::Repeat &str);
const WriteCallback &operator<<(const WriteCallback &cb, const style::Wrap &str);
const WriteCallback &operator<<(const WriteCallback &cb, const style::WrapContourMode &str);
const WriteCallback &operator<<(const WriteCallback &cb, const style::FillRule &str);
const WriteCallback &operator<<(const WriteCallback &cb, const style::AnchorType &str);
const WriteCallback &operator<<(const WriteCallback &cb, const style::RunThrough &str);
const WriteCallback &operator<<(const WriteCallback &cb, const style::FootnotesPosition &str);
const WriteCallback &operator<<(const WriteCallback &cb, const style::StartNumberingAt &str);

const WriteCallback &operator<<(const WriteCallback &cb, const style::Style::Family &str);
const WriteCallback &operator<<(const WriteCallback &cb, const float &str);
const WriteCallback &operator<<(const WriteCallback &cb, const uint32_t &str);
const WriteCallback &operator<<(const WriteCallback &cb, const int32_t &str);
const WriteCallback &operator<<(const WriteCallback &cb, const bool &str);
const WriteCallback &operator<<(const WriteCallback &cb, const style::Metric &str);
const WriteCallback &operator<<(const WriteCallback &cb, const style::TextPosition &str);
const WriteCallback &operator<<(const WriteCallback &cb, const style::Border &str);
const WriteCallback &operator<<(const WriteCallback &cb, const char *str);
const WriteCallback &operator<<(const WriteCallback &cb, const StringView &str);
const WriteCallback &operator<<(const WriteCallback &cb, const Escaped &str);
const WriteCallback &operator<<(const WriteCallback &cb, const PercentValue &str);

inline style::Metric operator"" _percents ( long double val ) { return style::Metric(float(val), style::Metric::Percent); }
inline style::Metric operator"" _cm ( long double val ) { return style::Metric(float(val), style::Metric::Cm); }
inline style::Metric operator"" _mm ( long double val ) { return style::Metric(float(val), style::Metric::Mm); }
inline style::Metric operator"" _in ( long double val ) { return style::Metric(float(val), style::Metric::In); }
inline style::Metric operator"" _pt ( long double val ) { return style::Metric(float(val), style::Metric::Pt); }
inline style::Metric operator"" _pc ( long double val ) { return style::Metric(float(val), style::Metric::Pc); }
inline style::Metric operator"" _px ( long double val ) { return style::Metric(float(val), style::Metric::Px); }
inline style::Metric operator"" _em ( long double val ) { return style::Metric(float(val), style::Metric::Em); }

inline style::Metric operator"" _percents ( unsigned long long int val ) { return style::Metric(float(val), style::Metric::Percent); }
inline style::Metric operator"" _cm ( unsigned long long int val ) { return style::Metric(float(val), style::Metric::Cm); }
inline style::Metric operator"" _mm ( unsigned long long int val ) { return style::Metric(float(val), style::Metric::Mm); }
inline style::Metric operator"" _in ( unsigned long long int val ) { return style::Metric(float(val), style::Metric::In); }
inline style::Metric operator"" _pt ( unsigned long long int val ) { return style::Metric(float(val), style::Metric::Pt); }
inline style::Metric operator"" _pc ( unsigned long long int val ) { return style::Metric(float(val), style::Metric::Pc); }
inline style::Metric operator"" _px ( unsigned long long int val ) { return style::Metric(float(val), style::Metric::Px); }
inline style::Metric operator"" _em ( unsigned long long int val ) { return style::Metric(float(val), style::Metric::Em); }

}

#endif /* UTILS_SRC_ODSTYLE_H_ */
