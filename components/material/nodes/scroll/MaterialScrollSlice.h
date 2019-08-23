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

#ifndef LIBS_MATERIAL_NODES_SCROLL_MATERIALSCROLLSLICE_H_
#define LIBS_MATERIAL_NODES_SCROLL_MATERIALSCROLLSLICE_H_

#include "MaterialScroll.h"
#include "base/CCMap.h"

NS_MD_BEGIN

class ScrollHandlerSlice : public Scroll::Handler {
public:
	using DataCallback = Function<Rc<Scroll::Item> (ScrollHandlerSlice *h, data::Value &&, const Vec2 &)>;

	virtual ~ScrollHandlerSlice() { }
	virtual bool init(Scroll *, const DataCallback & = nullptr);
	virtual void setDataCallback(const DataCallback &);

	virtual Scroll::ItemMap run(Request, DataMap &&) override;

protected:
	virtual cocos2d::Vec2 getOrigin(Request) const;
	virtual Rc<Scroll::Item> onItem(data::Value &&, const Vec2 &);

	Vec2 _originFront;
	Vec2 _originBack;
	DataCallback _dataCallback;
};

NS_MD_END

#endif /* LIBS_MATERIAL_NODES_SCROLL_MATERIALSCROLLSLICE_H_ */
