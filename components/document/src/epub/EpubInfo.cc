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
#include "EpubInfo.h"

#include "SPFilesystem.h"
#include "SPBitmap.h"
#include "SPHtmlParser.h"
#include "SPLocale.h"
#include "unzip.h"

NS_EPUB_BEGIN

struct EpubFileApi {
	cocos2d::zlib_filefunc64_def pathFunc;
	cocos2d::zlib_filefunc64_def fileFunc;

	static void *api_open(void *, const void *filename, int mode) {
		return new filesystem::ifile(filesystem::openForReading(*(const StringView *)filename));
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

bool Info::isEpub(const StringView &path) {
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
								if (size_t(lread) >= "application/epub+zip"_len) {
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

bool Info::init(const StringView &path) {
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
	ret.setString(_tocFile, "tocFile");

	auto & meta = ret.emplace("meta");
	auto & raw = meta.emplace("raw");
	for (auto &it : _meta.meta) {
		auto &val = raw.emplace();
		if (!it.id.empty()) {
			val.setString(it.id, "id");
		}
		if (!it.name.empty()) {
			val.setString(it.name, "name");
		}
		if (!it.value.empty()) {
			val.setString(it.value, "value");
		}
		if (!it.scheme.empty()) {
			val.setString(it.scheme, "scheme");
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

		html::parse(r, StringViewUtf8((const char *)container.data(), container.size()));
		return r.result;
	}
	return ret;
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

		struct Item {
			String id;
			String href;
			String type;
			String props;

			void clear() {
				id.clear();
				href.clear();
				type.clear();
				props.clear();
			}
		} item;

		enum Section {
			None,
			Package,
			Metadata,
			Manifest,
			Spine
		} section = None;

		Reader(String *rootPath, Map<String, ManifestFile> *manifest, Vector<SpineFile> *spineVec)
		: rootPath(rootPath), manifest(manifest), spineVec(spineVec) {}

		inline void onBeginTag(Parser &p, Tag &tag) {
			switch (section) {
			case Metadata:
				if (tag.name.is("dc:")) {
					metaProps.emplace_back();
					auto &prop = metaProps.back();
					prop.name = String(tag.name.data() + 3, tag.name.size() - 3);
				} else if (tag.name.compare("meta") || tag.name.compare("opf:meta")) {
					metaProps.emplace_back();
				}
				break;
			case Manifest:
				if (tag.name.compare("item") || tag.name.compare("opf:item")) {
					item.clear();
				}
				break;
			case Spine:
				if (tag.name.compare("itemref") || tag.name.compare("opf:itemref")) {
					spineVec->emplace_back();
					SpineFile &spine = spineVec->back();
					spine.linear = true;
				}
				break;
			default: break;
			}
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
			case Package:
				if ((tag.name.compare("spine") || tag.name.compare("opf:spine")) && (name.compare("toc") || name.compare("opf:toc"))) {
					auto it = manifestIds.find(value.str());
					if (it != manifestIds.end()) {
						tocFile = it->second->path;
					}
				}
				break;
			case Metadata:
				if (tag.name.is("dc:")) {
					auto &prop = metaProps.back();
					if (name.compare("id")) {
						prop.id = value.str();
					} else if (name.compare("lang") || name.compare("xml-lang")) {
						prop.lang = locale::common(value.str());
					} else if (name.compare("scheme") || name.compare("opf:scheme")) {
						prop.scheme = value.str();
					}
				} else if (tag.name.compare("meta") || tag.name.compare("opf:meta")) {
					MetaProp &meta = metaProps.back();
					if (name.compare("id")) {
						meta.id = value.str();
					} else if (name.compare("property") || name.compare("name")) {
						meta.name = value.str();
					} else if (name.compare("scheme") || name.compare("opf:scheme")) {
						meta.scheme = value.str();
					} else if (name.compare("xml:lang")) {
						meta.lang = value.str();
					} else if (name.compare("content")) {
						meta.value = value.str();
					} else if (name.compare("refines")) {
						meta.refines = value.str();
					}
				}
				break;
			case Manifest:
				if (tag.name.compare("item") || tag.name.compare("opf:item")) {
					if (name.compare("id")) {
						item.id = value.str();
					} else if (name.compare("href")) {
						item.href = value.str();
					} else if (name.compare("media-type")) {
						item.type = value.str();
					} else if (name.compare("properties")) {
						item.props = value.str();
					}
				}
				break;
			case Spine:
				if (tag.name.compare("itemref") || tag.name.compare("opf:itemref")) {
					SpineFile &spine = spineVec->back();
					if (name.compare("idref")) {
						auto itemIt = manifestIds.find(value.str());
						if (itemIt != manifestIds.end()) {
							spine.entry = itemIt->second;
						}
					} else if (name.compare("linear") && value.compare("no")) {
						spine.linear = false;
					} else if (name.compare("properties")) {
						string::split(value, " ", [&] (const StringView &str) {
							spine.props.emplace(str.str());
						});
					}
				}
				break;
			default: break;
			}
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
		}

		void updateMeta(MetaProp &prop, const String &key) {
			auto metaIt = metaIds.find(key);
			if (metaIt != metaIds.end()) {
				MetaProp *refined = nullptr;
				Vector<size_t> path = metaIt->second;
				for (auto &it : path) {
					if (refined == nullptr) {
						refined = &metaProps.at(it);
					} else {
						refined = &refined->extra.at(it);
					}
				}
				if (refined) {
					refined->extra.emplace_back(std::move(prop));
					auto &ref = refined->extra.back();
					if (!ref.id.empty()) {
						path.push_back(refined->extra.size() - 1);
						metaIds.emplace(ref.id, std::move(path));
					}
				}
			}
		}

		inline void popMetaTag() {
			MetaProp &prop = metaProps.back();
			if (!prop.refines.empty()) {
				updateMeta(prop, prop.refines.substr(1));
				metaProps.pop_back();
			} else {
				if (!prop.id.empty()) {
					metaIds.emplace(prop.id, Vector<size_t>{metaProps.size() - 1});
				}
			}
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
				} else if (tag.name.is("dc:") || tag.name.compare("meta") || tag.name.compare("opf:meta")) {
					popMetaTag();
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
		}

		inline void onManifestTag() {
			if (!item.id.empty() && !item.href.empty() && !item.type.empty()) {
				String path(*rootPath); path.append("/").append(item.href);
				auto fileIt = manifest->find(path);
				if (fileIt != manifest->end()) {
					fileIt->second.id = std::move(item.id);
					fileIt->second.mime = std::move(item.type);
					if (!item.props.empty()) {
						string::split(item.props, " ", [&fileIt] (const StringView &r) {
							fileIt->second.props.insert(r.str());
						});
					}
					manifestIds.emplace(fileIt->second.id, &fileIt->second);
					auto &m = fileIt->second.mime;
					if (m == "text/html" || m.compare(0, "application/xhtml"_len, "application/xhtml") == 0) {
						fileIt->second.type = ManifestFile::Source;
					} else if (m == "text/css") {
						fileIt->second.type = ManifestFile::Css;
					}
				}
			}
		}

		inline void onInlineTag(Parser &p, Tag &tag) {
			switch (section) {
			case Metadata:
				if (tag.name.is("dc:") || tag.name.compare("meta") || tag.name.compare("opf:meta")) {
					popMetaTag();
				}
				break;
			case Manifest:
				if (tag.name.compare("item") || tag.name.compare("opf:item")) {
					onManifestTag();
				}
				break;
			case Spine:
				if (tag.name.compare("itemref") || tag.name.compare("opf:itemref")) {
					SpineFile &spine = spineVec->back();
					if (!spine.entry) {
						spineVec->pop_back();
					}
				}
				break;
			default: break;
			}
		}

		inline void onTagContent(Parser &p, Tag &tag, StringReader &s) {
			switch (section) {
			case Metadata:
				if (tag.name.is("dc:") || tag.name.compare("meta") || tag.name.compare("opf:meta")) {
					auto &prop = metaProps.back();
					prop.value = s.str();
					string::trim(prop.value);
				}
				break;
			default: break;
			}
		}

		String version;
		String uid;
		String tocFile;

		Vector<MetaProp> metaProps;
		Map<String, Vector<size_t>> metaIds;
		Map<String, const ManifestFile *> manifestIds;

		String *rootPath = nullptr;
		Map<String, ManifestFile> *manifest = nullptr;
		Vector<SpineFile> *spineVec = nullptr;
	} r(&_rootPath, &_manifest, &_spine);

	html::parse(r, StringViewUtf8((const char *)opf.data(), opf.size()));

	_meta.meta = std::move(r.metaProps);
	for (auto &it : _meta.meta) {
		if (it.id == r.uid) {
			_uniqueId = it.value;
		}

		if (it.name == "title") {
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
		} else if (it.name == "contributor" || it.name == "creator") {
			_meta.authors.emplace_back(AuthorMeta{it.value,
				(it.name == "contributor")?AuthorMeta::Contributor:AuthorMeta::Creator});

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
		} else if (it.name == "dcterms:modified") {
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
	} else {
		_tocFile = r.tocFile;
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
