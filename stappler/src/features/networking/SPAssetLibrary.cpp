//
//  SPAssetLibrary.cpp
//  stappler
//
//  Created by SBKarr on 17.09.14.
//  Copyright (c) 2014 SBKarr. All rights reserved.
//

#include "SPDefine.h"
#include "SPAssetLibrary.h"

#include "SPAsset.h"
#include "SPAssetDownload.h"

#include "SPData.h"
#include "SPDevice.h"
#include "SPString.h"
#include "SPStorage.h"
#include "SPFilesystem.h"

#include "base/CCVector.h"

NS_SP_BEGIN

SP_DECLARE_EVENT_CLASS(AssetLibrary, onLoaded);

static storage::Handle *s_assetStorage = nullptr;

static storage::Handle *getAssetStorage() {
	if (!s_assetStorage) {
		const auto &libpath = filesystem::writablePath("library");
		filesystem::mkdir(libpath);
		s_assetStorage = storage::create("AssetStorage", libpath + "/assets.v1.db");
	}
	return s_assetStorage;
}

static AssetLibrary *s_instance = nullptr;

AssetLibrary *AssetLibrary::getInstance() {
	if (!s_instance) {
		s_instance = new AssetLibrary();
		storage::defaultStorage();
		s_instance->init();
	}
	return s_instance;
}

void AssetLibrary::importAssetData(data::Value &value) {
	if (s_instance) {
		s_instance->_assetsClass.insert(std::move(value))->perform();
	} else {
		s_instance = new AssetLibrary();
		s_instance->_assetsClass.insert(std::move(value))->perform();
		s_instance->init();
	}
}

AssetLibrary::AssetLibrary()
:_assetsClass(storage::Scheme("stappler_assets_v2", {
		storage::Scheme::Field("id", storage::Scheme::Type::Integer, storage::Scheme::PrimaryKey),
		storage::Scheme::Field("size", storage::Scheme::Type::Integer, 0, 8),
		storage::Scheme::Field("mtime", storage::Scheme::Type::Integer, 0, 8),
		storage::Scheme::Field("touch", storage::Scheme::Type::Integer, 0, 8),
		storage::Scheme::Field("ttl", storage::Scheme::Type::Integer, 0, 8),
		storage::Scheme::Field("unupdated", storage::Scheme::Type::Integer, 0, 1),
		storage::Scheme::Field("download", storage::Scheme::Type::Integer, 0, 1),
		storage::Scheme::Field("url", storage::Scheme::Type::Text),
		storage::Scheme::Field("path", storage::Scheme::Type::Text),
		storage::Scheme::Field("cache", storage::Scheme::Type::Text),
		storage::Scheme::Field("contentType", storage::Scheme::Type::Text),
		storage::Scheme::Field("etag", storage::Scheme::Type::Text),
}, {}, getAssetStorage()))
, _stateClass(storage::Scheme("stappler_asset_state", {
		storage::Scheme::Field("path", storage::Scheme::Type::Text, storage::Scheme::PrimaryKey),
		storage::Scheme::Field("url", storage::Scheme::Type::Text),
		storage::Scheme::Field("ctime", storage::Scheme::Type::Integer, 0, 8),
		storage::Scheme::Field("asset", storage::Scheme::Type::Integer),
}, {},  getAssetStorage())) {
}

void AssetLibrary::init() {
    Device::getInstance();
	Downloads *downloads = new Downloads();

	auto &thread = storage::thread(getAssetStorage());
	thread.perform([this, downloads] (cocos2d::Ref *) -> bool {
		storage::get("SP.AssetLibrary.Time", [this] (const std::string &key, data::Value &value) {
			_correctionTime.setMicroseconds(value.getInteger("time"));
			_correctionNegative = value.getBool("negative");
		}, getAssetStorage());

		data::Value data;
		_assetsClass.get([&data] (data::Value &d) {
			if (!d.isArray()) {
				return;
			}

			data = std::move(d);
		})->filterBy("download", 1)->perform();

		restoreDownloads(data, *downloads);
		cleanup();
		return true;
	}, [this, downloads] (cocos2d::Ref *, bool) {
		for (auto &it : *downloads) {
			_assets.insert(std::make_pair(it.first->getId(), it.first));
			_downloads.insert(it.first, it.second);
			startDownload(it.second);
		}
		delete downloads;

		_loaded = true;
		getAssets(_tmpRequests);
		for (auto &it : _tmpMultiRequest) {
			getAssets(it.first, it.second);
		}
		onLoaded(this);
	}, this);
}

