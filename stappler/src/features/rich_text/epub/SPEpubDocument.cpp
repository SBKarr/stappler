/*
 * EpubDocument.cpp
 *
 *  Created on: 11 июл. 2016 г.
 *      Author: sbkarr
 */

#include "SPDefine.h"
#include "SPEpubDocument.h"
#include "SPEpubReader.h"
#include "SPLocale.h"

NS_EPUB_BEGIN

bool Document::isEpub(const String &path) {
	return Info::isEpub(path);
}

Document::Document() { }

bool Document::init(const FilePath &path) {
	_info = Rc<Info>::create(path.get());
	if (_info && _info->valid()) {
		// preprocess css
		auto &manifest = _info->getManifest();
		for (auto &it : manifest) {
			const ManifestFile &file = it.second;
			if (file.type == ManifestFile::Css) {
				auto data = _info->getFileData(it.second);
				if (!data.empty()) {
					processCss(file.path, CharReaderBase((const char *)data.data(), data.size()));
				}
			} else if (file.type == ManifestFile::Image) {
				_images.emplace(it.first, Image(file.width, file.height, file.size, file.path));
			}
		}

		auto &spineRef = _info->getSpine();
		for (const SpineFile &it : spineRef) {
			auto file = _info->getFileData(it);
			if (!file.empty() && it.entry->type == ManifestFile::Source) {
				processHtml(it.entry->path, CharReaderBase((const char *)file.data(), file.size()), it.linear);
			}
		}
		return true;
	}
	return false;
}
bool Document::isFileExists(const String &path) const {
	auto &manifest = _info->getManifest();
	auto fileIt = manifest.find(path);
	if (fileIt != manifest.end()) {
		return true;
	}
	return false;
}
Bytes Document::getFileData(const String &path) {
	auto &manifest = _info->getManifest();
	auto fileIt = manifest.find(path);
	if (fileIt != manifest.end()) {
		auto &f = fileIt->second;
		return _info->getFileData(f);
	}
	return Bytes();
}
Bitmap Document::getImageBitmap(const String &path, const Bitmap::StrideFn &fn) {
	auto &manifest = _info->getManifest();
	auto imageIt = manifest.find(path);
	if (imageIt != manifest.end()) {
		auto &img = imageIt->second;
		auto vec = _info->getFileData(img);
		if (vec.empty()) {
			return Bitmap();
		}

		return Bitmap(vec, fn);
	}
	return Bitmap();
}

bool Document::valid() const {
	return _info->valid();
}
Document::operator bool () const {
	return _info->valid();
}

data::Value Document::encode() const {
	return _info->encode();
}

const String & Document::getUniqueId() const {
	return _info->getUniqueId();
}
const String & Document::getModificationTime() const {
	return _info->getModificationTime();
}
const String & Document::getCoverFile() const {
	return _info->getCoverFile();
}

bool Document::isFileExists(const String &path, const String &root) const {
	return _info->isFileExists(path, root);
}
size_t Document::getFileSize(const String &path, const String &root) const {
	return _info->getFileSize(path, root);
}
Bytes Document::getFileData(const String &path, const String &root) const {
	return _info->getFileData(path, root);
}

bool Document::isImage(const String &path, const String &root) const {
	return _info->isImage(path, root);
}
bool Document::isImage(const String &path, size_t &width, size_t &height, const String &root) const {
	return _info->isImage(path, width, height, root);
}

Vector<SpineFile> Document::getSpine(bool linearOnly) const {
	Vector<SpineFile> ret;
	auto &spineRef = _info->getSpine();

	for (auto &it : spineRef) {
		if (it.linear || !linearOnly) {
			ret.push_back(it);
		}
	}

	return ret;
}

String Document::getTitle() const {
	auto &meta = _info->getMeta();
	for (const TitleMeta &it : meta.titles) {
		if (it.type == TitleMeta::Main) {
			return it.title;
		}
	}
	if (!meta.titles.empty()) {
		return meta.titles.front().title;
	}
	return String();
}
String Document::getCreators() const {
	auto &meta = _info->getMeta();
	bool first = true, sm = false;
	StringStream ret;
	for (const AuthorMeta &it : meta.authors) {
		if (it.type == AuthorMeta::Creator) {
			if (!first) { sm = true; }
			if (first) { first = false; } else { ret << "; "; }
			ret << it.name;
		}
	}
	if (sm) {
		ret << ";";
	}
	return ret.str();
}
String Document::getLanguage() const {
	auto &meta = _info->getMeta();
	for (auto &it : meta.meta) {
		if (it.prop == "language") {
			return locale::common(it.value);
		}
	}
	return String();
}

void Document::processHtml(const String &path, const CharReaderBase &html, bool linear) {
	epub::Reader r;
	Vector<Pair<String, String>> meta;
	_content.emplace_back(rich_text::HtmlPage{path, rich_text::Node(), rich_text::HtmlPage::FontMap{}, linear});
	rich_text::HtmlPage &c = _content.back();

	if (r.readHtml(c, html, _cssStrings, _mediaQueries, meta, _css)) {
		processMeta(c, meta);
	} else {
		_content.pop_back();
	}
}

NS_EPUB_END
