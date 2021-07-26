
#include "ODStyle.h"
#include "ODContent.h"

namespace opendocument::style {

Value::Value() {
	memset(this, 0, sizeof(Value));
}

bool Margin::isSingle() const {
	return top == bottom && left == right && left == top && top.isValid();
}


#define LIST_SETTER(N, T, field) template<> Style::ListLevel & Style::ListLevel::set<Name::N, T>(const T &v) { field.emplace_back(Name::N).set(v); return *this; }
#define LIST_SETTER_STRING(N, field) template<> Style::ListLevel & Style::ListLevel::set<Name::N, StringView>(const StringView &v) \
		{ field.emplace_back(Name::N).value.stringId.value = stappler::hash::hash32(v.data(), v.size()); if (emplaceString) { (*emplaceString)(v); } return *this; }

LIST_SETTER(		TextBackgroundColor, Color3B, text)
LIST_SETTER(		TextColor, Color3B, text)
LIST_SETTER_STRING (TextFontFamily, text)
LIST_SETTER(		TextFontSize, Metric, text)
LIST_SETTER(		TextFontStyle, FontStyle, text)
LIST_SETTER(		TextFontVariant, FontVariant, text)
LIST_SETTER(		TextFontWeight, FontWeight, text)
LIST_SETTER(		TextHyphenate, bool, text)
LIST_SETTER(		TextHyphenationPushCharCount, uint32_t, text)
LIST_SETTER(		TextHyphenationRemainCharCount, uint32_t, text)
LIST_SETTER(		TextLetterSpacing, Metric, text)
LIST_SETTER(		TextTransform, TextTransform, text)
LIST_SETTER(		TextFontFamilyGeneric, FontFamilyGeneric, text)
LIST_SETTER_STRING (TextFontName, text)
LIST_SETTER(		TextFontPitch, FontPitch, text)
LIST_SETTER_STRING (TextFontStyleName, text)
LIST_SETTER(		TextPosition, TextPosition, text)
LIST_SETTER(		TextUnderlineColor, Color3B, text)
LIST_SETTER(		TextUnderlineMode, LineMode, text)
LIST_SETTER(		TextUnderlineStyle, LineStyle, text)
LIST_SETTER(		TextUnderlineType, LineType, text)
LIST_SETTER(		TextUnderlineWidth, Metric, text)
LIST_SETTER(		TextScale, float, text)
LIST_SETTER_STRING (TextLanguage, text)
LIST_SETTER_STRING (TextCountry, text)

LIST_SETTER_STRING (ListStyleNumPrefix, listStyle)
LIST_SETTER_STRING (ListStyleNumSuffix, listStyle)
LIST_SETTER_STRING (ListStyleNumFormat, listStyle)
LIST_SETTER(		ListStyleNumLetterSync, bool, listStyle)
LIST_SETTER_STRING (ListStyleBulletChar, listStyle)
LIST_SETTER(		ListStyleBulletRelativeSize, float, listStyle)
LIST_SETTER(		ListStyleDisplayLevels, uint32_t, listStyle)
LIST_SETTER(		ListStyleStartValue, uint32_t, listStyle)

LIST_SETTER(		ListLevelHeight, Metric, listLevel)
LIST_SETTER(		ListLevelTextAlign, TextAlign, listLevel)
LIST_SETTER(		ListLevelWidth, Metric, listLevel)
LIST_SETTER_STRING (ListLevelFontName, listLevel)
LIST_SETTER(		ListLevelVerticalPos, VerticalPos, listLevel)
LIST_SETTER(		ListLevelVerticalRel, VerticalRel, listLevel)
LIST_SETTER(		ListLevelY, Metric, listLevel)
LIST_SETTER(		ListLevelPositionMode, ListLevelPositionMode, listLevel)
LIST_SETTER(		ListLevelMinLabelDistance, Metric, listLevel)
LIST_SETTER(		ListLevelMinLabelWidth, Metric, listLevel)
LIST_SETTER(		ListLevelSpaceBefore, Metric, listLevel)

LIST_SETTER(		ListLevelLabelMarginLeft, Metric, listLevelLabel)
LIST_SETTER(		ListLevelLabelTextIndent, Metric, listLevelLabel)
LIST_SETTER(		ListLevelLabelFollowedBy, LabelFollowedBy, listLevelLabel)
LIST_SETTER(		ListLevelLabelTabStopPosition, Metric, listLevelLabel)


#define STYLE_SETTER(N, T, field) template<> Style & Style::set<Name::N, T>(const T &v) { field.emplace_back(Name::N).set(v); return *this; }
#define STYLE_SETTER_STRING(N, field) template<> Style & Style::set<Name::N, StringView>(const StringView &v) \
		{ field.emplace_back(Name::N).value.stringId.value = stappler::hash::hash32(v.data(), v.size()); if (emplaceString) { emplaceString(v); } return *this; }

STYLE_SETTER(		TextBackgroundColor, Color3B, text)
STYLE_SETTER(		TextColor, Color3B, text)
STYLE_SETTER_STRING(TextFontFamily, text)
STYLE_SETTER(		TextFontSize, Metric, text)
STYLE_SETTER(		TextFontStyle, FontStyle, text)
STYLE_SETTER(		TextFontVariant, FontVariant, text)
STYLE_SETTER(		TextFontWeight, FontWeight, text)
STYLE_SETTER(		TextHyphenate, bool, text)
STYLE_SETTER(		TextHyphenationPushCharCount, uint32_t, text)
STYLE_SETTER(		TextHyphenationRemainCharCount, uint32_t, text)
STYLE_SETTER(		TextLetterSpacing, Metric, text)
STYLE_SETTER(		TextTransform, TextTransform, text)
STYLE_SETTER(		TextFontFamilyGeneric, FontFamilyGeneric, text)
STYLE_SETTER_STRING(TextFontName, text)
STYLE_SETTER(		TextFontPitch, FontPitch, text)
STYLE_SETTER_STRING(TextFontStyleName, text)
STYLE_SETTER(		TextPosition, TextPosition, text)
STYLE_SETTER(		TextUnderlineColor, Color3B, text)
STYLE_SETTER(		TextUnderlineMode, LineMode, text)
STYLE_SETTER(		TextUnderlineStyle, LineStyle, text)
STYLE_SETTER(		TextUnderlineType, LineType, text)
STYLE_SETTER(		TextUnderlineWidth, Metric, text)
STYLE_SETTER(		TextScale, float, text)
STYLE_SETTER_STRING(TextLanguage, text)
STYLE_SETTER_STRING(TextCountry, text)

STYLE_SETTER(		ParagraphBackgroundColor, Color3B, paragraph)
STYLE_SETTER(		ParagraphBorderTopStyle, LineStyle, paragraph)
STYLE_SETTER(		ParagraphBorderTopWidth, Metric, paragraph)
STYLE_SETTER(		ParagraphBorderTopColor, Color3B, paragraph)
STYLE_SETTER(		ParagraphBorderRightStyle, LineStyle, paragraph)
STYLE_SETTER(		ParagraphBorderRightWidth, Metric, paragraph)
STYLE_SETTER(		ParagraphBorderRightColor, Color3B, paragraph)
STYLE_SETTER(		ParagraphBorderBottomStyle, LineStyle, paragraph)
STYLE_SETTER(		ParagraphBorderBottomWidth, Metric, paragraph)
STYLE_SETTER(		ParagraphBorderBottomColor, Color3B, paragraph)
STYLE_SETTER(		ParagraphBorderLeftStyle, LineStyle, paragraph)
STYLE_SETTER(		ParagraphBorderLeftWidth, Metric, paragraph)
STYLE_SETTER(		ParagraphBorderLeftColor, Color3B, paragraph)
STYLE_SETTER(		ParagraphBreakAfter, BreakMode, paragraph)
STYLE_SETTER(		ParagraphBreakBefore, BreakMode, paragraph)
STYLE_SETTER(		ParagraphKeepTogether, KeepMode, paragraph)
STYLE_SETTER(		ParagraphKeepWithNext, KeepMode, paragraph)
STYLE_SETTER(		ParagraphLineHeight, Metric, paragraph)
STYLE_SETTER(		ParagraphMarginTop, Metric, paragraph)
STYLE_SETTER(		ParagraphMarginRight, Metric, paragraph)
STYLE_SETTER(		ParagraphMarginBottom, Metric, paragraph)
STYLE_SETTER(		ParagraphMarginLeft, Metric, paragraph)
STYLE_SETTER(		ParagraphPaddingTop, Metric, paragraph)
STYLE_SETTER(		ParagraphPaddingRight, Metric, paragraph)
STYLE_SETTER(		ParagraphPaddingBottom, Metric, paragraph)
STYLE_SETTER(		ParagraphPaddingLeft, Metric, paragraph)
STYLE_SETTER(		ParagraphTextAlign, TextAlign, paragraph)
STYLE_SETTER(		ParagraphTextAlignLast, TextAlignLast, paragraph)
STYLE_SETTER(		ParagraphTextIndent, Metric, paragraph)
STYLE_SETTER(		ParagraphBackgroundTransparency, float, paragraph)
STYLE_SETTER(		ParagraphAutoTextIndent, bool, paragraph)
STYLE_SETTER(		ParagraphFontIndependentLineSpacing, bool, paragraph)
STYLE_SETTER(		ParagraphJoinBorder, bool, paragraph)
STYLE_SETTER(		ParagraphJustifySingleWord, bool, paragraph)
STYLE_SETTER(		ParagraphLineBreak, LineBreak, paragraph)
STYLE_SETTER(		ParagraphLineHeightAtLeast, Metric, paragraph)
STYLE_SETTER(		ParagraphLineSpacing, uint32_t, paragraph)
STYLE_SETTER(		ParagraphPageNumber, uint32_t, paragraph)
STYLE_SETTER(		ParagraphPunctuationWrap, PunctuationWrap, paragraph)
STYLE_SETTER(		ParagraphRegisterTrue, bool, paragraph)
STYLE_SETTER(		ParagraphTabStopDistance, uint32_t, paragraph)
STYLE_SETTER(		ParagraphVerticalAlign, VerticalAlign, paragraph)
STYLE_SETTER(		ParagraphLineNumber, uint32_t, paragraph)
STYLE_SETTER(		ParagraphNumberLines, bool, paragraph)
STYLE_SETTER(		ParagraphHyphenationLadderCount, uint32_t, paragraph)
STYLE_SETTER(		ParagraphWidows, uint32_t, paragraph)
STYLE_SETTER(		ParagraphOrphans, uint32_t, paragraph)

STYLE_SETTER(		PageBackgroundColor, Color3B, pageLayout)
STYLE_SETTER(		PageBorderTopStyle, LineStyle, pageLayout)
STYLE_SETTER(		PageBorderTopWidth, Metric, pageLayout)
STYLE_SETTER(		PageBorderTopColor, Color3B, pageLayout)
STYLE_SETTER(		PageBorderRightStyle, LineStyle, pageLayout)
STYLE_SETTER(		PageBorderRightWidth, Metric, pageLayout)
STYLE_SETTER(		PageBorderRightColor, Color3B, pageLayout)
STYLE_SETTER(		PageBorderBottomStyle, LineStyle, pageLayout)
STYLE_SETTER(		PageBorderBottomWidth, Metric, pageLayout)
STYLE_SETTER(		PageBorderBottomColor, Color3B, pageLayout)
STYLE_SETTER(		PageBorderLeftStyle, LineStyle, pageLayout)
STYLE_SETTER(		PageBorderLeftWidth, Metric, pageLayout)
STYLE_SETTER(		PageBorderLeftColor, Color3B, pageLayout)
STYLE_SETTER(		PageMarginTop, Metric, pageLayout)
STYLE_SETTER(		PageMarginRight, Metric, pageLayout)
STYLE_SETTER(		PageMarginBottom, Metric, pageLayout)
STYLE_SETTER(		PageMarginLeft, Metric, pageLayout)
STYLE_SETTER(		PagePaddingTop, Metric, pageLayout)
STYLE_SETTER(		PagePaddingRight, Metric, pageLayout)
STYLE_SETTER(		PagePaddingBottom, Metric, pageLayout)
STYLE_SETTER(		PagePaddingLeft, Metric, pageLayout)
STYLE_SETTER(		PageHeight, Metric, pageLayout)
STYLE_SETTER(		PageWidth, Metric, pageLayout)
STYLE_SETTER(		PageFirstPageNumber, uint32_t, pageLayout)
STYLE_SETTER(		PageFootnoteMaxHeight, Metric, pageLayout)

STYLE_SETTER(		PageHeaderMinHeight, Metric, pageLayout)
STYLE_SETTER(		PageHeaderHeight, Metric, pageLayout)
STYLE_SETTER(		PageHeaderMarginTop, Metric, pageLayout)
STYLE_SETTER(		PageHeaderMarginRight, Metric, pageLayout)
STYLE_SETTER(		PageHeaderMarginBottom, Metric, pageLayout)
STYLE_SETTER(		PageHeaderMarginLeft, Metric, pageLayout)
STYLE_SETTER(		PageHeaderPaddingTop, Metric, pageLayout)
STYLE_SETTER(		PageHeaderPaddingRight, Metric, pageLayout)
STYLE_SETTER(		PageHeaderPaddingBottom, Metric, pageLayout)
STYLE_SETTER(		PageHeaderPaddingLeft, Metric, pageLayout)

STYLE_SETTER(		PageFooterMinHeight, Metric, pageLayout)
STYLE_SETTER(		PageFooterHeight, Metric, pageLayout)
STYLE_SETTER(		PageFooterMarginTop, Metric, pageLayout)
STYLE_SETTER(		PageFooterMarginRight, Metric, pageLayout)
STYLE_SETTER(		PageFooterMarginBottom, Metric, pageLayout)
STYLE_SETTER(		PageFooterMarginLeft, Metric, pageLayout)
STYLE_SETTER(		PageFooterPaddingTop, Metric, pageLayout)
STYLE_SETTER(		PageFooterPaddingRight, Metric, pageLayout)
STYLE_SETTER(		PageFooterPaddingBottom, Metric, pageLayout)
STYLE_SETTER(		PageFooterPaddingLeft, Metric, pageLayout)

STYLE_SETTER(		TableBackgroundColor, Color3B, table)
STYLE_SETTER(		TableBreakAfter, BreakMode, table)
STYLE_SETTER(		TableBreakBefore, BreakMode, table)
STYLE_SETTER(		TableKeepWithNext, KeepMode, table)
STYLE_SETTER(		TableMarginTop, Metric, table)
STYLE_SETTER(		TableMarginRight, Metric, table)
STYLE_SETTER(		TableMarginBottom, Metric, table)
STYLE_SETTER(		TableMarginLeft, Metric, table)
STYLE_SETTER(		TablePageNumber, uint32_t, table)
STYLE_SETTER(		TableMayBreakBetweenRows, bool, table)
STYLE_SETTER(		TableRelWidth, Metric, table)
STYLE_SETTER(		TableWidth, Metric, table)
STYLE_SETTER(		TableAlign, TableAlign, table)
STYLE_SETTER(		TableBorderModel, BorderModel, table)

STYLE_SETTER(		TableColumnBreakAfter, BreakMode, tableColumn)
STYLE_SETTER(		TableColumnBreakBefore, BreakMode, tableColumn)
STYLE_SETTER(		TableColumnRelWidth, uint32_t, tableColumn)
STYLE_SETTER(		TableColumnWidth, Metric, tableColumn)
STYLE_SETTER(		TableColumnUseOptimalWidth, bool, tableColumn)

STYLE_SETTER(		TableRowBackgroundColor, Color3B, tableRow)
STYLE_SETTER(		TableRowBreakAfter, BreakMode, tableRow)
STYLE_SETTER(		TableRowBreakBefore, BreakMode, tableRow)
STYLE_SETTER(		TableRowKeepTogether, KeepMode, tableRow)
STYLE_SETTER(		TableRowMinHeight, Metric, tableRow)
STYLE_SETTER(		TableRowHeight, Metric, tableRow)
STYLE_SETTER(		TableRowUseOptimalHeight, bool, tableRow)

STYLE_SETTER(		TableCellBackgroundColor, Color3B, tableCell)
STYLE_SETTER(		TableCellBorderTopStyle, LineStyle, tableCell)
STYLE_SETTER(		TableCellBorderTopWidth, Metric, tableCell)
STYLE_SETTER(		TableCellBorderTopColor, Color3B, tableCell)
STYLE_SETTER(		TableCellBorderRightStyle, LineStyle, tableCell)
STYLE_SETTER(		TableCellBorderRightWidth, Metric, tableCell)
STYLE_SETTER(		TableCellBorderRightColor, Color3B, tableCell)
STYLE_SETTER(		TableCellBorderBottomStyle, LineStyle, tableCell)
STYLE_SETTER(		TableCellBorderBottomWidth, Metric, tableCell)
STYLE_SETTER(		TableCellBorderBottomColor, Color3B, tableCell)
STYLE_SETTER(		TableCellBorderLeftStyle, LineStyle, tableCell)
STYLE_SETTER(		TableCellBorderLeftWidth, Metric, tableCell)
STYLE_SETTER(		TableCellBorderLeftColor, Color3B, tableCell)
STYLE_SETTER(		TableCellPaddingTop, Metric, tableCell)
STYLE_SETTER(		TableCellPaddingRight, Metric, tableCell)
STYLE_SETTER(		TableCellPaddingBottom, Metric, tableCell)
STYLE_SETTER(		TableCellPaddingLeft, Metric, tableCell)
STYLE_SETTER(		TableCellDecimalPlaces, uint32_t, tableCell)
STYLE_SETTER(		TableCellPrintContent, bool, tableCell)
STYLE_SETTER(		TableCellRepeatContent, bool, tableCell)
STYLE_SETTER(		TableCellShrinkToFit, bool, tableCell)
STYLE_SETTER(		TableCellTextAlignSource, TextAlignSource, tableCell)
STYLE_SETTER(		TableCellVerticalAlign, VerticalAlign, tableCell)

STYLE_SETTER(		DrawAutoGrowHeight, bool, graphic)
STYLE_SETTER(		DrawAutoGrowWidth, bool, graphic)
STYLE_SETTER(		DrawRed, float, graphic)
STYLE_SETTER(		DrawGreen, float, graphic)
STYLE_SETTER(		DrawBlue, float, graphic)
STYLE_SETTER(		DrawColorInversion, bool, graphic)
STYLE_SETTER(		DrawColorMode, ColorMode, graphic)
STYLE_SETTER(		DrawContrast, float, graphic)
STYLE_SETTER(		DrawDecimalPlaces, uint32_t, graphic)
STYLE_SETTER(		DrawAspect, DrawAspect, graphic)
STYLE_SETTER(		DrawFill, DrawFill, graphic)
STYLE_SETTER(		DrawFillColor, Color3B, graphic)
STYLE_SETTER_STRING(DrawFillGradientName, graphic)
STYLE_SETTER_STRING(DrawFillHatchName, graphic)
STYLE_SETTER(		DrawFillHatchSolid, bool, graphic)
STYLE_SETTER(		DrawFillImageHeight, float, graphic)
STYLE_SETTER(		DrawFillImageWidth, float, graphic)
STYLE_SETTER_STRING(DrawFillImageName, graphic)
STYLE_SETTER(		DrawFillImageRefPoint, FillImageRefPoint, graphic)
STYLE_SETTER(		DrawFillImageRefPointX, float, graphic)
STYLE_SETTER(		DrawFillImageRefPointY, float, graphic)
STYLE_SETTER(		DrawFitToContour, bool, graphic)
STYLE_SETTER(		DrawFitToSize, bool, graphic)
STYLE_SETTER(		DrawFrameDisplayBorder, bool, graphic)
STYLE_SETTER(		DrawFrameDisplayScrollbar, bool, graphic)
STYLE_SETTER(		DrawFrameMarginHorizontal, uint32_t, graphic)
STYLE_SETTER(		DrawFrameMarginVertical, uint32_t, graphic)
STYLE_SETTER(		DrawGamma, float, graphic)
STYLE_SETTER(		DrawGradientStepCount, uint32_t, graphic)
STYLE_SETTER(		DrawImageOpacity, float, graphic)
STYLE_SETTER(		DrawLuminance, float, graphic)
STYLE_SETTER_STRING(DrawMarkerEnd, graphic)
STYLE_SETTER(		DrawMarkerEndCenter, bool, graphic)
STYLE_SETTER(		DrawMarkerEndWidth, Metric, graphic)
STYLE_SETTER_STRING(DrawMarkerStart, graphic)
STYLE_SETTER(		DrawMarkerStartCenter, bool, graphic)
STYLE_SETTER(		DrawMarkerStartWidth, Metric, graphic)
STYLE_SETTER(		DrawOpacity, float, graphic)
STYLE_SETTER_STRING(DrawOpacityName, graphic)
STYLE_SETTER(		DrawParallel, bool, graphic)
STYLE_SETTER(		DrawSecondaryFillColor, Color3B, graphic)
STYLE_SETTER(		DrawStroke, DrawStroke, graphic)
STYLE_SETTER_STRING(DrawStrokeDash, graphic)
STYLE_SETTER_STRING(DrawStrokeDashNames, graphic)
STYLE_SETTER(		DrawStrokeLinejoin, StrokeLinejoin, graphic)
STYLE_SETTER(		DrawStrokeLinecap, StrokeLinecap, graphic)
STYLE_SETTER(		DrawSymbolColor, Color3B, graphic)
STYLE_SETTER(		DrawVisibleAreaHeight, Metric, graphic)
STYLE_SETTER(		DrawVisibleAreaTop, Metric, graphic)
STYLE_SETTER(		DrawVisibleAreaLeft, Metric, graphic)
STYLE_SETTER(		DrawVisibleAreaWidth, Metric, graphic)
STYLE_SETTER(		DrawUnit, DrawUnit, graphic)
STYLE_SETTER(		DrawBackgroundColor, Color3B, graphic)
STYLE_SETTER(		DrawBorderTopStyle, LineStyle, graphic)
STYLE_SETTER(		DrawBorderTopWidth, Metric, graphic)
STYLE_SETTER(		DrawBorderTopColor, Color3B, graphic)
STYLE_SETTER(		DrawBorderRightStyle, LineStyle, graphic)
STYLE_SETTER(		DrawBorderRightWidth, Metric, graphic)
STYLE_SETTER(		DrawBorderRightColor, Color3B, graphic)
STYLE_SETTER(		DrawBorderBottomStyle, LineStyle, graphic)
STYLE_SETTER(		DrawBorderBottomWidth, Metric, graphic)
STYLE_SETTER(		DrawBorderBottomColor, Color3B, graphic)
STYLE_SETTER(		DrawBorderLeftStyle, LineStyle, graphic)
STYLE_SETTER(		DrawBorderLeftWidth, Metric, graphic)
STYLE_SETTER(		DrawBorderLeftColor, Color3B, graphic)
STYLE_SETTER(		DrawMarginTop, Metric, graphic)
STYLE_SETTER(		DrawMarginRight, Metric, graphic)
STYLE_SETTER(		DrawMarginBottom, Metric, graphic)
STYLE_SETTER(		DrawMarginLeft, Metric, graphic)
STYLE_SETTER(		DrawPaddingTop, Metric, graphic)
STYLE_SETTER(		DrawPaddingRight, Metric, graphic)
STYLE_SETTER(		DrawPaddingBottom, Metric, graphic)
STYLE_SETTER(		DrawPaddingLeft, Metric, graphic)
STYLE_SETTER(		DrawMinHeight, Metric, graphic)
STYLE_SETTER(		DrawMaxHeight, Metric, graphic)
STYLE_SETTER(		DrawMinWidth, Metric, graphic)
STYLE_SETTER(		DrawMaxWidth, Metric, graphic)
STYLE_SETTER(		DrawBackgroundTransparency, float, graphic)
STYLE_SETTER(		DrawEditable, bool, graphic)
STYLE_SETTER(		DrawFlowWithText, bool, graphic)
STYLE_SETTER(		DrawHorizontalPos, HorizontalPos, graphic)
STYLE_SETTER(		DrawHorizontalRel, HorizontalRel, graphic)
STYLE_SETTER(		DrawMirror, Mirror, graphic)
STYLE_SETTER(		DrawNumberWrappedParagraphs, uint32_t, graphic)
STYLE_SETTER(		DrawPrintContent, bool, graphic)
STYLE_SETTER(		DrawOverflowBehavior, OverflowBehavior, graphic)
STYLE_SETTER(		DrawProtect, Protect, graphic)
STYLE_SETTER(		DrawRelHeight, Metric, graphic)
STYLE_SETTER(		DrawRelWidth, Metric, graphic)
STYLE_SETTER(		DrawRepeat, Repeat, graphic)
STYLE_SETTER(		DrawRunThrough, RunThrough, graphic)
STYLE_SETTER(		DrawShrinkToFit, bool, graphic)
STYLE_SETTER(		DrawVerticalPos, VerticalPos, graphic)
STYLE_SETTER(		DrawVerticalRel, VerticalRel, graphic)
STYLE_SETTER(		DrawWrap, Wrap, graphic)
STYLE_SETTER(		DrawWrapContour, bool, graphic)
STYLE_SETTER(		DrawWrapContourMode, WrapContourMode, graphic)
STYLE_SETTER(		DrawWrapDynamicThreshold, Metric, graphic)
STYLE_SETTER(		DrawWidth, Metric, graphic)
STYLE_SETTER(		DrawHeight, Metric, graphic)
STYLE_SETTER(		DrawX, Metric, graphic)
STYLE_SETTER(		DrawY, Metric, graphic)
STYLE_SETTER(		DrawStrokeWidth, Metric, graphic)
STYLE_SETTER(		DrawStrokeOpacity, float, graphic)
STYLE_SETTER(		DrawStrokeColor, Color3B, graphic)
STYLE_SETTER(		DrawFillRule, FillRule, graphic)
STYLE_SETTER(		DrawAnchorPageNumber, uint32_t, graphic)
STYLE_SETTER(		DrawAnchorType, AnchorType, graphic)
STYLE_SETTER_STRING(DrawClip, graphic)

STYLE_SETTER_STRING(NoteNumFormat, note)
STYLE_SETTER(		NoteNumLetterSync, bool, note)
STYLE_SETTER_STRING(NoteNumPrefix, note)
STYLE_SETTER_STRING(NoteNumSuffix, note)
STYLE_SETTER(		NoteStartValue, uint32_t, note)
STYLE_SETTER(		NoteFootnotesPosition, FootnotesPosition, note)
STYLE_SETTER(		NoteStartNumberingAt, StartNumberingAt, note)

template<> Style & Style::set<Name::ListConsecutiveNumbering, bool>(const bool &v) {
	consecutiveNumbering = v;
	return *this;
}

template<> Style & Style::set<Name::ParagraphMargin, Metric>(const Metric &v) {
	paragraph.emplace_back(Name::ParagraphMarginTop).value.metric = v;
	paragraph.emplace_back(Name::ParagraphMarginRight).value.metric = v;
	paragraph.emplace_back(Name::ParagraphMarginBottom).value.metric = v;
	paragraph.emplace_back(Name::ParagraphMarginLeft).value.metric = v;
	return *this;
}

template<> Style & Style::set<Name::ParagraphPadding, Metric>(const Metric &v) {
	paragraph.emplace_back(Name::ParagraphPaddingTop).value.metric = v;
	paragraph.emplace_back(Name::ParagraphPaddingRight).value.metric = v;
	paragraph.emplace_back(Name::ParagraphPaddingBottom).value.metric = v;
	paragraph.emplace_back(Name::ParagraphPaddingLeft).value.metric = v;
	return *this;
}

template<> Style & Style::set<Name::ParagraphBorderBottom, Border>(const Border &v) {
	paragraph.emplace_back(Name::ParagraphBorderBottomWidth).value.metric = v.width;
	paragraph.emplace_back(Name::ParagraphBorderBottomColor).value.color = v.color;
	paragraph.emplace_back(Name::ParagraphBorderBottomStyle).value.lineStyle = v.style;
	return *this;
}

template<> Style & Style::set<Name::ParagraphBorderTop, Border>(const Border &v) {
	paragraph.emplace_back(Name::ParagraphBorderTopWidth).value.metric = v.width;
	paragraph.emplace_back(Name::ParagraphBorderTopColor).value.color = v.color;
	paragraph.emplace_back(Name::ParagraphBorderTopStyle).value.lineStyle = v.style;
	return *this;
}

template<> Style & Style::set<Name::ParagraphBorderLeft, Border>(const Border &v) {
	paragraph.emplace_back(Name::ParagraphBorderLeftWidth).value.metric = v.width;
	paragraph.emplace_back(Name::ParagraphBorderLeftColor).value.color = v.color;
	paragraph.emplace_back(Name::ParagraphBorderLeftStyle).value.lineStyle = v.style;
	return *this;
}

template<> Style & Style::set<Name::ParagraphBorderRight, Border>(const Border &v) {
	paragraph.emplace_back(Name::ParagraphBorderRightWidth).value.metric = v.width;
	paragraph.emplace_back(Name::ParagraphBorderRightColor).value.color = v.color;
	paragraph.emplace_back(Name::ParagraphBorderRightStyle).value.lineStyle = v.style;
	return *this;
}

template<> Style & Style::set<Name::ParagraphBorder, Border>(const Border &v) {
	set<Name::ParagraphBorderBottom>(v);
	set<Name::ParagraphBorderTop>(v);
	set<Name::ParagraphBorderRight>(v);
	set<Name::ParagraphBorderLeft>(v);
	return *this;
}

template<> Style & Style::set<Name::PageMargin, Metric>(const Metric &v) {
	pageLayout.emplace_back(Name::PageMarginTop).value.metric = v;
	pageLayout.emplace_back(Name::PageMarginRight).value.metric = v;
	pageLayout.emplace_back(Name::PageMarginBottom).value.metric = v;
	pageLayout.emplace_back(Name::PageMarginLeft).value.metric = v;
	return *this;
}

template<> Style & Style::set<Name::PagePadding, Metric>(const Metric &v) {
	pageLayout.emplace_back(Name::PagePaddingTop).value.metric = v;
	pageLayout.emplace_back(Name::PagePaddingRight).value.metric = v;
	pageLayout.emplace_back(Name::PagePaddingBottom).value.metric = v;
	pageLayout.emplace_back(Name::PagePaddingLeft).value.metric = v;
	return *this;
}

template<> Style & Style::set<Name::PageHeaderMargin, Metric>(const Metric &v) {
	pageLayout.emplace_back(Name::PageHeaderMarginTop).value.metric = v;
	pageLayout.emplace_back(Name::PageHeaderMarginRight).value.metric = v;
	pageLayout.emplace_back(Name::PageHeaderMarginBottom).value.metric = v;
	pageLayout.emplace_back(Name::PageHeaderMarginLeft).value.metric = v;
	return *this;
}

template<> Style & Style::set<Name::PageHeaderPadding, Metric>(const Metric &v) {
	pageLayout.emplace_back(Name::PageHeaderPaddingTop).value.metric = v;
	pageLayout.emplace_back(Name::PageHeaderPaddingRight).value.metric = v;
	pageLayout.emplace_back(Name::PageHeaderPaddingBottom).value.metric = v;
	pageLayout.emplace_back(Name::PageHeaderPaddingLeft).value.metric = v;
	return *this;
}

template<> Style & Style::set<Name::PageFooterMargin, Metric>(const Metric &v) {
	pageLayout.emplace_back(Name::PageFooterMarginTop).value.metric = v;
	pageLayout.emplace_back(Name::PageFooterMarginRight).value.metric = v;
	pageLayout.emplace_back(Name::PageFooterMarginBottom).value.metric = v;
	pageLayout.emplace_back(Name::PageFooterMarginLeft).value.metric = v;
	return *this;
}

template<> Style & Style::set<Name::PageFooterPadding, Metric>(const Metric &v) {
	pageLayout.emplace_back(Name::PageFooterPaddingTop).value.metric = v;
	pageLayout.emplace_back(Name::PageFooterPaddingRight).value.metric = v;
	pageLayout.emplace_back(Name::PageFooterPaddingBottom).value.metric = v;
	pageLayout.emplace_back(Name::PageFooterPaddingLeft).value.metric = v;
	return *this;
}

template<> Style & Style::set<Name::TableMargin, Metric>(const Metric &v) {
	table.emplace_back(Name::TableMarginTop).value.metric = v;
	table.emplace_back(Name::TableMarginRight).value.metric = v;
	table.emplace_back(Name::TableMarginBottom).value.metric = v;
	table.emplace_back(Name::TableMarginLeft).value.metric = v;
	return *this;
}

template<> Style & Style::set<Name::TableCellBorderBottom, Border>(const Border &v) {
	tableCell.emplace_back(Name::TableCellBorderBottomWidth).value.metric = v.width;
	tableCell.emplace_back(Name::TableCellBorderBottomColor).value.color = v.color;
	tableCell.emplace_back(Name::TableCellBorderBottomStyle).value.lineStyle = v.style;
	return *this;
}

template<> Style & Style::set<Name::TableCellBorderTop, Border>(const Border &v) {
	tableCell.emplace_back(Name::TableCellBorderTopWidth).value.metric = v.width;
	tableCell.emplace_back(Name::TableCellBorderTopColor).value.color = v.color;
	tableCell.emplace_back(Name::TableCellBorderTopStyle).value.lineStyle = v.style;
	return *this;
}

template<> Style & Style::set<Name::TableCellBorderLeft, Border>(const Border &v) {
	tableCell.emplace_back(Name::TableCellBorderLeftWidth).value.metric = v.width;
	tableCell.emplace_back(Name::TableCellBorderLeftColor).value.color = v.color;
	tableCell.emplace_back(Name::TableCellBorderLeftStyle).value.lineStyle = v.style;
	return *this;
}

template<> Style & Style::set<Name::TableCellBorderRight, Border>(const Border &v) {
	tableCell.emplace_back(Name::TableCellBorderRightWidth).value.metric = v.width;
	tableCell.emplace_back(Name::TableCellBorderRightColor).value.color = v.color;
	tableCell.emplace_back(Name::TableCellBorderRightStyle).value.lineStyle = v.style;
	return *this;
}

template<> Style & Style::set<Name::TableCellBorder, Border>(const Border &v) {
	set<Name::TableCellBorderBottom>(v);
	set<Name::TableCellBorderTop>(v);
	set<Name::TableCellBorderRight>(v);
	set<Name::TableCellBorderLeft>(v);
	return *this;
}

template<> Style & Style::set<Name::TableCellPadding, Metric>(const Metric &v) {
	tableCell.emplace_back(Name::TableCellPaddingTop).value.metric = v;
	tableCell.emplace_back(Name::TableCellPaddingRight).value.metric = v;
	tableCell.emplace_back(Name::TableCellPaddingBottom).value.metric = v;
	tableCell.emplace_back(Name::TableCellPaddingLeft).value.metric = v;
	return *this;
}


template<> Style & Style::set<Name::DrawBorderBottom, Border>(const Border &v) {
	graphic.emplace_back(Name::DrawBorderBottomWidth).value.metric = v.width;
	graphic.emplace_back(Name::DrawBorderBottomColor).value.color = v.color;
	graphic.emplace_back(Name::DrawBorderBottomStyle).value.lineStyle = v.style;
	return *this;
}

template<> Style & Style::set<Name::DrawBorderTop, Border>(const Border &v) {
	graphic.emplace_back(Name::DrawBorderTopWidth).value.metric = v.width;
	graphic.emplace_back(Name::DrawBorderTopColor).value.color = v.color;
	graphic.emplace_back(Name::DrawBorderTopStyle).value.lineStyle = v.style;
	return *this;
}

template<> Style & Style::set<Name::DrawBorderLeft, Border>(const Border &v) {
	graphic.emplace_back(Name::DrawBorderLeftWidth).value.metric = v.width;
	graphic.emplace_back(Name::DrawBorderLeftColor).value.color = v.color;
	graphic.emplace_back(Name::DrawBorderLeftStyle).value.lineStyle = v.style;
	return *this;
}

template<> Style & Style::set<Name::DrawBorderRight, Border>(const Border &v) {
	graphic.emplace_back(Name::DrawBorderRightWidth).value.metric = v.width;
	graphic.emplace_back(Name::DrawBorderRightColor).value.color = v.color;
	graphic.emplace_back(Name::DrawBorderRightStyle).value.lineStyle = v.style;
	return *this;
}

template<> Style & Style::set<Name::DrawBorder, Border>(const Border &v) {
	set<Name::DrawBorderBottom>(v);
	set<Name::DrawBorderTop>(v);
	set<Name::DrawBorderRight>(v);
	set<Name::DrawBorderLeft>(v);
	return *this;
}

template<> Style & Style::set<Name::DrawMargin, Metric>(const Metric &v) {
	graphic.emplace_back(Name::DrawMarginTop).value.metric = v;
	graphic.emplace_back(Name::DrawMarginRight).value.metric = v;
	graphic.emplace_back(Name::DrawMarginBottom).value.metric = v;
	graphic.emplace_back(Name::DrawMarginLeft).value.metric = v;
	return *this;
}

template<> Style & Style::set<Name::DrawPadding, Metric>(const Metric &v) {
	graphic.emplace_back(Name::DrawPaddingTop).value.metric = v;
	graphic.emplace_back(Name::DrawPaddingRight).value.metric = v;
	graphic.emplace_back(Name::DrawPaddingBottom).value.metric = v;
	graphic.emplace_back(Name::DrawPaddingLeft).value.metric = v;
	return *this;
}


Style::ListLevel::ListLevel(Type t, Style *s, uint32_t l, Function<void(const StringView &)> *cb)
: type(t), textStyle(s), level(l), emplaceString(cb) { }

Style::Style(Type t, Family f) : type(t), family(f), parent(nullptr) { }

Style::Style(Type t, Family f, const StringView &name, const Style *p, Function<void(const StringView &)> &&empl, const StringView &cl)
: type(t), family(f), name(name.str<Interface>()), parent(p), styleClass(cl.str<Interface>()), emplaceString(std::move(empl)) { }

Style::ListLevel &Style::addListLevel(ListLevel::Type t, Style *s) {
	pool::push(list.get_allocator());
	auto l = list.emplace_back(new (list.get_allocator()) ListLevel(t, s, list.size() + 1, &emplaceString));
	pool::pop();
	return *l;
}

Style::Parameter::Parameter(Name name) : name(name) { }

void Style::write(const WriteCallback &cb, const Callback<StringView(StringId)> &stringIdCb, bool pretty) const {
	if (type != Default) {
		if (family == Family::PageLayout) {
			if (pretty) { cb << "\t"; }
			cb << "<style:page-layout style:name=\"" << Escaped(name) << "\"";
		} else if (family == Family::Note) {
			if (pretty) { cb << "\t"; }
			cb << "<text:notes-configuration text:note-class=\"" << Escaped(name) << "\"";
			if (parent) { cb << " text:citation-style-name=\"" << Escaped(parent->getName()) << "\""; }
			writeNoteProperties(cb, stringIdCb, pretty);
			cb << "/>";
			if (pretty) { cb << "\n"; }
			return;
		} else if (family == Family::List) {
			if (pretty) { cb << "\t"; }
			cb << "<text:list-style style:name=\"" << Escaped(name) << "\" text:consecutive-numbering=\"" << consecutiveNumbering << "\"";
		} else {
			writeCustomStyle(cb, stringIdCb, pretty);
			return;
		}

		if (parent) { cb << " style:parent-style-name=\"" << Escaped(parent->getName()) << "\""; }
		if (!styleClass.empty()) { cb << " style:class=\"" << Escaped(styleClass) << "\""; }
	} else {
		if (family == Family::PageLayout) {
			cb << "<style:default-page-layout";
		} else if (family == Family::List || family == Family::Note) {
			if (pretty) { cb << "\n"; }
			std::cout << "Default list styles is not supported\n";
			return;
		} else {
			writeDefaultStyle(cb, stringIdCb, pretty);
			return;
		}
	}

	if (family == Family::PageLayout && pageLayout.empty()) {
		cb << "/>";
		if (pretty) { cb << "\n"; }
		return;
	} else if (family == Family::List && list.empty()) {
		cb << "/>";
		if (pretty) { cb << "\n"; }
		return;
	} else if (family != Family::PageLayout && family != Family::List && text.empty() && paragraph.empty() && table.empty()
			&& tableColumn.empty() && tableRow.empty() && tableCell.empty() && graphic.empty()) {
		cb << "/>";
		if (pretty) { cb << "\n"; }
		return;
	}

	cb << ">";
	if (pretty) { cb << "\n"; }

	if (family == Family::PageLayout) {
		writePageProperties(cb, stringIdCb, pretty);
	} else if (family == Family::List) {
		for (auto &it : list) {
			writeListProperties(cb, stringIdCb, pretty, *it);
		}
	} else {
		writeStyleProperties(cb, stringIdCb, pretty);
	}

	if (pretty) { cb << "\t"; }
	if (type != Default) {
		if (family == Family::PageLayout) {
			cb << "</style:page-layout>";
		} else if (family == Family::List) {
			cb << "</text:list-style>";
		} else {
			cb << "</style:style>";
		}
	} else {
		cb << ((family == Family::PageLayout) ? StringView("</style:default-page-layout>") : StringView("</style:default-style>"));
	}
	if (pretty) { cb << "\n"; }
}

void Style::writeDefaultStyle(const WriteCallback &cb, const Callback<StringView(StringId)> &stringIdCb, bool pretty) const {
	auto writeStyle = [&] (Family target) {
		if (pretty) { cb << "\t"; }
		cb << "<style:default-style style:family=\"" << target << "\">";
		if (pretty) { cb << "\n"; }
		writeStyleProperties(cb, stringIdCb, pretty, target);
		if (target == Paragraph) {
			if (!text.empty()) { writeStyleProperties(cb, stringIdCb, pretty, Text); }
		}
		if (pretty) { cb << "\t"; }
		cb << "</style:default-style>";
		if (pretty) { cb << "\n"; }
	};

	if (!text.empty()) { writeStyle(Text); }
	if (!paragraph.empty()) { writeStyle(Paragraph); }
	if (!table.empty()) { writeStyle(Table); }
	if (!tableColumn.empty()) { writeStyle(TableColumn); }
	if (!tableRow.empty()) { writeStyle(TableRow); }
	if (!tableCell.empty()) { writeStyle(TableCell); }
	if (!graphic.empty()) { writeStyle(Graphic); }
}

void Style::writeCustomStyle(const WriteCallback &cb, const Callback<StringView(StringId)> &stringIdCb, bool pretty) const {
	auto writeStyle = [&] (Family target) {
		if (pretty) { cb << "\t"; }
		cb << "<style:style style:name=\"" << Escaped(name) << "\" style:family=\"" << target << "\"";

		if (target == Paragraph || target == Table) {
			if (master) { cb << " style:master-page-name=\"" << Escaped(master->getName()) << "\""; }
		}
		if (target == Paragraph && defaultOutline) {
			cb << " style:default-outline-level=\"" << uint32_t(defaultOutline) << "\"";
		}

		if (parent) { cb << " style:parent-style-name=\"" << Escaped(parent->getName()) << "\""; }
		if (!styleClass.empty()) { cb << " style:class=\"" << Escaped(styleClass) << "\""; }
		cb << ">";
		if (pretty) { cb << "\n"; }

		if (target == Paragraph) {
			if (!paragraph.empty()) { writeStyleProperties(cb, stringIdCb, pretty, Paragraph); }
			if (!text.empty()) { writeStyleProperties(cb, stringIdCb, pretty, Text); }
		} else {
			writeStyleProperties(cb, stringIdCb, pretty, target);
		}

		if (pretty) { cb << "\t"; }
		cb << "</style:style>";
		if (pretty) { cb << "\n"; }
	};

	if (family == Paragraph) {
		if (!paragraph.empty() || !text.empty()) { writeStyle(Paragraph); }
	} else {
		if (!text.empty()) { writeStyle(Text); }
		if (!paragraph.empty()) { writeStyle(Paragraph); }
	}

	if (!table.empty()) { writeStyle(Table); }
	if (!tableColumn.empty()) { writeStyle(TableColumn); }
	if (!tableRow.empty()) { writeStyle(TableRow); }
	if (!tableCell.empty()) { writeStyle(TableCell); }
	if (!graphic.empty()) { writeStyle(Graphic); }
}

Style & Style::setDefaultOutlineLevel(uint32_t size) {
	defaultOutline = size;
	return *this;
}

Style & Style::setMasterPage(const MasterPage *page) {
	master = page;
	return *this;
}

bool Style::empty() const {
	switch (family) {
	case PageLayout:
		return pageLayout.empty();
		break;
	default:
		return text.empty() && paragraph.empty();
		break;
	}
	return false;
}

void Style::writePageProperties(const WriteCallback &cb, const Callback<StringView(StringId)> &stringIdCb, bool pretty) const {
	if (pretty) { cb << "\t\t"; }
	cb << "<style:page-layout-properties";

	auto writeMargin = [] (const WriteCallback &cb, const Margin &margin) {
		if (margin.isSingle()) {
			if (margin.top.isValid()) {
				cb << " fo:margin=\"" << margin.top << "\"";
			}
		} else {
			if (margin.top.isValid()) { cb << " fo:margin-top=\"" << margin.top << "\""; }
			if (margin.left.isValid()) { cb << " fo:margin-left=\"" << margin.left << "\""; }
			if (margin.bottom.isValid()) { cb << " fo:margin-bottom=\"" << margin.bottom << "\""; }
			if (margin.right.isValid()) { cb << " fo:margin-right=\"" << margin.right << "\""; }
		}
	};

	auto writePadding = [] (const WriteCallback &cb, const Padding &padding) {
		if (padding.isSingle()) {
			if (padding.top.isValid()) {
				cb << " fo:padding=\"" << padding.top << "\"";
			}
		} else {
			if (padding.top.isValid()) { cb << " fo:padding-top=\"" << padding.top << "\""; }
			if (padding.left.isValid()) { cb << " fo:padding-left=\"" << padding.left << "\""; }
			if (padding.bottom.isValid()) { cb << " fo:padding-bottom=\"" << padding.bottom << "\""; }
			if (padding.right.isValid()) { cb << " fo:padding-right=\"" << padding.right << "\""; }
		}
	};

	bool hasHeaderInfo = false;
	bool hasFooterInfo = false;

	Border borderleft, borderright, bordertop, borderbottom;
	Margin margin, headerMargin, footerMargin;
	Padding padding, headerPadding, footerPadding;
	Metric headerHeight, headerMinHeight, footerHeight, footerMinHeight;

	for (auto &it : pageLayout) {
		switch (it.name) {
		case Name::PageBackgroundColor: cb << " fo:background-color=\"" << it.value.color << "\""; break;
		case Name::PageBorderTopStyle: bordertop.style = it.value.lineStyle; break;
		case Name::PageBorderTopWidth: bordertop.width = it.value.metric; break;
		case Name::PageBorderTopColor: bordertop.color = it.value.color; break;
		case Name::PageBorderRightStyle: borderright.style = it.value.lineStyle; break;
		case Name::PageBorderRightWidth: borderright.width = it.value.metric; break;
		case Name::PageBorderRightColor: borderright.color = it.value.color; break;
		case Name::PageBorderBottomStyle: borderbottom.style = it.value.lineStyle; break;
		case Name::PageBorderBottomWidth: borderbottom.width = it.value.metric; break;
		case Name::PageBorderBottomColor: borderbottom.color = it.value.color; break;
		case Name::PageBorderLeftStyle: borderleft.style = it.value.lineStyle; break;
		case Name::PageBorderLeftWidth: borderleft.width = it.value.metric; break;
		case Name::PageBorderLeftColor: borderleft.color = it.value.color; break;
		case Name::PageMarginTop: margin.top = it.value.metric; break;
		case Name::PageMarginRight: margin.right = it.value.metric; break;
		case Name::PageMarginBottom: margin.bottom = it.value.metric; break;
		case Name::PageMarginLeft: margin.left = it.value.metric; break;
		case Name::PagePaddingTop: padding.top = it.value.metric; break;
		case Name::PagePaddingRight: padding.right = it.value.metric; break;
		case Name::PagePaddingBottom: padding.bottom = it.value.metric; break;
		case Name::PagePaddingLeft: padding.left = it.value.metric; break;
		case Name::PageHeight: cb << " fo:page-height=\"" << it.value.metric << "\""; break;
		case Name::PageWidth: cb << " fo:page-width=\"" << it.value.metric << "\""; break;
		case Name::PageFirstPageNumber: cb << " style:first-page-number=\"" << it.value.unsignedValue << "\""; break;
		case Name::PageFootnoteMaxHeight: cb << " style:footnote-max-height=\"" << it.value.metric << "\""; break;
		case Name::PageHeaderMinHeight: hasHeaderInfo = true; headerMinHeight = it.value.metric; break;
		case Name::PageHeaderHeight: hasHeaderInfo = true; headerHeight = it.value.metric; break;
		case Name::PageHeaderMarginTop: hasHeaderInfo = true; headerMargin.top = it.value.metric; break;
		case Name::PageHeaderMarginRight: hasHeaderInfo = true; headerMargin.right = it.value.metric; break;
		case Name::PageHeaderMarginBottom: hasHeaderInfo = true; headerMargin.bottom = it.value.metric; break;
		case Name::PageHeaderMarginLeft: hasHeaderInfo = true; headerMargin.left = it.value.metric; break;
		case Name::PageHeaderPaddingTop: hasHeaderInfo = true; headerPadding.top = it.value.metric; break;
		case Name::PageHeaderPaddingRight: hasHeaderInfo = true; headerPadding.right = it.value.metric; break;
		case Name::PageHeaderPaddingBottom: hasHeaderInfo = true; headerPadding.bottom = it.value.metric; break;
		case Name::PageHeaderPaddingLeft: hasHeaderInfo = true; headerPadding.left = it.value.metric; break;
		case Name::PageFooterMinHeight: hasFooterInfo = true; footerMinHeight = it.value.metric; break;
		case Name::PageFooterHeight: hasFooterInfo = true; footerHeight = it.value.metric; break;
		case Name::PageFooterMarginTop: hasFooterInfo = true; footerMargin.top = it.value.metric; break;
		case Name::PageFooterMarginRight: hasFooterInfo = true; footerMargin.right = it.value.metric; break;
		case Name::PageFooterMarginBottom: hasFooterInfo = true; footerMargin.bottom = it.value.metric; break;
		case Name::PageFooterMarginLeft: hasFooterInfo = true; footerMargin.left = it.value.metric; break;
		case Name::PageFooterPaddingTop: hasFooterInfo = true; footerMargin.top = it.value.metric; break;
		case Name::PageFooterPaddingRight: hasFooterInfo = true; footerMargin.right = it.value.metric; break;
		case Name::PageFooterPaddingBottom: hasFooterInfo = true; footerMargin.bottom = it.value.metric; break;
		case Name::PageFooterPaddingLeft: hasFooterInfo = true; footerMargin.left = it.value.metric; break;
		default: break;
		}
	}

	if (borderleft.style != LineStyle::None) { cb << " fo:border-left=\"" << borderleft << "\""; }
	if (borderright.style != LineStyle::None) { cb << " fo:border-right=\"" << borderright << "\""; }
	if (bordertop.style != LineStyle::None) { cb << " fo:border-top=\"" << bordertop << "\""; }
	if (borderright.style != LineStyle::None) { cb << " fo:border-right=\"" << borderright << "\""; }

	writeMargin(cb, margin);
	writePadding(cb, padding);

	cb << "/>";
	if (pretty) { cb << "\n"; }

	if (type != Default) {
		if (pretty) { cb << "\t\t"; }
		if (hasHeaderInfo) {
			cb << "<style:header-style><style:header-footer-properties";
			writeMargin(cb, headerMargin);
			writePadding(cb, headerPadding);
			if (headerMinHeight.isValid()) {
				cb << " fo:min-height=\"" << headerMinHeight << "\"";
			}
			if (headerHeight.isValid()) {
				cb << " svg:height=\"" << headerMinHeight << "\"";
			}
			cb << "/></style:header-style>";
		} else {
			cb << "<style:header-style/>";
		}
		if (pretty) { cb << "\n\t\t"; }
		if (hasFooterInfo) {
			cb << "<style:footer-style><style:header-footer-properties";
			writeMargin(cb, footerMargin);
			writePadding(cb, footerPadding);
			if (footerMinHeight.isValid()) {
				cb << " fo:min-height=\"" << headerMinHeight << "\"";
			}
			if (footerHeight.isValid()) {
				cb << " svg:height=\"" << headerMinHeight << "\"";
			}
			cb << "/></style:footer-style>";
		} else {
			cb << "<style:footer-style/>";
		}
		if (pretty) { cb << "\n"; }
	}
}

static void writeTextProperties(const WriteCallback &cb, const Callback<StringView(StringId)> &stringIdCb,
		bool pretty, const Vector<Style::Parameter> &text) {
	if (pretty) { cb << "\t\t"; }
	cb << "<style:text-properties";
	for (auto &it : text) {
		switch (it.name) {
		case Name::TextBackgroundColor: cb << " fo:background-color=\"" << it.value.color << "\""; break;
		case Name::TextColor: cb << " fo:color=\"" << it.value.color << "\""; break;
		case Name::TextFontFamily: cb << " fo:font-family=\"" << stringIdCb(it.value.stringId) << "\""; break;
		case Name::TextFontSize: cb << " fo:font-size=\"" << it.value.metric << "\""; break;
		case Name::TextFontStyle: cb << " fo:font-style=\"" << it.value.fontStyle << "\""; break;
		case Name::TextFontVariant: cb << " fo:font-variant=\"" << it.value.fontVariant << "\""; break;
		case Name::TextFontWeight: cb << " fo:font-weight=\"" << it.value.fontWeight << "\""; break;
		case Name::TextHyphenate: cb << " fo:hyphenate=\"" << it.value.boolValue << "\""; break;
		case Name::TextHyphenationPushCharCount: cb << " fo:hyphenation-push-char-count=\"" << it.value.unsignedValue << "\""; break;
		case Name::TextHyphenationRemainCharCount: cb << " fo:hyphenation-remain-char-count=\"" << it.value.unsignedValue << "\""; break;
		case Name::TextLetterSpacing: cb << " fo:letter-spacing=\"" << it.value.metric << "\""; break;
		case Name::TextTransform: cb << " fo:text-transform=\"" << it.value.textTransform << "\""; break;
		case Name::TextFontFamilyGeneric: cb << " style:font-family-generic=\"" << it.value.fontFamilyGeneric << "\""; break;
		case Name::TextFontName: cb << " style:font-name=\"" << stringIdCb(it.value.stringId) << "\""; break;
		case Name::TextFontPitch: cb << " style:font-pitch=\"" << it.value.fontPitch << "\""; break;
		case Name::TextFontStyleName: cb << " style:font-style-name=\"" << stringIdCb(it.value.stringId) << "\""; break;
		case Name::TextPosition: cb << " style:text-position=\"" << it.value.textPosition << "\""; break;
		case Name::TextUnderlineColor: cb << " style:text-underline-color=\"" << it.value.color << "\""; break;
		case Name::TextUnderlineMode: cb << " style:text-underline-mode=\"" << it.value.lineMode << "\""; break;
		case Name::TextUnderlineStyle: cb << " style:text-underline-style=\"" << it.value.lineStyle << "\""; break;
		case Name::TextUnderlineType: cb << " style:text-underline-type=\"" << it.value.lineType << "\""; break;
		case Name::TextUnderlineWidth: cb << " style:text-underline-width=\"" << it.value.metric << "\""; break;
		case Name::TextScale: cb << " style:text-scale=\"" << PercentValue(it.value.floatValue) << "\""; break;
		case Name::TextLanguage: cb << " fo:language=\"" << stringIdCb(it.value.stringId) << "\""; break;
		case Name::TextCountry: cb << " fo:country=\"" << stringIdCb(it.value.stringId) << "\""; break;
		default: break;
		}
	}
	cb << "/>";
	if (pretty) { cb << "\n"; }
}

void Style::writeListProperties(const WriteCallback &cb, const Callback<StringView(StringId)> &stringIdCb, bool pretty, const ListLevel &item) const {
	if (pretty) { cb << "\t\t"; }
	switch (item.type) {
	case ListLevel::Bullet: cb << "<text:list-level-style-bullet"; break;
	case ListLevel::Number: cb << "<text:list-level-style-number"; break;
	}

	cb << " text:level=\"" << item.level << "\"";
	if (item.textStyle) {
		cb << " text:style-name=\"" << item.textStyle->getName() << "\"";
	}

	for (auto &it : item.listStyle) {
		switch (it.name) {
		case Name::ListStyleNumPrefix: cb << " style:num-prefix=\"" << stringIdCb(it.value.stringId) << "\""; break;
		case Name::ListStyleNumSuffix: cb << " style:num-suffix=\"" << stringIdCb(it.value.stringId) << "\""; break;
		case Name::ListStyleNumFormat: cb << " style:num-format=\"" << stringIdCb(it.value.stringId) << "\""; break;
		case Name::ListStyleNumLetterSync: cb << " style:num-letter-sync=\"" << it.value.boolValue << "\""; break;
		case Name::ListStyleBulletChar: cb << " text:bullet-char=\"" << stringIdCb(it.value.stringId) << "\""; break;
		case Name::ListStyleBulletRelativeSize: cb << " text:bullet-relative-size=\"" << PercentValue(it.value.floatValue) << "\""; break;
		case Name::ListStyleDisplayLevels: cb << " text:display-levels=\"" << it.value.unsignedValue << "\""; break;
		case Name::ListStyleStartValue: cb << " text:start-value=\"" << it.value.unsignedValue << "\""; break;
		default: break;
		}
	}

	if (item.text.empty() && item.listLevel.empty() && item.listLevelLabel.empty()) {
		cb << "/>";
		if (pretty) { cb << "\n"; }
		return;
	}

	cb << ">";
	if (pretty) { cb << "\n"; }


	if (!item.text.empty()) {
		if (pretty) { cb << "\t"; }
		writeTextProperties(cb, stringIdCb, pretty, item.text);
	}

	if (!item.listLevel.empty()) {
		if (pretty) { cb << "\t\t\t"; }
		cb << "<style:list-level-properties";
		for (auto &it : item.listLevel) {
			switch (it.name) {
			case Name::ListLevelHeight: cb << " fo:height=\"" << it.value.metric << "\""; break;
			case Name::ListLevelTextAlign: cb << " fo:text-align=\"" << it.value.textAlign << "\""; break;
			case Name::ListLevelWidth: cb << " fo:width=\"" << it.value.metric << "\""; break;
			case Name::ListLevelFontName: cb << " style:font-name=\"" << stringIdCb(it.value.stringId) << "\""; break;
			case Name::ListLevelVerticalPos: cb << " style:vertical-pos=\"" << it.value.verticalPos << "\""; break;
			case Name::ListLevelVerticalRel: cb << " style:vertical-rel=\"" << it.value.verticalRel << "\""; break;
			case Name::ListLevelY: cb << " svg:y=\"" << it.value.metric << "\""; break;
			case Name::ListLevelPositionMode: cb << " text:list-level-position-and-space-mode=\"" << it.value.listLevelMode << "\""; break;
			case Name::ListLevelMinLabelDistance: cb << " text:min-label-distance=\"" << it.value.metric << "\""; break;
			case Name::ListLevelMinLabelWidth: cb << " text:min-label-width=\"" << it.value.metric << "\""; break;
			case Name::ListLevelSpaceBefore: cb << " text:space-before=\"" << it.value.metric << "\""; break;
			default: break;
			}
		}

		if (item.listLevelLabel.empty()) {
			cb << "/>";
			if (pretty) { cb << "\n"; }
		} else {
			cb << ">";
			if (pretty) { cb << "\n"; }

			if (pretty) { cb << "\t\t\t\t"; }
			cb << "<style:list-level-label-alignment";
			for (auto &it : item.listLevelLabel) {
				switch (it.name) {
				case Name::ListLevelLabelMarginLeft: cb << " fo:margin-left=\"" << it.value.metric << "\""; break;
				case Name::ListLevelLabelTextIndent: cb << " fo:text-indent=\"" << it.value.metric << "\""; break;
				case Name::ListLevelLabelFollowedBy: cb << " text:label-followed-by=\"" << it.value.labelFollowedBy << "\""; break;
				case Name::ListLevelLabelTabStopPosition: cb << " text:list-tab-stop-position=\"" << it.value.metric << "\""; break;
				default: break;
				}
			}
			cb << "/>";
			if (pretty) { cb << "\n"; }

			if (pretty) { cb << "\t\t\t"; }
			cb << "</style:list-level-properties>";
			if (pretty) { cb << "\n"; }
		}

	}

	if (pretty) { cb << "\t\t"; }
	switch (item.type) {
	case ListLevel::Bullet: cb << "</text:list-level-style-bullet>"; break;
	case ListLevel::Number: cb << "</text:list-level-style-number>"; break;
	}
	if (pretty) { cb << "\n"; }
}

void Style::writeNoteProperties(const WriteCallback &cb, const Callback<StringView(StringId)> &stringIdCb, bool pretty) const {
	for (auto &it : note) {
		switch (it.name) {
		case Name::NoteNumFormat: cb << " style:num-format=\"" << stringIdCb(it.value.stringId) << "\""; break; // StringView  style:num-format
		case Name::NoteNumLetterSync: cb << " style:num-letter-sync=\"" << it.value.boolValue << "\""; break; // bool
		case Name::NoteNumPrefix: cb << " style:num-prefix=\"" << stringIdCb(it.value.stringId) << "\""; break; // StringView
		case Name::NoteNumSuffix: cb << " style:num-suffix=\"" << stringIdCb(it.value.stringId) << "\""; break; // StringView
		case Name::NoteStartValue: cb << " text:start-value=\"" << it.value.unsignedValue << "\""; break;
		case Name::NoteFootnotesPosition: cb << " text:footnotes-position=\"" << it.value.footnotesPosition << "\""; break;
		case Name::NoteStartNumberingAt: cb << " text:start-numbering-at=\"" << it.value.startNumberingAt << "\""; break;
		default: break;
		}
	}
}

void Style::writeStyleProperties(const WriteCallback &cb, const Callback<StringView(StringId)> &stringIdCb, bool pretty, Family target) const {
	if (!text.empty() && (target == Undefined || target == Text)) {
		writeTextProperties(cb, stringIdCb, pretty, text);
	}

	if (!paragraph.empty() && (target == Undefined || target == Paragraph)) {
		if (pretty) { cb << "\t\t"; }
		cb << "<style:paragraph-properties";
		Border borderleft, borderright, bordertop, borderbottom;
		Margin margin;
		Padding padding;
		for (auto &it : paragraph) {
			switch (it.name) {
			case Name::ParagraphBackgroundColor: cb << " fo:background-color=\"" << it.value.color << "\""; break;
			case Name::ParagraphBorderTopStyle: bordertop.style = it.value.lineStyle; break;
			case Name::ParagraphBorderTopWidth: bordertop.width = it.value.metric; break;
			case Name::ParagraphBorderTopColor: bordertop.color = it.value.color; break;
			case Name::ParagraphBorderRightStyle: borderright.style = it.value.lineStyle; break;
			case Name::ParagraphBorderRightWidth: borderright.width = it.value.metric; break;
			case Name::ParagraphBorderRightColor: borderright.color = it.value.color; break;
			case Name::ParagraphBorderBottomStyle: borderbottom.style = it.value.lineStyle; break;
			case Name::ParagraphBorderBottomWidth: borderbottom.width = it.value.metric; break;
			case Name::ParagraphBorderBottomColor: borderbottom.color = it.value.color; break;
			case Name::ParagraphBorderLeftStyle: borderleft.style = it.value.lineStyle; break;
			case Name::ParagraphBorderLeftWidth: borderleft.width = it.value.metric; break;
			case Name::ParagraphBorderLeftColor: borderleft.color = it.value.color; break;
			case Name::ParagraphBreakAfter: cb << " fo:break-after=\"" << it.value.breakMode << "\""; break;
			case Name::ParagraphBreakBefore: cb << " fo:break-before=\"" << it.value.breakMode << "\""; break;
			case Name::ParagraphKeepTogether: cb << " fo:keep-together=\"" << it.value.keepMode << "\""; break;
			case Name::ParagraphKeepWithNext: cb << " fo:keep-with-next=\"" << it.value.keepMode << "\""; break;
			case Name::ParagraphLineHeight: cb << " fo:line-height=\"" << it.value.metric << "\""; break;
			case Name::ParagraphMarginTop: margin.top = it.value.metric; break;
			case Name::ParagraphMarginRight: margin.right = it.value.metric; break;
			case Name::ParagraphMarginBottom: margin.bottom = it.value.metric; break;
			case Name::ParagraphMarginLeft: margin.left = it.value.metric; break;
			case Name::ParagraphPaddingTop: padding.top = it.value.metric; break;
			case Name::ParagraphPaddingRight: padding.right = it.value.metric; break;
			case Name::ParagraphPaddingBottom: padding.bottom = it.value.metric; break;
			case Name::ParagraphPaddingLeft: padding.left = it.value.metric; break;
			case Name::ParagraphTextAlign: cb << " fo:text-align=\"" << it.value.textAlign << "\""; break;
			case Name::ParagraphTextAlignLast: cb << " fo:text-align-last=\"" << it.value.textAlignLast << "\""; break;
			case Name::ParagraphTextIndent: cb << " fo:text-indent=\"" << it.value.metric << "\""; break;
			case Name::ParagraphBackgroundTransparency: cb << " style:background-transparency=\"" << PercentValue(it.value.floatValue) << "\""; break;
			case Name::ParagraphAutoTextIndent: cb << " style:auto-text-indent=\"" << it.value.boolValue << "\""; break;
			case Name::ParagraphFontIndependentLineSpacing: cb << " style:font-independent-line-spacing=\"" << it.value.boolValue << "\""; break;
			case Name::ParagraphJoinBorder: cb << " style:join-border=\"" << it.value.boolValue << "\""; break;
			case Name::ParagraphJustifySingleWord: cb << " style:justify-single-word=\"" << it.value.boolValue << "\""; break;
			case Name::ParagraphLineBreak: cb << " style:line-break=\"" << it.value.lineBreak << "\""; break;
			case Name::ParagraphLineHeightAtLeast: cb << " style:line-height-at-least=\"" << it.value.metric << "\""; break;
			case Name::ParagraphLineSpacing: cb << " style:line-spacing=\"" << it.value.metric << "\""; break;
			case Name::ParagraphPageNumber: cb << " style:page-number=\"" << it.value.unsignedValue << "\""; break;
			case Name::ParagraphPunctuationWrap: cb << " style:punctuation-wrap=\"" << it.value.punctuationWrap << "\""; break;
			case Name::ParagraphRegisterTrue: cb << " style:register-true=\"" << it.value.boolValue << "\""; break;
			case Name::ParagraphTabStopDistance: cb << " style:tab-stop-distance=\"" << it.value.metric << "\""; break;
			case Name::ParagraphVerticalAlign: cb << " style:vertical-align=\"" << it.value.verticalAlign << "\""; break;
			case Name::ParagraphLineNumber: cb << " text:line-number=\"" << it.value.unsignedValue << "\""; break;
			case Name::ParagraphNumberLines: cb << " text:number-lines=\"" << it.value.boolValue << "\""; break;
			case Name::ParagraphHyphenationLadderCount:
				if (it.value.unsignedValue == stappler::maxOf<uint32_t>()) {
					cb << " fo:hyphenation-ladder-count=\"no-limit\"";
				} else {
					cb << " fo:hyphenation-ladder-count=\"" << it.value.unsignedValue << "\"";
				}
				break;
			case Name::ParagraphOrphans: cb << " fo:orphans=\"" << it.value.unsignedValue << "\""; break;
			case Name::ParagraphWidows: cb << " fo:widows=\"" << it.value.unsignedValue << "\""; break;
			default: break;
			}
		}

		if (borderleft.style != LineStyle::None) { cb << " fo:border-left=\"" << borderleft << "\""; }
		if (borderright.style != LineStyle::None) { cb << " fo:border-right=\"" << borderright << "\""; }
		if (bordertop.style != LineStyle::None) { cb << " fo:border-top=\"" << bordertop << "\""; }
		if (borderbottom.style != LineStyle::None) { cb << " fo:border-bottom=\"" << borderbottom << "\""; }

		if (margin.isSingle()) {
			cb << " fo:margin=\"" << margin.top << "\"";
		} else {
			if (margin.top.isValid()) { cb << " fo:margin-top=\"" << margin.top << "\""; }
			if (margin.left.isValid()) { cb << " fo:margin-left=\"" << margin.left << "\""; }
			if (margin.bottom.isValid()) { cb << " fo:margin-bottom=\"" << margin.bottom << "\""; }
			if (margin.right.isValid()) { cb << " fo:margin-right=\"" << margin.right << "\""; }
		}

		if (padding.isSingle()) {
			cb << " fo:padding=\"" << padding.top << "\"";
		} else {
			if (padding.top.isValid()) { cb << " fo:padding-top=\"" << padding.top << "\""; }
			if (padding.left.isValid()) { cb << " fo:padding-left=\"" << padding.left << "\""; }
			if (padding.bottom.isValid()) { cb << " fo:padding-bottom=\"" << padding.bottom << "\""; }
			if (padding.right.isValid()) { cb << " fo:padding-right=\"" << padding.right << "\""; }
		}

		cb << "/>";
		if (pretty) { cb << "\n"; }
	}

	if (!table.empty() && (target == Undefined || target == Table)) {
		if (pretty) { cb << "\t\t"; }
		cb << "<style:table-properties";

		Margin margin;
		for (auto &it : table) {
			switch (it.name) {
			case Name::TableBackgroundColor: cb << " fo:background-color=\"" << it.value.color << "\""; break;
			case Name::TableBreakAfter: cb << " fo:break-after=\"" << it.value.breakMode << "\""; break;
			case Name::TableBreakBefore: cb << " fo:break-before=\"" << it.value.breakMode << "\""; break;
			case Name::TableKeepWithNext: cb << " fo:keep-with-next=\"" << it.value.keepMode << "\""; break;
			case Name::TableMarginTop: margin.top = it.value.metric; break;
			case Name::TableMarginRight: margin.right = it.value.metric; break;
			case Name::TableMarginBottom: margin.bottom = it.value.metric; break;
			case Name::TableMarginLeft: margin.left = it.value.metric; break;
			case Name::TablePageNumber: cb << " style:page-number=\"" << it.value.unsignedValue << "\""; break;
			case Name::TableMayBreakBetweenRows: cb << " style:may-break-between-rows=\"" << it.value.boolValue << "\""; break;
			case Name::TableRelWidth: cb << " style:rel-width=\"" << it.value.metric << "\""; break;
			case Name::TableWidth: cb << " style:width=\"" << it.value.metric << "\""; break;
			case Name::TableAlign: cb << " table:align=\"" << it.value.tableAlign << "\""; break;
			case Name::TableBorderModel: cb << " table:border-model=\"" << it.value.borderModel << "\""; break;
			default: break;
			}
		}

		if (margin.isSingle()) {
			cb << " fo:margin=\"" << margin.top << "\"";
		} else {
			if (margin.top.isValid()) { cb << " fo:margin-top=\"" << margin.top << "\""; }
			if (margin.left.isValid()) { cb << " fo:margin-left=\"" << margin.left << "\""; }
			if (margin.bottom.isValid()) { cb << " fo:margin-bottom=\"" << margin.bottom << "\""; }
			if (margin.right.isValid()) { cb << " fo:margin-right=\"" << margin.right << "\""; }
		}

		cb << "/>";
		if (pretty) { cb << "\n"; }
	}

	if (!tableColumn.empty() && (target == Undefined || target == TableColumn)) {
		if (pretty) { cb << "\t\t"; }
		cb << "<style:table-column-properties";

		for (auto &it : tableColumn) {
			switch (it.name) {
			case Name::TableColumnBreakAfter: cb << " fo:break-after=\"" << it.value.breakMode << "\""; break;
			case Name::TableColumnBreakBefore: cb << " fo:break-before=\"" << it.value.breakMode << "\""; break;
			case Name::TableColumnUseOptimalWidth: cb << " style:use-optimal-column-width=\"" << it.value.boolValue << "\""; break;
			case Name::TableColumnRelWidth: cb << " style:rel-column-width=\"" << it.value.unsignedValue << "*\""; break;
			case Name::TableColumnWidth: cb << " style:column-width=\"" << it.value.metric << "\""; break;
			default: break;
			}
		}

		cb << "/>";
		if (pretty) { cb << "\n"; }
	}

	if (!tableRow.empty() && (target == Undefined || target == TableRow)) {
		if (pretty) { cb << "\t\t"; }
		cb << "<style:table-row-properties";

		for (auto &it : tableRow) {
			switch (it.name) {
			case Name::TableRowBackgroundColor: cb << " fo:background-color=\"" << it.value.color << "\""; break;
			case Name::TableRowBreakAfter: cb << " fo:break-after=\"" << it.value.breakMode << "\""; break;
			case Name::TableRowBreakBefore: cb << " fo:break-before=\"" << it.value.breakMode << "\""; break;
			case Name::TableRowKeepTogether: cb << " fo:keep-together=\"" << it.value.keepMode << "\""; break;
			case Name::TableRowMinHeight: cb << " style:min-row-height=\"" << it.value.metric << "\""; break;
			case Name::TableRowHeight: cb << " style:row-height=\"" << it.value.metric << "\""; break;
			case Name::TableRowUseOptimalHeight: cb << " style:use-optimal-row-height=\"" << it.value.metric << "\""; break;
			default: break;
			}
		}

		cb << "/>";
		if (pretty) { cb << "\n"; }
	}

	if (!tableCell.empty() && (target == Undefined || target == TableCell)) {
		if (pretty) { cb << "\t\t"; }
		cb << "<style:table-cell-properties";

		Border borderleft, borderright, bordertop, borderbottom;
		Padding padding;
		for (auto &it : tableCell) {
			switch (it.name) {
			case Name::TableCellBackgroundColor: cb << " fo:background-color=\"" << it.value.color << "\""; break;
			case Name::TableCellBorderTopStyle: bordertop.style = it.value.lineStyle; break;
			case Name::TableCellBorderTopWidth: bordertop.width = it.value.metric; break;
			case Name::TableCellBorderTopColor: bordertop.color = it.value.color; break;
			case Name::TableCellBorderRightStyle: borderright.style = it.value.lineStyle; break;
			case Name::TableCellBorderRightWidth: borderright.width = it.value.metric; break;
			case Name::TableCellBorderRightColor: borderright.color = it.value.color; break;
			case Name::TableCellBorderBottomStyle: borderbottom.style = it.value.lineStyle; break;
			case Name::TableCellBorderBottomWidth: borderbottom.width = it.value.metric; break;
			case Name::TableCellBorderBottomColor: borderbottom.color = it.value.color; break;
			case Name::TableCellBorderLeftStyle: borderleft.style = it.value.lineStyle; break;
			case Name::TableCellBorderLeftWidth: borderleft.width = it.value.metric; break;
			case Name::TableCellBorderLeftColor: borderleft.color = it.value.color; break;
			case Name::TableCellPaddingTop: padding.top = it.value.metric; break;
			case Name::TableCellPaddingRight: padding.right = it.value.metric; break;
			case Name::TableCellPaddingBottom: padding.bottom = it.value.metric; break;
			case Name::TableCellPaddingLeft: padding.left = it.value.metric; break;
			case Name::TableCellDecimalPlaces: cb << " style:decimal-places=\"" << it.value.unsignedValue << "\""; break;
			case Name::TableCellPrintContent: cb << " style:print-content=\"" << it.value.boolValue << "\""; break;
			case Name::TableCellRepeatContent: cb << " style:repeat-content=\"" << it.value.boolValue << "\""; break;
			case Name::TableCellShrinkToFit: cb << " style:shrink-to-fit=\"" << it.value.boolValue << "\""; break;
			case Name::TableCellTextAlignSource: cb << " style:text-align-source=\"" << it.value.textAlignSource << "\""; break;
			case Name::TableCellVerticalAlign: cb << " style:vertical-align=\"" << it.value.verticalAlign << "\""; break;
			default: break;
			}
		}

		if (padding.isSingle()) {
			cb << " fo:padding=\"" << padding.top << "\"";
		} else {
			if (padding.top.isValid()) { cb << " fo:padding-top=\"" << padding.top << "\""; }
			if (padding.left.isValid()) { cb << " fo:padding-left=\"" << padding.left << "\""; }
			if (padding.bottom.isValid()) { cb << " fo:padding-bottom=\"" << padding.bottom << "\""; }
			if (padding.right.isValid()) { cb << " fo:padding-right=\"" << padding.right << "\""; }
		}

		if (borderleft.style != LineStyle::None) { cb << " fo:border-left=\"" << borderleft << "\""; }
		if (borderright.style != LineStyle::None) { cb << " fo:border-right=\"" << borderright << "\""; }
		if (bordertop.style != LineStyle::None) { cb << " fo:border-top=\"" << bordertop << "\""; }
		if (borderbottom.style != LineStyle::None) { cb << " fo:border-bottom=\"" << borderbottom << "\""; }

		cb << "/>";
		if (pretty) { cb << "\n"; }

	} else if (!graphic.empty() && (target == Undefined || target == Graphic)) {
		if (pretty) { cb << "\t\t"; }
		cb << "<style:graphic-properties";
		Border borderleft, borderright, bordertop, borderbottom;
		Margin margin;
		Padding padding;

		for (auto &it : graphic) {
			switch (it.name) {
			case Name::DrawAutoGrowHeight: cb << " draw:auto-grow-height=\"" << it.value.boolValue << "\"";  break; // bool  draw:auto-grow-height
			case Name::DrawAutoGrowWidth: cb << " draw:auto-grow-width=\"" << it.value.boolValue << "\""; break; // bool  draw:auto-grow-width
			case Name::DrawRed: cb << " draw:red=\"" << PercentValue(it.value.floatValue) << "\""; break; // float
			case Name::DrawGreen: cb << " draw:green=\"" << PercentValue(it.value.floatValue) << "\""; break; // float
			case Name::DrawBlue: cb << " draw:blue=\"" << PercentValue(it.value.floatValue) << "\""; break; // float  draw:blue
			case Name::DrawColorInversion: cb << " draw:color-inversion=\"" << it.value.boolValue << "\""; break; // bool  draw:color-inversion
			case Name::DrawColorMode: cb << " draw:color-mode=\"" << it.value.colorMode << "\""; break; // enum ColorMode  draw:color-mode
			case Name::DrawContrast: cb << " draw:contrast=\"" << PercentValue(it.value.floatValue) << "\""; break; // float  draw:contrast
			case Name::DrawDecimalPlaces: cb << " draw:decimal-places=\"" << it.value.unsignedValue << "\""; break; // uint32_t  draw:decimal-places
			case Name::DrawAspect: cb << " draw:draw-aspect=\"" << it.value.drawAspect << "\""; break; // enum DrawAspect  draw:draw-aspect
			case Name::DrawFill: cb << " draw:fill=\"" << it.value.drawFill << "\""; break; // enum DrawFill  draw:fill
			case Name::DrawFillColor: cb << " draw:fill-color=\"" << it.value.color << "\""; break; // Color3B  draw:fill-color
			case Name::DrawFillGradientName: cb << " draw:fill-gradient-name=\"" << stringIdCb(it.value.stringId) << "\""; break; // StringView  draw:fill-gradient-name
			case Name::DrawFillHatchName: cb << " draw:fill-hatch-name=\"" << stringIdCb(it.value.stringId) << "\""; break; // StringView  draw:fill-hatch-name
			case Name::DrawFillHatchSolid: cb << " draw:fill-hatch-solid=\"" << it.value.boolValue << "\""; break; // bool  draw:fill-hatch-solid
			case Name::DrawFillImageHeight: cb << " draw:fill-image-height=\"" << PercentValue(it.value.floatValue) << "\""; break; // float  draw:fill-image-height
			case Name::DrawFillImageWidth: cb << " draw:fill-image-width=\"" << PercentValue(it.value.floatValue) << "\""; break; // float  draw:fill-image-width
			case Name::DrawFillImageName: cb << " draw:fill-image-name=\"" << stringIdCb(it.value.stringId) << "\""; break; // StringView  draw:fill-image-name
			case Name::DrawFillImageRefPoint: cb << " draw:fill-image-ref-point=\"" << it.value.fillImageRefPoint << "\""; break; // enum FillImageRefPoint  draw:fill-image-ref-point
			case Name::DrawFillImageRefPointX: cb << " draw:fill-image-ref-point-x=\"" << PercentValue(it.value.floatValue) << "\""; break; // float  draw:fill-image-ref-point-x
			case Name::DrawFillImageRefPointY: cb << " draw:fill-image-ref-point-y=\"" << PercentValue(it.value.floatValue) << "\""; break; // float  draw:fill-image-ref-point-y
			case Name::DrawFitToContour: cb << " draw:fit-to-contour=\"" << it.value.boolValue << "\""; break;// bool  draw:fit-to-contour
			case Name::DrawFitToSize: cb << " draw:fit-to-size=\"" << it.value.boolValue << "\""; break;// bool  draw:fit-to-size
			case Name::DrawFrameDisplayBorder: cb << " draw:frame-display-border=\"" << it.value.boolValue << "\""; break;// bool  draw:frame-display-border
			case Name::DrawFrameDisplayScrollbar: cb << " draw:frame-display-scrollbar=\"" << it.value.boolValue << "\""; break;// bool  draw:frame-display-scrollbar
			case Name::DrawFrameMarginHorizontal: cb << " draw:frame-margin-horizontal=\"" << it.value.unsignedValue << "px\""; break;// uint32_t (as px)  draw:frame-margin-horizontal
			case Name::DrawFrameMarginVertical: cb << " draw:frame-margin-vertical=\"" << it.value.unsignedValue << "px\""; break;// uint32_t (as px)  draw:frame-margin-vertical
			case Name::DrawGamma: cb << " draw:gamma=\"" << PercentValue(it.value.floatValue) << "\""; break;// float  draw:gamma
			case Name::DrawGradientStepCount: cb << " draw:gradient-step-count=\"" << it.value.unsignedValue << "\""; break;// uint32_t  draw:gradient-step-count
			case Name::DrawImageOpacity: cb << " draw:image-opacity=\"" << PercentValue(it.value.floatValue) << "\""; break;// float  draw:image-opacity
			case Name::DrawLuminance: cb << " draw:luminance=\"" << PercentValue(it.value.floatValue) << "\""; break;// float  draw:luminance
			case Name::DrawMarkerEnd: cb << " draw:marker-end=\"" << stringIdCb(it.value.stringId) << "\""; break;// StringView  draw:marker-end
			case Name::DrawMarkerEndCenter: cb << " draw:marker-end-center=\"" << it.value.boolValue << "\""; break;// bool  draw:marker-end-center
			case Name::DrawMarkerEndWidth: cb << " draw:marker-end-width=\"" << it.value.metric << "\""; break;// Metric  draw:marker-end-width
			case Name::DrawMarkerStart: cb << " draw:marker-start=\"" << stringIdCb(it.value.stringId) << "\""; break;// StringView  draw:marker-start
			case Name::DrawMarkerStartCenter: cb << " draw:marker-start-center=\"" << it.value.boolValue << "\""; break;// bool  draw:marker-start-center
			case Name::DrawMarkerStartWidth: cb << " draw:marker-start-width=\"" << it.value.metric << "\""; break;// Metric  draw:marker-start-width
			case Name::DrawOpacity: cb << " draw:opacity=\"" << PercentValue(it.value.floatValue) << "\""; break;// float  draw:opacity
			case Name::DrawOpacityName: cb << " draw:opacity-name=\"" << stringIdCb(it.value.stringId) << "\""; break;// StringView  draw:opacity-name
			case Name::DrawParallel: cb << " draw:draw:parallel=\"" << it.value.boolValue << "\""; break;// bool  draw:parallel
			case Name::DrawSecondaryFillColor: cb << " draw:secondary-fill-color=\"" << it.value.color << "\""; break;// Color3B  draw:secondary-fill-color
			case Name::DrawStroke: cb << " draw:stroke=\"" << it.value.drawStroke << "\""; break;// enum DrawStroke  draw:stroke
			case Name::DrawStrokeDash: cb << " draw:stroke-dash=\"" << stringIdCb(it.value.stringId) << "\""; break;// StringView  draw:stroke-dash
			case Name::DrawStrokeDashNames: cb << " draw:stroke-dash-names=\"" << stringIdCb(it.value.stringId) << "\""; break;// StringView  draw:stroke-dash-names
			case Name::DrawStrokeLinejoin: cb << " draw:stroke-linejoin=\"" << it.value.strokeLinejoin << "\""; break;// enum StrokeLinejoin  draw:stroke-linejoin
			case Name::DrawStrokeLinecap: cb << " svg:stroke-linecap=\"" << it.value.strokeLinecap << "\""; break;// enum StrokeLinecap  svg:stroke-linecap
			case Name::DrawSymbolColor: cb << " draw:symbol-color=\"" << it.value.color << "\""; break;// Color3B  draw:symbol-color
			case Name::DrawVisibleAreaHeight: cb << " draw:visible-area-height=\"" << it.value.metric << "\""; break;// Metric  draw:visible-area-height
			case Name::DrawVisibleAreaLeft: cb << " draw:visible-area-left=\"" << it.value.metric << "\""; break;// Metric  draw:visible-area-left
			case Name::DrawVisibleAreaTop: cb << " draw:visible-area-top=\"" << it.value.metric << "\""; break;// Metric  draw:visible-area-top
			case Name::DrawVisibleAreaWidth: cb << " draw:visible-area-width=\"" << it.value.metric << "\""; break;// Metric  draw:visible-area-width
			case Name::DrawUnit: cb << " draw:unit=\"" << it.value.drawUnit << "\""; break; // enum DrawUnit  draw:unit
			case Name::DrawBackgroundColor: cb << " fo:background-color=\"" << it.value.color << "\""; break; // Color3B  fo:background-color
			case Name::DrawBorderTopStyle: bordertop.style = it.value.lineStyle; break;
			case Name::DrawBorderTopWidth: bordertop.width = it.value.metric; break;
			case Name::DrawBorderTopColor: bordertop.color = it.value.color; break;
			case Name::DrawBorderRightStyle: borderright.style = it.value.lineStyle; break;
			case Name::DrawBorderRightWidth: borderright.width = it.value.metric; break;
			case Name::DrawBorderRightColor: borderright.color = it.value.color; break;
			case Name::DrawBorderBottomStyle: borderbottom.style = it.value.lineStyle; break;
			case Name::DrawBorderBottomWidth: borderbottom.width = it.value.metric; break;
			case Name::DrawBorderBottomColor: borderbottom.color = it.value.color; break;
			case Name::DrawBorderLeftStyle: borderleft.style = it.value.lineStyle; break;
			case Name::DrawBorderLeftWidth: borderleft.width = it.value.metric; break;
			case Name::DrawBorderLeftColor: borderleft.color = it.value.color; break;
			case Name::DrawMarginTop: margin.top = it.value.metric; break;
			case Name::DrawMarginRight: margin.right = it.value.metric; break;
			case Name::DrawMarginBottom: margin.bottom = it.value.metric; break;
			case Name::DrawMarginLeft: margin.left = it.value.metric; break;
			case Name::DrawPaddingTop: padding.top = it.value.metric; break;
			case Name::DrawPaddingRight: padding.right = it.value.metric; break;
			case Name::DrawPaddingBottom: padding.bottom = it.value.metric; break;
			case Name::DrawPaddingLeft: padding.left = it.value.metric; break;
			case Name::DrawMinHeight: cb << " fo:min-height=\"" << it.value.metric << "\""; break; // Metric  fo:min-height
			case Name::DrawMaxHeight: cb << " fo:max-height=\"" << it.value.metric << "\""; break; // Metric  fo:max-height
			case Name::DrawMinWidth: cb << " fo:min-width=\"" << it.value.metric << "\""; break; // Metric  fo:min-width
			case Name::DrawMaxWidth: cb << " fo:max-width=\"" << it.value.metric << "\""; break; // Metric  fo:max-width
			case Name::DrawBackgroundTransparency: cb << " style:background-transparency=\"" << PercentValue(it.value.floatValue) << "\""; break; // float style:background-transparency
			case Name::DrawEditable: cb << " style:editable=\"" << it.value.boolValue << "\""; break;// bool style:editable
			case Name::DrawFlowWithText: cb << " style:flow-with-text=\"" << it.value.boolValue << "\""; break;// bool style:flow-with-text
			case Name::DrawHorizontalPos: cb << " style:horizontal-pos=\"" << it.value.horizontalPos << "\""; break;// enum HorizontalPos  style:horizontal-pos 20.290,
			case Name::DrawHorizontalRel: cb << " style:horizontal-rel=\"" << it.value.horizontalRel << "\""; break;// enum HorizontalRel  style:horizontal-rel 20.291,
			case Name::DrawMirror: cb << " style:mirror=\"" << it.value.mirror << "\""; break;// enum Mirror  style:mirror
			case Name::DrawNumberWrappedParagraphs:
				if (it.value.unsignedValue == stappler::maxOf<uint32_t>()) {
					cb << " style:number-wrapped-paragraphs=\"no-limit\"";
				} else {
					cb << " style:number-wrapped-paragraphs=\"" << it.value.unsignedValue << "\"";
				}
				break;// uint32_t or maxOf<uint32_t> as no-limit  style:number-wrapped-paragraphs
			case Name::DrawPrintContent: cb << " style:print-content=\"" << it.value.boolValue << "\""; break;// bool style:print-content
			case Name::DrawOverflowBehavior: cb << " style:overflow-behavior=\"" << it.value.overflowBehavior << "\""; break;// enum OverflowBehavior  style:overflow-behavior
			case Name::DrawProtect: cb << " style:protect=\"" << it.value.protect << "\""; break;// enum Protect  style:protect
			case Name::DrawRelHeight: cb << " style:rel-height=\"" << it.value.metric << "\""; break;// Metric with percent: break;scale or scale-min  style:rel-height 20.331,
			case Name::DrawRelWidth: cb << " style:rel-width=\"" << it.value.metric << "\""; break; // Metric with percent: break;scale or scale-min  style:rel-width 20.332.1
			case Name::DrawRepeat: cb << " style:repeat=\"" << it.value.repeat << "\""; break;// enum Repeat  style:repeat
			case Name::DrawRunThrough: cb << " style:run-through=\"" << it.value.runThrough << "\""; break;
			case Name::DrawShrinkToFit: cb << " style:shrink-to-fit=\"" << it.value.boolValue << "\""; break;// bool  style:shrink-to-fit
			case Name::DrawVerticalPos: cb << " style:vertical-pos=\"" << it.value.verticalPos << "\""; break;// enum VerticalPos  style:vertical-pos 20.290,
			case Name::DrawVerticalRel: cb << " style:vertical-rel=\"" << it.value.verticalRel << "\""; break;// enum VerticalRel  style:vertical-rel 20.291,
			case Name::DrawWrap: cb << " style:wrap=\"" << it.value.wrap << "\""; break;// enum Wrap  style:wrap
			case Name::DrawWrapContour: cb << " style:wrap-contour=\"" << it.value.boolValue << "\""; break;// bool  style:wrap-contour 20.391
			case Name::DrawWrapContourMode: cb << " style:wrap-contour-mode=\"" << it.value.wrapContourMode << "\""; break;// enum WrapContourMode  style:wrap-contour-mode
			case Name::DrawWrapDynamicThreshold: cb << " style:wrap-dynamic-threshold=\"" << it.value.metric << "\""; break;// Metric  style:wrap-dynamic-threshold
			case Name::DrawWidth: cb << " svg:width=\"" << it.value.metric << "\""; break; // Metric  svg:width
			case Name::DrawHeight: cb << " svg:height=\"" << it.value.metric << "\""; break;// Metric  svg:height
			case Name::DrawX: cb << " svg:x=\"" << it.value.metric << "\""; break;// Metric  svg:x
			case Name::DrawY: cb << " svg:y=\"" << it.value.metric << "\""; break;// Metric  svg:y
			case Name::DrawStrokeWidth: cb << " svg:stroke-width=\"" << it.value.metric << "\""; break;// Metric  svg:stroke-width
			case Name::DrawStrokeOpacity: cb << " svg:stroke-opacity=\"" << PercentValue(it.value.floatValue) << "\""; break;// float  svg:stroke-opacity
			case Name::DrawStrokeColor: cb << " svg:stroke-color=\"" << it.value.color << "\""; break;// Color3B  svg:stroke-color
			case Name::DrawFillRule: cb << " svg:fill-rule=\"" << it.value.fillRule << "\""; break;// enum FillRule  svg:fill-rule
			case Name::DrawAnchorPageNumber: cb << " text:anchor-page-number=\"" << it.value.unsignedValue << "\""; break;// uint32_t  text:anchor-page-number
			case Name::DrawAnchorType: cb << " text:anchor-type=\"" << it.value.anchorType << "\""; break;// enum AnchorType  text:anchor-type
			case Name::DrawClip: cb << " fo:clip=\"" << stringIdCb(it.value.stringId) << "\""; break;
			default: break;
			}
		}

		if (borderleft.style != LineStyle::None) { cb << " fo:border-left=\"" << borderleft << "\""; }
		if (borderright.style != LineStyle::None) { cb << " fo:border-right=\"" << borderright << "\""; }
		if (bordertop.style != LineStyle::None) { cb << " fo:border-top=\"" << bordertop << "\""; }
		if (borderbottom.style != LineStyle::None) { cb << " fo:border-bottom=\"" << borderbottom << "\""; }

		if (margin.isSingle()) {
			cb << " fo:margin=\"" << margin.top << "\"";
		} else {
			if (margin.top.isValid()) { cb << " fo:margin-top=\"" << margin.top << "\""; }
			if (margin.left.isValid()) { cb << " fo:margin-left=\"" << margin.left << "\""; }
			if (margin.bottom.isValid()) { cb << " fo:margin-bottom=\"" << margin.bottom << "\""; }
			if (margin.right.isValid()) { cb << " fo:margin-right=\"" << margin.right << "\""; }
		}

		if (padding.isSingle()) {
			cb << " fo:padding=\"" << padding.top << "\"";
		} else {
			if (padding.top.isValid()) { cb << " fo:padding-top=\"" << padding.top << "\""; }
			if (padding.left.isValid()) { cb << " fo:padding-left=\"" << padding.left << "\""; }
			if (padding.bottom.isValid()) { cb << " fo:padding-bottom=\"" << padding.bottom << "\""; }
			if (padding.right.isValid()) { cb << " fo:padding-right=\"" << padding.right << "\""; }
		}

		cb << "/>";
		if (pretty) { cb << "\n"; }
	}
}

String Metric::toString() const {
	StringStream cb;
	switch (metric) {
	case Metric::Units::None: cb << "none"; break;
	case Metric::Units::Percent: cb << floatValue * 100.0f << "%"; break;
	case Metric::Units::Float: cb << floatValue; break;
	case Metric::Units::Integer: cb << signedValue; break;
	case Metric::Units::Custom:
		switch (customValue) {
		case Metric::Auto: cb << "auto"; break;
		case Metric::Normal: cb << "normal"; break;
		case Metric::Bold: cb << "bold"; break;
		case Metric::Thin: cb << "thin"; break;
		case Metric::Medium: cb << "medium"; break;
		case Metric::Thick: cb << "thick"; break;
		case Metric::Scale: cb << "scale"; break;
		case Metric::ScaleMin: cb << "scale-min"; break;
		}
		break;
	case Metric::Units::Cm: cb << floatValue << "cm"; break;
	case Metric::Units::Mm: cb << floatValue << "mm"; break;
	case Metric::Units::In: cb << floatValue << "in"; break;
	case Metric::Units::Pt: cb << floatValue << "pt"; break;
	case Metric::Units::Pc: cb << floatValue << "pc"; break;
	case Metric::Units::Px: cb << floatValue << "px"; break;
	case Metric::Units::Em: cb << floatValue << "em"; break;
	}
	return cb.str();
}

}


