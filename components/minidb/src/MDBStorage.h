/**
Copyright (c) 2020 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef COMPONENTS_MINIDB_SRC_MDBSTORAGE_H_
#define COMPONENTS_MINIDB_SRC_MDBSTORAGE_H_

#include "MDBHandle.h"

NS_MDB_BEGIN

class Storage : public mem::AllocBase {
public:
	using PageCallback = mem::Callback<bool(void *mem, uint32_t size)>;

	static Storage *open(mem::pool_t *, mem::StringView path);

	static void destroy(Storage *);

	bool init(const mem::Map<mem::StringView, const db::Scheme *> &);

	mem::pool_t *getPool() const { return _pool; }
	mem::StringView getSourceName() const { return _sourceName; }
	uint32_t getPageSize() const;
	uint32_t getPageCount() const;

	bool isValid() const;
	operator bool() const;

protected:
	friend class Manifest;
	friend class Transaction;

	Manifest * retainManifest() const;
	void swapManifest(Manifest *) const;
	void releaseManifest(Manifest *) const;

	void free();

	Storage(mem::pool_t *, mem::StringView path);

	mem::pool_t *_pool = nullptr;
	mem::StringView _sourceName;
	mem::BytesView _sourceMemory;

	mutable mem::Mutex _mutex;
	mutable Manifest * _manifest;
};

NS_MDB_END

#endif /* COMPONENTS_MINIDB_SRC_MDBSTORAGE_H_ */
