/*
 * SPRichTextRendererResult.h
 *
 *  Created on: 02 мая 2015 г.
 *      Author: sbkarr
 */

#ifndef LIBS_STAPPLER_FEATURES_RICH_TEXT_SPRICHTEXTRENDERERRESULT_H_
#define LIBS_STAPPLER_FEATURES_RICH_TEXT_SPRICHTEXTRENDERERRESULT_H_

#include "SPRichTextRendererTypes.h"
#include "SPRichTextLayout.h"
#include "base/CCVector.h"
#include "SPTextureRef.h"

NS_SP_EXT_BEGIN(rich_text)

class Result : public cocos2d::Ref {
public:
	struct PageData {
		Margin margin;
		Rect viewRect;
		Rect texRect;
		size_t num;
		bool isSplit;
	};

	Result(const MediaParameters &, font::Source *cfg, Document *doc);

	font::Source *getFontSet() const;
	const MediaParameters &getMedia() const;
	Document *getDocument() const;

	void pushObject(Object &&);
	void pushIndex(const String &, const Vec2 &);
	void finalize();

	void setBackgroundColor(const Color4B &c);
	const Color4B & getBackgroundColor() const;

	void setContentSize(const Size &);
	const Size &getContentSize() const;

	const Size &getSurfaceSize() const;

	const Vector<Object> & getObjects() const;
	const Vector<Object> & getRefs() const;
	const Map<String, Vec2> & getIndex() const;
	size_t getNumPages() const;

	PageData getPageData(Renderer *r, size_t idx, float offset) const;

protected:
	MediaParameters _media;
	Size _size;

	Vector<Object> _objects;
	Vector<Object> _refs;
	Map<String, Vec2> _index;

	Color4B _background;

	Rc<font::Source> _fontSet;
	Rc<Document> _document;

	size_t _numPages = 1;

};

NS_SP_EXT_END(rich_text)

#endif /* LIBS_STAPPLER_FEATURES_RICH_TEXT_SPRICHTEXTRENDERERRESULT_H_ */
