/*
 * EpubDocumentInfo.cpp
 *
 *  Created on: 15 июл. 2016 г.
 *      Author: sbkarr
 */

#include "SPDefine.h"
#include "SPEpubInfo.h"
#include "SPFilesystem.h"
#include "SPBitmap.h"
#include "SPLocale.h"
#include "SPHtmlParser.h"

#include "unzip.h"
#include "tinyxml2.h"

NS_EPUB_BEGIN

struct EpubFileApi {
	cocos2d::zlib_filefunc64_def pathFunc;
	cocos2d::zlib_filefunc64_def fileFunc;

	static void *api_open(void *, const void *filename, int mode) {
		return new filesystem::ifile(filesystem::openForReading(*(const String *)filename));
	}

	static void *api_open_file(void *, const void *filename, int mode) {
		auto filePtr = (filesystem::ifile *)filename;
		filePtr->seek(0, io::Seek::Set);
		return (void *)filename;
	}

	static uLong api_read(void *, void * stream, void * buf, uLong size) {
		auto filePtr = (filesystem::ifile *)stream;
		return filePtr->read((uint8_t *)buf, size);
	}

	static uLong api_write(void *, void * stream, const void * buf, uLong size) {
		return 0;
	}

	static cocos2d::ZPOS64_T api_tell(void * opaque, void * stream) {
		auto filePtr = (filesystem::ifile *)stream;
		return filePtr->tell();
	}

	static long api_seek(void *, void * stream, cocos2d::ZPOS64_T offset, int origin) {
		auto filePtr = (filesystem::ifile *)stream;
		io::Seek seek = io::Seek::Set;
		if (origin == SEEK_SET) {
			seek = io::Seek::Set;
		} else if (origin == SEEK_CUR) {
			seek = io::Seek::Current;
		} else if (origin == SEEK_END) {
			seek = io::Seek::End;
		}
		return (filePtr->seek(offset, seek) != maxOf<size_t>())?0:-1;
	}

	static int api_close(void *, void * stream) {
		auto filePtr = (filesystem::ifile *)stream;
		filePtr->close();
		delete filePtr;
		return 0;
	}

	static int api_close_file(void *, void * stream) {
		return 0;
	}

	static int api_error(void *, void * stream) {
		return 0;
	}

	EpubFileApi() {
		pathFunc.opaque = this;
		pathFunc.zopen64_file = &api_open;
		pathFunc.zread_file = &api_read;
		pathFunc.zwrite_file = &api_write;
		pathFunc.ztell64_file = &api_tell;
		pathFunc.zseek64_file = &api_seek;
		pathFunc.zclose_file = &api_close;
		pathFunc.zerror_file = &api_error;

		fileFunc.opaque = this;
		fileFunc.zopen64_file = &api_open_file;
		fileFunc.zread_file = &api_read;
		fileFunc.zwrite_file = &api_write;
		fileFunc.ztell64_file = &api_tell;
		fileFunc.zseek64_file = &api_seek;
		fileFunc.zclose_file = &api_close_file;
		fileFunc.zerror_file = &api_error;
	}
};

static EpubFileApi s_fileApi;

bool Info::isEpub(const String &path) {
	bool ret = false;
	auto file = filesystem::openForReading(path);
	if (!file) {
		return false;
	}

	uint8_t buf[4] = { 0 };
	if (file.read(buf, 4) == 4) {
		// ZIP magic: 0x50, 0x4b, 0x03, 0x04
		if (buf[0] == 0x50 && buf[1] == 0x4b && buf[2] == 0x03 && buf[3] == 0x04) {
			auto filePtr = cocos2d::unzOpen2_64((void *)&file, &s_fileApi.fileFunc);
			if (filePtr) {
				if (cocos2d::unzLocateFile(filePtr, "mimetype", 0) == UNZ_OK) {
					cocos2d::unz_file_info64 info;
					if (cocos2d::unzGetCurrentFileInfo64(filePtr, &info, nullptr, 0, nullptr, 0, nullptr, 0) == UNZ_OK) {
						if (info.uncompressed_size >= "application/epub+zip"_len) {
							if (cocos2d::unzOpenCurrentFile(filePtr) == UNZ_OK) {
								Bytes buf; buf.resize("application/epub+zip"_len);
								auto lread = cocos2d::unzReadCurrentFile(filePtr, buf.data(), unsigned(info.uncompressed_size));
								if (lread == "application/epub+zip"_len) {
									if (memcmp(buf.data(), "application/epub+zip", "application/epub+zip"_len) == 0) {
										ret = true;
									}
								}
								cocos2d::unzCloseCurrentFile(filePtr);
							}
						}
					}
				}

				cocos2d::unzClose(filePtr);
			}
		}
	}
	return ret;
}

