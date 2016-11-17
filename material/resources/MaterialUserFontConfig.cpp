/*
 * MaterialUserFontController.cpp
 *
 *  Created on: 7 нояб. 2016 г.
 *      Author: sbkarr
 */

#include "Material.h"
#include "MaterialUserFontConfig.h"
#include "MaterialResourceManager.h"

#include "SPThreadManager.h"
#include "SPThread.h"

NS_MD_BEGIN

bool UserFontConfig::init(FontFaceMap && map, float scale) {
	if (!font::Controller::init(std::move(map), { "fonts/", "common/fonts/" }, scale, nullptr)) {
		return false;
	}

	return true;
}

UserFontConfig::Source * UserFontConfig::getSafeSource() const {
	if (ThreadManager::getInstance()->isMainThread()) {
		return getSource();
	} else {
		return _threadSource;
	}
}

void UserFontConfig::onSourceUpdated(Source *source) {
	font::Controller::onSourceUpdated(source);
	auto ptr = new Rc<Source>(source);
	ResourceManager::thread().perform([this, ptr] (Ref *) -> bool {
		_threadSource.swap(*ptr); // no retain/release
		return true;
	}, [this, ptr] (Ref *, bool) {
		delete ptr;
	});
	ResourceManager::onUserFont(ResourceManager::getInstance(), this);
}

NS_MD_END
