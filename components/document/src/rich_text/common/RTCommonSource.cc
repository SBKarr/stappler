// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2017 Roman Katuntsev <sbkarr@stappler.org>

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

#include "SPDefine.h"
#include "RTCommonSource.h"

#include "SPFilesystem.h"
#include "SPResource.h"
#include "SPThread.h"
#include "SPString.h"
#include "SPTextureCache.h"
#include "SPAssetLibrary.h"

NS_RT_BEGIN

SP_DECLARE_EVENT(CommonSource, "RichTextSource", onError);
SP_DECLARE_EVENT(CommonSource, "RichTextSource", onDocument);

String CommonSource::getPathForUrl(const String &url) {
	auto dir = filesystem::writablePath("Documents");
	filesystem::mkdir(dir);
	return toString(dir, "/", string::stdlibHashUnsigned(url));
}

CommonSource::~CommonSource() { }

bool CommonSource::init() {
	if (!FontController::init(FontFaceMap(), {"fonts/", "common/fonts/"}, [this] (const layout::FontSource *s, const String &str) -> Bytes {
		if (str.compare(0, "document://"_len, "document://") == 0) {
			auto doc = getDocument();
			if (doc) {
				return doc->getFileData(str.substr("document://"_len));
			}
		} else if (str.compare(0, "local://"_len, "local://") == 0) {
			auto path = str.substr("local://"_len);
			if (filesystem::exists(path)) {
				return filesystem::readFile(path);
			} else if (filesystem::exists("fonts/" + path)) {
				return filesystem::readFile("fonts/" + path);
			} else if (filesystem::exists("common/fonts/" + path)) {
				return filesystem::readFile("common/fonts/" + path);
			}
		}
		return Bytes();
	}, false)) {
		return false;
	}

	_documentAsset.setCallback(std::bind(&CommonSource::onDocumentAssetUpdated, this, std::placeholders::_1));

	return true;
}

bool CommonSource::init(SourceAsset *asset, bool enabled) {
	if (!init()) {
		return false;
	}

	_enabled = enabled;
	onDocumentAsset(asset);
	return true;
}

void CommonSource::setHyphens(layout::HyphenMap *map) {
	_hyphens = map;
}

layout::HyphenMap *CommonSource::getHyphens() const {
	return _hyphens;
}

Document *CommonSource::getDocument() const {
	return static_cast<Document *>(_document.get());
}
SourceAsset *CommonSource::getAsset() const {
	return _documentAsset;
}

Map<String, CommonSource::AssetMeta> CommonSource::getExternalAssetMeta() const {
	Map<String, CommonSource::AssetMeta> ret;
	for (auto &it : _networkAssets) {
		ret.emplace(it.first, it.second.meta);
	}
	return ret;
}

const Map<String, CommonSource::AssetData> &CommonSource::getExternalAssets() const {
	return _networkAssets;
}

bool CommonSource::isReady() const {
	return _document;
}

bool CommonSource::isActual() const {
	if (!_document) {
		return false;
	}

	if (!_documentAsset) {
		return true;
	}

	if (_documentLoading) {
		return false;
	}

	if (_loadedAssetMTime >= _documentAsset->getMTime()) {
		return true;
	} else if (_documentAsset->isUpdateAvailable()) {
		return true;
	}

	return false;
}

bool CommonSource::isDocumentLoading() const {
	return _documentLoading;
}

void CommonSource::refresh() {
	updateDocument();
}

bool CommonSource::tryReadLock(Ref *ref) {
	auto it = _readLocks.find(ref);
	if (it != _readLocks.end()) {
		if (it->second.acquired) {
			++ it->second.count;
			return true;
		}
		return false;
	}

	auto vec = getAssetsVec();
	if (vec.empty()) {
		_readLocks.emplace(ref, LockStruct{vec, this, 1, true});
		return true;
	}

	if (SyncRWLock::tryReadLock(ref, vec)) {
		_readLocks.emplace(ref, LockStruct{vec, this, 1, true});
		return true;
	}
	return false;
}

