/*
 * MaterialUserFontController.h
 *
 *  Created on: 7 нояб. 2016 г.
 *      Author: sbkarr
 */

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
