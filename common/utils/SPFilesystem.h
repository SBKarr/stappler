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

#ifndef COMMON_UTILS_SPFILESYSTEM_H_
#define COMMON_UTILS_SPFILESYSTEM_H_

#include "SPIO.h"

NS_SP_EXT_BEGIN(filesystem)

class ifile {
public:
	ifile();
	explicit ifile(FILE *);
	explicit ifile(void *);
	explicit ifile(void *, size_t);

	~ifile();

	ifile(ifile &&);
	ifile & operator=(ifile &&);

	ifile(const ifile &) = delete;
	ifile & operator=(const ifile &) = delete;

	size_t read(uint8_t *buf, size_t nbytes);
	size_t seek(int64_t offset, io::Seek s);

	size_t tell() const;
	size_t size() const;

	bool eof() const;
	void close();

	bool is_open() const;
	operator bool() const { return is_open(); }

protected:
	bool _isBundled = false;
	size_t _size = 0;
	union {
		FILE *_nativeFile;
		void *_platformFile;
	};
};

// Check if file at path exists
bool exists(const StringView &path);

// check if path is valid and existed directory
bool isdir(const StringView &path);

// get file size
size_t size(const StringView &path);

// get file modification time
time_t mtime(const StringView &path);

// get file creation time
time_t ctime(const StringView &path);

// create dir at path (just mkdir, not mkdir -p)
bool mkdir(const StringView &path);

// mkdir -p (
bool mkdir_recursive(const StringView &path, bool appWide = true);

// touch (set mtime to now) file
bool touch(const StringView &path);

// move file from source to dest (tries to rename file, then copy-remove, rename will be successful only if both path is on single physical drive)
bool move(const StringView &source, const StringView &dest);

// copy file or directory to dest; use ftw_b for dirs, no directory tree check
bool copy(const StringView &source, const StringView &dest, bool stopOnError = true);

// remove file or directory
// if not recursive, only single file or empty dir will be removed
// if withDirs == false, only file s in directory tree will be removed
bool remove(const StringView &path, bool recursive = false, bool withDirs = false);

// file-tree-walk, walk across directory tree at path, callback will be called for each file or directory
// path in callback is absolute
// depth = -1 - unlimited
// dirFirst == true - directory will be shown before files inside them, useful for listings and copy
// dirFirst == false - directory will be shown after files, useful for remove
void ftw(const StringView &path, const Function<void(const StringView &path, bool isFile)> &, int depth = -1, bool dirFirst = false);

// same as ftw, but iteration can be stopped by returning false from callback
bool ftw_b(const StringView &path, const Function<bool(const StringView &path, bool isFile)> &, int depth = -1, bool dirFirst = false);

// returns application writable path (or path inside writable dir, if path is set
// if relative == false - do not merge paths, if provided path is absolute
//
// Writable path should be used for sharable, but not valuable contents,
// or caches, that should not be removed, when application is running or in background
// On android, writable path is on same drive or device, that used for application file
// This library use writable path to store fonts, icons caches and assets
String writablePath(const StringView &path = StringView(), bool relative = false);

// returns application documents path (or path inside documents dir, if path is set
// if relative == false - do not merge paths, if provided path is absolute
//
// Documents path should be used for valuable data, like documents, created by user,
// or content, that will be hard to recreate
// This library stores StoreKit and purchases data in documents dir
String documentsPath(const StringView &path = StringView(), bool relative = false);

// returns application current work dir from getcwd (or path inside current dir, if path is set
// if relative == false - do not merge paths, if provided path is absolute
//
// Current work dir is technical concept. Use it only if there is good reason for it
String currentDir(const StringView &path = StringView(), bool relative = false);

// returns application caches dir (or path inside caches dir, if path is set
// if relative == false - do not merge paths, if provided path is absolute
//
// Caches dir used to store caches or content, that can be easily recreated,
// and that can be removed/erased, when application is active or in background
// On android, caches will be placed on SD card, if it's available
String cachesPath(const StringView &path = StringView(), bool relative = false);

// write data into file on path
bool write(const StringView &path, const uint8_t *data, size_t len);

template <typename BytesView>
inline bool write(const StringView &path, const BytesView &view) {
	return write(path, (const uint8_t *)view.data(), size_t(view.size()));
}

ifile openForReading(const StringView &path);

// read file to string (if it was a binary file, string will be invalid)
String readTextFile(const StringView &path);

// read binary data from file
Bytes readFile(const StringView &path, size_t off = 0, size_t size = maxOf<size_t>());
bool readFile(const io::Consumer &stream, uint8_t *buf, size_t bsize, const StringView &path, size_t off, size_t size);

