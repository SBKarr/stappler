/*
 * SPHtmlDocument.cpp
 *
 *  Created on: 14 апр. 2015 г.
 *      Author: sbkarr
 */

#include "SPDefine.h"
#include "SPRichTextDocument.h"
#include "SPRichTextParser.h"
#include "SPString.h"
#include "SPFilesystem.h"

#include "SPRichTextReader.h"

#include "SPRichTextRendererTypes.h"
#include "SPRichTextMultipartParser.h"

//#define SP_RTDOC_LOG(...) log::format("Document", __VA_ARGS__)
#define SP_RTDOC_LOG(...)

NS_SP_EXT_BEGIN(rich_text)

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

static style::FontStyleParameters Document_compileFont(
		Document *doc,
		const style::ParameterList &spanStyle,
		const Vector<MediaQueryId> &media) {
	Document_RenderInterface iface(doc, &media);
	return spanStyle.compileFontStyle(&iface);
}

static bool Document_addStyle(Vector<style::FontStyleParameters> &vec, const style::FontStyleParameters &style) {
	for (auto &it : vec) {
		if (it == style) {
			return false;
		}
	}
	vec.push_back(style);
	return true;
}

static bool gen_comb_norep_lex_init(Vector<MediaQueryId> &vector, MediaQueryId n, MediaQueryId k) {
	for(MediaQueryId j = 0; j < k; j++) {
		vector.push_back(j);
	}

	return true;
}

static bool gen_comb_norep_lex_next(Vector<MediaQueryId> &vector, MediaQueryId n, MediaQueryId k) {
	int32_t j; //index
	if (vector[k - 1] < n - 1) {
		vector[k - 1] ++;
		return true;
	}

	for (j = k - 2; j >= 0; j--) {
		if (vector[j] < n - k + j) {
			break;
		}
	}

	if(j < 0) {
		return false;
	}

	vector[j]++;
	while(j < k - 1) {
		vector[j + 1] = vector[j] + 1;
		j++;
	}

	return true;
}

static size_t Document_fontFaceScore(const style::FontStyleParameters &params, const style::FontFace &face) {
	size_t ret = 0;
	if (face.fontStyle == style::FontStyle::Normal) {
		ret += 1;
	}
	if (face.fontWeight == style::FontWeight::Normal) {
		ret += 1;
	}
	if (face.fontStretch == style::FontStretch::Normal) {
		ret += 1;
	}

	if (face.fontStyle == params.fontStyle && (face.fontStyle == style::FontStyle::Oblique || face.fontStyle == style::FontStyle::Italic)) {
		ret += 100;
	} else if ((face.fontStyle == style::FontStyle::Oblique || face.fontStyle == style::FontStyle::Italic)
			&& (params.fontStyle == style::FontStyle::Oblique || params.fontStyle == style::FontStyle::Italic)) {
		ret += 75;
	}

	auto weightDiff = (int)toInt(style::FontWeight::W900) - abs((int)toInt(params.fontWeight) - (int)toInt(face.fontWeight));
	ret += weightDiff * 10;

	auto stretchDiff = (int)toInt(style::FontStretch::UltraExpanded) - abs((int)toInt(params.fontStretch) - (int)toInt(face.fontStretch));
	ret += stretchDiff * 5;

	return ret;
}

static const style::FontFace *Document_selectFontFaceName(const String &name, const style::FontStyleParameters &params, const HtmlPage::FontMap &page, const HtmlPage::FontMap &def) {
	const style::FontFace *ret = nullptr;
	size_t retScore = 0;

	auto faceIt = page.find(name);
	if (faceIt != page.end()) {
		for (auto &fit : faceIt->second) {
			auto fscore = Document_fontFaceScore(params, fit);
			if (fscore > retScore) {
				ret = &fit;
				retScore = fscore;
			}
		}
	}

	faceIt = def.find(name);
	if (faceIt != def.end()) {
		for (auto &fit : faceIt->second) {
			auto fscore = Document_fontFaceScore(params, fit);
			if (fscore > retScore) {
				ret = &fit;
				retScore = fscore;
			}
		}
	}

	if (retScore == 0) {
		return nullptr;
	}

	if (ret) {
		SP_RTDOC_LOG("FontFace: %s : select %s : %lu", params.getConfigName().c_str(), ret->src.front().c_str(), retScore);
	}

	return ret;
}

static void Document_initFontSet(const FontStyle &style, Set<char16_t> &set) {
	set.insert(u' ');
	set.insert(char16_t(0x00AD));
	set.insert(char16_t(8226));
}

