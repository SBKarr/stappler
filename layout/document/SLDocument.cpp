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

class DocumentFormatStorage {
public:
	static DocumentFormatStorage *getInstance();

	void emplace(Document::DocumentFormat *);
	void erase(Document::DocumentFormat *);

	std::set<Document::DocumentFormat *> get();

private:
	static DocumentFormatStorage *s_sharedInstance;
	std::mutex formatListMutex;
	std::set<Document::DocumentFormat *> formatList;
};

DocumentFormatStorage *DocumentFormatStorage::s_sharedInstance = nullptr;

DocumentFormatStorage *DocumentFormatStorage::getInstance() {
	if (!s_sharedInstance) {
		s_sharedInstance = new DocumentFormatStorage();
	}
	return s_sharedInstance;
}

void DocumentFormatStorage::emplace(Document::DocumentFormat *ptr) {
	formatListMutex.lock();
	formatList.emplace(ptr);
	formatListMutex.unlock();
}

void DocumentFormatStorage::erase(Document::DocumentFormat *ptr) {
	formatListMutex.lock();
	formatList.erase(ptr);
	formatListMutex.unlock();
}

std::set<Document::DocumentFormat *> DocumentFormatStorage::get() {
	std::set<Document::DocumentFormat *> ret;

	formatListMutex.lock();
	ret = formatList;
	formatListMutex.unlock();

	return ret;
}

Document::DocumentFormat::DocumentFormat(check_file_fn chFileFn, load_file_fn ldFileFn, check_data_fn chDataFn, load_data_fn ldDataFn)
: check_data(chDataFn), check_file(chFileFn), load_data(ldDataFn), load_file(ldFileFn) {
	DocumentFormatStorage::getInstance()->emplace(this);
}

Document::DocumentFormat::~DocumentFormat() {
	DocumentFormatStorage::getInstance()->erase(this);
}

bool Document_isAloowedByContentType(const String &ct) {
	StringView ctView(ct);

	return ctView.is("text/html") || ctView.is("multipart/mixed") || ctView.is("multipart/form-data");
}

bool Document_canOpenData(const DataReader<ByteOrder::Network> &data) {
	StringView r((const char *)data.data(), data.size());
	r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
	if (r.is("<html>") || r.is("<!doctype") || r.is("Content-Type: multipart/mixed; boundary=")) {
		return true;
	}

	return false;
}

bool Document_canOpen(const FilePath &path, const String &ct) {
	if (Document_isAloowedByContentType(ct)) {
		return true;
	}

	auto ext = filepath::lastExtension(path.get());
	if (ext == "html" || ext == "htm") {
		return true;
	}

	if (auto file = filesystem::openForReading(path.get())) {
		StackBuffer<256> data;
		if (io::Producer(file).seekAndRead(0, data, 512) > 0) {
			return Document_canOpenData(DataReader<ByteOrder::Network>(data.data(), data.size()));
		}
	}

	return false;
}

bool Document_canOpen(const DataReader<ByteOrder::Network> &data, const String &ct) {
	if (Document_isAloowedByContentType(ct)) {
		return true;
	}

	return Document_canOpenData(data);
}

bool Document::canOpenDocumnt(const FilePath &path, const String &ct) {
	std::set<DocumentFormat *> formatList(DocumentFormatStorage::getInstance()->get());

	for (auto &it : formatList) {
		if (it->check_file && it->check_file(path.get(), ct)) {
			return true;
		}
	}

	return Document_canOpen(path, ct);
}

bool Document::canOpenDocumnt(const DataReader<ByteOrder::Network> &data, const String &ct) {
	std::set<DocumentFormat *> formatList(DocumentFormatStorage::getInstance()->get());

	for (auto &it : formatList) {
		if (it->check_data && it->check_data(data, ct)) {
			return true;
		}
	}

	return Document_canOpen(data, ct);
}

Rc<Document> Document::openDocument(const FilePath &path, const String &ct) {
	Rc<Document> ret;
	std::set<DocumentFormat *> formatList(DocumentFormatStorage::getInstance()->get());

	for (auto &it : formatList) {
		if (it->check_file && it->check_file(path.get(), ct)) {
			ret = it->load_file(path.get(), ct);
			break;
		}
	}
	if (!ret) {
		ret = Rc<Document>::create(path, ct);
	}
	return ret;
}