struct DocumentFile {
	unzFile file;
	size_t pos;
	size_t size;
};

Info::Info() { }

Info::~Info() {
	if (_file) {
		cocos2d::unzClose(_file);
	}
}

Info::Info(Info && doc)
: _file(doc._file), _rootFile(std::move(doc._rootFile))
, _rootPath(std::move(doc._rootPath)), _tocFile(std::move(doc._tocFile)), _uniqueId(std::move(doc._uniqueId))
, _modified(std::move(doc._modified)), _coverFile(std::move(doc._coverFile))
, _manifest(std::move(doc._manifest)), _spine(std::move(doc._spine))
, _meta(std::move(doc._meta))  {
	doc._file = nullptr;
}

Info & Info::operator=(Info && doc) {
	_file = doc._file;
	_rootFile = std::move(doc._rootFile);
	_rootPath = std::move(doc._rootPath);
	_tocFile = std::move(doc._tocFile);
	_uniqueId = std::move(doc._uniqueId);
	_modified = std::move(doc._modified);
	_coverFile = std::move(doc._coverFile);
	_manifest = std::move(doc._manifest);
	_spine = std::move(doc._spine);
	_meta = std::move(doc._meta);
	doc._file = nullptr;
	return *this;
}

bool Info::init(const String &path) {
	_file = cocos2d::unzOpen2_64((void *)&path, &s_fileApi.pathFunc);
	if (_file) {
		_manifest = getFileList(_file);
		_rootFile = getRootPath();
		_rootPath = filepath::root(_rootFile);

		processPublication();
	}
	return valid();
}

Bytes Info::openFile(const String &file) const {
	auto containerIt = _manifest.find(file);
	if (containerIt != _manifest.end()) {
		return openFile(containerIt->second);
	}
	return Bytes();
}

Bytes Info::openFile(const ManifestFile &file) const {
	Bytes ret;
	cocos2d::unz64_file_pos pos;
	pos.pos_in_zip_directory = file.zip_pos;
	pos.num_of_file = file.file_num;
	if (cocos2d::unzGoToFilePos64(_file, &pos) == UNZ_OK) {
		if (cocos2d::unzOpenCurrentFile(_file) == UNZ_OK) {
			ret.resize(file.size);
			if (cocos2d::unzReadCurrentFile(_file, ret.data(), unsigned(file.size)) != (int)file.size) {
				ret.clear();
			}
			cocos2d::unzCloseCurrentFile(_file);
		}
	}
	return ret;
}

bool Info::valid() const {
	return _file && _manifest.size() > 2 && !_rootFile.empty();
}