template <size_t Buffer = 1_KiB>
bool readFile(const io::Consumer &stream, const StringView &path,
		size_t off = 0, size_t size = maxOf<size_t>()) {
	uint8_t b[Buffer];
	return readFile(stream, b, Buffer, path, off, size);
}

NS_SP_EXT_END(filesystem)


NS_SP_EXT_BEGIN(filesystem_native)

enum Access {
	Exists,
	Read,
	Write,
	Execute
};

String nativeToPosix(const StringView &path);
String posixToNative(const StringView &path);

bool remove_fn(const StringView &path);
bool mkdir_fn(const StringView &path);

bool access_fn(const StringView &path, Access mode);

bool isdir_fn(const StringView &path);
size_t size_fn(const StringView &path);
time_t mtime_fn(const StringView &path);
time_t ctime_fn(const StringView &path);

bool touch_fn(const StringView &path);

void ftw_fn(const StringView &path, const Function<void(const StringView &path, bool isFile)> &, int depth, bool dirFirst);
bool ftw_b_fn(const StringView &path, const Function<bool(const StringView &path, bool isFile)> &, int depth, bool dirFirst);

bool rename_fn(const StringView &source, const StringView &dest);

String getcwd_fn();

FILE *fopen_fn(const StringView &, const StringView &mode);

NS_SP_EXT_END(filesystem_native)


NS_SP_EXT_BEGIN(filepath)

// check if filepath is absolute
bool isAbsolute(const StringView &path);

// check if filepath is local (not in application bundle or apk)
bool isCanonical(const StringView &path);

// check if filepath is in application bundle
bool isBundled(const StringView &path);

// check if filepath above it's current root
bool isAboveRoot(const StringView &path);

// check for ".", ".." and double slashes in path
bool validatePath(const StringView & path);

// remove any ".", ".." and double slashes from path
String reconstructPath(const StringView & path);

// returns current absolute path for file (canonical prefix will be decoded), this path should not be cached
// if writable flag is false, platform path will be returned with canonical prefix %PLATFORM%
// if writable flag is true, system should resolve platform prefix to absolute path, if possible
// if platform path can not be resolved (etc, it's in archive or another FS), empty string will be returned
String absolute(const StringView &, bool writable = false);

// encodes path for long-term storage (default application dirs will be replaced with canonical prefix,
// like %CACHE%/dir)
String canonical(const StringView &path);

// extract root from path by removing last component (/dir/file.tar.bz -> /dir)
String root(const StringView &path);

// extract last component (/dir/file.tar.bz -> file.tar.bz)
StringView lastComponent(const StringView &path);
StringView lastComponent(const StringView &path, size_t allowedComponents);

// extract full filename extension (/dir/file.tar.gz -> tar.gz)
StringView fullExtension(const StringView &path);

// extract last filename extension (/dir/file.tar.gz -> gz)
StringView lastExtension(const StringView &path);

// /dir/file.tar.bz -> file
StringView name(const StringView &path);

// /dir/file.tar.bz -> 2
size_t extensionCount(const StringView &path);

Vector<StringView> split(const StringView &);

// merges two path component, removes or adds '/' where needed
String merge(const StringView &root, const StringView &path);
String merge(const Vector<String> &);

template <class... Args>
inline String merge(const StringView &root, const StringView &path, Args&&... args) {
	return merge(merge(root, path), std::forward<Args>(args)...);
}

// translate some MIME Content-Type to common extensions
String extensionForContentType(const StringView &type);

// replace root path component in filepath
// replace(/my/dir/first/file, /my/dir/first, /your/dir/second)
// [/my/dir/first -> /your/dir/second] /file
// /my/dir/first/file -> /your/dir/second/file
String replace(const StringView &path, const StringView &source, const StringView &dest);

NS_SP_EXT_END(filepath)


NS_SP_EXT_BEGIN(io)

template <>
struct ProducerTraits<filesystem::ifile> {
	using type = filesystem::ifile;
	static size_t ReadFn(void *ptr, uint8_t *buf, size_t nbytes) {
		return ((type *)ptr)->read(buf, nbytes);
	}

	static size_t SeekFn(void *ptr, int64_t offset, Seek s) {
		return ((type *)ptr)->seek(offset, s);
	}
	static size_t TellFn(void *ptr) {
		return ((type *)ptr)->tell();
	}
};

NS_SP_EXT_END(io)

#endif /* COMMON_UTILS_SPFILESYSTEM_H_ */