Rc<Document> Document::openDocument(const DataReader<ByteOrder::Network> &data, const String &ct) {
	Rc<Document> ret;
	std::set<DocumentFormat *> formatList(DocumentFormatStorage::getInstance()->get());

	for (auto &it : formatList) {
		if (it->check_data && it->check_data(data, ct)) {
			ret = it->load_data(data, ct);
			break;
		}
	}
	if (!ret) {
		ret = Rc<Document>::create(data, ct);
	}
	return ret;
}


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

Document::Document() { }

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

bool Document::init(const StringDocument &html) {
	if (html.get().empty()) {
		return false;
	}

	_data = Bytes((const uint8_t *)html.get().data(), (const uint8_t *)(html.get().data() + html.get().size()));
	return init(_data, String());
}

bool Document::init(const FilePath &path, const String &ct) {
	if (path.get().empty()) {
		return false;
	}

	_filePath = path.get();

	auto data = filesystem::readFile(path.get());
	return init(data, ct);
}

bool Document::init(const DataReader<ByteOrder::Network> &vec, const String &ct) {
	if (vec.empty()) {
		return false;
	}
	auto data = vec.data();
	_contentType = ct;
	if (_contentType.compare(0, 9, "text/html") == 0 ||
			(ct.empty() && (memcmp("<html>", data, 6) == 0 || strncasecmp("<!doctype", (const char *)data, 9) == 0))) {
		processHtml("", StringView((const char *)data, vec.size()));
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

		if (!_pages.empty()) {
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

	if (!_pages.empty()) {
		return true;
	}

	return false;
}

void Document::storeData(const DataReader<ByteOrder::Network> &data) {
	_data = Bytes(data.data(), data.data() + data.size());
}

bool Document::prepare() {
	if (!_pages.empty()) {
		updateNodes();
		return true;
	}
	return false;
}

String Document::resolveName(const String &str) {
	StringView r(str);
	r.trimChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();

	if (r.is("url(") && r.back() == ')') {
		r = StringView(r.data() + 4, r.size() - 5);
	}

	r.trimChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();

	if (r.is('\'')) {
		r.trimChars<StringView::Chars<'\''>>();
	} else if (r.is('"')) {
		r.trimChars<StringView::Chars<'"'>>();
	}

	auto f = r.readUntil<StringView::Chars<'?'>>();

	return f.str();
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

static Document::ImageMap::const_iterator getImageFromMap(const Document::ImageMap &images, const String &name) {
	auto it = name.find('?');
	if (it != String::npos) {
		return images.find(name.substr(0, it));
	} else {
		return images.find(name);
	}
}

Bytes Document::getImageData(const String &iname) {
	auto imageIt = getImageFromMap(_images, resolveName(iname));
	if (imageIt != _images.end()) {
		auto &img = imageIt->second;
		if (img.encoding == "base64") {
			auto vec = readData(img.offset, img.length);
			return base64::decode(CoderSource(vec.data(), vec.size()));
		} else {
			return readData(img.offset, img.length);
		}
	}
	return Bytes();
}

Pair<uint16_t, uint16_t> Document::getImageSize(const String &iname) {
	auto imageIt = getImageFromMap(_images, resolveName(iname));
	if (imageIt != _images.end()) {
		return pair(imageIt->second.width, imageIt->second.height);
	}
	return pair(uint16_t(0), uint16_t(0));
}

void Document::updateNodes() {
	NodeId nextId = 0;
	for (auto &it : _pages) {
		ContentPage &page = it.second;
		page.root.foreach([&] (Node &node, size_t level) {
			node.setNodeId(nextId);
			auto htmlId = node.getHtmlId();
			if (!htmlId.empty()) {
				page.ids.insert(pair(htmlId, &node));
				SP_RTDOC_LOG("id : %s", node.getHtmlId().c_str());
			}
			nextId ++;
		});
	}
}

const Vector<String> &Document::getSpine() const {
	return _spine;
}

const ContentPage *Document::getRoot() const {
	if (!_spine.empty()) {
		return getContentPage(_spine.front());
	}

	if (!_pages.empty()) {
		return &_pages.begin()->second;
	}
	return nullptr;
}

const ContentPage *Document::getContentPage(const String &name) const {
	auto it = _pages.find(name);
	if (it != _pages.end()) {
		return &it->second;
	}
	return nullptr;
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

const Map<String, ContentPage> & Document::getContentPages() const {
	return _pages;
}

const Node *Document::getNodeById(const String &path, const String &str) const {
	if (auto page = getContentPage(path)) {
		auto it = page->ids.find(str);
		if (it != page->ids.end()) {
			return it->second;
		}
	}
	return nullptr;
}

Pair<const ContentPage *, const Node *> Document::getNodeByIdGlobal(const String &id) const {
	for (auto &it : _pages) {
		auto &page = it.second;
		auto id_it = page.ids.find(id);
		if (id_it != page.ids.end()) {
			return Pair<const ContentPage *, const Node *>(&page, id_it->second);
		}
	}
	return Pair<const ContentPage *, const Node *>(nullptr, nullptr);
}

void Document::processCss(const String &path, const StringView &str) {
	Reader r;
	auto it = _pages.emplace(path, ContentPage{path, Node("html", path)}).first;
	it->second.queries = style::MediaQuery::getDefaultQueries(it->second.strings);
	if (!r.readCss(it->second, str)) {
		_pages.erase(it);
	}
}

void Document::processHtml(const String &path, const StringView &html, bool linear) {
	Reader r;
	Vector<Pair<String, String>> meta;
	auto it = _pages.emplace(path, ContentPage{path, Node("html", path), linear}).first;
	it->second.queries = style::MediaQuery::getDefaultQueries(it->second.strings);
	if (r.readHtml(it->second, html, meta)) {
		processMeta(it->second, meta);
	} else {
		_pages.erase(it);
	}
}

void Document::processMeta(ContentPage &c, const Vector<Pair<String, String>> &vec) {
	for (auto &it : vec) {
		if (it.first == "gallery") {
			auto git = _gallery.find(it.second);
			if (git == _gallery.end()) {
				_gallery.emplace(it.second, Vector<String>{});
			}
		} else {
			auto git = _gallery.find(it.first);
			if (git != _gallery.end()) {
				StringView reader(it.second);
				while (!reader.empty()) {
					StringView str = reader.readUntil<StringView::Chars<';'>>();
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

Style Document::beginStyle(const Node &node, const Vector<const Node *> &stack, const MediaParameters &media) const {
	const Node *parent = nullptr;
	if (stack.size() > 1) {
		parent = stack.at(stack.size() - 2);
	}

	Style style(style::getStyleForTag(node.getHtmlName(), parent?StringView(parent->getHtmlName()):StringView()));

	auto &attr = node.getAttributes();

	for (auto &it : attr) {
		onStyleAttribute(style, node.getHtmlName(), it.first, it.second, media);
	}

	return style;
}

Style Document::endStyle(const Node &node, const Vector<const Node *> &stack, const MediaParameters &media) const {
	return Style();
}

void Document::onStyleAttribute(Style &style, const StringView &tag, const StringView &name, const StringView &value, const MediaParameters &) const {
	if (name == "align") {
		style.read("text-align", value);
		style.read("text-indent", "0px");
	} else if (name == "width") {
		style.set<style::ParameterName::Width>(style::Metric(StringView(value).readInteger(), style::Metric::Px));
	} else if (name == "height") {
		style.set<style::ParameterName::Height>(style::Metric(StringView(value).readInteger(), style::Metric::Px));
	} else if ((tag == "li" || tag == "ul" || tag == "ol") && name == "type") {
		if (value == "disc") {
			style.set<style::ParameterName::ListStyleType>(style::ListStyleType::Disc);
		} else if (value == "circle") {
			style.set<style::ParameterName::ListStyleType>(style::ListStyleType::Circle);
		} else if (value == "square") {
			style.set<style::ParameterName::ListStyleType>(style::ListStyleType::Square);
		} else if (value == "A") {
			style.set<style::ParameterName::ListStyleType>(style::ListStyleType::UpperAlpha);
		} else if (value == "a") {
			style.set<style::ParameterName::ListStyleType>(style::ListStyleType::LowerAlpha);
		} else if (value == "I") {
			style.set<style::ParameterName::ListStyleType>(style::ListStyleType::UpperRoman);
		} else if (value == "i") {
			style.set<style::ParameterName::ListStyleType>(style::ListStyleType::LowerRoman);
		} else if (value == "1") {
			style.set<style::ParameterName::ListStyleType>(style::ListStyleType::Decimal);
		}
	}
}

NS_LAYOUT_END
