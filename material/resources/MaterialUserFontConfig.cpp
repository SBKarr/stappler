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

#include "Material.h"
#include "MaterialUserFontConfig.h"
#include "MaterialResourceManager.h"

#include "SPThreadManager.h"
#include "SPThread.h"

NS_MD_BEGIN

bool UserFontConfig::init(FontFaceMap && map, float scale) {
	if (!font::FontController::init(std::move(map), { "fonts/", "common/fonts/" }, scale, nullptr)) {
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
	font::FontController::onSourceUpdated(source);
	auto ptr = new Rc<Source>(source);
	ResourceManager::thread().perform([this, ptr] (const Task &) -> bool {
		_threadSource.swap(*ptr); // no retain/release
		return true;
	}, [this, ptr] (const Task &, bool) {
		delete ptr;
	});
	ResourceManager::onUserFont(ResourceManager::getInstance(), this);
}

NS_MD_END