data::Value Info::encode() const {
	data::Value ret;
	ret.setString(_uniqueId, "uniqueId");
	ret.setString(_modified, "modified");
	ret.setString(_rootFile, "rootFile");
	ret.setString(_coverFile, "coverFile");

	auto & meta = ret.emplace("meta");
	auto & raw = meta.emplace("raw");
	for (auto &it : _meta.meta) {
		auto &val = raw.emplace();
		if (!it.id.empty()) {
			val.setString(it.id, "id");
		}
		if (!it.prop.empty()) {
			val.setString(it.prop, "prop");
		}
		if (!it.value.empty()) {
			val.setString(it.value, "value");
		}
		if (!it.extra.empty()) {
			auto & ex = val.emplace("extra");
			for (auto &eit : it.extra) {
				auto &valEx = ex.emplace();
				if (!eit.name.empty()) {
					valEx.setString(eit.name, "name");
				}
				if (!eit.value.empty()) {
					valEx.setString(eit.value, "value");
				}
				if (!eit.scheme.empty()) {
					valEx.setString(eit.scheme, "scheme");
				}
				if (!eit.lang.empty()) {
					valEx.setString(eit.lang, "lang");
				}
			}
		}
	}

	auto & titles = meta.emplace("titles");
	for (auto &it : _meta.titles) {
		auto & title = titles.emplace();
		if (!it.title.empty()) {
			title.setString(it.title, "title");
		}
		if (it.sequence != 0) {
			title.setInteger(it.sequence, "sequence");
		}
		switch (it.type) {
		case TitleMeta::Main: title.setString("main", "type"); break;
		case TitleMeta::Subtitle: title.setString("subtitle", "type"); break;
		case TitleMeta::Short: title.setString("short", "type"); break;
		case TitleMeta::Expanded: title.setString("expanded", "type"); break;
		case TitleMeta::Edition: title.setString("edition", "type"); break;
		case TitleMeta::Collection: title.setString("collection", "type"); break;
		}
		if (!it.localizedTitle.empty()) {
			auto &loc = title.emplace("localized");
			for (auto &lit : it.localizedTitle) {
				loc.setString(lit.second, lit.first);
			}
		}
	}

	auto & authors = meta.emplace("authors");
	for (auto &it : _meta.authors) {
		auto & author = authors.emplace();
		if (!it.name.empty()) {
			author.setString(it.name, "name");
		}
		if (!it.role.empty()) {
			author.setString(it.role, "role");
		}
		if (!it.roleScheme.empty()) {
			author.setString(it.roleScheme, "scheme");
		}
		switch (it.type) {
		case AuthorMeta::Creator: author.setString("creator", "type"); break;
		case AuthorMeta::Contributor: author.setString("contributor", "type"); break;
		}
		if (!it.localizedName.empty()) {
			auto &loc = author.emplace("localized");
			for (auto &lit : it.localizedName) {
				loc.setString(lit.second, lit.first);
			}
		}
	}

	auto & manifest = ret.emplace("manifest");
	for (auto &it : _manifest) {
		auto &val = manifest.emplace(it.second.id);
		if (!it.second.path.empty()) {
			val.setString(it.second.path, "path");
		}
		if (!it.second.mime.empty()) {
			val.setString(it.second.mime, "mime");
		}
		if (!it.second.props.empty()) {
			auto &props = val.emplace("props");
			for (auto &sit : it.second.props) {
				props.addString(sit);
			}
		}
	}

	auto & spine = ret.emplace("spine");
	for (auto &it : _spine) {
		auto &val = spine.emplace();
		if (!it.entry->path.empty()) {
			val.setString(it.entry->path, "path");
		}
		if (!it.props.empty()) {
			auto &props = val.emplace("props");
			for (auto &sit : it.props) {
				props.addString(sit);
			}
		}
		val.setBool(it.linear, "linear");
	}

	return ret;
}

Map<String, ManifestFile> Info::getFileList(FilePtr file) {
	Map<String, ManifestFile> ret;
	char buf[1_KiB] = { 0 };
	cocos2d::unz_file_info64 info;
	cocos2d::unz64_file_pos infoPos;

	auto err = cocos2d::unzGoToFirstFile64(file, &info, buf, 1_KiB);
	if (err != UNZ_OK) {
		return ret;
	}

	while (err == UNZ_OK) {
		err = cocos2d::unzGetFilePos64(file, &infoPos);
		if (err != UNZ_OK) {
			break;
		}

		err = cocos2d::unzOpenCurrentFile(file);
		if (err != UNZ_OK) {
			break;
		}

		DocumentFile docFile{file, 0, (size_t)info.uncompressed_size};

		size_t width = 0, height = 0;
		if (Bitmap::getImageSize(docFile, width, height)) {
			ret.emplace(String(buf), ManifestFile{
				String(buf),
				(size_t)info.uncompressed_size,
				infoPos.pos_in_zip_directory,
				infoPos.num_of_file,
				ManifestFile::Image,
				uint16_t(width),
				uint16_t(height)
			});
			//log::format("EpubFile", "%s %llu %lu %lu", buf, info.uncompressed_size, width, height);
		} else {
			ret.emplace(String(buf), ManifestFile{
				String(buf),
				(size_t)info.uncompressed_size,
				infoPos.pos_in_zip_directory,
				infoPos.num_of_file,
				ManifestFile::Unknown, 0, 0
			});
			//log::format("EpubFile", "%s %llu", buf, info.uncompressed_size);
		}
		cocos2d::unzCloseCurrentFile(file);
		err = cocos2d::unzGoToNextFile64(file, &info, buf, 1_KiB);
	}
	return ret;
}


