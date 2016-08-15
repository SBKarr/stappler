/*
 * SPRichTextRenderer.h
 *
 *  Created on: 23 апр. 2015 г.
 *      Author: sbkarr
 */

#ifndef LIBS_STAPPLER_FEATURES_RICH_TEXT_SPRICHTEXTRENDERER_H_
#define LIBS_STAPPLER_FEATURES_RICH_TEXT_SPRICHTEXTRENDERER_H_

#include "2d/CCComponent.h"
#include "SPRichTextSource.h"
#include "SPRichTextRendererTypes.h"
#include "SPRichTextResult.h"

NS_SP_EXT_BEGIN(rich_text)

class Renderer : public cocos2d::Component {
public:
	using RenderingCallback = Function<void(Result *, bool started)>;
	using Source = rich_text::Source;

	virtual bool init(const Vector<String> &ids = {});
	virtual void onContentSizeDirty() override;
	virtual void onVisit(cocos2d::Renderer *r, const Mat4& t, uint32_t f, const ZPath &zPath) override;

	virtual void setSource(Source *source);
	virtual Source *getSource() const;

	virtual Document *getDocument() const;
	virtual Result *getResult() const;

	virtual MediaResolver getMediaResolver(const Vector<String> & = {}) const;


public: /* media type resolver */
	void setSurfaceSize(const Size &size);

	// size of rendering surface (size for media-queries)
	const Size &getSurfaceSize() const;
	const Margin & getPageMargin() const;
	bool isPageSplitted() const;

	void setDpi(int dpi);
	void setDensity(float density);

	void setMediaType(style::MediaType value);
	void setOrientationValue(style::Orientation value);
	void setPointerValue(style::Pointer value);
	void setHoverValue(style::Hover value);
	void setLightLevelValue(style::LightLevel value);
	void setScriptingValue(style::Scripting value);
	void setHyphens(rich_text::HyphenMap *);

	void setPageMargin(const Margin &);

	void addOption(const String &);
	void removeOption(const String &);
	bool hasOption(const String &) const;

	void addFlag(RenderFlag::Flag flag);
	void removeFlag(RenderFlag::Flag flag);
	bool hasFlag(RenderFlag::Flag flag) const;

public: /* font routines */
	virtual void setRenderingCallback(const RenderingCallback &);
	virtual void onResult(Result *result);

protected:
	virtual bool requestRendering();
	virtual void onSource();
	virtual void pushVersionOptions();

	bool _renderingDirty = false;
	bool _renderingInProgress = false;
	bool _isPageSplitted = false;

	Margin _pageMargin;
	data::Listener<Source> _source;
	Vector<String> _ids;
	Size _surfaceSize;
	MediaParameters _media;
	Rc<Result>_result;
	Rc<rich_text::HyphenMap> _hyphens;
	RenderingCallback _renderingCallback = nullptr;
};

NS_SP_EXT_END(rich_text)

#endif /* LIBS_STAPPLER_FEATURES_RICH_TEXT_SPRICHTEXTRENDERER_H_ */
