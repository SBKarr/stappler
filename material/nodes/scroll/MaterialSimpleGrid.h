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

#ifndef LIBS_MATERIAL_NODES_SCROLL_MATERIALSIMPLEGRID_H_
#define LIBS_MATERIAL_NODES_SCROLL_MATERIALSIMPLEGRID_H_

#include "MaterialScroll.h"

NS_MD_BEGIN

class ScrollHandlerGrid : public Scroll::Handler {
public:
	virtual ~ScrollHandlerGrid() { }
	virtual bool init(Scroll *) override;
	virtual bool init(Scroll *, const Padding &p);
	virtual Scroll::ItemMap run(Request, DataMap &&) override;

	virtual void setCellMinWidth(float v);
	virtual void setCellAspectRatio(float v);

	virtual void setAutoPaddings(bool);
	virtual bool isAutoPaddings() const;

protected:
	virtual Rc<Scroll::Item> onItem(data::Value &&, Scroll::Source::Id);

	bool _autoPaddings = false;
	float _currentWidth = 0.0f;

	float _cellAspectRatio = 1.0f;
	float _cellMinWidth = 1.0f;

	float _widthPadding = 0.0f;
	Padding _padding;
	cocos2d::Size _currentCellSize;
	uint32_t _currentCols = 0;
};

NS_MD_END

#endif /* LIBS_MATERIAL_NODES_SCROLL_MATERIALSIMPLEGRID_H_ */
