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

NS_SP_EXT_BEGIN(draw)

class GLCacheNode {
public:
	void cleanup();
	void bindTexture(GLuint);
	void useProgram(GLuint);
	void enableVertexAttribs(uint32_t);
	void blendFunc(GLenum sfactor, GLenum dfactor);
	void blendFunc(const cocos2d::BlendFunc &);

protected:
	uint32_t _attributeFlags = 0;

	GLuint _currentTexture = 0;
	GLuint _currentProgram = 0;

	GLenum _blendingSource;
	GLenum _blendingDest;
};

NS_SP_EXT_END(draw)

#endif /* STAPPLER_SRC_NODES_DRAW_SPDRAWGLCACHENODE_H_ */