Document::Image::Image(const MultipartParser::Image &img)
: type(Type::Embed), width(img.width), height(img.height), offset(img.offset), length(img.length)
, encoding(img.encoding), name(img.name), ref("embed://" + img.name) { }

Document::Image::Image(uint16_t width, uint16_t height, size_t size, const String &path)
: type(Type::Embed), width(width), height(height), offset(0), length(size), name(path), ref("embed://" + path) { }

String Document::getImageName(const String &name) {
	auto src = name;
	auto pos = src.find('?');
	if (pos != String::npos) {
		src = src.substr(0, pos);
	}
	return src;
}

Document::Document() {
	_mediaQueries = style::MediaQuery::getDefaultQueries(_cssStrings);
}

Vector<String> Document::getImageOptions(const String &src) {
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

bool Document::isFileExists(const String &name) const {
	auto imageIt = _images.find(name);
	if (imageIt != _images.end()) {
		return true;
	}
	return false;
}

Bytes Document::getFileData(const String &name) {
	auto imageIt = _images.find(name);
	if (imageIt != _images.end()) {
		auto &img = imageIt->second;
		return readData(img.offset, img.length);
	}
	return Bytes();
}

Bitmap Document::getImageBitmap(const String &name, const Bitmap::StrideFn &fn) {
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

void Document::setDefaultFontFaces(const HtmlPage::FontMap &map) {
	_defaultFontFaces = map;
}
void Document::setDefaultFontFaces(HtmlPage::FontMap &&map) {
	_defaultFontFaces = std::move(map);
}

Document::~Document() { }

void Document::updateNodes() {
	_fontConfig.clear();
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

			processLists(_fontConfig, node, it.fonts);
			processSpans(_fontConfig, node, it.fonts);
		});
	}

	// default chars
	FontStyle defStyle; defStyle.fontFamily = "default";
	auto name = defStyle.getConfigName();
	auto fontsIt = _fontConfig.find(name);
	if (fontsIt == _fontConfig.end()) {
		fontsIt = _fontConfig.emplace(name, FontConfigValue{defStyle, selectFontFace(defStyle, _defaultFontFaces, _defaultFontFaces)}).first;
		Document_initFontSet(defStyle, fontsIt->second.set);
	}

	auto &set = fontsIt->second.set;
	for (auto c = u'0'; c <= u'9'; ++ c) {
		set.insert(c);
	}

	for (auto &it : _fontConfig) {
		it.second.chars.reserve(it.second.set.size() + 1);
		for (auto &c : it.second.set) {
			it.second.chars.push_back(c);
		}
		it.second.set.clear();

		SP_RTDOC_LOG("%s: %s", it.first.c_str(), string::toUtf8(it.second.chars).c_str());
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

const Document::FontConfig &Document::getFontConfig() const {
	return _fontConfig;
}

bool Document::resolveMediaQuery(MediaQueryId queryId) const {
	return false;
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

Vector<const style::FontFace *> Document::selectFontFace(const style::FontStyleParameters &params, const HtmlPage::FontMap &page, const HtmlPage::FontMap &def) {
	Vector<const style::FontFace *> ret;

	string::split(params.fontFamily, ",", [&] (const CharReaderBase &ir) {
		CharReaderBase r(ir);
		r.skipChars<CharReaderBase::CharGroup<chars::CharGroupId::WhiteSpace>>();
		if (r[0] == '"') {
			++ r;
			auto str = r.readUntil<CharReaderBase::Chars<'"'>>();
			if (!str.empty()) {
				const style::FontFace *fc = Document_selectFontFaceName(str.str(), params, page, def);
				if (fc) {
					ret.push_back(fc);
				}
			}
		} else if (r[0] == '\'') {
			++ r;
			auto str = r.readUntil<CharReaderBase::Chars<'\''>>();
			if (!str.empty()) {
				const style::FontFace *fc = Document_selectFontFaceName(str.str(), params, page, def);
				if (fc) {
					ret.push_back(fc);
				}
			}
		} else {
			auto str = r.readUntil<CharReaderBase::CharGroup<chars::CharGroupId::WhiteSpace>>();
			if (!str.empty()) {
				const style::FontFace *fc = Document_selectFontFaceName(str.str(), params, page, def);
				if (fc) {
					ret.push_back(fc);
				}
			}
		}
	});
	if (params.fontFamily != "default") {
		auto dfc = Document_selectFontFaceName("default", params, page, def);
		if (dfc) {
			ret.push_back(dfc);
		}
	}
	return ret;
}

void Document::processLists(FontConfig &fonts, const Node &node, const HtmlPage::FontMap &fontFaces) {
	auto display = node.getStyle().get(style::ParameterName::Display);
	if (display.empty()) {
		return;
	}

	if (node.getHtmlName() == "ul" || node.getHtmlName() == "ol") {
		auto &nodes = node.getNodes();
		for (auto &nit : nodes) {
			auto &style = nit.getStyle();
			auto d = style.get(style::ParameterName::Display);
			auto l = style.get(style::ParameterName::ListStyleType);
			for (auto &dit : d) {
				if (dit.value.display == style::Display::ListItem) {
					for (auto &lit : l) {
						auto t = lit.value.listStyleType;
						if (t != style::ListStyleType::None) {
							forEachFontStyle(style, [&] (const style::FontStyleParameters &params) {
								processList(fonts, t, params, fontFaces);
							});
						}
					}
				}
			}
		}
	}
}

void Document::processList(FontConfig &fonts, style::ListStyleType t, const style::FontStyleParameters &params, const HtmlPage::FontMap &fontFaces) {
	auto name = params.getConfigName();
	auto fontsIt = fonts.find(name);
	if (fontsIt == fonts.end()) {
		fontsIt = fonts.emplace(name, FontConfigValue{params, selectFontFace(params, fontFaces, _defaultFontFaces)}).first;
		Document_initFontSet(params, fontsIt->second.set);
	}

	if (params.listStyleType != style::ListStyleType::None) {
		auto &chars = fontsIt->second.set;
		switch (params.listStyleType) {
		case style::ListStyleType::None: break;
		case style::ListStyleType::Circle: chars.insert(char16_t(8226)); break;
		case style::ListStyleType::Disc: chars.insert(char16_t(9702)); break;
		case style::ListStyleType::Square: chars.insert(char16_t(9632)); break;
		case style::ListStyleType::XMdash: chars.insert(char16_t(8212)); break;
		case style::ListStyleType::Decimal:
		case style::ListStyleType::DecimalLeadingZero: for (char16_t i = u'0'; i <= u'9'; i ++) { chars.insert(i); } break;
		case style::ListStyleType::LowerAlpha: for (char16_t i = u'a'; i <= u'z'; i ++) { chars.insert(i); } break;
		case style::ListStyleType::LowerGreek: for (char16_t i = 0x03B1; i <= 0x03C9; i ++) { chars.insert(i); } break;
		case style::ListStyleType::LowerRoman: for (char16_t i = 0x2170; i <= 0x217F; i ++) { chars.insert(i); } break;
		case style::ListStyleType::UpperAlpha: for (char16_t i = u'A'; i <= u'Z'; i ++) { chars.insert(i); } break;
		case style::ListStyleType::UpperRoman: for (char16_t i = 0x2160; i <= 0x216F; i ++) { chars.insert(i); } break;
		}

		chars.insert(u'.');
		chars.insert(u'-');
	}
}

void Document::processSpans(FontConfig &fonts, const Node &node, const HtmlPage::FontMap &fontFaces) {
	forEachFontStyle(node.getStyle(), [&] (const style::FontStyleParameters &params) {
		processSpan(fonts, node, params, fontFaces);
	});
}

void Document::processSpan(FontConfig &fonts, const Node &node, const style::FontStyleParameters &params, const HtmlPage::FontMap &fontFaces) {
	auto name = params.getConfigName();
	auto fontsIt = fonts.find(name);
	if (fontsIt == fonts.end()) {
		fontsIt = fonts.emplace(name, FontConfigValue{params, selectFontFace(params, fontFaces, _defaultFontFaces)}).first;
		Document_initFontSet(params, fontsIt->second.set);
	}

	auto altFontsIt = fonts.end();
	if (params.fontVariant == style::FontVariant::SmallCaps) {
		auto altName = params.getConfigName(true);
		altFontsIt = fonts.find(altName);
		if (altFontsIt == fonts.end()) {
			auto capsParams = params.getSmallCaps();
			altFontsIt = fonts.emplace(altName, FontConfigValue{capsParams, selectFontFace(capsParams, fontFaces, _defaultFontFaces)}).first;
			Document_initFontSet(capsParams, altFontsIt->second.set);
		}
	}

	auto &value = node.getValue();
	auto &chars = fontsIt->second.set;
	for (auto &c : value) {
		auto transforms = node.getStyle().get(style::ParameterName::TextTransform);
		bool ucase = false; bool lcase = false;
		for (auto &it : transforms) {
			if (it.value.textTransform == style::TextTransform::Lowercase) {
				lcase = true;
			} else if (it.value.textTransform == style::TextTransform::Uppercase) {
				ucase = true;
			}
		}

		if (!string::isspace(c)) {
			if (params.fontVariant == style::FontVariant::SmallCaps) {
				if (lcase) {
					altFontsIt->second.set.insert(string::toupper(c));
				}
				if (ucase) {
					chars.insert(string::toupper(c));
				}
				if (c == string::toupper(c)) {
					chars.insert(c);
				} else if (!lcase) {
					altFontsIt->second.set.insert(string::toupper(c));
				}
			} else {
				chars.insert(c);
				if (lcase && ucase) {
					auto l = string::tolower(c);
					auto u = string::toupper(c);
					if (l != c) {
						chars.insert(l);
					}
					if (u != c) {
						chars.insert(u);
					}
				} else if (lcase) {
					auto l = string::tolower(c);
					if (l != c) {
						chars.insert(l);
					}
				} else if (ucase) {
					auto u = string::toupper(c);
					if (u != c) {
						chars.insert(u);
					}
				}
			}
		}
	}
}

void Document::forEachFontStyle(const style::ParameterList &spanStyle, const Function<void(const style::FontStyleParameters &)> &cb) {
	auto families = spanStyle.get(style::ParameterName::FontFamily);
	auto sizes = spanStyle.get(style::ParameterName::FontSize);
	auto styles = spanStyle.get(style::ParameterName::FontStyle);
	auto weights = spanStyle.get(style::ParameterName::FontWeight);
	auto variant = spanStyle.get(style::ParameterName::FontVariant);

	Set<MediaQueryId> mediaQueries;

	for (auto &it : families) {
		_fontFamily.insert(getCssString(it.value.stringId));
		if (it.mediaQuery != MediaQueryNone()) {
			mediaQueries.insert(it.mediaQuery);
		}
	}
	for (auto &it : sizes) { if (it.mediaQuery != MediaQueryNone()) { mediaQueries.insert(it.mediaQuery); } }
	for (auto &it : styles) { if (it.mediaQuery != MediaQueryNone()) { mediaQueries.insert(it.mediaQuery); } }
	for (auto &it : weights) { if (it.mediaQuery != MediaQueryNone()) { mediaQueries.insert(it.mediaQuery); } }
	for (auto &it : variant) { if (it.mediaQuery != MediaQueryNone()) { mediaQueries.insert(it.mediaQuery); } }

	mediaQueries.erase(MediaQueryNone());

	// fill default
	Vector<FontStyle> processedStyles;
	Vector<MediaQueryId> currentMedia;
	Vector<MediaQueryId> mediaVec;
	Vector<MediaQueryId> compiledVec;

	for (auto &it : mediaQueries) {
		mediaVec.push_back(it);
	}

	auto style = Document_compileFont(this, spanStyle, compiledVec);
	if (Document_addStyle(processedStyles, style)) {
		cb(style);
	}

	MediaQueryId n = mediaQueries.size();
	for (MediaQueryId i = 0; i < mediaQueries.size(); i++) {
		MediaQueryId k = i + 1;
		currentMedia.clear();
		if (bool genRes = gen_comb_norep_lex_init(currentMedia, n, k)) {
			while(genRes) {
				compiledVec.clear();
				for (auto &it : currentMedia) {
					compiledVec.push_back(mediaVec[it]);
				}
				style = Document_compileFont(this, spanStyle, compiledVec);
				if (Document_addStyle(processedStyles, style)) {
					cb(style);
				}
				genRes = gen_comb_norep_lex_next(currentMedia, n, k);
			 }
		}
	}
}

void Document::processCss(const String &path, const CharReaderBase &str) {
	Reader r;
	_css.emplace(path, r.readCss(path, str, _cssStrings, _mediaQueries));
}

void Document::processHtml(const String &path, const CharReaderBase &html, bool linear) {
	Reader r;
	Vector<Pair<String, String>> meta;
	_content.emplace_back(HtmlPage{path, Node("html", path), HtmlPage::FontMap{}, linear});
	HtmlPage &c = _content.back();

	if (r.readHtml(c, html, _cssStrings, _mediaQueries, meta, _css)) {
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

NS_SP_EXT_END(rich_text)
