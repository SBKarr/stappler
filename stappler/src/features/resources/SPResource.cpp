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

IconSet *generateFromSVGIcons(IconSet::Config &&cfg);
FontSet *generateFromConfig(FontSet::Config &&cfg, const cocos2d::Map<std::string, Asset *> &assets);

static Thread s_resourceThread("ResourceLibraryThread");

std::string getFontAssetPath(const std::string &url) {
	std::string dir = filesystem::cachesPath("font_assets");
	return toString(dir, "/", string::stdlibHashUnsigned(url), ".asset");
}

void generateIconSet(IconSet::Config &&cfg, const IconSet::Callback &callback) {
	auto &thread = s_resourceThread;

	IconSet::Config *cfgPtr = new (IconSet::Config) (std::move(cfg));
	IconSet **newSet = new (IconSet *)(nullptr);
	thread.perform([newSet, cfgPtr] (cocos2d::Ref *) -> bool {
		*newSet = generateFromSVGIcons(std::move(*cfgPtr));
		return true;
	}, [newSet, cfgPtr, callback] (cocos2d::Ref *, bool) {
		if (*newSet) {
			if (callback) {
				callback(*newSet);
			}
			(*newSet)->release();
		}
		delete newSet;
		delete cfgPtr;
	});
}

void requestFontSet(FontSet::Config &&cfg, const FontSet::Callback &callback, cocos2d::Map<std::string, Asset *> &&a) {
	auto &thread = s_resourceThread;
	auto *cfgPtr = new (FontSet::Config) (std::move(cfg));
	auto *assets = new cocos2d::Map<std::string, Asset *>();
	FontSet **newSet = new (FontSet *)(nullptr);

	for (auto &it : a) {
		if (it.second->tryReadLock(cfgPtr)) {
			assets->insert(it.first, it.second);
		}
	}

	thread.perform([newSet, cfgPtr, assets] (cocos2d::Ref *) -> bool {
		(*newSet) = generateFromConfig(std::move(*cfgPtr), *assets);
		return true;
	}, [newSet, cfgPtr, callback, assets] (cocos2d::Ref *, bool) {
		if (*newSet) {
			if (callback) {
				callback(*newSet);
			}
			(*newSet)->release();
		}
		for (auto &it : *assets) {
			it.second->releaseReadLock(cfgPtr);
		}
		delete assets;
		delete newSet;
		delete cfgPtr;
	});
}

void generateFontSet(FontSet::Config &&cfg, const FontSet::Callback &callback) {
	auto &thread = s_resourceThread;
	if (thread.isOnThisThread()) {
		FontSet *newSet = generateFromConfig(std::move(cfg), cocos2d::Map<std::string, Asset *>());
		if (callback && newSet) {
			callback(newSet);
		}
		if (newSet) {
			newSet->release();
		}
	} else {

		auto urls = getConfigUrls(cfg);
		if (urls.empty()) {
			requestFontSet(std::move(cfg), callback, cocos2d::Map<std::string, Asset *>());
		} else {
			auto *cfgPtr = new (FontSet::Config) (std::move(cfg));
			acquireFontAsset(urls, [cfgPtr, callback] (const std::vector<Asset *> &vec) {
				cocos2d::Map<std::string, Asset *> assets;
				for (auto &a : vec) {
					if (a->isReadAvailable() && !a->isWriteLocked()) {
						assets.insert(a->getUrl(), a);
					}
				}

				requestFontSet(std::move(*cfgPtr), callback, std::move(assets));
				delete cfgPtr;
			});
		}
	}
}

void acquireFontAsset(const std::set<std::string> &urls, const std::function<void(const std::vector<Asset *> &)> &cb) {
	filesystem::mkdir(filesystem::cachesPath("font_assets"));
	std::vector<AssetLibrary::AssetRequest> vec;
	for (auto &it : urls) {
		vec.push_back(AssetLibrary::AssetRequest(nullptr, it, getFontAssetPath(it)));
	}
	AssetLibrary::getInstance()->getAssets(vec, cb);
}


std::set<std::string> getConfigUrls(const FontSet::Config &cfg) {
	return getRequestsUrls(cfg.requests);
}

std::set<std::string> getRequestsUrls(const std::vector<FontRequest> &vec) {
	std::set<std::string> ret;
	for (auto &req : vec) {
		for (auto &r : req.receipt) {
			if (isReceiptUrl(r)) {
				ret.insert(r);
			}
		}
	}
	return ret;
}

std::set<std::string> getRequestUrls(const FontRequest &req) {
	return getReceiptUrls(req.receipt);
}

std::set<std::string> getReceiptUrls(const FontRequest::Receipt &r) {
	std::set<std::string> ret;
	for (auto &sources : r) {
		if (isReceiptUrl(sources)) {
			ret.insert(sources);
		}
	}
	return ret;
}

bool isReceiptUrl(const std::string &str) {
	return str.compare(0, 7, "http://") == 0 || str.compare(0, 8, "https://") == 0;
}

Thread &thread() {
	return s_resourceThread;
}

uint64_t getReceiptHash(const FontRequest::Receipt &r,  const cocos2d::Map<std::string, Asset *> &assets) {
	StringStream str;
	for (auto &it : r) {
		if (resource::isReceiptUrl(it)) {
			auto assetIt = assets.find(it);
			if (assetIt != assets.end()) {
				str << it << ":" << assetIt->second->getETag() << ":" << assetIt->second->getMTime() << ";";
			}
		} else {
			str << it << ";";
		}
	}
	auto fallback = getFallbackFont();
	if (!fallback.empty()) {
		str << fallback << ";";
	}
	return string::stdlibHashUnsigned(str.str());
}

NS_SP_EXT_END(resource)
