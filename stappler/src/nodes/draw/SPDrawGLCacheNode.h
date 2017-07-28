/*
 * SPDrawGLCache.h
 *
 *  Created on: 13 февр. 2017 г.
 *      Author: sbkarr
 */

#ifndef STAPPLER_SRC_NODES_DRAW_SPDRAWGLCACHENODE_H_
#define STAPPLER_SRC_NODES_DRAW_SPDRAWGLCACHENODE_H_

#include "SPDefine.h"
#include "base/ccTypes.h"
#include "platform/CCGL.h"

#ifndef GL_RED_EXT
#define GL_RED_EXT GL_RED
#endif

#ifndef GL_RG_EXT
#define GL_RG_EXT GL_RG
#endif

#ifndef GL_R8_EXT
#define GL_R8_EXT GL_R8
#endif

#ifndef GL_RG8_EXT
#define GL_RG8_EXT GL_RG8
#endif

NS_SP_EXT_BEGIN(draw)

using BlendFunc = cocos2d::BlendFunc;

struct GLBlending {
	enum Equation {
		FuncAdd,
		FuncSubstract,
		FuncReverseSubstract,
		Min,
		Max,
	};

	static GLenum getEnumForEq(Equation eq);
	static Equation getEqForEnum(GLenum eq);

	static GLBlending loadFromGL();

	GLBlending() {
		blendFunc(BlendFunc::DISABLE);
	}

	GLBlending(const BlendFunc &c) {
		blendFunc(c);
	}

	GLBlending(const BlendFunc &c, const Equation &ec) {
		blendFunc(c);
		blendEq(ec);
	}

	GLBlending(const BlendFunc &c, const Equation &ec, const Equation &ea) {
		blendFunc(c);
		blendEq(ec, ea);
	}

	GLBlending(const BlendFunc &c, const BlendFunc &a) {
		blendFunc(c, a);
	}

	GLBlending(const BlendFunc &c, const BlendFunc &a, const Equation &ec) {
		blendFunc(c, a);
		blendEq(ec);
	}

	GLBlending(const BlendFunc &c, const BlendFunc &a, const Equation &ec, const Equation &ea) {
		blendFunc(c, a);
		blendEq(ec, ea);
	}


	void blendFunc(const BlendFunc &c) {
		colorFunc = c;
		separateFunc = false;
	}
	void blendFunc(const BlendFunc &c, const BlendFunc &a) {
		if (c != a) {
			colorFunc = c;
			alphaFunc = a;
			separateFunc = true;
		} else {
			blendFunc(c);
		}
	}

	void blendEq(const Equation &c) {
		colorEq = c;
		separateEq = false;
	}
	void blendEq(const Equation &c, const Equation &a) {
		if (c != a) {
			colorEq = c;
			alphaEq = a;
			separateEq = true;
		} else {
			blendEq(c);
		}
	}

	bool empty() const {
		return !separateFunc && colorFunc.src == GL_ONE && colorFunc.dst == GL_ZERO;
	}

	GLenum colorEqEnum() const {
		return getEnumForEq(colorEq);
	};

	GLenum alphaEqEnum() const {
		return getEnumForEq(alphaEq);
	};

	bool operator==(const GLBlending &a) const {
		return colorFunc == a.colorFunc && alphaFunc == a.alphaFunc
				&& colorEq == a.colorEq && alphaEq == a.alphaEq
				&& separateFunc == a.separateFunc && separateEq == a.separateEq;
	}

	bool operator!=(const GLBlending &a) const {
		return colorFunc != a.colorFunc || alphaFunc != a.alphaFunc
				|| colorEq != a.colorEq || alphaEq != a.alphaEq
				|| separateFunc != a.separateFunc || separateEq != a.separateEq;
	}

	BlendFunc colorFunc;
	BlendFunc alphaFunc;

	Equation colorEq = FuncAdd;
	Equation alphaEq = FuncAdd;

	bool separateFunc = false;
	bool separateEq = false;
};

class GLCacheNode {
public:
	void load();
	void cleanup();
	void bindTexture(GLuint);
	void useProgram(GLuint);
	void enableVertexAttribs(uint32_t);
	void setBlending(const GLBlending &);

	void blendFunc(GLenum sfactor, GLenum dfactor);
	void blendFunc(const BlendFunc &);

protected:
	uint32_t _attributeFlags = 0;

	GLuint _currentTexture = 0;
	GLuint _currentProgram = 0;

	GLBlending _blending;

	GLuint _origTexture = 0;
	GLuint _origProgram = 0;
	GLBlending _origBlending;
};

NS_SP_EXT_END(draw)

#endif /* STAPPLER_SRC_NODES_DRAW_SPDRAWGLCACHENODE_H_ */
