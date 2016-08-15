/*
 * SPGLProgramSet.cpp
 *
 *  Created on: 22 мая 2015 г.
 *      Author: sbkarr
 */

#include "SPDefine.h"
#include "SPGLProgramSet.h"
#include "SPDevice.h"

#include "renderer/CCGLProgram.h"
#include "renderer/CCGLProgramCache.h"

#define STRINGIFY(A)  #A

NS_SP_BEGIN

namespace shaders {

#include "shaders/DrawNodeA8.frag"
#include "shaders/DrawNodeA8.vert"

#include "shaders/DynamicBatchI8.frag"
#include "shaders/DynamicBatchI8.vert"
	
#include "shaders/DynamicBatchAI88.frag"
#include "shaders/DynamicBatchAI88.vert"
	
#include "shaders/DynamicBatchA8Highp.frag"
#include "shaders/DynamicBatchA8Highp.vert"

}

static GLProgramSet *s_sharedSet = nullptr;

GLProgramSet *GLProgramSet::getInstance() {
	if (!s_sharedSet) {
		s_sharedSet = new GLProgramSet();
	}
	return s_sharedSet;
}

cocos2d::GLProgram *GLProgramSet::getProgram(Program p) {
	auto it =_programs.find(p);
	if (it != _programs.end()) {
		return it->second;
	}
	return nullptr;
}

static void GLProgramSet_loadProgram(cocos2d::GLProgram *p, const char *vert, const char *frag) {
	p->initWithByteArrays(vert, frag);
	p->link();
	p->updateUniforms();
}

void GLProgramSet_reloadPrograms(const std::map<GLProgramSet::Program, cocos2d::GLProgram *> &programs, bool reset) {
	for (auto &it : programs) {
		if (reset) {
			it.second->reset();
		}
		switch(it.first) {
		case GLProgramSet::DrawNodeA8:
			GLProgramSet_loadProgram(it.second, shaders::DrawNodeA8_vert, shaders::DrawNodeA8_frag); break;
		case GLProgramSet::DynamicBatchI8:
			GLProgramSet_loadProgram(it.second, shaders::DynamicBatchI8_vert, shaders::DynamicBatchI8_frag); break;
		case GLProgramSet::DynamicBatchAI88:
			GLProgramSet_loadProgram(it.second, shaders::DynamicBatchAI88_vert, shaders::DynamicBatchAI88_frag); break;
		case GLProgramSet::DynamicBatchA8Highp:
			GLProgramSet_loadProgram(it.second, shaders::DynamicBatchA8Highp_vert, shaders::DynamicBatchA8Highp_frag); break;
		}
	}
}

GLProgramSet::GLProgramSet() {
	onEvent(stappler::Device::onAndroidReset, [this] (const stappler::Event *) {
		reloadPrograms();
	});

    cocos2d::GLProgram *p = nullptr;

	p = new cocos2d::GLProgram();
	_programs.insert(std::make_pair(DrawNodeA8, p));

	p = new cocos2d::GLProgram();
	_programs.insert(std::make_pair(DynamicBatchI8, p));
	
	p = new cocos2d::GLProgram();
	_programs.insert(std::make_pair(DynamicBatchAI88, p));
	
	p = new cocos2d::GLProgram();
	_programs.insert(std::make_pair(DynamicBatchA8Highp, p));

	GLProgramSet_reloadPrograms(_programs, false);
}

void GLProgramSet::reloadPrograms() {
	GLProgramSet_reloadPrograms(_programs, true);
}


NS_SP_END
