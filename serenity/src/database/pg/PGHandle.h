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

#include "SqlHandle.h"
#include "StorageScheme.h"

NS_SA_EXT_BEGIN(pg)

enum class TransactionLevel {
	ReadCommited,
	RepeatableRead,
	Serialized,
};

class Handle : public sql::SqlHandle {
public:
	using Value = data::Value;

	Handle(apr_pool_t *, ap_dbd_t *);

	Handle(const Handle &) = delete;
	Handle &operator=(const Handle &) = delete;

	Handle(Handle &&);
	Handle &operator=(Handle &&);

	operator bool () const;

	virtual void makeQuery(const Callback<void(sql::SqlQuery &)> &cb);

	virtual bool selectQuery(const sql::SqlQuery &, const Callback<void(sql::Result &)> &cb);
	virtual bool performSimpleQuery(const StringView &);
	virtual bool performSimpleSelect(const StringView &, const Callback<void(sql::Result &)> &cb);

public: // adapter interface
	virtual bool init(Server &serv, const Map<String, const Scheme *> &) override;

protected:
	virtual bool beginTransaction() override { return beginTransaction_pg(TransactionLevel::ReadCommited); }
	virtual bool endTransaction() override { return endTransaction_pg(); }

	bool beginTransaction_pg(TransactionLevel l);
	void cancelTransaction_pg();
	bool endTransaction_pg();

	using ViewIdVec = Vector<Pair<const storage::Scheme::ViewScheme *, int64_t>>;

	apr_pool_t *pool;
	ap_dbd_t *handle;
    PGconn *conn = nullptr;
    ExecStatusType lastError = PGRES_EMPTY_QUERY;
    TransactionLevel level = TransactionLevel::ReadCommited;
};

NS_SA_EXT_END(pg)

#endif /* SERENITY_SRC_DATABASE_PG_PGHANDLE_H_ */
