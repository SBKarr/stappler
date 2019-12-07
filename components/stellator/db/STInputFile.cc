/**
Copyright (c) 2016-2019 Roman Katuntsev <sbkarr@stappler.org>

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

#include "STInputFile.h"
#include "STStorageField.h"

NS_DB_BEGIN

InputFile::InputFile(mem::String &&name, mem::String && type, mem::String && enc, mem::String && orig, size_t s, int64_t id)
: name(std::move(name)), type(std::move(type)), encoding(std::move(enc))
, original(std::move(orig)), writeSize(0), headerSize(s), id(id) {
	file = file_t::open_tmp(config::getUploadTmpFilePrefix(), false);
	path = file.path();
}

InputFile::~InputFile() {
	close();
}

bool InputFile::isOpen() const {
	return file.is_open();
}

size_t InputFile::write(const char *s, size_t n) {
	auto tmp = s;
	for (size_t i = 0; i < n; ++ i) {
		if (*tmp == 0) {
			isBinary = true;
		}
		++ tmp;
	}
	writeSize += n;
	return file.xsputn(s, n);
}

void InputFile::close() {
	file.close_remove();
}

bool InputFile::save(const mem::StringView &ipath) const {
	return const_cast<file_t &>(file).close_rename(stappler::filesystem::cachesPath(ipath).c_str());
}

mem::Bytes InputFile::readBytes() {
	mem::Bytes ret;
	ret.resize(writeSize);
	file.seek(0, stappler::io::Seek::Set);
	file.xsgetn((char *)ret.data(), ret.size());
	return ret;
}

mem::String InputFile::readText() {
	if (!isBinary) {
		mem::String ret;
		ret.resize(writeSize);
		file.seek(0, stappler::io::Seek::Set);
		file.xsgetn((char *)ret.data(), ret.size());
		return ret;
	}
	return mem::String();
}

static size_t processExtraVarSize(const FieldExtra *s) {
	size_t ret = 256;
	for (auto it : s->fields) {
		auto t = it.second.getType();
		if (t == Type::Text || t == Type::Bytes) {
			auto f = static_cast<const FieldText *>(it.second.getSlot());
			ret = std::max(f->maxLength, ret);
		} else if (t == Type::Extra) {
			auto f = static_cast<const FieldExtra *>(it.second.getSlot());
			ret = std::max(processExtraVarSize(f), ret);
		}
	}
	return ret;
}

static size_t updateFieldLimits(const mem::Map<mem::String, Field> &vec) {
	size_t ret = 256 * vec.size();
	for (auto &it : vec) {
		auto t = it.second.getType();
		if (t == Type::Text || t == Type::Bytes) {
			auto f = static_cast<const FieldText *>(it.second.getSlot());
			ret += f->maxLength;
		} else if (t == Type::Data || t == Type::Array) {
			ret += config::getMaxExtraFieldSize();
		} else if (t == Type::Extra) {
			auto f = static_cast<const FieldExtra *>(it.second.getSlot());
			ret += updateFieldLimits(f->fields);
		} else {
			ret += 256;
		}
	}
	return ret;
}

void InputConfig::updateLimits(const mem::Map<mem::String, Field> &fields) {
	maxRequestSize = 256 * fields.size();
	for (auto &it : fields) {
		auto t = it.second.getType();
		if (t == Type::File) {
			auto f = static_cast<const FieldFile *>(it.second.getSlot());
			maxFileSize = std::max(f->maxSize, maxFileSize);
			maxRequestSize += f->maxSize + 256;
		} else if (t == Type::Image) {
			auto f = static_cast<const FieldImage *>(it.second.getSlot());
			maxFileSize = std::max(f->maxSize, maxFileSize);
			maxRequestSize += f->maxSize + 256;
		} else if (t == Type::Text || t == Type::Bytes) {
			auto f = static_cast<const FieldText *>(it.second.getSlot());
			maxVarSize = std::max(f->maxLength, maxVarSize);
			maxRequestSize += f->maxLength;
		} else if (t == Type::Data || t == Type::Array) {
			maxRequestSize += config::getMaxExtraFieldSize();
		} else if (t == Type::Extra) {
			auto f = static_cast<const FieldExtra *>(it.second.getSlot());
			maxRequestSize += updateFieldLimits(f->fields);
			maxVarSize = std::max(processExtraVarSize(f), maxVarSize);
		}
	}
}

NS_DB_END
