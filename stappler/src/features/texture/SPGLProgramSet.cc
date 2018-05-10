// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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

#include "SPDefine.h"
#include "SPGLProgramSet.h"
#include "SPDevice.h"

#include "renderer/CCGLProgram.h"
#include "renderer/CCGLProgramCache.h"
#include "renderer/CCTexture2D.h"

#define STRINGIFY(A)  #A

NS_SP_BEGIN

GLProgramDesc::GLProgramDesc(Default def) {
	switch (def) {
	case Default::DrawNodeA8:
		set(Attr::MatrixP | Attr::Color | Attr::TexCoords, PixelFormat::A8);
		break;
	case Default::DynamicBatchI8:
		set(Attr::MatrixMVP | Attr::Color | Attr::TexCoords, PixelFormat::I8);
		break;
	case Default::DynamicBatchR8ToA8:
		set(Attr::MatrixMVP | Attr::Color | Attr::TexCoords, PixelFormat::R8, PixelFormat::A8);
		break;
	case Default::DynamicBatchAI88:
		set(Attr::MatrixMVP | Attr::Color | Attr::TexCoords, PixelFormat::RGBA8888);
		break;
	case Default::DynamicBatchA8Highp:
		set(Attr::MatrixMVP | Attr::Color | Attr::TexCoords | Attr::HighP, PixelFormat::A8);
		break;
	case Default::DynamicBatchColor:
		set(Attr::MatrixMVP | Attr::Color);
		break;

	case Default::RawTexture:
		set(Attr::MatrixMVP | Attr::TexCoords | Attr::MediumP);
		break;
	case Default::RawTextureColor:
		set(Attr::Color | Attr::TexCoords, PixelFormat::RGBA8888);
		break;
	case Default::RawTextureColorA8:
		set(Attr::Color | Attr::TexCoords, PixelFormat::A8);
		break;
	case Default::RawTextureColorA8Highp:
		set(Attr::Color | Attr::TexCoords | Attr::HighP, PixelFormat::A8);
		break;

	case Default::RawRect:
		set(Attr::MatrixMVP | Attr::CustomRect | Attr::MediumP);
		break;
	case Default::RawRects:
		set(Attr::MatrixMVP | Attr::CustomFill | Attr::MediumP);
		break;

	}
}

GLProgramDesc::GLProgramDesc(Attr flags, PixelFormat internal, PixelFormat reference)
: flags(flags) {
	set(flags, internal, reference);
}

int32_t GLProgramDesc::hash() const {
	return uint16_t(flags) << 16 | uint16_t(color);
}

bool GLProgramDesc::operator < (const GLProgramDesc &desc) const {
	return hash() < desc.hash();
}
bool GLProgramDesc::operator > (const GLProgramDesc &desc) const {
	return hash() > desc.hash();
}
bool GLProgramDesc::operator == (const GLProgramDesc &desc) const {
	return hash() == desc.hash();
}
bool GLProgramDesc::operator != (const GLProgramDesc &desc) const {
	return hash() != desc.hash();
}

void GLProgramDesc::set(Attr attr, PixelFormat internal, PixelFormat reference) {
	flags = attr;
	if (reference == PixelFormat::AUTO) {
		reference = internal;
	}

	bool useColor = ((flags & Attr::Color) != Attr::None);
	bool useTexCoord = ((flags & Attr::TexCoords) != Attr::None);
	bool uniColor = ((flags & Attr::AmbientColor) != Attr::None);

	if ((useColor || uniColor) && useTexCoord) {
		if (internal == reference) {
			switch (internal) {
			case PixelFormat::A8: color = ColorHash::TextureA8_Direct; break;
			case PixelFormat::I8:
			case PixelFormat::R8:
				color = ColorHash::TextureI8_Direct;
				break;
			default:
				color = ColorHash::Texture_Direct;
				break;
			}
		} else {
			switch (reference) {
			case PixelFormat::A8: color = ColorHash::TextureA8_Ref; break;
			case PixelFormat::I8: color = ColorHash::TextureI8_Ref; break;
			case PixelFormat::AI88: color = ColorHash::TextureAI88_Ref; break;
			default: color = ColorHash::Texture_Direct; break;
			}
		}
	} else if (useColor) {
		if (internal == reference) {
			color = ColorHash::Draw_Direct;
		} else {
			switch (internal) {
			case PixelFormat::RGBA8888:
			case PixelFormat::RGBA4444:
			case PixelFormat::RGB888:
			case PixelFormat::RGB565:
			case PixelFormat::RG88:
				switch (reference) {
				case PixelFormat::AI88: color = ColorHash::Draw_AI88; break;
				case PixelFormat::A8: color = ColorHash::Draw_A8; break;
				case PixelFormat::I8: color = ColorHash::Draw_I8; break;
				default: color = ColorHash::Draw_Direct; break;
				}
				break;
			case PixelFormat::R8:
				switch (reference) {
				case PixelFormat::A8: color = ColorHash::Draw_A8; break;
				case PixelFormat::I8: color = ColorHash::Draw_I8; break;
				default: color = ColorHash::Draw_Direct; break;
				}
				break;
			default:
				color = ColorHash::Draw_Direct;
				break;
			}
		}
	}
}

