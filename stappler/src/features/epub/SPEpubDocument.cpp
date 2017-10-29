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

#include "SPLayout.h"
#include "SPEpubDocument.h"
#include "SPEpubReader.h"
#include "SPLocale.h"
#include "SPHtmlParser.h"
#include "SLFont.h"

NS_EPUB_BEGIN

bool Document::isEpub(const String &path) {
	return Info::isEpub(path);
}

Document::Document() { }

bool Document::init(const FilePath &path) {
	_info = Rc<Info>::create(path.get());
	if (_info && _info->valid()) {
		auto &tocFile = _info->getTocFile();
		if (!tocFile.empty()) {
			readTocFile(tocFile);
		}

		// preprocess css
		auto &manifest = _info->getManifest();
		for (auto &it : manifest) {
			const ManifestFile &file = it.second;
			if (file.type == ManifestFile::Css) {
				auto data = _info->getFileData(it.second);
				if (!data.empty()) {
					processCss(file.path, StringView((const char *)data.data(), data.size()));
				}
			} else if (file.type == ManifestFile::Image) {
				_images.emplace(it.first, Image(file.width, file.height, file.size, file.path));
			}
		}

		auto &spineRef = _info->getSpine();
		for (const SpineFile &it : spineRef) {
			auto file = _info->getFileData(it);
			if (!file.empty() && it.entry->type == ManifestFile::Source) {
				processHtml(it.entry->path, StringView((const char *)file.data(), file.size()), it.linear);
			}
		}
		return true;
	}
	return false;
}
bool Document::isFileExists(const String &ipath) const {
	String path(resolveName(ipath));
	auto &manifest = _info->getManifest();
	auto fileIt = manifest.find(path);
	if (fileIt != manifest.end()) {
		return true;
	}
	return false;
}
Bytes Document::getFileData(const String &ipath) {
	String path(resolveName(ipath));
	auto &manifest = _info->getManifest();
	auto fileIt = manifest.find(path);
	if (fileIt != manifest.end()) {
		auto &f = fileIt->second;
		return _info->getFileData(f);
	}
	return Bytes();
}
Bitmap Document::getImageBitmap(const String &ipath, const Bitmap::StrideFn &fn) {
	String path(resolveName(ipath));
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
		if (it.name == "language") {
			return locale::common(it.value);
		}
	}
	return String();
}

void Document::processHtml(const String &path, const StringView &html, bool linear) {
	epub::Reader r;
	Vector<Pair<String, String>> meta;
	_content.emplace_back(layout::HtmlPage{path, layout::Node("html", path), layout::HtmlPage::FontMap{}, linear});
	layout::HtmlPage &c = _content.back();

	if (r.readHtml(c, html, _cssStrings, _mediaQueries, meta, _css)) {
		layout::FontSource::mergeFontFace(_fontFaces, c.fonts);
		processMeta(c, meta);
	} else {
		_content.pop_back();
	}
}

void Document::readTocFile(const String &fileName) {
	auto &manifest = _info->getManifest();
	auto fileIt = manifest.find(fileName);
	if (fileIt == manifest.end()) {
		return;
	}

	const ManifestFile &file = fileIt->second;
	if (file.mime == "application/x-dtbncx+xml") {
		readNcxNav(file.path);
	} else if (file.mime == "application/xhtml+xml" || file.mime == "application/xhtml" || file.mime == "text/html") {
		readXmlNav(file.path);
	}
}

void Document::readNcxNav(const String &filePath) {
	auto toc = _info->getFileData(filePath, "");
	struct NcxReader {
		using Parser = html::Parser<NcxReader>;
		using Tag = Parser::Tag;
		using StringReader = Parser::StringReader;

		enum Section {
			None,
			Ncx,
			Head,
			DocTitle,
			NavMap,
			NavPoint,
			NavPointLabel,
		} section = None;

		Info *info;
		String path;
		Vector<ContentRecord *> contents;

		NcxReader(Info *info, const String &path, ContentRecord *c) : info(info), path(path) { contents.push_back(c); }

		inline void onTagAttribute(Parser &p, Tag &tag, StringReader &name, StringReader &value) {
			switch (section) {
			case NavPoint:
				if (tag.name.compare("content") && name.compare("src")) {
					contents.back()->href = info->resolvePath(value.str(), path);
				}
				break;
			default: break;
			}
		}

		inline void onPushTag(Parser &p, Tag &tag) {
			switch (section) {
			case None:
				if (tag.name.compare("ncx")) {
					section = Ncx;
				}
				break;
			case Ncx:
				if (tag.name.compare("head")) {
					section = Head;
				} else if (tag.name.compare("doctitle")) {
					section = DocTitle;
				} else if (tag.name.compare("navmap")) {
					section = NavMap;
				}
				break;
			case Head: break;
			case DocTitle: break;
			case NavMap:
				if (tag.name.compare("navpoint")) {
					section = NavPoint;
					contents.back()->childs.emplace_back(ContentRecord());
					contents.emplace_back(&contents.back()->childs.back());
				}
				break;
			case NavPoint:
				if (tag.name.compare("navpoint")) {
					section = NavPoint;
					contents.back()->childs.emplace_back(ContentRecord());
					contents.emplace_back(&contents.back()->childs.back());
				} else if (tag.name.compare("navlabel")) {
					section = NavPointLabel;
				}
				break;
			default: break;
			}
		}

		inline void onPopTag(Parser &p, Tag &tag) {
			switch (section) {
			case None: break;
			case Ncx:
				if (tag.name.compare("ncx")) {
					section = None;
				}
				break;
			case Head:
				if (tag.name.compare("head")) {
					section = Ncx;
				}
				break;
			case DocTitle:
				if (tag.name.compare("doctitle")) {
					section = Ncx;
				}
				break;
			case NavMap:
				if (tag.name.compare("navmap")) {
					section = Ncx;
				}
				break;
			case NavPoint:
				if (tag.name.compare("navpoint")) {
					contents.pop_back();
					if (p.tagStack.at(p.tagStack.size() - 2).name.compare("navmap")) {
						section = NavMap;
					}
				}
				break;
			case NavPointLabel:
				if (tag.name.compare("navlabel")) {
					section = NavPoint;
				}
				break;
			default: break;
			}
		}

		inline void onTagContent(Parser &p, Tag &tag, StringReader &s) {
			switch (section) {
			case DocTitle:
			case NavPointLabel:
				if (tag.name.compare("text")) {
					contents.back()->label = s.str();
					string::trim(contents.back()->label);
				}
				break;
			default: break;
			}
		}
	} r(_info, filePath, &_contents);

	html::parse(r, StringViewUtf8((const char *)toc.data(), toc.size()));

	if (_contents.label.empty()) {
		_contents.label = getTitle();
	}
}