String Info::getRootPath() {
	String ret;
	auto container = openFile("META-INF/container.xml");
	if (!container.empty()) {
		struct EpubXmlContentReader {
			using Parser = html::Parser<EpubXmlContentReader>;
			using Tag = Parser::Tag;
			using StringReader = Parser::StringReader;

			inline void onTagAttribute(Parser &p, Tag &tag, StringReader &name, StringReader &value) {
				if (tag.name.compare("rootfile") && name.compare("full-path")) {
					result = value.str();
					p.cancel();
				}
			}

			String result;
		} r;

		html::parse(r, CharReaderUtf8((const char *)container.data(), container.size()));
		return r.result;
	}
	return ret;
}

static void DocumentInfo_updateMeta(Vector<MetaProp> &meta, const String &id,
		const String &prop, const String &value, const String &scheme, const String &lang) {
	for (auto &it : meta) {
		if (it.id == id) {
			it.extra.emplace_back(MetaRefine{id, prop, value, scheme, lang});
			break;
		}
	}
}

static void DocumentInfo_updateDoc(Vector<MetaRefine> &doc, const String &id,
		const String &prop, const String &value, const String &scheme, const String &lang) {
	for (auto &it : doc) {
		if (it.id == id) {
			it.extra.emplace_back(MetaRefine{id, prop, value, scheme, lang});
			break;
		}
	}
}

static void DocumentInfo_parseMetaNode(Vector<MetaRefine> &doc, Vector<MetaProp> &meta,
		Map<String, tinyxml2::XMLElement *>&ids, tinyxml2::XMLElement *node) {
	String id;
	if (auto idStr = node->Attribute("id")) {
		id = idStr;
		ids.emplace(idStr, node);
	}

	String name(node->Value());
	if (name.compare(0, 3, "dc:") == 0) {
		if (auto child = node->FirstChild()) {
			if (auto textNode =child->ToText()) {
				auto lang = node->Attribute("xml:lang");
				meta.emplace_back(MetaProp{id, name.substr(3), textNode->Value(), locale::common(lang?lang:""), { }});
			}
		}
	} else if (name == "meta" || name == "opf:meta") {
		auto refines = node->Attribute("refines");
		if (auto child = node->FirstChild()) {
			if (auto textNode = child->ToText()) {
				auto value = textNode->Value();
				if (value) {
					auto prop = node->Attribute("property");
					auto scheme = node->Attribute("scheme");
					auto lang = node->Attribute("xml:lang");
					if (refines && refines[0] != 0) {
						String idStr(refines + 1);
						auto idIt = ids.find(idStr);
						if (idIt != ids.end()) {
							if (strcmp(idIt->second->Value(), "meta") == 0) {
								DocumentInfo_updateDoc(doc, idStr, prop?prop:"", value?value:"",
										scheme?scheme:"", locale::common(lang?lang:""));
							} else {
								DocumentInfo_updateMeta(meta, idStr, prop?prop:"", value?value:"",
										scheme?scheme:"", locale::common(lang?lang:""));
							}
						}
					} else {
						doc.emplace_back(MetaRefine{id, prop?prop:"", value?value:"",
								scheme?scheme:"", locale::common(lang?lang:""), { }});
					}
				}
			}
		}
	}
}

