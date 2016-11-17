/*
 * SPResourceLibrary.cpp
 *
 *  Created on: 12 апр. 2015 г.
 *      Author: sbkarr
 */

#include "SPDefine.h"
#include "SPFont.h"

#include "SPFilesystem.h"
#include "SPImage.h"
#include "SPString.h"
#include "SPThread.h"
#include "SPData.h"
#include "SPAsset.h"
#include "SPAssetLibrary.h"
#include "SPResource.h"

NS_SP_EXT_BEGIN(resource)

static Thread s_resourceThread("ResourceLibraryThread");

std::string getFontAssetPath(const std::string &url) {
	std::string dir = filesystem::cachesPath("font_assets");
	return toString(dir, "/", string::stdlibHashUnsigned(url), ".asset");
}

void acquireFontAsset(const Set<String> &urls, const Function<void(const Vector<Asset *> &)> &cb) {
	filesystem::mkdir(filesystem::cachesPath("font_assets"));
	Vector<AssetLibrary::AssetRequest> vec;
	for (auto &it : urls) {
		vec.push_back(AssetLibrary::AssetRequest(nullptr, it, getFontAssetPath(it)));
	}
	AssetLibrary::getInstance()->getAssets(vec, cb);
}


bool isReceiptUrl(const std::string &str) {
	return str.compare(0, 7, "http://") == 0 || str.compare(0, 8, "https://") == 0
			|| str.compare(0, 6, "ftp://") == 0 || str.compare(0, 7, "ftps://") == 0;
}

Thread &thread() {
	return s_resourceThread;
}

static String s_fallbackFont;

void setFallbackFont(const String &str) {
	s_fallbackFont = str;
}

const String &getFallbackFont() {
	return s_fallbackFont;
}

NS_SP_EXT_END(resource)