void CommonSource::retainReadLock(Ref *ref, const Function<void()> &cb) {
	auto it = _readLocks.find(ref);
	if (it != _readLocks.end()) {
		++ it->second.count;
		if (it->second.acquired) {
			cb();
		} else {
			it->second.waiters.emplace_back(cb);
		}
		return;
	}

	auto vec = getAssetsVec();
	if (vec.empty()) {
		_readLocks.emplace(ref, LockStruct{vec, this, 1, true});
		cb();
	} else {
		_readLocks.emplace(ref, LockStruct{vec, this, 1, false, Vector<Function<void()>>{cb}});
		SyncRWLock::retainReadLock(ref, vec, [this, ref, vec] {
			auto it = _readLocks.find(ref);
			if (it == _readLocks.end()) {
				SyncRWLock::releaseReadLock(ref, vec);
			} else {
				it->second.acquired = true;
				for (auto &cb : it->second.waiters) {
					cb();
				}
				it->second.waiters.clear();
			}
		});
	}
}

void CommonSource::releaseReadLock(Ref *ref) {
	auto it = _readLocks.find(ref);
	if (it != _readLocks.end()) {
		if (it->second.acquired) {
			if (it->second.count == 1) {
				SyncRWLock::releaseReadLock(ref, it->second.vec);
				_readLocks.erase(it);
			} else {
				-- it->second.count;
			}
		}
	}
}

void CommonSource::setEnabled(bool val) {
	if (_enabled != val) {
		_enabled = val;
		if (_enabled && _documentAsset) {
			onDocumentAssetUpdated(data::Subscription::Flag((uint8_t)Asset::CacheDataUpdated));
		}
	}
}
bool CommonSource::isEnabled() const {
	return _enabled;
}

void CommonSource::onDocumentAsset(SourceAsset *a) {
	_documentAsset = a;
	if (_documentAsset) {
		_loadedAssetMTime = 0;
		if (_enabled) {
			_documentAsset->download();
		}
		onDocumentAssetUpdated(data::Subscription::Flag((uint8_t)Asset::FileUpdated));
	}
}

void CommonSource::onDocumentAssetUpdated(data::Subscription::Flags f) {
	if (f.hasFlag((uint8_t)Asset::DownloadFailed)) {
		onError(this, Error::NetworkError);
	}

	if (_documentAsset->isDownloadAvailable() && !_documentAsset->isDownloadInProgress()) {
		if (f.hasFlag((uint8_t)Asset::DownloadFailed)) {
			if (isnan(_retryUpdate)) {
				_retryUpdate = 20.0f;
			}
		} else {
			if (_enabled) {
				_documentAsset->download();
			}
			onUpdate(this);
		}
	}
	if (_loadedAssetMTime < _documentAsset->getMTime()) {
		tryLoadDocument();
	} else if ((f.initial() && _loadedAssetMTime == 0) || f.hasFlag((uint8_t)Asset::Update::FileUpdated)) {
		_loadedAssetMTime = 0;
		tryLoadDocument();
	}

	if (f.hasFlag((uint8_t)Asset::FileUpdated) || f.hasFlag((uint8_t)Asset::DownloadSuccessful) || f.hasFlag((uint8_t)Asset::DownloadFailed)) {
		onDocument(this, _documentAsset.get());
	}
}

void CommonSource::tryLoadDocument() {
	if (!_enabled) {
		return;
	}

	if (!_documentAsset->tryLockDocument(_loadedAssetMTime)) {
		return;
	}

	auto &thread = TextureCache::thread();
	Rc<Document> *doc = new Rc<Document>(nullptr);
	Rc<SourceAsset> *asset = new Rc<SourceAsset>(_documentAsset.get());
	Set<String> *assets = new Set<String>();

	_loadedAssetMTime = _documentAsset->getMTime();
	_documentLoading = true;
	onUpdate(this);

	thread.perform([this, doc, asset, assets] (const Task &) -> bool {
		*doc = asset->get()->openDocument();
		if (*doc) {
			auto &pages = (*doc)->getContentPages();
			for (auto &p_it : pages) {
				for (auto &str : p_it.second.assets) {
					assets->emplace(str);
				}
			}
		}
		return true;
	}, [this, doc, asset, assets] (const Task &, bool success) {
		if (success && *doc) {
			auto l = [this, doc, asset] {
				_documentLoading = false;
				asset->get()->releaseDocument();
				onDocumentLoaded(doc->get());
				delete doc;
				delete asset;
			};
			if (onExternalAssets(doc->get(), *assets)) {
				l();
			} else {
				waitForAssets(move(l));
			}
		}
		delete assets;
	}, this);
}

