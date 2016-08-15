/*
 * DatabaseHandle.h
 *
 *  Created on: 24 янв. 2016 г.
 *      Author: sbkarr
 */

#ifndef SERENITY_SRC_DATABASE_DATABASEHANDLE_H_
#define SERENITY_SRC_DATABASE_DATABASEHANDLE_H_

#include "StorageAdapter.h"

NS_SA_EXT_BEGIN(database)

enum class TransactionLevel {
	ReadCommited,
	RepeatableRead,
	Serialized,
};

enum class TransactionStatus {
	None,
	Commit,
	Rollback,
};

class Handle : public storage::Adapter {
public:
	struct Result {
		Vector<String> names;
		Vector<Vector<String>> data;

		data::Value toValue() const;
		data::Value toValue(const Vector<String> &) const;
	};

	using Scheme = storage::Scheme;
	using Field = storage::Field;
	using Query = storage::Query;

	Handle(apr_pool_t *, ap_dbd_t *);

	Handle(const Handle &) = delete;
	Handle &operator=(const Handle &) = delete;

	Handle(Handle &&);
	Handle &operator=(Handle &&);

	operator bool () const { return pool != nullptr && handle != nullptr; }

	Result select(const String &);
	uint64_t selectId(const String &);
	data::Value select(Scheme *, const String &);
	data::Value select(const storage::Field *, const String &);
	size_t perform(const String &);

	String escape(const String &) const;

	template <typename T>
	bool performInTransaction(T && t, TransactionLevel l = TransactionLevel::ReadCommited, bool acquireExisted = true) {
		if (!acquireExisted || transaction == TransactionStatus::None) {
			if (beginTransaction(l)) {
				t();
				return endTransaction();
			}
		} else if (toInt(level) >= toInt(l)) {
			t();
			return true;
		}
		return false;
	}

public:
	virtual bool createObject(Scheme *, data::Value &data) override;
	virtual bool saveObject(Scheme *, uint64_t oid, const data::Value &newObject, const Vector<String> &fields) override;
	virtual data::Value patchObject(Scheme *, uint64_t oid, const data::Value &data) override;
	virtual bool removeObject(Scheme *, uint64_t oid) override;
	virtual data::Value getObject(Scheme *, uint64_t, bool forUpdate) override;
	virtual data::Value getObject(Scheme *, const String &, bool forUpdate) override;
	virtual bool init(Server &serv, const Map<String, Scheme *> &) override;

	virtual data::Value selectObjects(Scheme *, const Query &) override;

	virtual size_t countObjects(Scheme *, const Query &) override;


	virtual data::Value getProperty(Scheme *, uint64_t oid, const Field &) override;
	virtual data::Value getProperty(Scheme *, const data::Value &, const Field &) override;

	virtual data::Value setProperty(Scheme *, uint64_t oid, const Field &, data::Value &&) override;
	virtual data::Value setProperty(Scheme *, const data::Value &, const Field &, data::Value &&) override;

	virtual void clearProperty(Scheme *, uint64_t oid, const Field &) override;
	virtual void clearProperty(Scheme *, const data::Value &, const Field &) override;

	virtual data::Value appendProperty(Scheme *, uint64_t oid, const Field &, data::Value &&) override;
	virtual data::Value appendProperty(Scheme *, const data::Value &, const Field &, data::Value &&) override;


	virtual data::Value getSessionData(const Bytes &) override;
	virtual bool setSessionData(const Bytes &, const data::Value &, TimeInterval) override;
	virtual bool clearSessionData(const Bytes &) override;

	virtual storage::Resolver *createResolver(Scheme *, const data::TransformMap *) override;

	virtual void setMinNextOid(uint64_t) override;

	void makeSessionsCleanup();
	int64_t processBroadcasts(Server &, int64_t);

	virtual bool supportsAtomicPatches() const override;

	bool patchArray(Scheme *, uint64_t oid, const storage::Field *, data::Value &);

	virtual bool setData(const String &, const data::Value &, TimeInterval = config::getKeyValueStorageTime()) override;
	virtual data::Value getData(const String &) override;
	virtual bool clearData(const String &) override;

	virtual User * authorizeUser(Scheme *, const String &name, const String &password) override;

	virtual void broadcast(const Bytes &) override;

public: // helpers
	void writeAliasRequest(apr::ostringstream &, Scheme *s, const String &);
	void writeQueryStatement(apr::ostringstream &query, Scheme *scheme, const Field &f, const Query::Select &it);
	void writeQueryRequest(apr::ostringstream &, Scheme *s, const Vector<Query::Select> &);

protected:
	data::Value resultToData(Scheme *s, Result &);
	data::Value resultToData(const storage::Field *s, Result &);

	void performPostUpdate(apr::ostringstream &, Scheme *s, data::Value &obj,
			int64_t id, data::Value &upd, bool clear);
	bool insertIntoSet(apr::ostringstream &, Scheme *s, int64_t id,
			const storage::FieldObject *field, const storage::Field *f, data::Value &d);
	bool insertIntoArray(apr::ostringstream &, Scheme *s, int64_t id,
			const storage::Field *field, data::Value &d);

	bool beginTransaction(TransactionLevel l);
	void cancelTransaction();
	bool endTransaction();

	apr_pool_t *pool;
	ap_dbd_t *handle;
    PGconn *conn = nullptr;
    TransactionLevel level = TransactionLevel::ReadCommited;
    TransactionStatus transaction = TransactionStatus::None;
};

NS_SA_EXT_END(database)

#endif /* SERENITY_SRC_DATABASE_DATABASEHANDLE_H_ */