void Document::readXmlNav(const String &filePath) {
	auto toc = _info->getFileData(filePath, "");
	struct TocReader {
		using Parser = html::Parser<TocReader>;
		using Tag = Parser::Tag;
		using StringReader = Parser::StringReader;

		enum Section {
			None,
			PreNav,
			Nav,
			Heading,
			Ol,
			Li,
		} section = None;

		Info *info;
		String path;
		Vector<ContentRecord *> contents;

		TocReader(Info *info, const String &path, ContentRecord *c) : info(info), path(path) { contents.push_back(c); }

		inline void onTagAttribute(Parser &p, Tag &tag, StringReader &name, StringReader &value) {
			switch (section) {
			case None:
				if (tag.name.compare("nav") && name.compare("epub:type") && value.compare("toc")) {
					section = PreNav;
				}
				break;
			case Li:
				if (tag.name.compare("a") && name.compare("href")) {
					contents.back()->href = info->resolvePath(value.str(), path);
				}
				break;
			case Heading:
				if (name.compare("title") || name.compare("alt")) {
					contents.back()->label += value.str();
				}
				break;
			default: break;
			}
		}

		inline void onPushTag(Parser &p, Tag &tag) {
			switch (section) {
			case PreNav:
				if (tag.name.compare("nav")) {
					section = Nav;
				}
				break;
			case Nav:
				if (tag.name.compare("h1") || tag.name.compare("h2") || tag.name.compare("h3")
						|| tag.name.compare("h4") || tag.name.compare("h5") || tag.name.compare("h6")) {
					section = Heading;
				} else if (tag.name.compare("ol")) {
					section = Ol;
				}
				break;
			case Ol:
				if (tag.name.compare("li")) {
					section = Li;
					contents.back()->childs.emplace_back(ContentRecord());
					contents.emplace_back(&contents.back()->childs.back());
				}
			case Li:
				if (tag.name.compare("a") || tag.name.compare("span")) {
					section = Heading;
				} else if (tag.name.compare("ol")) {
					section = Ol;
				}
				break;
			default: break;
			}
		}

		inline void onPopTag(Parser &p, Tag &tag) {
			switch (section) {
			case Nav:
				if (tag.name.compare("nav")) {
					section = None;
				}
				break;
			case Heading:
			case Ol:
				if (tag.name.compare("h1") || tag.name.compare("h2") || tag.name.compare("h3")
						|| tag.name.compare("h4") || tag.name.compare("h5") || tag.name.compare("h6")
						|| tag.name.compare("ol") || tag.name.compare("a") || tag.name.compare("span")) {
					auto &last = p.tagStack.at(p.tagStack.size() - 2);
					if (last.name.compare("nav")) {
						section = Nav;
					} else if (last.name.compare("li")) {
						section = Li;
					}
				}
				break;
			case Li:
				if (tag.name.compare("li")) {
					contents.pop_back();
					section = Ol;
				}
				break;
			default: break;
			}
		}

		inline void onTagContent(Parser &p, Tag &tag, StringReader &s) {
			switch (section) {
			case Heading:
				contents.back()->label += s.str();
				break;
			default: break;
			}
		}
	} r(_info, filePath, &_contents);

	html::parse(r, StringViewUtf8((const char *)toc.data(), toc.size()));

	if (_contents.label.empty()) {
		_contents.label = getTitle();
	}
}

data::Value Document::encodeContents(const ContentRecord &rec) {
	data::Value ret;
	ret.setString(rec.label, "label");
	ret.setString(rec.href, "href");
	if (!rec.childs.empty()) {
		data::Value &childs = ret.emplace("childs");
		for (auto &it : rec.childs) {
			childs.addValue(encodeContents(it));
		}
	}
	return ret;
}

NS_EPUB_END
