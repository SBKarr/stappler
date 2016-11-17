/*
 * SPGLProgramSet.h
 *
 *  Created on: 22 мая 2015 г.
 *      Author: sbkarr
 */

#ifndef LIBS_STAPPLER_FEATURES_DYNAMIC_ATLAS_SPGLPROGRAMSET_H_
#define LIBS_STAPPLER_FEATURES_DYNAMIC_ATLAS_SPGLPROGRAMSET_H_

#include "SPEventHandler.h"
#include "renderer/CCGLProgram.h"

NS_SP_BEGIN

class GLProgramSet : public Ref {
public:
	enum Program : uint32_t {
		DrawNodeA8 = 1 << 0,
		DynamicBatchI8 = 1 << 1,
		DynamicBatchAI88 = 1 << 2,
		DynamicBatchA8Highp = 1 << 3, // for font on high-density screens

		RawTexture = 1 << 4,
		RawTextureColor = 1 << 5,
		RawTextureColorA8 = 1 << 6,
		RawTextureColorA8Highp = 1 << 7,

		RawRect = 1 << 8,
		RawRectBorder = 1 << 9,
		RawRects = 1 << 10,

		DrawNodeSet = DrawNodeA8 | DynamicBatchI8 | DynamicBatchAI88 | DynamicBatchA8Highp,
		RawDrawingSet = RawTexture | DynamicBatchA8Highp | RawRect | RawRectBorder | RawRects,
	};

	bool init();
	bool init(uint32_t mask);

	cocos2d::GLProgram *getProgram(Program);

	GLProgramSet();

	void reloadPrograms();

protected:
	Map<Program, Rc<cocos2d::GLProgram>> _programs;
};

NS_SP_END

#endif /* LIBS_STAPPLER_FEATURES_DYNAMIC_ATLAS_SPGLPROGRAMSET_H_ */
