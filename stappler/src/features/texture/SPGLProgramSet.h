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

#ifndef LIBS_STAPPLER_FEATURES_DYNAMIC_ATLAS_SPGLPROGRAMSET_H_
#define LIBS_STAPPLER_FEATURES_DYNAMIC_ATLAS_SPGLPROGRAMSET_H_

#include "SPEventHandler.h"
#include "renderer/CCGLProgram.h"
#include "renderer/CCTexture2D.h"

NS_SP_BEGIN

class GLProgramDesc {
public:
	using PixelFormat = cocos2d::Texture2D::PixelFormat;

	enum class Default : uint32_t {
		DrawNodeA8,
		DynamicBatchI8,
		DynamicBatchR8ToA8,
		DynamicBatchAI88,
		DynamicBatchA8Highp, // for font on high-density screens
		DynamicBatchColor,

		RawTexture,
		RawTextureColor,
		RawTextureColorA8,
		RawTextureColorA8Highp,

		RawRect,
		RawRectBorder,
		RawRects,
		RawAAMaskR,
		RawAAMaskRGBA,
	};

	enum class Attr : uint16_t {
		None = 0,

		Color = 1 << 0,
		TexCoords = 1 << 1,

		HighP = 1 << 2,
		MediumP = 1 << 3,

		MatrixP = 1 << 4,
		MatrixMVP = 1 << 5,

		CustomRect = 1 << 6,
		CustomBorder = 1 << 7,
		CustomFill = 1 << 8,
		AmbientColor = 1 << 9,
		AlphaTestLT = 1 << 10,
		AlphaTestGT = 1 << 11
	};

	GLProgramDesc(Default);
	GLProgramDesc(Attr, PixelFormat internal = PixelFormat::AUTO, PixelFormat reference = PixelFormat::AUTO);

	int32_t hash() const;

	String makeVertex() const;
	String makeFragment() const;

	GLProgramDesc() = default;
	GLProgramDesc(const GLProgramDesc &) = default;
	GLProgramDesc(GLProgramDesc &&) = default;
	GLProgramDesc &operator=(const GLProgramDesc &) = default;
	GLProgramDesc &operator=(GLProgramDesc &&) = default;

	bool operator < (const GLProgramDesc &) const;
	bool operator > (const GLProgramDesc &) const;
	bool operator == (const GLProgramDesc &) const;
	bool operator != (const GLProgramDesc &) const;

private:
	enum class ColorHash : uint16_t {
		None = 0,
		TextureA8_Direct,
		TextureI8_Direct,
		Texture_Direct,
		TextureA8_Ref,
		TextureI8_Ref,
		TextureAI88_Ref,
		Draw_Direct,
		Draw_AI88,
		Draw_I8,
		Draw_A8,
	};

	void set(Attr, PixelFormat internal = PixelFormat::AUTO, PixelFormat reference = PixelFormat::AUTO);

	Attr flags = Attr::None;
	ColorHash color = ColorHash::None;
};

SP_DEFINE_ENUM_AS_MASK(GLProgramDesc::Attr)

class GLProgramSet : public Ref {
public:
	bool init();
	// bool init(uint32_t mask);

	cocos2d::GLProgram *getProgram(const GLProgramDesc &);
	GLProgramDesc getProgramDesc(cocos2d::GLProgram *);

	GLProgramSet();

	void reloadPrograms();

protected:
	Map<GLProgramDesc, Rc<cocos2d::GLProgram>> _programs;
};

NS_SP_END

#endif /* LIBS_STAPPLER_FEATURES_DYNAMIC_ATLAS_SPGLPROGRAMSET_H_ */
