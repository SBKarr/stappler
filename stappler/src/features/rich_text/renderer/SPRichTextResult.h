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

#ifndef LIBS_STAPPLER_FEATURES_RICH_TEXT_SPRICHTEXTRENDERERRESULT_H_
#define LIBS_STAPPLER_FEATURES_RICH_TEXT_SPRICHTEXTRENDERERRESULT_H_

#include "SPRichTextRendererTypes.h"
#include "SPRichTextLayout.h"
#include "base/CCVector.h"

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

	struct BoundIndex {
		size_t idx = maxOf<int64_t>();
		float start;
		float end;
		int64_t page;
		String label;
		String href;
	};

	bool init(const MediaParameters &, font::Source *cfg, Document *doc);

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
	const Vector<BoundIndex> & getBounds() const;
	const Map<String, Vec2> & getIndex() const;
	size_t getNumPages() const;

	PageData getPageData(size_t idx, float offset) const;

	BoundIndex getBoundsForPosition(float) const;

	size_t getSizeInMemory() const;

protected:
	void processContents(const Document::ContentRecord & rec);

	MediaParameters _media;
	Size _size;

	Vector<Object> _objects;
	Vector<Object> _refs;
	Vector<BoundIndex> _bounds;
	Map<String, Vec2> _index;

	Color4B _background;

	Rc<font::Source> _fontSet;
	Rc<Document> _document;

	size_t _numPages = 1;
};

NS_SP_EXT_END(rich_text)

#endif /* LIBS_STAPPLER_FEATURES_RICH_TEXT_SPRICHTEXTRENDERERRESULT_H_ */
