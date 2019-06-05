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

#ifndef STELLATOR_DB_STSTORAGEFILE_H_
#define STELLATOR_DB_STSTORAGEFILE_H_

#include "STStorageObject.h"

NS_DB_BEGIN

class File : public Object {
public:
	static mem::String getFilesystemPath(uint64_t oid);

	static bool validateFileField(const Field &, const InputFile &);
	static bool validateFileField(const Field &, const mem::StringView &type, const mem::Bytes &data);

	static mem::Value createFile(const Transaction &, const Field &, InputFile &);
	static mem::Value createFile(const Transaction &, const mem::StringView &type, const mem::StringView &path, int64_t = 0);
	static mem::Value createFile(const Transaction &, const mem::StringView &type, const mem::Bytes &data, int64_t = 0);

	static mem::Value createImage(const Transaction &, const Field &, InputFile &);
	static mem::Value createImage(const Transaction &, const Field &, const mem::StringView &type, const mem::Bytes &data, int64_t = 0);

	static mem::Value getData(const Transaction &, uint64_t id);
	static void setData(const Transaction &, uint64_t id, const mem::Value &);

	// remove file from filesystem
	static bool removeFile(const mem::Value &);
	static bool removeFile(int64_t);

	// remove file from storage and filesystem
	static bool purgeFile(const Transaction &, const mem::Value &);
	static bool purgeFile(const Transaction &, int64_t);

	static const Scheme *getScheme();
};

NS_DB_END

#endif /* STELLATOR_DB_STSTORAGEFILE_H_ */
