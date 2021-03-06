/**
Copyright (c) 2016-2017 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef LAYOUT_RENDERER_SLRESULT_H_
#define LAYOUT_RENDERER_SLRESULT_H_

#include "SLRendererTypes.h"
#include "SLLayout.h"

NS_LAYOUT_BEGIN

class Result : public Ref {
public:
	struct PageData {
		Margin margin;
		Rect viewRect; // rectangle in scroll view
		Rect texRect; // rectangle in prepared layout
		size_t num;
		bool isSplit;
	};

	struct BoundIndex {
		size_t idx = maxOf<size_t>();
		size_t level = 0;
		float start;
		float end;
		int64_t page;
		String label;
		String href;
	};

	bool init(const MediaParameters &, FontSource *cfg, Document *doc);

	FontSource *getFontSet() const;
	const MediaParameters &getMedia() const;
	Document *getDocument() const;

	// void pushObject(Object &&);
	void pushIndex(StringView, const Vec2 &);
	void finalize();

	void setBackgroundColor(const Color4B &c);
	const Color4B & getBackgroundColor() const;

	void setContentSize(const Size &);
	const Size &getContentSize() const;

	const Size &getSurfaceSize() const;

	const Vector<Object *> & getObjects() const;
	const Vector<Link *> & getRefs() const;
	const Vector<BoundIndex> & getBounds() const;
	const Map<String, Vec2> & getIndex() const;
	size_t getNumPages() const;

	PageData getPageData(size_t idx, float offset) const;

	BoundIndex getBoundsForPosition(float) const;

	Background *emplaceBackground(const Layout &, const Rect &, const BackgroundStyle &);
	PathObject *emplaceOutline(const Layout &, const Rect &, const Color4B &, float = 0.0f, style::BorderStyle = style::BorderStyle::None);
	void emplaceBorder(Layout &, const Rect &, const OutlineStyle &, float width);
	PathObject *emplacePath(const Layout &);
	Label *emplaceLabel(const Layout &, bool isBullet = false);
	Link *emplaceLink(const Layout &, const Rect &, StringView, StringView);
	//Outline *emplaceBorder(const Rect &, const Outline::Params &);

	//size_t getSizeInMemory() const;

	const Object *getObject(size_t size) const;
	const Label *getLabelByHash(StringView, size_t idx) const;

	const Map<CssStringId, String> &getStrings() const;

	StringView addString(CssStringId, StringView);
	StringView addString(StringView);

protected:
	void processContents(const Document::ContentRecord & rec, size_t level);

	MemoryStorage<Label, 16_KiB> _labels;
	MemoryStorage<Background, 8_KiB> _backgrounds;
	MemoryStorage<Link, 4_KiB> _links;
	MemoryStorage<PathObject, 4_KiB> _paths;

	MediaParameters _media;
	Size _size;

	Vector<Object *> _objects;
	Vector<Link *> _refs;
	Vector<BoundIndex> _bounds;
	Map<String, Vec2> _index;

	Color4B _background;

	Rc<FontSource> _fontSet;
	Rc<Document> _document;

	size_t _numPages = 1;

	Map<CssStringId, String> _strings;
};

NS_LAYOUT_END

#endif /* LAYOUT_RENDERER_SLRESULT_H_ */
