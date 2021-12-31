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

	Handle(const Driver *, Driver::Handle);

	Handle(const Handle &) = delete;
	Handle &operator=(const Handle &) = delete;

	Handle(Handle &&);
	Handle &operator=(Handle &&);

	operator bool () const;

	const Driver *getDriver() const { return driver; }
	Driver::Handle getHandle() const;
	Driver::Connection getConnection() const;

	virtual bool isNotificationsSupported() const override { return true; }

	virtual void makeQuery(const mem::Callback<void(sql::SqlQuery &)> &cb) override;

	virtual bool selectQuery(const db::sql::SqlQuery &, const mem::Callback<bool(Result &)> &cb,
			const mem::Callback<void(const mem::Value &)> &err = nullptr) override;
	virtual bool performSimpleQuery(const mem::StringView &,
			const mem::Callback<void(const mem::Value &)> &err = nullptr) override;
	virtual bool performSimpleSelect(const mem::StringView &, const mem::Callback<void(Result &)> &cb,
			const mem::Callback<void(const mem::Value &)> &err = nullptr) override;

	virtual bool isSuccess() const override;

	void close();

public: // adapter interface
	virtual bool init(const Interface::Config &cfg, const mem::Map<mem::StringView, const Scheme *> &) override;

protected:
	virtual bool beginTransaction() override { return beginTransaction_pg(TransactionLevel::ReadCommited); }
	virtual bool endTransaction() override { return endTransaction_pg(); }

	bool beginTransaction_pg(TransactionLevel l);
	void cancelTransaction_pg();
	bool endTransaction_pg();

	using ViewIdVec = mem::Vector<mem::Pair<const Scheme::ViewScheme *, int64_t>>;

	const Driver *driver = nullptr;
	Driver::Handle handle = Driver::Handle(nullptr);
	Driver::Connection conn = Driver::Connection(nullptr);
	Driver::Status lastError = Driver::Status::Empty;
	mem::Value lastErrorInfo;
	TransactionLevel level = TransactionLevel::ReadCommited;
};

NS_DB_PQ_END

#endif /* SERENITY_SRC_DATABASE_PG_PGHANDLE_H_ */