static void DocumentInfo_parseManifestNode(const String &root, Map<String, ManifestFile> &manifest,
		Map<String, const ManifestFile *> &manifestIds, tinyxml2::XMLElement *node) {
	auto id = node->Attribute("id");
	auto href = node->Attribute("href");
	auto mediaType = node->Attribute("media-type");
	auto properties = node->Attribute("properties");

	if (id && href && mediaType) {
		String path(root); path.append("/").append(href);
		auto fileIt = manifest.find(path);
		if (fileIt != manifest.end()) {
			fileIt->second.id = id;
			fileIt->second.mime = mediaType;

			if (properties) {
				string::split(String(properties), " ", [&] (const CharReaderBase &r) {
					fileIt->second.props.insert(r.str());
				});
			}

			manifestIds.emplace(id, &fileIt->second);

			// verify mime
			auto &m = fileIt->second.mime;
			if (m == "text/html" || m.compare(0, "application/xhtml"_len, "application/xhtml") == 0) {
				fileIt->second.type = ManifestFile::Source;
			} else if (m == "text/css") {
				fileIt->second.type = ManifestFile::Css;
			}
		}
	}
}

static void DocumentInfo_parseSpineNode(const Map<String, const ManifestFile *> &manifestIds,
		Vector<SpineFile> &spine, tinyxml2::XMLElement *node) {
	auto idref  = node->Attribute("idref");
	auto linear = node->Attribute("linear");
	auto properties = node->Attribute("properties");

	if (idref) {
		auto itemIt = manifestIds.find(idref);
		if (itemIt != manifestIds.end()) {
			spine.emplace_back(SpineFile{
				itemIt->second,
				(properties?string::splitToSet(properties, " "):Set<String>()),
				(linear && strcmp(linear, "no") == 0)?false:true});
		}
	}
}

