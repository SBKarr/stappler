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

#ifndef STELLATOR_DB_STINPUTFILE_H_
#define STELLATOR_DB_STINPUTFILE_H_

#include "STStorage.h"

NS_DB_BEGIN

struct InputFile : public mem::AllocBase {
	mem::String path;
	mem::String name;
	mem::String type;
	mem::String encoding;
	mem::String original;
	file_t file;

	size_t writeSize;
	size_t headerSize;
	int64_t id;

	InputFile(mem::String && name, mem::String && type, mem::String && enc, mem::String && orig, size_t s, int64_t id);
	~InputFile();

	bool isOpen() const;

	size_t write(const char *, size_t);
	void close();

	bool save(const mem::StringView &) const;

	int64_t negativeId() const { return - id - 1; }

	InputFile(const InputFile &) = delete;
	InputFile(InputFile &&) = delete;

	InputFile &operator=(const InputFile &) = delete;
	InputFile &operator=(InputFile &&) = delete;
};

NS_DB_END

#endif /* STELLATOR_DB_STINPUTFILE_H_ */