void CommonSource::onDocumentLoaded(Document *doc) {
	if (_document != doc) {
		_document = doc;
		if (_document) {
			_dirty = true;
			_dirtyFlags = DirtyFontFace;
			updateSource();
		}
		onDocument(this);
	}
}

void CommonSource::acquireAsset(const String &url, const Function<void(SourceAsset *)> &fn) {
	StringView urlView(url);
	if (urlView.is("http://") || urlView.is("https://")) {
		auto path = getPathForUrl(url);
		auto lib = AssetLibrary::getInstance();
		retain();
		lib->getAsset([this, fn] (Asset *a) {
			fn(Rc<SourceNetworkAsset>::create(a));
			release();
		}, url, path, config::getDocumentAssetTtl());
	} else {
		fn(nullptr);
	}
}

bool CommonSource::isExternalAsset(Document *doc, const String &asset) {
	bool isDocFile = doc->isFileExists(asset);
	if (!isDocFile) {
		StringView urlView(asset);
		if (urlView.is("http://") || urlView.is("https://")) {
			return true;
		}
	}
	return false;
}

bool CommonSource::onExternalAssets(Document *doc, const Set<String> &assets) {
	for (auto &it : assets) {
		if (isExternalAsset(doc, it)) {
			auto n_it = _networkAssets.find(it);
			if (n_it == _networkAssets.end()) {
				auto a_it = _networkAssets.emplace(it, AssetData{it}).first;
				AssetData * data = &a_it->second;
				data->asset.setCallback(std::bind(&CommonSource::onExternalAssetUpdated, this, data, std::placeholders::_1));
				addAssetRequest(data);
				acquireAsset(it, [this, data] (SourceAsset *a) {
					if (a) {
						data->asset = a;
						if (data->asset->isReadAvailable()) {
							if (data->asset->tryReadLock()) {
								readExternalAsset(*data);
								data->asset->releaseReadLock();
							}
						}
						if (data->asset->isDownloadAvailable()) {
							data->asset->download();
						}
					}
					removeAssetRequest(data);
				});
				log::text("External asset", it);
			}
		}
	}
	return !hasAssetRequests();
}

void CommonSource::onExternalAssetUpdated(AssetData *a, data::Subscription::Flags f) {
	if (f.hasFlag((uint8_t)Asset::Update::FileUpdated)) {
		bool updated = false;
		if (a->asset->tryReadLock()) {
			if (readExternalAsset(*a)) {
				updated = true;
			}
			a->asset->releaseReadLock();
		}
		if (updated) {
			if (_document) {
				_dirty = true;
			}
			onDocument(this);
		}
	}
}

bool CommonSource::readExternalAsset(AssetData &data) {
	data.meta.type = data.asset->getContentType().str();
	if (StringView(data.meta.type).starts_with("image/") || data.meta.type.empty()) {
		auto tmpImg = data.meta.image;
		size_t w = 0, h = 0;
		if (data.asset->getImageSize(w, h)) {
			data.meta.image.width = w;
			data.meta.image.height = h;
		}

		if (data.meta.image.width != tmpImg.width || data.meta.image.height != tmpImg.height) {
			return true;
		}
	} else if (StringView(data.meta.type).is("text/css")) {
		auto d = data.asset->getData();
		data.meta.css = Rc<layout::CssDocument>::create(StringView((const char *)d.data(), d.size()));
		return true;
	}
	return false;
}

