/**
Copyright (c) 2016-2018 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef SERENITY_SRC_STORAGE_STORAGEFILE_H_
#define SERENITY_SRC_STORAGE_STORAGEFILE_H_

#include "StorageObject.h"

NS_SA_EXT_BEGIN(storage)

class File : public Object {
public:
	static String getFilesystemPath(uint64_t oid);

	static bool validateFileField(const Field &, const InputFile &);
	static bool validateFileField(const Field &, const StringView &type, const Bytes &data);

	static data::Value createFile(Adapter *adapter, const Field &, InputFile &);
	static data::Value createFile(Adapter *adapter, const StringView &type, const StringView &path);
	static data::Value createFile(Adapter *adapter, const StringView &type, const Bytes &data);

	static data::Value createImage(Adapter *adapter, const Field &, InputFile &);
	static data::Value createImage(Adapter *adapter, const Field &, const StringView &type, const Bytes &data);

	static data::Value getData(Adapter *adapter, uint64_t id);
	static void setData(Adapter *adapter, uint64_t id, const data::Value &);

	// remove file from filesystem
	static bool removeFile(Adapter *adapter, const Field &, const data::Value &);

	// remove file from storage and filesystem
	static bool purgeFile(Adapter *adapter, const Field &, const data::Value &);

	static const Scheme *getScheme();
protected:
};

NS_SA_EXT_END(storage)

#endif /* SERENITY_SRC_STORAGE_STORAGEFILE_H_ */