String GLProgramDesc::makeVertex() const {
	bool useColor = ((flags & Attr::Color) != Attr::None);
	bool useTexCoord = ((flags & Attr::TexCoords) != Attr::None);
	bool isHighP = ((flags & Attr::HighP) != Attr::None);

	StringStream stream;
	stream << "attribute vec4 a_position;\n";
	if (useColor) { stream << "attribute vec4 a_color;\n"; }
	if (useTexCoord) { stream << "attribute vec2 a_texCoord;\n"; }

	if (useColor || useTexCoord) {
		stream << "#ifdef GL_ES\n";
		if (useColor) { stream << "varying lowp vec4 v_fragmentColor;\n"; }
		if (useTexCoord) { stream << "varying " << (isHighP ? (const char *)"mediump" : (const char *)"highp") << " vec2 v_texCoord;\n"; }
		stream << "#else\n";
		if (useColor) { stream << "varying vec4 v_fragmentColor;\n"; }
		if (useTexCoord) { stream << "varying vec2 v_texCoord;\n"; }
		stream << "#endif\n";
	}

	stream << "void main() {\n";
	if ((flags & Attr::MatrixP) != Attr::None) {
		stream << "\tgl_Position = CC_PMatrix * a_position;\n";
	} else if ((flags & Attr::MatrixMVP) != Attr::None) {
		stream << "\tgl_Position = CC_MVPMatrix * a_position;\n";
	} else {
		stream << "\tgl_Position = a_position;\n";
	}

	if (useColor) { stream << "\tv_fragmentColor = a_color;\n"; }
	if (useTexCoord) { stream << "\tv_texCoord = a_texCoord;\n"; }

	stream << "}\n";

	return stream.str();
}

