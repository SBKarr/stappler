/**
Copyright (c) 2017-2018 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef SERENITY_SRC_DATABASE_PG_PGHANDLE_H_
#define SERENITY_SRC_DATABASE_PG_PGHANDLE_H_

#include "STStorageScheme.h"
#include "STSqlHandle.h"
#include "STPqDriver.h"

NS_DB_PQ_BEGIN

enum class TransactionLevel {
	ReadCommited,
	RepeatableRead,
	Serialized,
};

class Handle : public db::sql::SqlHandle {
public:
	using Value = mem::Value;

	Handle(Driver *, Driver::Handle);

	Handle(const Handle &) = delete;
	Handle &operator=(const Handle &) = delete;

	Handle(Handle &&);
	Handle &operator=(Handle &&);

	operator bool () const;

	Driver::Handle getHandle() const;
	Driver::Connection getConnection() const;

	void setStorageTypeMap(const mem::Vector<mem::Pair<uint32_t, Interface::StorageType>> *);
	void setCustomTypeMap(const mem::Vector<mem::Pair<uint32_t, mem::String>> *);

	virtual void makeQuery(const stappler::Callback<void(sql::SqlQuery &)> &cb) override;

	virtual bool selectQuery(const db::sql::SqlQuery &, const stappler::Callback<void(sql::Result &)> &cb) override;
	virtual bool performSimpleQuery(const mem::StringView &) override;
	virtual bool performSimpleSelect(const mem::StringView &, const stappler::Callback<void(sql::Result &)> &cb) override;

	virtual bool isSuccess() const override;

	Interface::StorageType getTypeById(uint32_t) const;
	mem::StringView getTypeNameById(uint32_t) const;

	void close();

public: // adapter interface
	virtual bool init(const Interface::Config &cfg, const mem::Map<mem::StringView, const Scheme *> &) override;

protected:
	virtual bool beginTransaction() override { return beginTransaction_pg(TransactionLevel::ReadCommited); }
	virtual bool endTransaction() override { return endTransaction_pg(); }

	bool beginTransaction_pg(TransactionLevel l);
	void cancelTransaction_pg();
	bool endTransaction_pg();

	using ViewIdVec = mem::Vector<stappler::Pair<const Scheme::ViewScheme *, int64_t>>;

	Driver *driver = nullptr;
	Driver::Handle handle = Driver::Handle(nullptr);
	Driver::Connection conn = Driver::Connection(nullptr);
	Driver::Status lastError = Driver::Status::Empty;
	TransactionLevel level = TransactionLevel::ReadCommited;

	const mem::Vector<mem::Pair<uint32_t, StorageType>> *storageTypes = nullptr;
	const mem::Vector<mem::Pair<uint32_t, mem::String>> *customTypes = nullptr;
};

NS_DB_PQ_END

#endif /* SERENITY_SRC_DATABASE_PG_PGHANDLE_H_ */
