/*
 * SPRichTextSource.cpp
 *
 *  Created on: 30 июля 2015 г.
 *      Author: sbkarr
 */

#include "SPDefine.h"
#include "SPRichTextSource.h"
#include "SPAssetLibrary.h"
#include "SPResource.h"
#include "SPFilesystem.h"
#include "SPThread.h"
#include "SPString.h"
#include "SPTextureCache.h"
#include "SPEpubDocument.h"

NS_SP_EXT_BEGIN(rich_text)

SP_DECLARE_EVENT(Source, "RichTextSource", onError);
SP_DECLARE_EVENT(Source, "RichTextSource", onDocument);

Source::~Source() { }

bool Source::init() {
	if (!font::Controller::init(FontFaceMap(), {"fonts/", "common/fonts/"}, [this] (const font::Source *s, const String &str) -> Bytes {
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
	})) {
		return false;
	}

	_documentAsset.setCallback(std::bind(&Source::onDocumentAssetUpdated, this, std::placeholders::_1));

	return true;
}

bool Source::init(const StringDocument &str) {
	if (!init()) {
		return false;
	}

	_string = str.get();
	updateDocument();
	return true;
}
bool Source::init(const FilePath &file) {
	if (!init()) {
		return false;
	}

	_file = file.get();
	updateDocument();
	return true;
}

bool Source::init(const String &url, const String &path,
		TimeInterval ttl, const String &cacheDir, const Asset::DownloadCallback &cb) {
	if (!init()) {
		return false;
	}

	retain();
	AssetLibrary::getInstance()->getAsset([this] (Asset *a) {
		onDocumentAsset(a);
		release();
	}, url, path, ttl, cacheDir, cb);

	return true;
}
bool Source::init(Asset *a, bool enabled) {
	if (!init()) {
		return false;
	}

	_enabled = enabled;
	onDocumentAsset(a);
	return true;
}

Document *Source::getDocument() const {
	return _document;
}
Asset *Source::getAsset() const {
	return _documentAsset;
}

bool Source::isReady() const {
	return _document;
}

bool Source::isActual() const {
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

bool Source::isDocumentLoading() const {
	return _documentLoading;
}

void Source::refresh() {
	updateDocument();
}

bool Source::tryReadLock(Ref *ref) {
	if (_documentAsset) {
		return _documentAsset->tryReadLock(ref);
	}
	return true;
}

void Source::retainReadLock(Ref *ref, const Function<void()> &cb) {
	if (_documentAsset) {
		_documentAsset->retainReadLock(ref, cb);
	} else {
		cb();
	}
}

void Source::releaseReadLock(Ref *ref) {
	if (_documentAsset) {
		_documentAsset->releaseWriteLock(ref);
	}
}

void Source::setEnabled(bool val) {
	if (_enabled != val) {
		_enabled = val;
		if (_enabled) {
			onDocumentAssetUpdated(data::Subscription::Flag((uint8_t)Asset::FileUpdated));
		}
	}
}
bool Source::isEnabled() const {
	return _enabled;
}

void Source::onDocumentAsset(Asset *a) {
	_documentAsset = a;
	if (_documentAsset) {
		_loadedAssetMTime = 0;
		_documentAsset->syncWithNetwork();
		onDocumentAssetUpdated(data::Subscription::Flag((uint8_t)Asset::FileUpdated));
	}
}

void Source::onDocumentAssetUpdated(data::Subscription::Flags f) {
	if (f.hasFlag((uint8_t)Asset::DownloadFailed)) {
		onError(this, Error::NetworkError);
	}

	if (_documentAsset->isDownloadAvailable() && !_documentAsset->isDownloadInProgress()) {
		if (_enabled) {
			_documentAsset->download();
		}
		onUpdate(this);
	}
	if (_loadedAssetMTime < _documentAsset->getMTime()) {
		tryLoadDocument();
	} else if (f.initial() || f.hasFlag((uint8_t)Asset::Update::FileUpdated)) {
		_loadedAssetMTime = 0;
		tryLoadDocument();
	}
	onDocument(this, _documentAsset.get());
}

static bool Source_tryLockAsset(Asset *a, uint64_t mtime, Source *source) {
	if (a && a->isReadAvailable() &&
			((a->getMTime() > mtime && a->tryReadLock(source) && !a->isDownloadInProgress())
			|| (a && a->isReadAvailable() && mtime == 0))) {
		return true;
	} else {
		return false;
	}
}

void Source::tryLoadDocument() {
	if (!_enabled) {
		return;
	}
	bool assetLocked = false;
	if (Source_tryLockAsset(_documentAsset, _loadedAssetMTime, this)) {
		_loadedAssetMTime = _documentAsset->getMTime();
		assetLocked = true;
	}

	if (_file.empty() && !assetLocked) {
		return;
	}

	auto &thread = TextureCache::thread();
	Rc<Document> *doc = new Rc<Document>(nullptr);

	auto filename = (assetLocked)?_documentAsset->getFilePath():_file;
	auto ct = (assetLocked)?_documentAsset->getContentType():"";

	_documentLoading = true;
	onUpdate(this);

	thread.perform([this, doc, filename, ct] (cocos2d::Ref *) -> bool {
		if (!_string.empty()) {
			*doc = Rc<Document>::create(_string);
		} else if (!filename.empty()) {
			*doc = openDocument(filename, ct);
		}
		return true;
	}, [this, doc, assetLocked, filename] (cocos2d::Ref *, bool success) {
		_documentLoading = false;
		if (assetLocked) {
			_documentAsset->releaseReadLock(this);
		}
		if (success && *doc) {
			onDocumentLoaded(*doc);
		}
		delete doc;
	}, this);
}

void Source::onDocumentLoaded(Document *doc) {
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

void Source::updateDocument() {
	if (_documentAsset && _loadedAssetMTime > 0) {
		_loadedAssetMTime = 0;
	}

	tryLoadDocument();
}

Rc<font::Source> Source::makeSource(AssetMap && map) {
	if (_document) {
		FontFaceMap faceMap(_fontFaces);
		mergeFontFace(faceMap, _document->getFontFaces());
		return Rc<font::Source>::create(std::move(faceMap), _callback, _scale, SearchDirs(_searchDir), std::move(map));
	}
	return Rc<font::Source>::create(FontFaceMap(_fontFaces), _callback, _scale, SearchDirs(_searchDir), std::move(map));
}

Rc<Document> Source::openDocument(const String &path, const String &ct) {
	Rc<Document> ret;
	if (epub::Document::isEpub(path)) {
		ret.set(Rc<epub::Document>::create(FilePath(path)));
	} else {
		ret = Rc<Document>::create(FilePath(path), ct);
	}
	if (ret) {
		if (ret->prepare()) {
			return ret;
		}
	}
	return Rc<Document>();
}

NS_SP_EXT_END(rich_text)