String GLProgramDesc::makeFragment() const {
	bool useColor = ((flags & Attr::Color) != Attr::None);
	bool useTexCoord = ((flags & Attr::TexCoords) != Attr::None);
	bool alphaTest = (flags & Attr::AlphaTestLT) != Attr::None || (flags & Attr::AlphaTestGT) != Attr::None;

	StringStream stream;
	stream << "#ifdef GL_ES\n";
	if ((flags & Attr::MediumP) != Attr::None) {
		stream << "precision mediump float;\n";
	} else if ((flags & Attr::HighP) != Attr::None) {
		stream << "precision highp float;\n";
	} else {
		stream << "precision lowp float;\n";
	}
	stream << "#endif\n";

	if ((flags & Attr::CustomRect) != Attr::None) {

		stream << R"Shader(
uniform vec2 u_size;
uniform vec2 u_position;
uniform vec4 CC_AmbientColor;
void main() {
	vec2 position = abs((gl_FragCoord.xy - u_position) - (u_size)) - u_size;
	gl_FragColor = vec4(CC_AmbientColor.xyz, CC_AmbientColor.a * clamp(-max(position.x, position.y), 0.0, 1.0));
})Shader";

	} else if ((flags & Attr::CustomFill) != Attr::None) {

		stream << R"Shader(
uniform vec4 CC_AmbientColor;
void main() {
	gl_FragColor = CC_AmbientColor;
})Shader";

	} else {
		bool uniColor = false;
		if ((flags & Attr::AmbientColor) != Attr::None) {
			stream << "uniform vec4 CC_AmbientColor;\n";
			uniColor = true;
		}
		if (alphaTest) {
			stream << "uniform float u_alphaTest;\n";
		}
		if (useColor) { stream << "varying vec4 v_fragmentColor;\n"; }
		if (useTexCoord) { stream << "varying vec2 v_texCoord;\n"; }

		String colorOutput("gl_FragColor");
		if (alphaTest) {
			colorOutput = "vec4 fc";
		}

		stream << "void main() {\n\t";
		if (useColor || uniColor) {
			if (uniColor && useColor) {
				stream << "vec4 c = v_fragmentColor * CC_AmbientColor;\n\t";
			} else if (uniColor) {
				stream << "vec4 c = CC_AmbientColor;\n\t";
			} else {
				stream << "vec4 c = v_fragmentColor;\n\t";
			}

			switch (color) {
			case ColorHash::TextureA8_Direct:
				stream << colorOutput << " = vec4( c.rgb, c.a * texture2D(CC_Texture0, v_texCoord).a );\n";
				break;
			case ColorHash::TextureI8_Direct:
				stream << colorOutput << " = vec4( c.rgb, c.a * (1.0 - texture2D(CC_Texture0, v_texCoord).r) );\n";
				break;
			case ColorHash::Texture_Direct:
				stream << colorOutput << " = texture2D(CC_Texture0, v_texCoord) * c;\n";
				break;
			case ColorHash::TextureA8_Ref:
				stream << colorOutput << " = vec4( c.rgb, c.a * texture2D(CC_Texture0, v_texCoord).r );\n";
				break;
			case ColorHash::TextureI8_Ref:
				stream << colorOutput << " = vec4( c.rgb, c.a * (1.0 - texture2D(CC_Texture0, v_texCoord).r) );\n";
				break;
			case ColorHash::TextureAI88_Ref:
				stream << "vec4 tex = texture2D(CC_Texture0, v_texCoord);\n\t";
				stream << colorOutput << " = vec4( c.rgb * tex.r, c.a * tex.g );\n";
				break;
			case ColorHash::Draw_Direct:
				stream << colorOutput << " = c;\n";
				break;
			case ColorHash::Draw_AI88:
				stream << colorOutput << " = vec4(c.r, c.a, 0.0, 1.0);\n";
				break;
			case ColorHash::Draw_I8:
				stream << colorOutput << " = vec4(c.r, 0.0, 0.0, 1.0);\n";
				break;
			case ColorHash::Draw_A8:
				stream << colorOutput << " = vec4(c.a, 0.0, 0.0, 1.0);\n";
				break;
			default:
				break;
			}
		} else if (useTexCoord) {
			stream << colorOutput << " = texture2D(CC_Texture0, v_texCoord);\n";
		}

		if (alphaTest) {
			if ((flags & Attr::AlphaTestLT) != Attr::None) {
				stream << "\tif(fc.a < u_alphaTest) { ";
			} else {
				stream << "\tif(fc.a > u_alphaTest) { ";
			}

			stream << "gl_FragColor = fc; } else { discard; }\n";
		}
		stream << "}\n";
	}

	return stream.str();
}

static void GLProgramSet_loadProgram(cocos2d::GLProgram *p, const GLProgramDesc &desc) {
	auto frag = desc.makeFragment();
	auto vert = desc.makeVertex();

	p->initWithByteArrays(vert.data(), frag.data());
	p->link();
	p->updateUniforms();
}

static void GLProgramSet_reloadPrograms(const Map<GLProgramDesc, Rc<cocos2d::GLProgram>> &programs, bool reset) {
	for (auto &it : programs) {
		cocos2d::GLProgram * prog = it.second;
		if (reset) {
			prog->reset();
		}

		GLProgramSet_loadProgram(prog, it.first);
	}
}

bool GLProgramSet::init() {
	return true;
}

cocos2d::GLProgram *GLProgramSet::getProgram(const GLProgramDesc &desc) {
	auto it =_programs.find(desc);
	if (it != _programs.end()) {
		return it->second;
	} else {
		it = _programs.emplace(desc, Rc<cocos2d::GLProgram>::alloc()).first;
		GLProgramSet_loadProgram(it->second, it->first);
		return it->second;
	}
}

GLProgramDesc GLProgramSet::getProgramDesc(cocos2d::GLProgram *prog) {
	for (auto &it : _programs) {
		if (it.second == prog) {
			return it.first;
		}
	}
	return GLProgramDesc();
}

GLProgramSet::GLProgramSet() { }

void GLProgramSet::reloadPrograms() {
	GLProgramSet_reloadPrograms(_programs, true);
}

NS_SP_END