namespace opendocument {

const WriteCallback &operator<<(const WriteCallback &cb, const style::Color3B &c) {
	static const char * hexChars = "0123456789abcdef";

	char b[8] = { 0 };
	b[0] = '#';

	b[1] = hexChars[((c.r & 0xF0) >> 4) & 0x0F];
	b[2] = hexChars[  c.r               & 0x0F];
	b[3] = hexChars[((c.g & 0xF0) >> 4) & 0x0F];
	b[4] = hexChars[  c.g               & 0x0F];
	b[5] = hexChars[((c.b & 0xF0) >> 4) & 0x0F];
	b[6] = hexChars[  c.b               & 0x0F];

	cb(StringView(b, 7));
	return cb;
}

const WriteCallback &operator<<(const WriteCallback &cb, const style::FontStyle &str) {
	switch (str) {
	case style::FontStyle::Normal: cb("normal"); break;
	case style::FontStyle::Italic: cb("italic"); break;
	case style::FontStyle::Oblique: cb("oblique"); break;
	}
	return cb;
}

const WriteCallback &operator<<(const WriteCallback &cb, const style::FontVariant &str) {
	switch (str) {
	case style::FontVariant::Normal: cb("normal"); break;
	case style::FontVariant::SmallCaps: cb("small-caps"); break;
	}
	return cb;
}
const WriteCallback &operator<<(const WriteCallback &cb, const style::FontWeight &str) {
	switch (str) {
	case style::FontWeight::Normal: cb("normal"); break;
	case style::FontWeight::Bold: cb("bold"); break;
	case style::FontWeight::W100: cb("100"); break;
	case style::FontWeight::W200: cb("200"); break;
	case style::FontWeight::W300: cb("300"); break;
	case style::FontWeight::W500: cb("500"); break;
	case style::FontWeight::W600: cb("600"); break;
	case style::FontWeight::W800: cb("800"); break;
	case style::FontWeight::W900: cb("900"); break;
	}
	return cb;
}
const WriteCallback &operator<<(const WriteCallback &cb, const style::TextTransform &str) {
	switch (str) {
	case style::TextTransform::Uppercase: cb("uppercase"); break;
	case style::TextTransform::Lowercase: cb("lowercase"); break;
	case style::TextTransform::None: cb("none"); break;
	}
	return cb;
}
const WriteCallback &operator<<(const WriteCallback &cb, const style::FontFamilyGeneric &str) {
	switch (str) {
	case style::FontFamilyGeneric::None: cb("none"); break;
	case style::FontFamilyGeneric::Decorative: cb("decorative"); break;
	case style::FontFamilyGeneric::Modern: cb("modern"); break;
	case style::FontFamilyGeneric::Roman: cb("roman"); break;
	case style::FontFamilyGeneric::Script: cb("script"); break;
	case style::FontFamilyGeneric::Swiss: cb("swiss"); break;
	case style::FontFamilyGeneric::System: cb("system"); break;
	}
	return cb;
}
const WriteCallback &operator<<(const WriteCallback &cb, const style::FontPitch &str) {
	switch (str) {
	case style::FontPitch::None: cb("none"); break;
	case style::FontPitch::Fixed: cb("fixed"); break;
	case style::FontPitch::Variable: cb("variable"); break;
	}
	return cb;
}
const WriteCallback &operator<<(const WriteCallback &cb, const style::LineMode &str) {
	switch (str) {
	case style::LineMode::Continuous: cb("continuous"); break;
	case style::LineMode::SkipWhiteSpace: cb("skip-white-space"); break;
	}
	return cb;
}
const WriteCallback &operator<<(const WriteCallback &cb, const style::LineStyle &str) {
	switch (str) {
	case style::LineStyle::None: cb("none"); break;
	case style::LineStyle::Dash: cb("dash"); break;
	case style::LineStyle::DotDash: cb("dot-dash"); break;
	case style::LineStyle::DotDotDash: cb("dot-dot-dash"); break;
	case style::LineStyle::Dotted: cb("dotted"); break;
	case style::LineStyle::LongDash: cb("long-dash"); break;
	case style::LineStyle::Solid: cb("solid"); break;
	case style::LineStyle::Wave: cb("wave"); break;
	}
	return cb;
}
const WriteCallback &operator<<(const WriteCallback &cb, const style::LineType &str) {
	switch (str) {
	case style::LineType::Double: cb("double"); break;
	case style::LineType::Single: cb("single"); break;
	}
	return cb;
}
const WriteCallback &operator<<(const WriteCallback &cb, const style::BreakMode &str) {
	switch (str) {
	case style::BreakMode::Auto: cb("auto"); break;
	case style::BreakMode::Column: cb("column"); break;
	case style::BreakMode::Page: cb("page"); break;
	}
	return cb;
}
const WriteCallback &operator<<(const WriteCallback &cb, const style::KeepMode &str) {
	switch (str) {
	case style::KeepMode::Auto: cb("auto"); break;
	case style::KeepMode::Always: cb("always"); break;
	}
	return cb;
}
const WriteCallback &operator<<(const WriteCallback &cb, const style::TextAlign &str) {
	switch (str) {
	case style::TextAlign::Start: cb("start"); break;
	case style::TextAlign::End: cb("end"); break;
	case style::TextAlign::Left: cb("left"); break;
	case style::TextAlign::Center: cb("center"); break;
	case style::TextAlign::Right: cb("right"); break;
	case style::TextAlign::Justify: cb("justify"); break;
	}
	return cb;
}
const WriteCallback &operator<<(const WriteCallback &cb, const style::TextAlignLast &str) {
	switch (str) {
	case style::TextAlignLast::Start: cb("start"); break;
	case style::TextAlignLast::Center: cb("center"); break;
	case style::TextAlignLast::Justify: cb("justify"); break;
	}
	return cb;
}
const WriteCallback &operator<<(const WriteCallback &cb, const style::LineBreak &str) {
	switch (str) {
	case style::LineBreak::Normal: cb("normal"); break;
	case style::LineBreak::Strict: cb("strict"); break;
	}
	return cb;
}
const WriteCallback &operator<<(const WriteCallback &cb, const style::PunctuationWrap &str) {
	switch (str) {
	case style::PunctuationWrap::Simple: cb("simple"); break;
	case style::PunctuationWrap::Hanging: cb("hanging"); break;
	}
	return cb;
}
const WriteCallback &operator<<(const WriteCallback &cb, const style::VerticalAlign &str) {
	switch (str) {
	case style::VerticalAlign::Top: cb("top"); break;
	case style::VerticalAlign::Middle: cb("middle"); break;
	case style::VerticalAlign::Bottom: cb("bottom"); break;
	case style::VerticalAlign::Auto: cb("auto"); break;
	case style::VerticalAlign::Baseline: cb("baseline"); break;
	case style::VerticalAlign::Automatic: cb("automatic"); break;
	}
	return cb;
}
const WriteCallback &operator<<(const WriteCallback &cb, const style::TableAlign &str) {
	switch (str) {
	case style::TableAlign::Left: cb("left"); break;
	case style::TableAlign::Center: cb("center"); break;
	case style::TableAlign::Right: cb("right"); break;
	case style::TableAlign::Margins: cb("margins"); break;
	}
	return cb;
}
const WriteCallback &operator<<(const WriteCallback &cb, const style::BorderModel &str) {
	switch (str) {
	case style::BorderModel::Collapsing: cb("collapsing"); break;
	case style::BorderModel::Separating: cb("separating"); break;
	}
	return cb;
}
const WriteCallback &operator<<(const WriteCallback &cb, const style::TextAlignSource &str) {
	switch (str) {
	case style::TextAlignSource::Fix: cb("fix"); break;
	case style::TextAlignSource::ValueType: cb("value-type"); break;
	}
	return cb;
}
const WriteCallback &operator<<(const WriteCallback &cb, const style::VerticalPos &str) {
	switch (str) {
	case style::VerticalPos::Top: cb("top"); break;
	case style::VerticalPos::Middle: cb("middle"); break;
	case style::VerticalPos::Bottom: cb("bottom"); break;
	case style::VerticalPos::FromTop: cb("from-top"); break;
	case style::VerticalPos::Below: cb("below"); break;
	}
	return cb;
}
const WriteCallback &operator<<(const WriteCallback &cb, const style::VerticalRel &str) {
	switch (str) {
	case style::VerticalRel::Page: cb("page"); break;
	case style::VerticalRel::PageContent: cb("page-content"); break;
	case style::VerticalRel::Frame: cb("frame"); break;
	case style::VerticalRel::FrameContent: cb("frame-content"); break;
	case style::VerticalRel::Paragraph: cb("paragraph"); break;
	case style::VerticalRel::ParagraphContent: cb("paragraph-content"); break;
	case style::VerticalRel::Char: cb("char"); break;
	case style::VerticalRel::Line: cb("line"); break;
	case style::VerticalRel::Baseline: cb("baseline"); break;
	case style::VerticalRel::Text: cb("text"); break;
	}
	return cb;
}
const WriteCallback &operator<<(const WriteCallback &cb, const style::HorizontalPos &str) {
	switch (str) {
	case style::HorizontalPos::Left: cb("left"); break;
	case style::HorizontalPos::Center: cb("center"); break;
	case style::HorizontalPos::Right: cb("right"); break;
	case style::HorizontalPos::FromLeft: cb("from-left"); break;
	case style::HorizontalPos::Inside: cb("inside"); break;
	case style::HorizontalPos::Outside: cb("outside"); break;
	case style::HorizontalPos::FromInside: cb("from-inside"); break;
	}
	return cb;
}
const WriteCallback &operator<<(const WriteCallback &cb, const style::HorizontalRel &str) {
	switch (str) {
	case style::HorizontalRel::Page: cb("page"); break;
	case style::HorizontalRel::PageContent: cb("page-content"); break;
	case style::HorizontalRel::PageStartMargin: cb("page-start-margin"); break;
	case style::HorizontalRel::PageEndMargin: cb("page-end-margin"); break;
	case style::HorizontalRel::Frame: cb("frame"); break;
	case style::HorizontalRel::FrameContent: cb("frame-content"); break;
	case style::HorizontalRel::FrameStartMargin: cb("frame-start-margin"); break;
	case style::HorizontalRel::FrameEndMargin: cb("frame-end-margin"); break;
	case style::HorizontalRel::Paragraph: cb("paragraph"); break;
	case style::HorizontalRel::ParagraphContent: cb("paragraph-content"); break;
	case style::HorizontalRel::ParagraphStartMargin: cb("paragraph-start-margin"); break;
	case style::HorizontalRel::ParagraphEndMargin: cb("paragraph-end-margin"); break;
	case style::HorizontalRel::Char: cb("char"); break;
	}
	return cb;
}
const WriteCallback &operator<<(const WriteCallback &cb, const style::ListLevelPositionMode &str) {
	switch (str) {
	case style::ListLevelPositionMode::LabelAlignment: cb("label-alignment"); break;
	case style::ListLevelPositionMode::LabelWidthAndPosition: cb("label-width-and-position"); break;
	}
	return cb;
}
const WriteCallback &operator<<(const WriteCallback &cb, const style::LabelFollowedBy &str) {
	switch (str) {
	case style::LabelFollowedBy::Listtab: cb("listtab"); break;
	case style::LabelFollowedBy::Space: cb("nothing"); break;
	case style::LabelFollowedBy::Nothing: cb("space"); break;
	}
	return cb;
}
const WriteCallback &operator<<(const WriteCallback &cb, const style::ColorMode &str) {
	switch (str) {
	case style::ColorMode::Greyscale: cb("greyscale"); break;
	case style::ColorMode::Mono: cb("mono"); break;
	case style::ColorMode::Watermark: cb("watermark"); break;
	case style::ColorMode::Standard: cb("standard"); break;
	}
	return cb;
}
const WriteCallback &operator<<(const WriteCallback &cb, const style::DrawAspect &str) {
	switch (str) {
	case style::DrawAspect::Content: cb("content"); break;
	case style::DrawAspect::Thumbnail: cb("thumbnail"); break;
	case style::DrawAspect::Icon: cb("icon"); break;
	case style::DrawAspect::PrintView: cb("print-view"); break;
	}
	return cb;
}
const WriteCallback &operator<<(const WriteCallback &cb, const style::DrawFill &str) {
	switch (str) {
	case style::DrawFill::None: cb("none"); break;
	case style::DrawFill::Solid: cb("solid"); break;
	case style::DrawFill::Bitmap: cb("bitmap"); break;
	case style::DrawFill::Gradient: cb("gradient"); break;
	case style::DrawFill::Hatch: cb("hatch"); break;
	}
	return cb;
}
const WriteCallback &operator<<(const WriteCallback &cb, const style::FillImageRefPoint &str) {
	switch (str) {
	case style::FillImageRefPoint::Top: cb("top"); break;
	case style::FillImageRefPoint::TopLeft: cb("top-left"); break;
	case style::FillImageRefPoint::TopRight: cb("top-right"); break;
	case style::FillImageRefPoint::Left: cb("left"); break;
	case style::FillImageRefPoint::Center: cb("center"); break;
	case style::FillImageRefPoint::Right: cb("right"); break;
	case style::FillImageRefPoint::Bottom: cb("bottom"); break;
	case style::FillImageRefPoint::BottomLeft: cb("bottom-left"); break;
	case style::FillImageRefPoint::BottomRight: cb("bottom-right"); break;
	}
	return cb;
}
const WriteCallback &operator<<(const WriteCallback &cb, const style::DrawStroke &str) {
	switch (str) {
	case style::DrawStroke::None: cb("none"); break;
	case style::DrawStroke::Dash: cb("dash"); break;
	case style::DrawStroke::Solid: cb("solid"); break;
	}
	return cb;
}
const WriteCallback &operator<<(const WriteCallback &cb, const style::StrokeLinejoin &str) {
	switch (str) {
	case style::StrokeLinejoin::Miter: cb("miter"); break;
	case style::StrokeLinejoin::Round: cb("round"); break;
	case style::StrokeLinejoin::Bevel: cb("bevel"); break;
	case style::StrokeLinejoin::Middle: cb("middle"); break;
	case style::StrokeLinejoin::None: cb("none"); break;
	}
	return cb;
}
const WriteCallback &operator<<(const WriteCallback &cb, const style::StrokeLinecap &str) {
	switch (str) {
	case style::StrokeLinecap::Butt: cb("butt"); break;
	case style::StrokeLinecap::Round: cb("round"); break;
	case style::StrokeLinecap::Square: cb("square"); break;
	}
	return cb;
}
const WriteCallback &operator<<(const WriteCallback &cb, const style::DrawUnit &str) {
	switch (str) {
	case style::DrawUnit::Automatic: cb("automatic"); break;
	case style::DrawUnit::Mm: cb("mm"); break;
	case style::DrawUnit::Cm: cb("cm"); break;
	case style::DrawUnit::M: cb("m"); break;
	case style::DrawUnit::Km: cb("km"); break;
	case style::DrawUnit::Pt: cb("pt"); break;
	case style::DrawUnit::Pc: cb("pc"); break;
	case style::DrawUnit::Inch: cb("inch"); break;
	case style::DrawUnit::Ft: cb("ft"); break;
	case style::DrawUnit::Mi: cb("mi"); break;
	}
	return cb;
}
const WriteCallback &operator<<(const WriteCallback &cb, const style::OverflowBehavior &str) {
	switch (str) {
	case style::OverflowBehavior::Clip: cb("clip"); break;
	case style::OverflowBehavior::AutoCreateNewFrame: cb("auto-create-new-frame"); break;
	}
	return cb;
}
const WriteCallback &operator<<(const WriteCallback &cb, const style::Protect &str) {
	if (str == style::Protect::None) {
		cb("none");
	} else {
		bool empty = true;
		if ((str & style::Protect::Content) != style::Protect::None) {
			if (empty) { cb("content"); } else { cb(" content"); empty = false; }
		}
		if ((str & style::Protect::Position) != style::Protect::None) {
			if (empty) { cb("position"); } else { cb(" position"); empty = false; }
		}
		if ((str & style::Protect::Size) != style::Protect::None) {
			if (empty) { cb("size"); } else { cb(" size"); empty = false; }
		}
	}
	return cb;
}
const WriteCallback &operator<<(const WriteCallback &cb, const style::Mirror &str) {
	switch (str) {
	case style::Mirror::None: cb("none"); break;
	case style::Mirror::Vertical: cb("vertical"); break;
	case style::Mirror::Horizontal: cb("horizontal"); break;
	case style::Mirror::HorizontalOnOdd: cb("horizontal-on-odd"); break;
	case style::Mirror::HHorizontalOnEven: cb("horizontal-on-even"); break;
	case style::Mirror::VerticalHorizontal: cb("vertical horizontal"); break;
	case style::Mirror::VerticalHorizontalOnOdd: cb("vertical horizontal-on-odd"); break;
	case style::Mirror::VerticalHHorizontalOnEven: cb("vertical horizontal-on-even"); break;
	}
	return cb;
}
const WriteCallback &operator<<(const WriteCallback &cb, const style::Repeat &str) {
	switch (str) {
	case style::Repeat::NoRepeat: cb("no-repeat"); break;
	case style::Repeat::Repeat: cb("repeat"); break;
	case style::Repeat::Stretch: cb("stretch"); break;
	}
	return cb;
}
const WriteCallback &operator<<(const WriteCallback &cb, const style::Wrap &str) {
	switch (str) {
	case style::Wrap::None: cb("none"); break;
	case style::Wrap::Biggest: cb("biggest"); break;
	case style::Wrap::Dynamic: cb("dynamic"); break;
	case style::Wrap::Left: cb("left"); break;
	case style::Wrap::Parallel: cb("parallel"); break;
	case style::Wrap::Right: cb("right"); break;
	case style::Wrap::RunThrough: cb("run-through"); break;
	}
	return cb;
}
const WriteCallback &operator<<(const WriteCallback &cb, const style::WrapContourMode &str) {
	switch (str) {
	case style::WrapContourMode::Full: cb("full"); break;
	case style::WrapContourMode::Outside: cb("outside"); break;
	}
	return cb;
}
const WriteCallback &operator<<(const WriteCallback &cb, const style::FillRule &str) {
	switch (str) {
	case style::FillRule::Evenodd: cb("evenodd"); break;
	case style::FillRule::Nonzero: cb("nonzero"); break;
	}
	return cb;
}
const WriteCallback &operator<<(const WriteCallback &cb, const style::AnchorType &str) {
	switch (str) {
	case style::AnchorType::Page: cb("page"); break;
	case style::AnchorType::Frame: cb("frame"); break;
	case style::AnchorType::Paragraph: cb("paragraph"); break;
	case style::AnchorType::Char: cb("char"); break;
	case style::AnchorType::AsChar: cb("as-char"); break;
	}
	return cb;
}
const WriteCallback &operator<<(const WriteCallback &cb, const style::RunThrough &str) {
	switch (str) {
	case style::RunThrough::Foreground: cb("foreground"); break;
	case style::RunThrough::Background: cb("background"); break;
	}
	return cb;
}
const WriteCallback &operator<<(const WriteCallback &cb, const style::FootnotesPosition &str) {
	switch (str) {
	case style::FootnotesPosition::Document: cb("document"); break;
	case style::FootnotesPosition::Page: cb("page"); break;
	case style::FootnotesPosition::Section: cb("section"); break;
	case style::FootnotesPosition::Text: cb("text"); break;
	}
	return cb;
}
const WriteCallback &operator<<(const WriteCallback &cb, const style::StartNumberingAt &str) {
	switch (str) {
	case style::StartNumberingAt::Chapter: cb("chapter"); break;
	case style::StartNumberingAt::Document: cb("document"); break;
	case style::StartNumberingAt::Page: cb("page"); break;
	}
	return cb;
}
const WriteCallback &operator<<(const WriteCallback &cb, const style::Style::Family &str) {
	switch (str) {
	case style::Style::Family::Chart: cb("chart"); break;
	case style::Style::Family::DrawingPage: cb("drawing-page"); break;
	case style::Style::Family::Graphic: cb("graphic"); break;
	case style::Style::Family::Paragraph: cb("paragraph"); break;
	case style::Style::Family::Presentation: cb("presentation"); break;
	case style::Style::Family::Ruby: cb("ruby"); break;
	case style::Style::Family::Table: cb("table"); break;
	case style::Style::Family::TableCell: cb("table-cell"); break;
	case style::Style::Family::TableColumn: cb("table-column"); break;
	case style::Style::Family::TableRow: cb("table-row"); break;
	case style::Style::Family::Text: cb("text"); break;
	case style::Style::Family::PageLayout: break;
	case style::Style::Family::List: break;
	case style::Style::Family::Note: break;
	case style::Style::Family::Undefined: break;
	}
	return cb;
}
const WriteCallback &operator<<(const WriteCallback &cb, const float &str) {
	char b[20] = { 0 };
	auto ret = snprintf(b, 19, "%.9g", str);
	cb(StringView(&b[0], ret));
	return cb;
}
const WriteCallback &operator<<(const WriteCallback &cb, const uint32_t &str) {
	char b[20] = { 0 };
	auto ret = snprintf(b, 19, "%d", str);
	cb(StringView(&b[0], ret));
	return cb;
}
const WriteCallback &operator<<(const WriteCallback &cb, const int32_t &str) {
	char b[20] = { 0 };
	auto ret = snprintf(b, 19, "%d", str);
	cb(StringView(&b[0], ret));
	return cb;
}
const WriteCallback &operator<<(const WriteCallback &cb, const bool &str) {
	if (str) { cb("true"); } else { cb("false"); }
	return cb;
}
const WriteCallback &operator<<(const WriteCallback &cb, const style::Metric &str) {
	switch (str.metric) {
	case style::Metric::Units::None: cb("none"); break;
	case style::Metric::Units::Percent: cb << PercentValue(str.floatValue); break;
	case style::Metric::Units::Float: cb << str.floatValue; break;
	case style::Metric::Units::Integer: cb << str.signedValue; break;
	case style::Metric::Units::Custom:
		switch (str.customValue) {
		case style::Metric::Auto: cb("auto"); break;
		case style::Metric::Normal: cb("normal"); break;
		case style::Metric::Bold: cb("bold"); break;
		case style::Metric::Thin: cb("thin"); break;
		case style::Metric::Medium: cb("medium"); break;
		case style::Metric::Thick: cb("thick"); break;
		case style::Metric::Scale: cb("scale"); break;
		case style::Metric::ScaleMin: cb("scale-min"); break;
		}
		break;
	case style::Metric::Units::Cm: cb << str.floatValue << "cm"; break;
	case style::Metric::Units::Mm: cb << str.floatValue << "mm"; break;
	case style::Metric::Units::In: cb << str.floatValue << "in"; break;
	case style::Metric::Units::Pt: cb << str.floatValue << "pt"; break;
	case style::Metric::Units::Pc: cb << str.floatValue << "pc"; break;
	case style::Metric::Units::Px: cb << str.floatValue << "px"; break;
	case style::Metric::Units::Em: cb << str.floatValue << "em"; break;
	}
	return cb;
}
const WriteCallback &operator<<(const WriteCallback &cb, const style::TextPosition &str) {
	if (str.align == style::TextPositionAlign::Baseline) {
		cb << PercentValue(str.value);
	} else {
		cb << ((str.align == style::TextPositionAlign::Sub) ? StringView("sub") : StringView("super")) << " " << PercentValue(str.value);
	}
	return cb;
}
const WriteCallback &operator<<(const WriteCallback &cb, const style::Border &str) {
	cb << str.width << " " << str.style << " " << str.color;
	return cb;
}

}
