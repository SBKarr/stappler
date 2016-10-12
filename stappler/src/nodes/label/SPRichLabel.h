/*
 * SPRichLabel.h
 *
 *  Created on: 08 мая 2015 г.
 *      Author: sbkarr
 */

#ifndef LIBS_STAPPLER_NODES_LABEL_SPRICHLABEL_H_
#define LIBS_STAPPLER_NODES_LABEL_SPRICHLABEL_H_

#include "SPRichTextStyle.h"
#include "SPRichTextFormatter.h"
#include "SPDynamicBatchNode.h"
#include "SPFont.h"
#include "base/ccTypes.h"

NS_SP_BEGIN

class RichLabel : public DynamicBatchNode, public cocos2d::LabelProtocol {
public:
	struct Style {
		using TextTransform = rich_text::style::TextTransform;
		using TextDecoration = rich_text::style::TextDecoration;
		using Hyphens = rich_text::style::Hyphens;
		using VerticalAlign = rich_text::style::VerticalAlign;

		enum class Name {
			TextTransform,
			TextDecoration,
			Hyphens,
			VerticalAlign,
			Color,
			Opacity,
			Font
		};

		union Value {
			TextTransform textTransform;
			TextDecoration textDecoration;
			Hyphens hyphens;
			VerticalAlign verticalAlign;
			cocos2d::Color3B color;
			uint8_t opacity;
			Font *font;

			Value();
		};

		struct Param {
			Name name;
			Value value;

			Param(const TextTransform &val) : name(Name::TextTransform) { value.textTransform = val; }
			Param(const TextDecoration &val) : name(Name::TextDecoration) { value.textDecoration = val; }
			Param(const Hyphens &val) : name(Name::Hyphens) { value.hyphens = val; }
			Param(const VerticalAlign &val) : name(Name::VerticalAlign) { value.verticalAlign = val; }
			Param(const cocos2d::Color3B &val) : name(Name::Color) { value.color = val; }
			Param(const uint8_t &val) : name(Name::Opacity) { value.opacity = val; }
			Param(Font * const &val) : name(Name::Font) { value.font = val; }
		};

		template <class T> static Style create(const T & value) { Style s; s.set(value); return s; }
		template <class T> Style & set(const T &value) { set(Param(value), true); return *this; }

		void set(const Param &, bool force = false);
		void merge(const Style &);
		void clear();

		std::vector<Param> params;
	};

	struct StyleSpec {
		size_t start = 0;
		size_t length = 0;
		Style style;

		StyleSpec(size_t s, size_t l, Style &&style)
		: start(s), length(l), style(std::move(style)) { }

		StyleSpec(size_t s, size_t l, const Style &style)
		: start(s), length(l), style(style) { }
	};

	using Alignment = rich_text::style::TextAlign;

	using CharSpec = Font::CharSpec;
	using LineSpec = rich_text::Formatter::LineSpec;

	using CharVec = std::vector< CharSpec >;
	using LineVec = std::vector< LineSpec >;
	using StyleVec = std::vector< StyleSpec >;
	using PosVec = std::vector< size_t >;
	using Formatter = rich_text::Formatter;

	 // symbol is a drawable label unit, SymbolIndex in a number of this unit in CharVec
	using SymbolIndex = ValueWrapper<size_t, class SymbolIndexTag>;

	// char is a memory string unit, CharIndex is a number of single char16_t in string16
	using CharIndex = ValueWrapper<size_t, class CharIndexTag>;

public: /* constructors/destructors, basic functions */

	static rich_text::TextStyle getDefaultStyle();

	static WideString getLocalizedString(const String &);
	static WideString getLocalizedString(const WideString &);

	static cocos2d::Size getLabelSize(Font *, const String &, float w = 0.0f, Alignment = Alignment::Left,
			float density = 0.0f, const rich_text::TextStyle & = getDefaultStyle(), bool localized = false);
	static cocos2d::Size getLabelSize(Font *, const WideString &, float w = 0.0f, Alignment = Alignment::Left,
			float density = 0.0f, const rich_text::TextStyle & = getDefaultStyle(), bool localized = false);

	static float getStringWidth(Font *, const String &, float density = 0.0f, bool localized = false);
	static float getStringWidth(Font *, const WideString &, float density = 0.0f, bool localized = false);

    virtual ~RichLabel();

    virtual bool init(Font *font, const std::string & = "", float = 0.0f, Alignment = Alignment::Left, float = 0.0f);
    virtual void updateLabel();

	virtual void visit(cocos2d::Renderer *r, const cocos2d::Mat4& t, uint32_t f, ZPath &zPath) override;
	virtual bool isLabelDirty() const;

public: /* string utils */

    virtual void setString(const std::string &newString) override;
    virtual void setString(const std::u16string &newString);
    virtual const std::string &getString(void) const override;
    virtual const std::u16string &getString16(void) const;

