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

#ifndef STAPPLER_SRC_NODES_DRAW_SPDRAWCACHE_H_
#define STAPPLER_SRC_NODES_DRAW_SPDRAWCACHE_H_

#include "SPDrawCanvas.h"
#include "SLImage.h"

NS_SP_EXT_BEGIN(draw)

class Cache : public Ref {
public:
	static Cache *getInstance();

	Rc<layout::Image> addImage(const StringView &path, bool replace = false);
	Rc<layout::Image> addImage(const StringView &key, const Bytes &data, bool replace = false);
	Rc<layout::Image> addImage(const StringView &key, layout::Image *);

	Rc<layout::Image> getImage(const StringView &key);
	bool hasImage(const StringView &key);

protected:
	Mutex _mutex;
	Map<String, Rc<layout::Image>> _images;
};

NS_SP_EXT_END(draw)

#endif /* STAPPLER_SRC_NODES_DRAW_SPDRAWCACHE_H_ */