void AssetLibrary::restoreDownloads(data::Value &d, Downloads &downloads) {
	if (d.isArray()) {
		for (auto &it : d.asArray()) {
			if (it.getBool("download") == true) {
				auto tmpPath =  getTempPath(it.getString("path"));
				Asset *a = new Asset(it, nullptr);
				touchAsset(a);
				auto d = Rc<AssetDownload>::create(a, tmpPath);
				downloads.insert(a, d);
			}
		}
	}
}

void AssetLibrary::cleanup() {
	if (!Device::getInstance()->isNetworkOnline()) {
		return;
	}

	auto time = getCorrectTime().toMicroseconds();

	auto q = toString("SELECT path FROM ", _assetsClass.getName(),
			" WHERE download == 0 AND ttl != 0 AND (touch + ttl) < ",
			time, ";");
	_assetsClass.perform(q, [this] (data::Value &val) {
		if (val.isArray()) {
			for (auto &it : val.asArray()) {
				auto path = filepath::absolute(it.getString("path"));
				auto cache = filepath::absolute(it.getString("cache"));
				auto tmpPath = getTempPath(path);

				filesystem::remove(path);
				filesystem::remove(tmpPath);
				if (!cache.empty()) {
					filesystem::remove(cache, true, true);
				}
			}
		}
	});
	_assetsClass.perform(toString("DELETE FROM ", _assetsClass.getName(),
			" WHERE download == 0 AND ttl != 0 AND touch + ttl * 2 < ",
			+ time, ";"));


	_stateClass.perform(toString("SELECT * FROM ", _stateClass.getName(), ";"), [this] (data::Value &val) {
		if (val.isArray()) {
			for (auto &it : val.asArray()) {
				auto path = filepath::absolute(it.getString("path"));
				filesystem::remove(path);
			}
		}
	});
	_stateClass.perform(toString("DELETE FROM ", _stateClass.getName(), ";"));
}

void AssetLibrary::startDownload(AssetDownload *d) {
	d->setCompletedCallback(std::bind(&AssetLibrary::removeDownload, this, d));
	d->run();
}

void AssetLibrary::removeDownload(AssetDownload *d) {
	auto a = d->getAsset();
	_downloads.erase(a);
}

std::string AssetLibrary::getTempPath(const std::string &path) const {
	return path + ".tmp";
}

bool AssetLibrary::isLiveAsset(uint64_t id) const {
	return getLiveAsset(id) != nullptr;
}
bool AssetLibrary::isLiveAsset(const std::string &url, const std::string &path) const {
	return getLiveAsset(url, path) != nullptr;
}

void AssetLibrary::updateAssets() {
	for (auto &it : _assets) {
		it.second->checkFile();
	}
}

void AssetLibrary::addAssetFile(const String &path, const String &url, uint64_t asset, uint64_t ctime) {
	_stateClass.insert(data::Value({
		pair("path", data::Value(path)),
		pair("url", data::Value(url)),
		pair("ctime", data::Value(int64_t(ctime))),
		pair("asset", data::Value(int64_t(asset))),
	}))->perform();
}
void AssetLibrary::removeAssetFile(const String &path) {
	_stateClass.remove()->select(path)->perform();
}

Time AssetLibrary::getCorrectTime() const {
	Time now = Time::now();
	if (_correctionNegative) {
		now -= _correctionTime;
	} else {
		now += _correctionTime;
	}

	return now;
}

Asset *AssetLibrary::getLiveAsset(uint64_t id) const {
	auto it = _assets.find(id);
	if (it != _assets.end()) {
		return it->second;
	}
	return nullptr;
}

Asset *AssetLibrary::getLiveAsset(const std::string &url, const std::string &path) const {
	return getLiveAsset(getAssetId(url, path));
}

uint64_t AssetLibrary::getAssetId(const std::string &url, const std::string &path) const {
	return string::stdlibHashUnsigned(url +"|"+ filepath::canonical(path));
}


void AssetLibrary::setServerDate(const Time &serv) {
	auto t = Time::now();
	if (t < serv) {
		_correctionNegative = false;
	} else {
		_correctionNegative = true;
	}
	_correctionTime = serv - t;

	data::Value d;
	d.setInteger(_correctionTime.toMicroseconds(), "time");
	d.setInteger(_correctionNegative, "negative");

	storage::set("SP.AssetLibrary.Time", std::move(d), nullptr, getAssetStorage());
}