	virtual void erase16(size_t start = 0, size_t len = std::u16string::npos);
	virtual void erase8(size_t start = 0, size_t len = std::string::npos);

	virtual void append(const std::string& value);
	virtual void append(const std::u16string& value);

	virtual void prepend(const std::string& value);
	virtual void prepend(const std::u16string& value);

public: /* styling parameters */

	virtual void setDensity(float value) override;

    virtual void setAlignment(Alignment alignment);
	virtual Alignment getAlignment() const;

    virtual void setWidth(float width);
	virtual float getWidth() const;

    virtual void setTextIndent(float value);
	virtual float getTextIndent() const;

	virtual void setFont(Font *font);
	virtual Font *getFont() const;

	virtual void setTextTransform(const Style::TextTransform &);
	virtual Style::TextTransform getTextTransform() const;

	virtual void setTextDecoration(const Style::TextDecoration &);
	virtual Style::TextDecoration getTextDecoration() const;

	virtual void setHyphens(const Style::Hyphens &);
	virtual Style::Hyphens getHyphens() const;

	virtual void setVerticalAlign(const Style::VerticalAlign &);
	virtual Style::VerticalAlign getVerticalAlign() const;

	virtual void setLineHeightAbsolute(float value);
	virtual void setLineHeightRelative(float value);
	virtual float getLineHeight() const;
	virtual bool isLineHeightAbsolute() const;

	virtual void setMaxWidth(float);
	virtual float getMaxWidth() const;

	virtual void setMaxLines(size_t);
	virtual size_t getMaxLines() const;

	virtual void setMaxChars(size_t);
	virtual size_t getMaxChars() const;

	virtual void setOpticalAlignment(bool value);
	virtual bool isOpticallyAligned() const;

	virtual void setFillerChar(char16_t);
	virtual char16_t getFillerChar() const;

	virtual void setLocaleEnabled(bool);
	virtual bool isLocaleEnabled() const;

public: /* ranged attributes */

	virtual void setTextRangeStyle(size_t start, size_t length, Style &&);

	virtual void appendTextWithStyle(const std::string &, Style &&);
	virtual void appendTextWithStyle(const std::u16string &, Style &&);

	virtual void prependTextWithStyle(const std::string &, Style &&);
	virtual void prependTextWithStyle(const std::u16string &, Style &&);

	virtual void clearStyles();

	virtual const StyleVec &getStyles() const;
	virtual const StyleVec &getCompiledStyles() const;

	virtual void setStyles(StyleVec &&);
	virtual void setStyles(const StyleVec &);


public: /* characters positioning utils */

	virtual size_t getLinesCount() { return _lines.size(); }
	virtual const LineSpec & getLine(uint32_t num) { return _lines[num]; }

	virtual bool empty() const { return _string16.empty(); }

	virtual SymbolIndex getSymbolIndex(CharIndex);
	virtual CharIndex getCharIndex(SymbolIndex);

	virtual SymbolIndex getSymbolCount();
	virtual CharIndex getCharCount();

	virtual Vec2 getCursorPosition(CharIndex);
	virtual Vec2 getCursorPosition(SymbolIndex);

	virtual Vec2 getSymbolOrigin(SymbolIndex);
	virtual Vec2 getSymbolAdvance(SymbolIndex);

protected:
	virtual void updateColor() override;
	virtual StyleVec compileStyle() const;

	virtual cocos2d::GLProgramState *getProgramStateA8() const override;

	virtual bool hasLocaleTags(const char16_t *, size_t) const;
	virtual std::u16string resolveLocaleTags(const char16_t *, size_t) const;

protected:
    std::u16string _string16;
    std::string _string8;

    float _width = 0;
    float _textIndent = 0;
	Alignment _alignment = Alignment::Left;

	bool _localeEnabled = false;
	bool _labelDirty = true;
	bool stylesDirty = true;

    Rc<FontSet> _fontSet = nullptr;

	Style::TextTransform _textTransform = Style::TextTransform::None;
	Style::TextDecoration _textDecoration = Style::TextDecoration::None;
	Style::Hyphens _hyphens = Style::Hyphens::Manual;
	Style::VerticalAlign _verticalAlign = Style::VerticalAlign::Baseline;
    Rc<Font> _baseFont = nullptr;

    bool _isLineHeightAbsolute = false;
    float _lineHeight = 0;

	CharVec _chars;
	LineVec _lines;
	PosVec _positions;  // vec[symbolIndex] = char_index
	PosVec _reversePositions; // vec[char_index] = symbol_index
	StyleVec _styles;
	StyleVec _compiledStyles;

	uint16_t _charsWidth;
	uint16_t _charsHeight;

	float _maxWidth = 0.0f;
	size_t _maxLines = 0;
	size_t _maxChars = 0;

	bool _opticalAlignment = false;
	char16_t _fillerChar = u'…';
};

NS_SP_END

#endif /* LIBS_STAPPLER_NODES_LABEL_SPRICHLABEL_H_ */
