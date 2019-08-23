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

#ifndef EPUB_EPUBINFO_H_
#define EPUB_EPUBINFO_H_

#include "SPLayout.h"

#define NS_EPUB_BEGIN NS_SP_EXT_BEGIN(epub)
#define NS_EPUB_END NS_SP_EXT_END(epub)

NS_EPUB_BEGIN

class Document;
class Reader;

struct MetaProp {
	String id;
	String name;
	String value;
	String scheme;
	String lang;
	String refines;
	Vector<MetaProp> extra;
};

struct CollectionMeta {
	String title;
	String type;
	String position;
	String uid;

	Map<String, String> localizedTitle;
};

struct TitleMeta {
	enum Type {
		Main,
		Subtitle,
		Short,
		Collection,
		Edition,
		Expanded
	};

	String title;
	Map<String, String> localizedTitle;

	int64_t sequence = 0;
	Type type = Main;
};

struct AuthorMeta {
	enum Type {
		Creator,
		Contributor,
	};

	String name;
	Type type = Creator;
	Map<String, String> localizedName;

	String role;
	String roleScheme;
};

struct MetaData {
	Vector<MetaProp> meta;
	Vector<TitleMeta> titles;
	Vector<AuthorMeta> authors;
	Vector<CollectionMeta> collections;
};

struct ManifestFile {
	// ZIP data
	String path;
	size_t size;
    uint64_t zip_pos;
    uint64_t file_num;

	enum Type {
		Unknown,
		Image,
		Source,
		Css,
	} type;

	uint16_t width;
	uint16_t height;

	// Manifest data
	String id;
	String mime;
	Set<String> props;
};

struct SpineFile {
	const ManifestFile * entry;
	Set<String> props;
	bool linear;
};

class Info : public Ref {
public:
	using FilePtr = void *;

public:
	static bool isEpub(const StringView &path);

	Info();
	~Info();

	Info(Info && doc);
	Info & operator=(Info && doc);

	Info(const Info & doc) = delete;
	Info & operator=(const Info & doc) = delete;

	bool init(const StringView &path);

	bool valid() const;

	data::Value encode() const;

	const MetaData &getMeta() const;
	const Vector<SpineFile> &getSpine() const;
	const Map<String, ManifestFile> &getManifest() const;

	const String & getUniqueId() const;
	const String & getModificationTime() const;
	const String & getCoverFile() const;
	const String & getTocFile() const;

	bool isFileExists(const String &path, const String &root) const;
	size_t getFileSize(const String &path, const String &root) const;
	Bytes getFileData(const String &path, const String &root) const;
	Bytes getFileData(const SpineFile &file) const;
	Bytes getFileData(const ManifestFile &file) const;

	bool isImage(const String &path, const String &root) const;
	bool isImage(const String &path, size_t &width, size_t &height, const String &root) const;

	String resolvePath(const String &path, const String &root) const;

	bool isHtml(const String &path);

protected:
	Bytes openFile(const String &) const;
	Bytes openFile(const ManifestFile &) const;
	Map<String, ManifestFile> getFileList(FilePtr file);
	String getRootPath();
	void processPublication();

	FilePtr _file = nullptr;
	String _rootFile;
	String _rootPath;
	String _tocFile;

	String _uniqueId;
	String _modified;
	String _coverFile;
	Map<String, ManifestFile> _manifest;
	Vector<SpineFile> _spine;
	MetaData _meta;
};

NS_EPUB_END

#endif /* EPUB_EPUBINFO_H_ */
