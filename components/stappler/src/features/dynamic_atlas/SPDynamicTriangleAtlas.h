/**
Copyright (c) 2019 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef STAPPLER_SRC_FEATURES_DYNAMIC_ATLAS_SPDYNAMICTRIANGLEATLAS_H_
#define STAPPLER_SRC_FEATURES_DYNAMIC_ATLAS_SPDYNAMICTRIANGLEATLAS_H_

#include "SPDynamicAtlas.h"
#include "SPDynamicTriangleArray.h"

NS_SP_BEGIN

class DynamicTriangleAtlas : public DynamicAtlas {
public:
	using ArraySet = Set<Rc<DynamicTriangleArray>>;

	virtual bool init(cocos2d::Texture2D *texture);
	virtual void clear() override;

	const ArraySet &getSet() const;
	ArraySet &getSet();

	void addArray(DynamicTriangleArray *);
	void removeArray(DynamicTriangleArray *);
	void updateArrays(ArraySet &&);

protected:
	virtual void setDirty() override;
	virtual void setup() override;

	void setupIndices();
	void onCapacityDirty(size_t newSize);
	void onQuadsDirty();

	virtual void visit() override;

	size_t calculateBufferSize() const;
	size_t calculateQuadsCount() const;

	ArraySet _set;
	Vector<GLushort> _indices;
};

NS_SP_END

#endif /* STAPPLER_SRC_FEATURES_DYNAMIC_ATLAS_SPDYNAMICTRIANGLEATLAS_H_ */
