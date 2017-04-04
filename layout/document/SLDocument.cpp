// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2016-2017 Roman Katuntsev <sbkarr@stappler.org>

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
#include "SLFont.h"
#include "SLDocument.h"
#include "SLRendererTypes.h"
#include "SLParser.h"
#include "SLReader.h"
#include "SPString.h"
#include "SPFilesystem.h"

#include "SLMultipartParser.h"

//#define SP_RTDOC_LOG(...) log::format("Document", __VA_ARGS__)
#define SP_RTDOC_LOG(...)

NS_LAYOUT_BEGIN

class Document_RenderInterface : public RendererInterface {
public:
	Document_RenderInterface(Document *d, const Vector<MediaQueryId> *m)
	: doc(d), media(m) { }

	virtual bool resolveMediaQuery(MediaQueryId queryId) const override {
		for (auto &it : (*media)) {
			if (it == queryId) {
				return true;
			}
		}
		return false;
	}

	virtual String getCssString(CssStringId stringId) const override {
		return doc->getCssString(stringId);
	}

protected:
	Document *doc = nullptr;
	const Vector<MediaQueryId> *media;
};

Document::Image::Image(const MultipartParser::Image &img)
: type(Type::Embed), width(img.width), height(img.height), offset(img.offset), length(img.length)
, encoding(img.encoding), name(img.name), ref("embed://" + img.name) { }

Document::Image::Image(uint16_t width, uint16_t height, size_t size, const String &path)
: type(Type::Embed), width(width), height(height), offset(0), length(size), name(path), ref("embed://" + path) { }

String Document::getImageName(const String &name) {
	String src(resolveName(name));
	auto pos = src.find('?');
	if (pos != String::npos) {
		src = src.substr(0, pos);
	}
	return src;
}

Document::Document() {
	_mediaQueries = style::MediaQuery::getDefaultQueries(_cssStrings);
}

Vector<String> Document::getImageOptions(const String &isrc) {
	String src(resolveName(isrc));
	auto pos = src.find('?');
	if (pos != String::npos && pos < src.length() - 1) {
		Vector<String> ret;

		StringReader s(src.substr(pos + 1));
		StringReader opt = s.readUntil<Chars<'&', ';'>>();
		while (!opt.empty()) {
			s ++;
			ret.push_back(opt.str());
			opt = s.readUntil<Chars<'&', ';'>>();
		}
		return ret;
	}
	return Vector<String>();
}

bool Document::init(const String &html) {
	if (html.empty()) {
		return false;
	}

	_data = Bytes((const uint8_t *)html.data(), (const uint8_t *)(html.data() + html.size()));
	processHtml("", CharReaderBase(html));
	if (_content.empty()) {
		return false;
	}

	return true;
}

bool Document::init(const FilePath &path, const String &ct) {
	if (path.get().empty()) {
		return false;
	}

	_filePath = path.get();
	return init(filesystem::readFile(path.get()), ct);
}

bool Document::init(const Bytes &vec, const String &ct) {
	if (vec.empty()) {
		return false;
	}
	auto data = vec.data();
	_contentType = ct;
	if (_contentType.compare(0, 9, "text/html") == 0 ||
			(ct.empty() && (memcmp("<html>", data, 6) == 0 || strncasecmp("<!doctype", (const char *)data, 9) == 0))) {
		processHtml("", CharReaderBase((const char *)data, vec.size()));
		updateNodes();
	} else if (_contentType.compare(0, 15, "multipart/mixed") == 0 || _contentType.compare(0, 19, "multipart/form-data") == 0 || _contentType.empty()) {
		MultipartParser parser;
		if (!ct.empty()) {
			if (!parser.parse(vec, ct, false)) {
				return false;
			}
		} else {
			if (!parser.parse(vec, false)) {
				return false;
			}
		}

		if (parser.html.empty()) {
			return false;
		}

		processHtml("", parser.html);

		if (!_content.empty()) {
			for (auto &it : parser.images) {
				String name = "embed://" + it.name;
				SP_RTDOC_LOG("image : %s %d %d %lu %lu", it.name.c_str(), it.width, it.height, it.offset, it.length);

				auto imgIt = _images.find(name);
				if (imgIt == _images.end()) {
					_images.insert(pair(name, it));
				}
			}
		}
	}


	return true;
}

bool Document::prepare() {
	if (!_content.empty()) {
		updateNodes();
		return true;
	}
	return false;
}

String Document::resolveName(const String &str) {
	CharReaderBase r(str);
	r.trimChars<CharReaderBase::CharGroup<CharGroupId::WhiteSpace>>();

	if (r.is("url(") && r.back() == ')') {
		r = CharReaderBase(r.data() + 4, r.size() - 5);
	}

	r.trimChars<CharReaderBase::CharGroup<CharGroupId::WhiteSpace>>();

	if (r.is('\'')) {
		r.trimChars<CharReaderBase::Chars<'\''>>();
	} else if (r.is('"')) {
		r.trimChars<CharReaderBase::Chars<'"'>>();
	}

	return r.str();
}

