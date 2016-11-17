/*
 * SPRichLabel.h
 *
 *  Created on: 08 мая 2015 г.
 *      Author: sbkarr
 */

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
    virtual void updateLabel();

    virtual void setStyle(const DescriptionStyle &);
    virtual const DescriptionStyle &getStyle() const;

    virtual void setSource(Source *);
    virtual Source *getSource() const;

	virtual void visit(cocos2d::Renderer *r, const Mat4& t, uint32_t f, ZPath &zPath) override;

	virtual void setDensity(float value) override;

	virtual size_t getLinesCount() const;
	virtual LineSpec getLine(uint32_t num) const;

	virtual uint16_t getFontHeight() const;

protected:
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