void Info::processPublication() {
	auto opf = openFile(_rootFile);
	if (opf.empty()) {
		return;
	}

	struct Reader {
		using Parser = html::Parser<Reader>;
		using Tag = Parser::Tag;
		using StringReader = Parser::StringReader;

		enum Section {
			None,
			Package,
			Metadata,
			Manifest,
			Spine
		} section = None;

		inline void onBeginTag(Parser &p, Tag &tag) {
			switch (section) {
			case Metadata:
				if (tag.name.is("dc:")) {
					metaProps.emplace_back();
					auto &prop = metaProps.back();
					prop.prop = String(tag.name.data() + 3, tag.name.size() - 3);
				} else if (tag.name.compare("meta") || tag.name.compare("opf:meta")) {
					metaRefine.emplace_back();
				}
				break;
			default: break;
			}
			log::format("onBeginTag", "%s", tag.name.str().c_str());
		}

		inline void onEndTag(Parser &p, Tag &tag) {
			log::format("onEndTag", "%s", tag.name.str().c_str());
		}

		inline void onTagAttribute(Parser &p, Tag &tag, StringReader &name, StringReader &value) {
			switch (section) {
			case None:
				if (tag.name.compare("package") || tag.name.compare("opf:package")) {
					if (name.compare("version")) {
						version = value.str();
					} else if (name.compare("unique-identifier")) {
						uid = value.str();
					}
				}
				break;
			case Metadata:
				if (tag.name.is("dc:")) {
					if (name.compare("id")) {
						auto &prop = metaProps.back();
						prop.id = value.str();
					} else if (name.compare("lang") || name.compare("xml-lang")) {
						prop.lang = locale::common(value.str());
					}
				} else if (tag.name.compare("meta") || tag.name.compare("opf:meta")) {
					MetaRefine &meta = metaRefine.back();
					if (name.compare("id")) {
						meta.id = value.str();
					} else if (name.compare("property") || name.compare("name")) {
						meta.name = value.str();
					} else if (name.compare("scheme")) {
						meta.scheme = value.str();
					} else if (name.compare("xml:lang")) {
						meta.lang = value.str();
					} else if (name.compare("content")) {
						meta.value = value.str();
					}
				}
				break;
			default: break;
			}
			log::format("onTagAttribute", "%s: %s = %s", tag.name.str().c_str(), name.str().c_str(), value.str().c_str());
		}

		inline void onPushTag(Parser &p, Tag &tag) {
			switch (section) {
			case None:
				if (tag.name.compare("package") || tag.name.compare("opf:package")) {
					section = Package;
				}
				break;
			case Package:
				if (tag.name.compare("metadata") || tag.name.compare("opf:metadata")) {
					section = Metadata;
				} else if (tag.name.compare("manifest") || tag.name.compare("opf:manifest")) {
					section = Manifest;
				} else if (tag.name.compare("spine") || tag.name.compare("opf:spine")) {
					section = Spine;
				}
				break;
			default: break;
			}
			log::format("onPushTag", "%s", tag.name.str().c_str());
		}

		inline void onPopTag(Parser &p, Tag &tag) {
			switch (section) {
			case Package:
				if (tag.name.compare("package") || tag.name.compare("opf:package")) {
					section = None;
				}
				break;
			case Metadata:
				if (tag.name.compare("metadata") || tag.name.compare("opf:metadata")) {
					section = Package;
				}
				break;
			case Manifest:
				if (tag.name.compare("manifest") || tag.name.compare("opf:manifest")) {
					section = Package;
				}
				break;
			case Spine:
				if (tag.name.compare("spine") || tag.name.compare("opf:spine")) {
					section = Package;
				}
				break;
			default: break;
			}
			log::format("onPopTag", "%s", tag.name.str().c_str());
		}

		inline void onInlineTag(Parser &p, Tag &tag) {
			log::format("onInlineTag", "%s", tag.name.str().c_str());
		}

		inline void onTagContent(Parser &p, Tag &tag, StringReader &s) {
			switch (section) {
			case Metadata:
				if (tag.name.is("dc:")) {
					auto &prop = metaProps.back();
					prop.value = s.str();
					string::trim(prop.value);
				} else if (tag.name.compare("meta")) {
					auto &meta = metaRefine.back();
					meta.value = s.str();
					string::trim(meta.value);
				}
				break;
			default: break;
			}
			log::format("onTagContent", "%s: %s", tag.name.str().c_str(), s.str().c_str());
		}

		String version;
		String uid;

		Vector<MetaRefine> metaRefine;
		Vector<MetaProp> metaProps;
	} r;

	html::parse<html::Tag>(r, CharReaderBase((const char *)opf.data(), opf.size()));

	tinyxml2::XMLDocument xmlDoc;
	if (xmlDoc.Parse((const char *)opf.data(), opf.size()) != tinyxml2::XML_SUCCESS) {
		return;
	}

	auto root = xmlDoc.RootElement();

	if (!root) {
		return;
	}

	Map<String, tinyxml2::XMLElement *> ids;
	Vector<MetaRefine> metaDoc;
	Vector<MetaProp> metaProps;
	Map<String, ManifestFile> &manifest = _manifest;
	Vector<SpineFile> &spine = _spine;

	Map<String, const ManifestFile *> manifestIds;

	String version(root->Attribute("version"));
	String uid(root->Attribute("unique-identifier"));
	if (auto idStr = root->Attribute("id")) {
		ids.emplace(idStr, root);
	}

	if (version.empty() || uid.empty()) {
		return;
	}

	for (auto node = root->FirstChild(); node != nullptr; node = node->NextSibling()) {
		if (node->ToComment()) {
			// skip comments
		} else if (strcmp(node->Value(), "metadata") == 0 || strcmp(node->Value(), "opf:metadata") == 0) {
			for (auto mdnode = node->FirstChild(); mdnode != nullptr; mdnode = mdnode->NextSibling()) {
				if (auto el = mdnode->ToElement()) {
					DocumentInfo_parseMetaNode(metaDoc, metaProps, ids, el);
				}
			}
		} else if (strcmp(node->Value(), "manifest") == 0 || strcmp(node->Value(), "opf:manifest") == 0) {
			for (auto mdnode = node->FirstChild(); mdnode != nullptr; mdnode = mdnode->NextSibling()) {
				if (auto el = mdnode->ToElement()) {
					String name(mdnode->Value());
					if (name == "item" || name == "opf:item") {
						DocumentInfo_parseManifestNode(_rootPath, manifest, manifestIds, el);
					}
				}
			}
		} else if (strcmp(node->Value(), "spine") == 0 || strcmp(node->Value(), "opf:spine") == 0) {
			if (auto el = node->ToElement()) {
				auto attr = el->Attribute("toc");
				if (!attr) {
					attr = el->Attribute("opf:toc");
				}
				if (attr) {
					auto it = manifestIds.find(attr);
					if (it != manifestIds.end()) {
						_tocFile = it->second->path;
					}
				}
			}
			for (auto mdnode = node->FirstChild(); mdnode != nullptr; mdnode = mdnode->NextSibling()) {
				if (auto el = mdnode->ToElement()) {
					String name(mdnode->Value());
					if (name == "itemref" || name == "opf:itemref") {
						DocumentInfo_parseSpineNode(manifestIds, spine, el);
					}
				}
			}
		}
	}

	_meta.meta = std::move(metaProps);
	for (auto &it : _meta.meta) {
		if (it.id == uid) {
			_uniqueId = it.value;
		}

		if (it.prop == "title") {
			_meta.titles.emplace_back(TitleMeta{it.value});
			TitleMeta &title = _meta.titles.back();

			if (!it.lang.empty()) {
				title.localizedTitle.emplace(it.lang, it.value);
			}

			for (auto &eit : it.extra) {
				if (eit.name == "alternate-script") {
					title.localizedTitle.emplace(eit.lang, it.value);
				} else if (eit.name == "display-seq") {
					title.sequence = StringToNumber<int64_t>(eit.value.c_str(), nullptr);
				} else if (eit.name == "title-type") {
					if (eit.value == "main") {
						title.type = TitleMeta::Main;
					} else if (eit.value == "subtitle") {
						title.type = TitleMeta::Subtitle;
					} else if (eit.value == "short") {
						title.type = TitleMeta::Short;
					} else if (eit.value == "collection") {
						title.type = TitleMeta::Collection;
					} else if (eit.value == "edition") {
						title.type = TitleMeta::Edition;
					} else if (eit.value == "expanded") {
						title.type = TitleMeta::Expanded;
					}
				}
			}
		}

		if (it.prop == "contributor" || it.prop == "creator") {
			_meta.authors.emplace_back(AuthorMeta{it.value,
				(it.prop == "contributor")?AuthorMeta::Contributor:AuthorMeta::Creator});

			AuthorMeta &author = _meta.authors.back();

			if (!it.lang.empty()) {
				author.localizedName.emplace(it.lang, it.value);
			}

			for (auto &eit : it.extra) {
				if (eit.name == "alternate-script") {
					author.localizedName.emplace(eit.lang, it.value);
				} else if (eit.name == "role") {
					author.role = eit.value;
					author.roleScheme = eit.scheme;
				}
			}
		}
	}

	for (auto &it : metaDoc) {
		if (it.name == "dcterms:modified") {
			_modified = it.value;
		} else if (it.name == "belongs-to-collection") {
			_meta.collections.emplace_back(CollectionMeta{it.value});
			auto &col = _meta.collections.back();
			if (!it.lang.empty()) {
				col.localizedTitle.emplace(it.lang, it.value);
			}

			for (auto &eit : it.extra) {
				if (eit.name == "collection-type") {
					col.type = eit.value;
				} else if (eit.name == "group-position") {
					col.position = eit.value;
				} else if (eit.name == "alternate-script") {
					col.localizedTitle.emplace(eit.lang, eit.value);
				} else if (eit.name == "dcterms:identifier") {
					col.uid = eit.value;
				}
			}
		}
	}

	String coverFile, tocFile;
	for (auto &it : _manifest) {
		if (it.second.props.find("cover-image") != it.second.props.end()) {
			coverFile = it.second.path;
		}
		if (it.second.props.find("nav") != it.second.props.end()) {
			tocFile = it.second.path;
		}
		if (!coverFile.empty() && !tocFile.empty()) {
			break;
		}
	}

	if (!coverFile.empty()) {
		_coverFile = coverFile;
	}

	if (!tocFile.empty()) {
		_tocFile = tocFile;
	}
}

