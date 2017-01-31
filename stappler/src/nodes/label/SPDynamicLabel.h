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

#ifndef LIBS_STAPPLER_NODES_LABEL_SPDYNAMICLABEL_H_
#define LIBS_STAPPLER_NODES_LABEL_SPDYNAMICLABEL_H_

#include "SPLayeredBatchNode.h"
#include "SPLabelParameters.h"
#include "SPFontFormatter.h"

NS_SP_BEGIN

class DynamicLabel : public LayeredBatchNode, public LabelParameters {
public:
	using FormatSpec = font::FormatSpec;
	using LineSpec = font::LineSpec;

	using TextureVec = Vector<Rc<cocos2d::Texture2D>>;
	using QuadVec = Vector<Rc<DynamicQuadArray>>;
	using ColorMapVec = Vector<Vector<bool>>;

	static void makeLabelQuads(Source *, const FormatSpec *, const Function<void(const TextureVec &newTex, QuadVec &&newQuads, ColorMapVec &&cMap)> &);
	static void makeLabelRects(Source *, const FormatSpec *, float scale, const Function<void(const Vector<Rect> &)> &);

    virtual ~DynamicLabel();

    virtual bool init(Source *, const DescriptionStyle &, const String & = "", float w = 0.0f, Alignment = Alignment::Left, float d = 0.0f);
    virtual void tryUpdateLabel(bool force = false);

    virtual void setStyle(const DescriptionStyle &);
    virtual const DescriptionStyle &getStyle() const;

    virtual void setSource(Source *);
    virtual Source *getSource() const;

	virtual void visit(cocos2d::Renderer *r, const Mat4& t, uint32_t f, ZPath &zPath) override;

	virtual void setDensity(float value) override;

	virtual size_t getCharsCount() const;
	virtual size_t getLinesCount() const;
	virtual LineSpec getLine(uint32_t num) const;

	virtual uint16_t getFontHeight() const;

	virtual Vec2 getCursorPosition(uint32_t charIndex, bool prefix = true) const;
	virtual Vec2 getCursorOrigin() const;

	// returns character index in FormatSpec for position in label or maxOf<uint32_t>()
	// pair.second - true if index match suffix or false if index match prefix
	// use convertToNodeSpace to get position
	virtual Pair<uint32_t, bool> getCharIndex(const Vec2 &) const;

protected:
    virtual void updateLabel();
	virtual void updateQuads(uint32_t f);
	virtual void onTextureUpdated();
	virtual void onLayoutUpdated();
	virtual void updateColor() override;
	virtual void updateColorQuads();
	virtual cocos2d::GLProgramState *getProgramStateA8() const override;

	virtual void updateQuadsBackground(Source *, FormatSpec *);
	virtual void updateQuadsForeground(Source *, const FormatSpec *);
	virtual void onQuads(const Time &, const Vector<Rc<cocos2d::Texture2D>> &newTex,
			Vector<Rc<DynamicQuadArray>> &&newQuads, Vector<Vector<bool>> &&cMap);

protected:
	EventListener *_listener = nullptr;
	Time _quadRequestTime;
	Rc<Source> _source;
	Rc<FormatSpec> _format;
	Vector<Vector<bool>> _colorMap;
	bool _formatDirty = true;
	bool _colorDirty = false;
	bool _inUpdate = false;

	size_t _updateCount = 0;
};

NS_SP_END

#endif /* LIBS_STAPPLER_NODES_LABEL_SPDYNAMICLABEL_H_ */
