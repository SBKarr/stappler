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

#include "SPDBInputFile.h"

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
	writeSize += n;
	return file.xsputn(s, n);
}

void InputFile::close() {
	file.close_remove();
}

bool InputFile::save(const mem::StringView &ipath) const {
	return const_cast<file_t &>(file).close_rename(stappler::filesystem::cachesPath(ipath).c_str());
}

NS_DB_END