const MetaData &Info::getMeta() const {
	return _meta;
}
const Map<String, ManifestFile> &Info::getManifest() const {
	return _manifest;
}
const Vector<SpineFile> &Info::getSpine() const {
	return _spine;
}
const String & Info::getUniqueId() const {
	return _uniqueId;
}
const String & Info::getModificationTime() const {
	return _modified;
}
const String & Info::getCoverFile() const {
	return _coverFile;
}
const String & Info::getTocFile() const {
	return _tocFile;
}

bool Info::isFileExists(const String &path, const String &root) const {
	auto str = resolvePath(path, root);
	return _manifest.find(str) != _manifest.end();
}
size_t Info::getFileSize(const String &path, const String &root) const {
	auto str = resolvePath(path, root);
	auto it = _manifest.find(str);
	if (it != _manifest.end()) {
		return it->second.size;
	}
	return 0;
}
Bytes Info::getFileData(const String &path, const String &root) const {
	auto str = resolvePath(path, root);
	return openFile(str);
}
Bytes Info::getFileData(const SpineFile &file) const {
	return openFile(*file.entry);
}
Bytes Info::getFileData(const ManifestFile &file) const {
	return openFile(file);
}

bool Info::isImage(const String &path, const String &root) const {
	auto str = resolvePath(path, root);
	auto it = _manifest.find(str);
	if (it != _manifest.end()) {
		return it->second.type == ManifestFile::Type::Image;
	}
	return false;
}
bool Info::isImage(const String &path, size_t &width, size_t &height, const String &root) const {
	auto str = resolvePath(path, root);
	auto it = _manifest.find(str);
	if (it != _manifest.end()) {
		if (it->second.type == ManifestFile::Type::Image) {
			width = it->second.width;
			height = it->second.height;
			return true;
		}
	}
	return false;
}