void CommonSource::updateDocument() {
	if (_documentAsset && _loadedAssetMTime > 0) {
		_loadedAssetMTime = 0;
	}

	tryLoadDocument();
}

void CommonSource::onSourceUpdated(FontSource *source) {
	FontController::onSourceUpdated(source);
}

Rc<font::FontSource> CommonSource::makeSource(AssetMap && map, bool schedule) {
	if (_document) {
		FontFaceMap faceMap(_fontFaces);
		auto &pages = _document->getContentPages();
		for (auto &it : pages) {
			font::FontSource::mergeFontFace(faceMap, it.second.fonts);
		}
		return Rc<font::FontSource>::create(std::move(faceMap), _callback, _scale, SearchDirs(_searchDir), std::move(map), false);
	}
	return Rc<font::FontSource>::create(FontFaceMap(_fontFaces), _callback, _scale, SearchDirs(_searchDir), std::move(map), false);
}

bool CommonSource::hasAssetRequests() const {
	return !_assetRequests.empty();
}
void CommonSource::addAssetRequest(AssetData *data) {
	_assetRequests.emplace(data);
}
void CommonSource::removeAssetRequest(AssetData *data) {
	if (!_assetRequests.empty()) {
		_assetRequests.erase(data);
		if (_assetRequests.empty()) {
			if (!_assetWaiters.empty()) {
				auto w = move(_assetWaiters);
				_assetWaiters.clear();
				for (auto &it : w) {
					it();
				}
			}
		}
	}
}

void CommonSource::waitForAssets(Function<void()> &&fn) {
	_assetWaiters.emplace_back(move(fn));
}

Vector<SyncRWLock *> CommonSource::getAssetsVec() const {
	Vector<SyncRWLock *> ret; ret.reserve(1 + _networkAssets.size());
	if (_documentAsset) {
		if (auto lock = _documentAsset->getRWLock()) {
			ret.emplace_back(lock);
		}
	}
	for (auto &it : _networkAssets) {
		if (it.second.asset) {
			if (auto lock = it.second.asset->getRWLock()) {
				ret.emplace_back(lock);
			}
		}
	}
	return ret;
}

bool CommonSource::isFileExists(const StringView &url) const {
	auto it = _networkAssets.find(url);
	if (it != _networkAssets.end() && it->second.asset && it->second.asset->isReadAvailable()) {
		return true;
	}

	if (_document) {
		return _document->isFileExists(url);
	}
	return false;
}

Pair<uint16_t, uint16_t> CommonSource::getImageSize(const StringView &url) const {
	auto it = _networkAssets.find(url);
	if (it != _networkAssets.end()) {
		if ((StringView(it->second.meta.type).starts_with("image/") || it->second.meta.type.empty()) && it->second.meta.image.width > 0 && it->second.meta.image.height > 0) {
			return pair(it->second.meta.image.width, it->second.meta.image.height);
		}
	}

	if (_document) {
		return _document->getImageSize(url);
	}
	return Pair<uint16_t, uint16_t>(0, 0);
}

Bytes CommonSource::getImageData(const StringView &url) const {
	auto it = _networkAssets.find(url);
	if (it != _networkAssets.end()) {
		if ((StringView(it->second.meta.type).starts_with("image/") || it->second.meta.type.empty()) && it->second.meta.image.width > 0 && it->second.meta.image.height > 0) {
			return it->second.asset->getData();
		}
	}

	if (_document) {
		return _document->getImageData(url);
	}
	return Bytes();
}

void CommonSource::update(float dt) {
	FontController::update(dt);
	if (!isnan(_retryUpdate)) {
		_retryUpdate -= dt;
		if (_retryUpdate <= 0.0f) {
			_retryUpdate = nan();
			if (_enabled && _documentAsset && _documentAsset->isDownloadAvailable() && !_documentAsset->isDownloadInProgress()) {
				_documentAsset->download();
			}
		}
	}
}

NS_RT_END
