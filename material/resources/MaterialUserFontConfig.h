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

#ifndef MATERIAL_RESOURCES_MATERIALUSERFONTCONFIG_H_
#define MATERIAL_RESOURCES_MATERIALUSERFONTCONFIG_H_

#include "Material.h"
#include "SPFont.h"

NS_MD_BEGIN

class UserFontConfig : public font::Controller {
public:
	using Source = font::Source;
	using FontParameters = font::FontParameters;
	using FontFace = font::FontFace;

	bool init(FontFaceMap && map, float scale);

	Source * getSafeSource() const;

protected:
	virtual void onSourceUpdated(Source *) override;

	Rc<Source> _threadSource;
};

NS_MD_END

#endif /* MATERIAL_RESOURCES_MATERIALUSERFONTCONFIG_H_ */