bool AssetLibrary::getAsset(const AssetCallback &cb, const std::string &url,
		const std::string &path, TimeInterval ttl, const std::string &cache, const DownloadCallback &dcb) {
	if (!_loaded) {
		_tmpRequests.push_back(AssetRequest(cb, url, path, ttl, cache, dcb));
		return true;
	}

	uint64_t id = getAssetId(url, path);
	if (auto a = getLiveAsset(id)) {
		if (cb) {
			cb(a);
			return true;
		}
	}
	auto it = _callbacks.find(id);
	if (it != _callbacks.end()) {
		it->second.push_back(cb);
	} else {
		CallbackVec vec;
		vec.push_back(cb);
		_callbacks.insert(std::make_pair(id, std::move(vec)));

		auto &thread = storage::thread(getAssetStorage());
		Asset ** assetPtr = new (Asset *) (nullptr);
		thread.perform([this, assetPtr, id, url, path, cache, ttl, dcb] (cocos2d::Ref *) -> bool {
			data::Value data;
			_assetsClass.get([&data] (data::Value &d) {
				if (d.isArray() && d.size() > 0) {
					data = std::move(d.getValue(0));
				}
			})->select(id)->perform();

			if (!data) {
				data.setInteger(reinterpretValue<int64_t>(id), "id");
				data.setString(url, "url");
				data.setString(filepath::canonical(path), "path");
				data.setInteger(ttl.toMicroseconds(), "ttl");
				data.setString(filepath::canonical(cache), "cache");
				data.setInteger(getCorrectTime().toMicroseconds(), "touch");
				_assetsClass.insert(data)->perform();
			}

			if (data.isDictionary()) {
				data.setString(filepath::absolute(data.getString("path")), "path");
				data.setString(filepath::absolute(data.getString("cache")), "cache");
			}

			(*assetPtr) = new Asset(data, dcb);
			return true;
		}, [this, assetPtr] (cocos2d::Ref *, bool) {
			onAssetCreated(*assetPtr);
			(*assetPtr)->autorelease();
			delete assetPtr;
		}, this);
	}

	return true;
}

AssetLibrary::AssetRequest::AssetRequest(const AssetCallback &cb, const std::string &url, const std::string &path,
		TimeInterval ttl, const std::string &cacheDir, const DownloadCallback &dcb)
: callback(cb), id(AssetLibrary::getInstance()->getAssetId(url, path))
, url(url), path(path), ttl(ttl), cacheDir(cacheDir), download(dcb) { }

bool AssetLibrary::getAssets(const std::vector<AssetRequest> &vec, const AssetVecCallback &cb) {
	if (!_loaded) {
		if (!cb) {
			for (auto &it : vec) {
				_tmpRequests.push_back(it);
			}
		} else {
			_tmpMultiRequest.push_back(std::make_pair(vec, cb));
		}
		return true;
	}

	ssize_t assetCount = (ssize_t)vec.size();
	auto requests = new std::vector<AssetRequest>;
	cocos2d::Vector<Asset *> *retVec = nullptr;
	if (cb) {
		retVec = new cocos2d::Vector<Asset *>;
	}
	for (auto &it : vec) {
		uint64_t id = it.id;
		if (auto a = getLiveAsset(id)) {
			if (it.callback) {
				it.callback(a);
			}
			if (retVec) {
				retVec->pushBack(a);
			}
		} else {
			auto cbit = _callbacks.find(id);
			if (cbit != _callbacks.end()) {
				cbit->second.push_back(it.callback);
				if (cb && retVec) {
					cbit->second.push_back([cb, retVec, assetCount] (Asset *a) {
						retVec->pushBack(a);
						if (retVec->size() == assetCount) {
							cb(retVec->vector());
							delete retVec;
						}
					});
				}
			} else {
				CallbackVec vec;
				vec.push_back(it.callback);
				_callbacks.insert(std::make_pair(id, std::move(vec)));
				requests->push_back(it);
			}
		}
	}

	if (requests->empty()) {
		if (cb && retVec) {
			if (retVec->size() == assetCount) {
				cb(retVec->vector());
				delete retVec;
			}
		}
		delete requests;
		return true;
	}

	auto &thread = storage::thread(getAssetStorage());
	auto assetsVec = new std::vector<Asset *>;
	thread.perform([this, assetsVec, requests] (cocos2d::Ref *) -> bool {
		performGetAssets(*assetsVec, *requests);

		return true;
	}, [this, assetsVec, requests, retVec, assetCount, cb] (cocos2d::Ref *, bool) {
		for (auto &it : (*assetsVec)) {
			if (retVec) {
				retVec->pushBack(it);
			}
			onAssetCreated(it);
		}
		if (cb && retVec) {
			if (retVec->size() == assetCount) {
				cb(retVec->vector());
				delete retVec;
			}
		}
		for (auto &it : (*assetsVec)) {
			it->release();
		}
		delete requests;
		delete assetsVec;
	}, this);

	return true;
}

