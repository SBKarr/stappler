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

#ifndef CLASSES_MATERIAL_TYPOGRAPHY_MATERIALLABEL_H_
#define CLASSES_MATERIAL_TYPOGRAPHY_MATERIALLABEL_H_

#include "SPDynamicLabel.h"
#include "Material.h"

NS_MD_BEGIN

enum class FontType {
	Headline, // 24sp Regular 87%
	Title, // 20sp Medium 87%
	Subhead, // 16sp Regular 87%
	Body_1, // 14sp Medium 87%
	Body_2, // 14sp Regular 87%
	Caption, // 12sp Regular 54%
	Button, // 14sp Medium CAPS 87%

	Tab_Large,
	Tab_Large_Selected,
	Tab_Small,
	Tab_Small_Selected,

	System_Headline = Headline, // 24sp Regular 87%
	System_Title = Title, // 20sp Medium 87%
	System_Subhead = Subhead, // 16sp Regular 87%
	System_Body_1 = Body_1, // 14sp Medium 87%
	System_Body_2 = Body_2, // 14sp Regular 87%
	System_Caption = Caption, // 12sp Regular 54%
	System_Button = Button, // 14sp Medium CAPS 87%
};

class Label : public DynamicLabel {
public:
	static DescriptionStyle getFontStyle(FontType);
	static DescriptionStyle getFontStyle(const String &);

	static void preloadChars(FontType, const Vector<char16_t> &);
	static void preloadChars(const String &, const Vector<char16_t> &);

	static Size getLabelSize(FontType, const String &, float w = 0.0f, float density = 0.0f, bool localized = false);
	static Size getLabelSize(FontType, const WideString &, float w = 0.0f, float density = 0.0f, bool localized = false);
	static Size getLabelSize(const String &, const String &, float w = 0.0f, float density = 0.0f, bool localized = false);
	static Size getLabelSize(const String &, const WideString &, float w = 0.0f, float density = 0.0f, bool localized = false);
	static Size getLabelSize(const DescriptionStyle &, const WideString &, float w = 0.0f, float density = 0.0f, bool localized = false);

	static float getStringWidth(FontType, const String &, float density = 0.0f, bool localized = false);
	static float getStringWidth(FontType, const WideString &, float density = 0.0f, bool localized = false);
	static float getStringWidth(const String &, const String &, float density = 0.0f, bool localized = false);
	static float getStringWidth(const String &, const WideString &, float density = 0.0f, bool localized = false);
	static float getStringWidth(const DescriptionStyle &, const WideString &, float density = 0.0f, bool localized = false);

	virtual ~Label();

	virtual bool init(FontType, Alignment = Alignment::Left, float w = 0);
	virtual bool init(const String &, Alignment = Alignment::Left, float w = 0);
	virtual bool init(const DescriptionStyle &, Alignment = Alignment::Left, float w = 0);

	virtual void setFont(FontType);
    virtual void setStyle(const DescriptionStyle &) override;

	virtual void onEnter() override;

	virtual void setAutoLightLevel(bool);
	virtual bool isAutoLightLevel() const;

	virtual void setDimColor(const Color &);
	virtual const Color &getDimColor() const;

	virtual void setNormalColor(const Color &);
	virtual const Color &getNormalColor() const;

	virtual void setWashedColor(const Color &);
	virtual const Color &getWashedColor() const;

protected:
	virtual void onUserFont();
	virtual void onLightLevel();

	EventListener *_lightLevelListener = nullptr;

	Color _dimColor = Color::White;
	Color _normalColor = Color::Black;
	Color _washedColor = Color::Black;
};

NS_MD_END

#endif /* CLASSES_MATERIAL_TYPOGRAPHY_MATERIALLABEL_H_ */
