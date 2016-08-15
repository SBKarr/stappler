/*
 * SPGLProgramSet.h
 *
 *  Created on: 22 мая 2015 г.
 *      Author: sbkarr
 */

#ifndef LIBS_STAPPLER_FEATURES_DYNAMIC_ATLAS_SPGLPROGRAMSET_H_
#define LIBS_STAPPLER_FEATURES_DYNAMIC_ATLAS_SPGLPROGRAMSET_H_

#include "SPEventHandler.h"
#include "base/CCMap.h"

NS_SP_BEGIN

class GLProgramSet : public EventHandler {
public:
	enum Program : uint32_t {
		DrawNodeA8,
		
		DynamicBatchI8,
		DynamicBatchAI88,
		
		DynamicBatchA8Highp, // for font on high-density screens
	};

	static GLProgramSet *getInstance();

	cocos2d::GLProgram *getProgram(Program);

protected:
	GLProgramSet();
	void reloadPrograms();

	std::map<Program, cocos2d::GLProgram *> _programs;
};

NS_SP_END

#endif /* LIBS_STAPPLER_FEATURES_DYNAMIC_ATLAS_SPGLPROGRAMSET_H_ */
