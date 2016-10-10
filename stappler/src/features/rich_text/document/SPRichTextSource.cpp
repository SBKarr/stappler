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
#include "SPEpubDocument.h"

NS_SP_EXT_BEGIN(rich_text)

SP_DECLARE_EVENT(Source, "RichTextSource", onError);
SP_DECLARE_EVENT(Source, "RichTextSource", onUpdate);
SP_DECLARE_EVENT(Source, "RichTextSource", onAsset);

Source::~Source() {
}

bool Source::init(const StringDocument &str) {
	if (!FontSource::init()) {
		return false;
	}

	_documentAsset.setCallback(std::bind(&Source::onDocumentAssetUpdated, this, std::placeholders::_1));
	_string = str.get();
	updateDocument();
	return true;
}
bool Source::init(const FilePath &file) {
	if (!FontSource::init()) {
		return false;
	}

	_documentAsset.setCallback(std::bind(&Source::onDocumentAssetUpdated, this, std::placeholders::_1));
	_file = file.get();
	updateDocument();
	return true;
}

bool Source::init(const String &url, const String &path,
		TimeInterval ttl, const String &cacheDir, const Asset::DownloadCallback &cb) {
	if (!FontSource::init()) {
		return false;
	}

	_documentAsset.setCallback(std::bind(&Source::onDocumentAssetUpdated, this, std::placeholders::_1));
	retain();
	AssetLibrary::getInstance()->getAsset([this] (Asset *a) {
		onDocumentAsset(a);
		release();
	}, url, path, ttl, cacheDir, cb);

	return true;
}
bool Source::init(Asset *a, bool enabled) {
	if (!FontSource::init()) {
		return false;
	}

	_enabled = enabled;
	_documentAsset.setCallback(std::bind(&Source::onDocumentAssetUpdated, this, std::placeholders::_1));
	onDocumentAsset(a);
	return true;
}

void Source::addFontFace(const String &family, style::FontFace &&face) {
	auto it = _defaultFontFaces.find(family);
	if (it == _defaultFontFaces.end()) {
		it = _defaultFontFaces.emplace(family, Vector<style::FontFace>()).first;
	}

	it->second.emplace_back(std::move(face));
}

void Source::addFontFace(const HtmlPage::FontMap &m) {
	for (auto &m_it : m) {
		auto it = _defaultFontFaces.find(m_it.first);
		if (it == _defaultFontFaces.end()) {
			_defaultFontFaces.emplace(m_it.first, m_it.second);
		} else {
			for (auto &v_it : m_it.second) {
				it->second.emplace_back(v_it);
			}
		}
	}
}

Document *Source::getDocument() const {
	return _document;
}
Asset *Source::getAsset() const {
	return _documentAsset;
}

bool Source::isReady() const {
	return _document && _fontSet;
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

void Source::setFontScale(float value) {
	if (value != _fontScale) {
		_fontScale = value;
		updateFontSource();
	}
}

float Source::getFontScale() const {
	return _fontScale;
}

void Source::onDocumentAsset(Asset *a) {
	_documentAsset = a;
	if (_documentAsset) {
		_loadedAssetMTime = 0;
		_documentAsset->syncWithNetwork();
		onDocumentAssetUpdated(data::Subscription::Flag((uint8_t)Asset::FileUpdated));
	}
}

void Source::onDocumentAssetUpdated(Subscription::Flags f) {
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
	onAsset(this, _documentAsset.get());
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
	bool assetLocked = false;
	if (Source_tryLockAsset(_documentAsset, _loadedAssetMTime, this)) {
		_loadedAssetMTime = _documentAsset->getMTime();
		assetLocked = true;
	}

	if (_name.empty() && _file.empty() && !assetLocked) {
		return;
	}

	auto &thread = resource::thread();
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
	}, [this, doc, assetLocked] (cocos2d::Ref *, bool success) {
		_documentLoading = false;
		if (assetLocked) {
			_documentAsset->releaseReadLock(this);
		}
		if (success && *doc) {
			onDocument(*doc);
		}
		delete doc;
	}, this);
}

void Source::onDocument(Document *doc) {
	if (_document != doc) {
		_document = doc;
		if (_document) {
			_receiptCallback = [this, doc] (const String &str) -> Bytes {
				return onReceiptFile(doc, str);
			};
			updateFontSource();
		} else {
			_receiptCallback = nullptr;
			onUpdate(this);
		}
	}
}

void Source::updateFontSource() {
	if (_document) {
		Vector<FontRequest> reqVec;
		auto docConfig = _document->getFontConfig();

		for (auto &it : docConfig) {
			auto &style = it.second;
			FontRequest req(it.first, (uint16_t)roundf(style.style.fontSize * screen::density() * _fontScale), it.second.chars);
			req.receipt = onFontRequest(style);
			reqVec.push_back(std::move(req));
		}

		setRequest(std::move(reqVec));
	}
}

void Source::updateDocument() {
	if (_documentAsset && _loadedAssetMTime > 0) {
		_loadedAssetMTime = 0;
	}

	tryLoadDocument();
}

void Source::onFontSet(FontSet *set) {
	FontSource::onFontSet(set);
	onUpdate(this);
}

void Source::updateRequest() {
	FontSource::updateRequest();
	onUpdate(this);
}

Rc<Document> Source::openDocument(const String &path, const String &ct) {
	Rc<Document> ret;
	if (epub::Document::isEpub(path)) {
		ret.set(Rc<epub::Document>::create(FilePath(path)));
	} else {
		ret = Rc<Document>::create(FilePath(path), ct);
	}
	if (ret) {
		ret->setDefaultFontFaces(_defaultFontFaces);
		if (ret->prepare()) {
			return ret;
		}
	}
	return Rc<Document>();
}

FontRequest::Receipt Source::onFontRequest(const Document::FontConfigValue &style) {
	FontRequest::Receipt ret;
	if (!style.face.empty()) {
		for (auto faceIt = style.face.rbegin(); faceIt != style.face.rend(); faceIt ++) {
			auto &srcs = (*faceIt)->src;
			for (auto it = srcs.rbegin(); it != srcs.rend(); it ++) {
				ret.push_back(*it);
			}
		}
	}
	return ret;
}

Bytes Source::onReceiptFile(Document *doc, const String &str) {
	if (str.compare(0, "document://"_len, "document://") == 0) {
		return doc->getFileData(str.substr("document://"_len));
	} else if (str.compare(0, "local://"_len, "local://") == 0) {
		auto path = str.substr("local://"_len);
		if (filesystem::exists(path)) {
			return filesystem::readFile(path);
		} else if (filesystem::exists("fonts/" + path)) {
			return filesystem::readFile("fonts/" + path);
		} else if (filesystem::exists("common/fonts/" + path)) {
			return filesystem::readFile("common/fonts/" + path);
		}
	} else {
		if (filesystem::exists(str)) {
			return filesystem::readFile(str);
		} else if (filesystem::exists(filepath::merge("fonts/", str))) {
			return filesystem::readFile(filepath::merge("fonts/", str));
		} else if (filesystem::exists(filepath::merge("common/fonts/", str))) {
			return filesystem::readFile(filepath::merge("common/fonts/", str));
		}
	}
	return Bytes();
}

NS_SP_EXT_END(rich_text)
