/*
 * SPDraw.cpp
 *
 *  Created on: 09 дек. 2014 г.
 *      Author: sbkarr
 */

#include "SPDefine.h"
#include "SPDraw.h"
#include "SPGLProgramSet.h"

#include "renderer/CCGLProgram.h"
#include "renderer/CCGLProgramCache.h"

NS_SP_EXT_BEGIN(draw)

cocos2d::GLProgram *getA8Program() {
	return GLProgramSet::getInstance()->getProgram(GLProgramSet::DrawNodeA8);
}

cocos2d::GLProgram *getRGBA8888Program() {
	return cocos2d::GLProgramCache::getInstance()->getGLProgram(cocos2d::GLProgram::SHADER_NAME_POSITION_TEXTURE_COLOR_NO_MVP);
}

NS_SP_EXT_END(draw)