Bytes Document::readData(size_t offset, size_t len) {
	if (!_data.empty()) {
		if (_data.size() <= (offset + len)) {
			auto str = _data.data() + offset;
			return Bytes((const uint8_t *)str, (const uint8_t *)(str + len));
		}
		return Bytes();
	}

	if (!_filePath.empty()) {
		return filesystem::readFile(_filePath, offset, len);
	}

	return Bytes();
}

const Document::FontFaceMap &Document::getFontFaces() const {
	return _fontFaces;
}
bool Document::isFileExists(const String &iname) const {
	String name(resolveName(iname));
	auto imageIt = _images.find(name);
	if (imageIt != _images.end()) {
		return true;
	}
	return false;
}

Bytes Document::getFileData(const String &iname) {
	String name(resolveName(iname));
	auto imageIt = _images.find(name);
	if (imageIt != _images.end()) {
		auto &img = imageIt->second;
		return readData(img.offset, img.length);
	}
	return Bytes();
}

Bitmap Document::getImageBitmap(const String &iname, const Bitmap::StrideFn &fn) {
	String name(resolveName(iname));
	auto it = name.find('?');
	ImageMap::const_iterator imageIt;
	if (it != String::npos) {
		imageIt = _images.find(name.substr(0, it));
	} else {
		imageIt = _images.find(name);
	}
	if (imageIt != _images.end()) {
		auto &img = imageIt->second;
		auto vec = readData(img.offset, img.length);
		if (img.encoding == "base64") {
			vec = base64::decode((const char *)vec.data(), vec.size());
		}

		if (vec.empty()) {
			return Bitmap();
		}

		return Bitmap(vec, fn);
	}
	return Bitmap();
}

void Document::updateNodes() {
	_ids.clear();

	NodeId nextId = 0;
	for (auto &it : _content) {
		it.root.foreach([&] (Node &node, size_t level) {
			node.setNodeId(nextId);
			auto htmlId = node.getHtmlId();
			if (!htmlId.empty()) {
				_ids.insert(pair(htmlId, &node));
				SP_RTDOC_LOG("id : %s", node.getHtmlId().c_str());
			}
			nextId ++;
		});
	}
}

const Node &Document::getRoot() const {
	return _content.front().root;
}

const Vector<HtmlPage> &Document::getContent() const {
	return _content;
}

const HtmlPage *Document::getContentPage(const String &name) const {
	for (auto &it : _content) {
		if (it.path == name) {
			return &it;
		}
	}
	return nullptr;
}

const Vector<style::MediaQuery> &Document::getMediaQueries() const {
	return _mediaQueries;
}
const Map<CssStringId, String> &Document::getCssStrings() const {
	return _cssStrings;
}

String Document::getCssString(CssStringId id) const {
	auto it = _cssStrings.find(id);
	if (it != _cssStrings.end()) {
		return it->second;
	}
	return "";
}

bool Document::hasImage(const String &name) const {
	return _images.find(getImageName(name)) != _images.end();
}

Pair<uint16_t, uint16_t> Document::getImageSize(const String &name) const {
	auto src = getImageName(name);

	auto it = _images.find(src);
	if (it == _images.end()) {
		return pair((uint16_t)0, (uint16_t)0);
	} else {
		return pair((uint16_t)it->second.width, (uint16_t)it->second.height);
	}
}

const Document::ImageMap & Document::getImages() const {
	return _images;
}

const Document::GalleryMap & Document::getGalleryMap() const {
	return _gallery;
}

const Document::ContentRecord & Document::getTableOfContents() const {
	return _contents;
}

const Node *Document::getNodeById(const String &str) const {
	auto it = _ids.find(str);
	if (it != _ids.end()) {
		return it->second;
	}
	return nullptr;
}

void Document::processCss(const String &path, const CharReaderBase &str) {
	Reader r;
	auto it = _css.emplace(path, r.readCss(path, str, _cssStrings, _mediaQueries)).first;
	FontSource::mergeFontFace(_fontFaces, it->second.fonts);
}

void Document::processHtml(const String &path, const CharReaderBase &html, bool linear) {
	Reader r;
	Vector<Pair<String, String>> meta;
	_content.emplace_back(HtmlPage{path, Node("html", path), HtmlPage::FontMap{}, linear});
	HtmlPage &c = _content.back();

	if (r.readHtml(c, html, _cssStrings, _mediaQueries, meta, _css)) {
		FontSource::mergeFontFace(_fontFaces, c.fonts);
		processMeta(c, meta);
	} else {
		_content.pop_back();
	}
}

void Document::processMeta(HtmlPage &c, const Vector<Pair<String, String>> &vec) {
	for (auto &it : vec) {
		if (it.first == "gallery") {
			auto git = _gallery.find(it.second);
			if (git == _gallery.end()) {
				_gallery.emplace(it.second, Vector<String>{});
			}
		} else {
			auto git = _gallery.find(it.first);
			if (git != _gallery.end()) {
				CharReaderBase reader(it.second);
				while (!reader.empty()) {
					CharReaderBase str = reader.readUntil<CharReaderBase::Chars<';'>>();
					if (!str.empty()) {
						git->second.emplace_back(str.str());
					}
					if (reader.is(';')) {
						++ reader;
					}
				}
			}
		}
	}
}

NS_LAYOUT_END
