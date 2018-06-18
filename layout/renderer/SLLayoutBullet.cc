/**
Copyright (c) 2018 Roman Katuntsev <sbkarr@stappler.org>

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
#include "SLLayout.h"
#include "SLBuilder.h"

NS_LAYOUT_BEGIN

static WideString Layout_getLatinBullet(int64_t v, bool uppercase) {
	static constexpr int mod = ('z' - 'a') + 1;
	if (v == 0) {
		return WideString(u"0");
	}

	WideString ret;
	bool isNegative = (v < 0);
	v = std::abs(v);
	do {
		if (uppercase) {
			ret = WideString(1, char16_t(u'A' + (v - 1) % mod)) + ret;
		} else {
			ret = WideString(1, char16_t(u'a' + (v - 1) % mod)) + ret;
		}
		v = (v - 1) / mod;
	} while (v > 0);
	if (isNegative) {
		ret = WideString(u"-") + ret;
	}
	return ret;
}
static WideString Layout_getGreekBullet(int64_t v) {
	static constexpr int mod = (u'ω' - u'α') + 1;
	if (v == 0) {
		return WideString(u"0");
	}

	WideString ret;
	bool isNegative = v < 0;
	v = std::abs(v) - 1;
	do {
		ret = WideString(1, char16_t(u'α' + v % mod)) + ret;
		v /= mod;
	} while (v > 0);
	if (isNegative) {
		ret = WideString(u"-") + ret;
	}
	return ret;
}

// original source from: http://rosettacode.org/wiki/Roman_numerals/Encode#C.2B.2B
// data from: https://en.wikipedia.org/wiki/Numerals_in_Unicode#Roman_numerals_in_Unicode
// Code x= 	0 	1 	2 	3 	4 	5 	6 	7 	8 	9 	A 	B 	C 	D 	E 	F
//Value[5] 	1 	2 	3 	4 	5 	6 	7 	8 	9 	10 	11 	12 	50 	100 500 1,000
//U+216x 	Ⅰ 	Ⅱ 	Ⅲ 	Ⅳ 	Ⅴ 	Ⅵ 	Ⅶ 	Ⅷ 	Ⅸ 	Ⅹ 	Ⅺ 	Ⅻ 	Ⅼ 	Ⅽ 	Ⅾ 	Ⅿ
//U+217x 	ⅰ 	ⅱ 	ⅲ 	ⅳ 	ⅴ 	ⅵ 	ⅶ 	ⅷ 	ⅸ 	ⅹ 	ⅺ 	ⅻ 	ⅼ 	ⅽ 	ⅾ 	ⅿ

static WideString Layout_getRomanBullet(int64_t v, bool uppercase) {
	struct romandata_t {
		int value;
		char16_t const* numeral;
	};
	static romandata_t const romandata_upper[] = {
			{ 1000, u"Ⅿ" }, { 900, u"ⅭⅯ" }, { 500, u"Ⅾ" }, { 400, u"ⅭⅮ" }, { 100, u"Ⅽ" }, { 90, u"ⅩⅭ" },
			{ 50, u"Ⅼ" }, { 40, u"ⅩⅬ" }, { 10, u"Ⅹ" }, { 9, u"ⅠⅩ" }, { 5, u"Ⅴ" }, { 4, u"ⅠⅤ" },
			{ 1, u"Ⅰ" }, { 0, nullptr }}; // end marker
	static romandata_t const romandata_lower[] = {
			{ 1000, u"ⅿ" }, { 900, u"ⅽⅿ" }, { 500, u"ⅾ" }, { 400, u"ⅽⅾ" }, { 100, u"ⅽ" }, { 90, u"ⅹⅽ" },
			{ 50, u"ⅼ" }, { 40, u"ⅹⅼ" }, { 10, u"ⅹ" }, { 9, u"ⅰⅹ" }, { 5, u"ⅴ" }, { 4, u"ⅰⅴ" },
			{ 1, u"ⅰ" }, { 0, nullptr }}; // end marker

	if (v == 0) {
		return WideString(u"0");
	}
	WideString result;
	if (v < 0) {
		result += u"-";
		v = std::abs(v);
	}
	if (v <= 12) {
		result += (uppercase?(u'Ⅰ' + (v - 1)):(u'ⅰ' + (v - 1)));
	} else {
		auto romandata = uppercase?romandata_upper:romandata_lower;
		for (romandata_t const* current = romandata; current->value > 0; ++current) {
			while (v >= current->value) {
				result += current->numeral;
				v -= current->value;
			}
		}
	}
	return result;
}

WideString Layout::getListItemBulletString() {
	auto parent = builder->getTopLayout();
	WideString str;

	switch (node.block.listStyleType) {
	case style::ListStyleType::None: break;
	case style::ListStyleType::Circle: str += u"◦ "; break;
	case style::ListStyleType::Disc: str += u"• "; break;
	case style::ListStyleType::Square: str += u"■ "; break;
	case style::ListStyleType::XMdash: str += u"— "; break;
	case style::ListStyleType::Decimal:
		str += string::toUtf16(toString(parent->listItemIndex, ". "));
		parent->listItemIndex += ((parent->listItem==Layout::ListReversed)?-1:1);
		break;
	case style::ListStyleType::DecimalLeadingZero:
		str += string::toUtf16(toString("0", parent->listItemIndex, ". "));
		parent->listItemIndex += ((parent->listItem==Layout::ListReversed)?-1:1);
		break;
	case style::ListStyleType::LowerAlpha:
		str.append(Layout_getLatinBullet(parent->listItemIndex, false)).append(u". ");
		parent->listItemIndex += ((parent->listItem==Layout::ListReversed)?-1:1);
		break;
	case style::ListStyleType::LowerGreek:
		str.append(Layout_getGreekBullet(parent->listItemIndex)).append(u". ");
		parent->listItemIndex += ((parent->listItem==Layout::ListReversed)?-1:1);
		break;
	case style::ListStyleType::LowerRoman:
		str.append(Layout_getRomanBullet(parent->listItemIndex, false)).append(u". ");
		parent->listItemIndex += ((parent->listItem==Layout::ListReversed)?-1:1);
		break;
	case style::ListStyleType::UpperAlpha:
		str.append(Layout_getLatinBullet(parent->listItemIndex, true)).append(u". ");
		parent->listItemIndex += ((parent->listItem==Layout::ListReversed)?-1:1);
		break;
	case style::ListStyleType::UpperRoman:
		str.append(Layout_getRomanBullet(parent->listItemIndex, true)).append(u". ");
		parent->listItemIndex += ((parent->listItem==Layout::ListReversed)?-1:1);
		break;
	}

	return str;
}

void Layout::processListItemBullet(float parentPosY) {
	Layout *parent = builder->getTopLayout();

	if (node.block.display == style::Display::ListItem && node.block.listStylePosition == style::ListStylePosition::Outside
			&& parent && parent->listItem != Layout::ListNone) {
		auto &media = builder->getMedia();
		const auto density = media.density;
		auto origin = Vec2(roundf(pos.position.x * density), roundf((parentPosY) * density));
		FontParameters fStyle = node.style->compileFontStyle(builder);
		if (fStyle.fontFamily == "default" && (toInt(fStyle.listStyleType) < toInt(style::ListStyleType::Decimal))) {
			fStyle.fontFamily = StringView("system");
		}
		ParagraphStyle pStyle = node.style->compileParagraphLayout(builder);
		pStyle.textIndent.value = 0.0f; pStyle.textIndent.metric = style::Metric::Units::Px;
		auto baseFont = builder->getFontSet()->getLayout(fStyle, media.fontScale)->getData();

		Label *label = builder->getResult()->emplaceLabel(*this, true);
		Formatter reader(builder->getFontSet(), &label->format, density);
		reader.setOpticalAlignment(false);
		InlineContext::initFormatter(*this, fStyle, pStyle, parentPosY, reader);

		TextStyle textStyle = node.style->compileTextLayout(builder);
		WideString str = getListItemBulletString();
		reader.read(fStyle, textStyle, str, 0, 0);

		reader.finalize();
		float offset = fixLabelPagination(*label);
		float finalHeight = reader.getHeight() / density + offset;
		float finalWidth = reader.getMaxLineX() / density;

		float x = 0, w = builder->getMedia().surfaceSize.width;
		std::tie(x, w) = builder->getFloatBounds(this, origin.y / density, finalHeight);

		label->bbox = Rect(x - pos.position.x - finalWidth -
				pStyle.listOffset.computeValueAuto(pos.size.width, builder->getMedia(), baseFont->metrics.height / density),
				origin.y / density - pos.position.y, finalWidth, finalHeight);
		objects.emplace_back(label);
	}
}

NS_LAYOUT_END