void AssetLibrary::performGetAssets(std::vector<Asset *> &assetsVec, const std::vector<AssetRequest> &requests) {
	data::Value data;
	auto cmd = _assetsClass.get([&data] (data::Value &d) {
		if (d.isArray() && d.size() > 0) {
			data = std::move(d);
		}
	});
	for (auto &it : requests) {
		cmd->select(it.id);
	}
	cmd->perform();

	std::map<uint64_t, data::Value> dataMap;
	if (data) {
		for (auto &it : data.asArray()) {
			auto id = it.getInteger("id");
			dataMap.insert(std::make_pair(id, std::move(it)));
		}
	}

	data = data::Value(data::Value::Type::ARRAY);

	for (auto &it : requests) {
		auto dataIt = dataMap.find(it.id);
		data::Value rdata;
		if (dataIt == dataMap.end()) {
			rdata.setInteger(reinterpretValue<int64_t>(it.id), "id");
			rdata.setString(it.url, "url");
			rdata.setString(filepath::canonical(it.path), "path");
			rdata.setInteger(it.ttl.toMicroseconds(), "ttl");
			rdata.setString(filepath::canonical(it.cacheDir), "cache");
			rdata.setInteger(getCorrectTime().toMicroseconds(), "touch");
			data.addValue(rdata);
		} else {
			rdata = std::move(dataIt->second);
		}

		if (rdata.isDictionary()) {
			rdata.setString(filepath::absolute(rdata.getString("path")), "path");
			rdata.setString(filepath::absolute(rdata.getString("cache")), "cache");
		}

		auto a = new Asset(rdata, it.download);
		assetsVec.push_back(a);
	}

	if (data.size() > 0) {
		 _assetsClass.insert(std::move(data))->perform();
	}
}

Asset *AssetLibrary::acquireLiveAsset(const std::string &url, const std::string &path) {
	uint64_t id = getAssetId(url, path);
	return getLiveAsset(id);
}

void AssetLibrary::onAssetCreated(Asset *asset) {
	if (asset) {
		_assets.insert(std::make_pair(asset->getId(), asset));
		auto it = _callbacks.find(asset->getId());
		if (it != _callbacks.end()) {
			for (auto &cb : it->second) {
				if (cb) {
					cb(asset);
				}
			}
			_callbacks.erase(it);
		}
	}
}

void AssetLibrary::removeAsset(Asset *asset) {
	_assets.erase(asset->getId());
	if (asset->isStorageDirty()) {
		saveAsset(asset);
	}
}

bool AssetLibrary::downloadAsset(Asset *asset) {
	auto it = _downloads.find(asset);
	if (it == _downloads.end()) {
		if (Device::getInstance()->isNetworkOnline()) {
			auto d = Rc<AssetDownload>::create(asset, getTempPath(asset->getFilePath()));
			_downloads.insert(asset, d);
			startDownload(d);
			return true;
		}
	}
	return false;
}

void AssetLibrary::touchAsset(Asset *asset) {
	asset->touchWithTime(getCorrectTime());
	saveAsset(asset);
}

void AssetLibrary::saveAsset(Asset *asset) {
	data::Value val;
	val.setInteger(reinterpretValue<int64_t>(asset->getId()), "id");
	val.setInteger(asset->getSize(), "size");
	val.setInteger(asset->getMTime(), "mtime");
	val.setInteger(asset->getTouch().toMicroseconds(), "touch");
	val.setInteger(asset->getTtl().toMicroseconds(), "ttl");
	val.setBool(asset->isUpdateAvailable() || asset->isDownloadInProgress(), "unupdated");
	val.setInteger((_downloads.find(asset) != _downloads.end()), "download");
	val.setString(asset->getUrl(), "url");
	val.setString(filepath::canonical(asset->getFilePath()), "path");
	val.setString(filepath::canonical(asset->getCachePath()), "cache");
	val.setString(asset->getContentType(), "contentType");
	val.setString(asset->getETag(), "etag");
	val.setString(data::toString(asset->getData()), "data");

	_assetsClass.insert(std::move(val))->perform();

	asset->setStorageDirty(false);
}

NS_SP_END
