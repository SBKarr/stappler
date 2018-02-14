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

#include "PGQuery.h"

NS_SA_EXT_BEGIN(pg)

class Handle : public storage::Adapter {
public:
	enum class DeltaAction {
		Create = 1,
		Update,
		Delete,
		Append,
		Erase
	};

	using Value = data::Value;

	Handle(apr_pool_t *, ap_dbd_t *);

	Handle(const Handle &) = delete;
	Handle &operator=(const Handle &) = delete;

	Handle(Handle &&);
	Handle &operator=(Handle &&);

	operator bool () const;

	Result select(const ExecQuery &);
	int64_t selectId(const ExecQuery &);
	size_t perform(const ExecQuery &);

	data::Value select(const Scheme &, const ExecQuery &);
	data::Value select(const Field &, const ExecQuery &);
	void select(data::Value &, const Field &, const ExecQuery &);

	bool performSimpleQuery(const String &);
	Result performSimpleSelect(const String &);

	data::Value getHistory(const Scheme &, const Time &, bool resolveUsers = false);
	data::Value getHistory(const FieldView &, const Scheme *, uint64_t tag, const Time &, bool resolveUsers = false);

	data::Value getDeltaData(const Scheme &, const Time &);
	data::Value getDeltaData(const Scheme &, const FieldView &, const Time &, uint64_t);

public:
	void makeSessionsCleanup();
	int64_t processBroadcasts(Server &, int64_t);

public: // adapter interface
	virtual bool init(Server &serv, const Map<String, const Scheme *> &) override;

	virtual User * authorizeUser(const Scheme &, const String &name, const String &password) override;

	virtual void broadcast(const Bytes &) override;

	virtual data::Value getData(const String &) override;
	virtual bool setData(const String &, const data::Value &, TimeInterval = config::getKeyValueStorageTime()) override;
	virtual bool clearData(const String &) override;

	virtual data::Value getSessionData(const Bytes &) override;
	virtual bool setSessionData(const Bytes &, const data::Value &, TimeInterval) override;
	virtual bool clearSessionData(const Bytes &) override;

	virtual Resource *makeResource(ResourceType, QueryList &&, const Field *) override;

	virtual int64_t getDeltaValue(const Scheme &) override;
	virtual int64_t getDeltaValue(const Scheme &, const FieldView &, uint64_t) override;

protected: // object interface
	virtual bool createObject(const Scheme &, data::Value &data) override;
	virtual bool saveObject(const Scheme &, uint64_t oid, const data::Value &newObject, const Vector<String> &fields) override;
	virtual data::Value patchObject(const Scheme &, uint64_t oid, const data::Value &data, const Vector<const Field *> &returnFields) override;
	virtual bool removeObject(const Scheme &, uint64_t oid) override;

	virtual data::Value selectObjects(const Scheme &, const Query &) override;
	virtual size_t countObjects(const Scheme &, const Query &) override;

	bool patchArray(const Scheme &, uint64_t oid, const Field &, data::Value &);
	bool patchRefSet(const Scheme &, uint64_t oid, const Field &, const Vector<int64_t> &objsToAdd);
	bool cleanupRefSet(const Scheme &, uint64_t oid, const Field &, const Vector<int64_t> &objsToRemove);

protected: // prop interface
	virtual data::Value getProperty(const Scheme &, uint64_t oid, const Field &, const Set<const Field *> &) override;
	virtual data::Value getProperty(const Scheme &, const data::Value &, const Field &, const Set<const Field *> &) override;

	virtual data::Value setProperty(const Scheme &, uint64_t oid, const Field &, data::Value &&) override;
	virtual data::Value setProperty(const Scheme &, const data::Value &, const Field &, data::Value &&) override;

	virtual bool clearProperty(const Scheme &, uint64_t oid, const Field &, data::Value &&) override;
	virtual bool clearProperty(const Scheme &, const data::Value &, const Field &, data::Value &&) override;

	virtual data::Value appendProperty(const Scheme &, uint64_t oid, const Field &, data::Value &&) override;
	virtual data::Value appendProperty(const Scheme &, const data::Value &, const Field &, data::Value &&) override;

	virtual Vector<int64_t> performQueryListForIds(const QueryList &, size_t count = maxOf<size_t>()) override;
	virtual data::Value performQueryList(const QueryList &, size_t count = maxOf<size_t>(), bool forUpdate = false, const Field * = nullptr) override;

	virtual bool removeFromView(const FieldView &, const Scheme *, uint64_t oid) override;
	virtual bool addToView(const FieldView &, const Scheme *, uint64_t oid, const data::Value &) override;

	virtual Vector<int64_t> getReferenceParents(const Scheme &, uint64_t oid, const Scheme *, const Field *) override;

protected:
	virtual bool beginTransaction() override { return beginTransaction_pg(TransactionLevel::ReadCommited); }
	virtual bool endTransaction() override { return endTransaction_pg(); }

	data::Value getKVData(Bytes &&);
	bool setKVData(Bytes &&, const data::Value &, TimeInterval);
	bool clearKVData(Bytes &&);

	void performPostUpdate(ExecQuery &, const Scheme &s, Value &obj, int64_t id, const Value &upd, bool clear);
	bool insertIntoSet(ExecQuery &, const Scheme &s, int64_t id, const FieldObject &field, const Field &f, const Value &d);
	bool insertIntoArray(ExecQuery &, const Scheme &s, int64_t id, const Field &field, const Value &d);
	bool insertIntoRefSet(ExecQuery &, const Scheme &s, int64_t id, const Field &field, const Vector<int64_t> &d);

	bool beginTransaction_pg(TransactionLevel l);
	void cancelTransaction_pg();
	bool endTransaction_pg();

	void finalizeBroadcast();

	void touchDelta(const Scheme &, int64_t id, DeltaAction);

	apr_pool_t *pool;
	ap_dbd_t *handle;
    PGconn *conn = nullptr;
    ExecStatusType lastError = PGRES_EMPTY_QUERY;
    TransactionLevel level = TransactionLevel::ReadCommited;

    Vector<Pair<apr_time_t, Bytes>> _bcasts;
};

NS_SA_EXT_END(pg)

#endif /* SERENITY_SRC_DATABASE_PG_PGHANDLE_H_ */