String Info::resolvePath(const String &path, const String &root) const {
	if (root.empty()) {
		return filepath::reconstructPath(path);
	} else if (isFileExists(root, "")) {
		return filepath::reconstructPath(filepath::merge(filepath::root(root), path));
	} else {
		return filepath::reconstructPath(filepath::merge(root, path));
	}
}

NS_EPUB_END


NS_SP_EXT_BEGIN(io)

template <>
struct ProducerTraits<epub::DocumentFile> {
	using type = epub::DocumentFile;
	static size_t ReadFn(void *ptr, uint8_t *buf, size_t nbytes) {
		auto val = cocos2d::unzReadCurrentFile(((type *)ptr)->file, buf, unsigned(nbytes));
		if (val > 0) {
			((type *)ptr)->pos += val;
			return val;
		}
		return 0;
	}

	static size_t SeekFn(void *ptr, int64_t offset, io::Seek s) {
		auto pos = offset;
		auto file = ((type *)ptr);
		if (s == io::Seek::Current) { pos += file->pos; }
		if (s == io::Seek::End) { pos = file->size + offset; }

		if (pos < (int64_t)file->pos) {
			cocos2d::unzCloseCurrentFile(file->file);
			cocos2d::unzOpenCurrentFile(file->file);
			file->pos = 0;
		}

		if (pos == (int64_t)file->pos) {
			return size_t(pos);
		} else {
			uint8_t buf[1_KiB] = { 0 };
			size_t offset = size_t(pos) - file->pos;
			while (offset > 0) {
				auto readLen = std::min(offset, size_t(1_KiB));
				if (cocos2d::unzReadCurrentFile(file->file, buf, unsigned(readLen)) != (int)readLen) {
					break;
				}
				file->pos += readLen;
				offset -= readLen;
			}
		}

		return ((type *)ptr)->pos;
	}
	static size_t TellFn(void *ptr) {
		auto file = ((type *)ptr);
		return size_t(cocos2d::unztell64(file->file));
	}
};

NS_SP_EXT_END(io)
