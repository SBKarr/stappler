/**
Copyright (c) 2016 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef SERENITY_SRC_STORAGE_STORAGEADAPTER_H_
#define SERENITY_SRC_STORAGE_STORAGEADAPTER_H_

#include "StorageQuery.h"

NS_SA_EXT_BEGIN(storage)

enum class TransactionStatus {
	None,
	Commit,
	Rollback,
};

class Adapter : public AllocPool {
public:
	static Adapter *FromContext();

	virtual ~Adapter() { }

public: // transactions
	template <typename T>
	bool performInTransaction(T && t) {
		if (isInTransaction()) {
			if (!t()) {
				cancelTransaction();
			}
		} else {
			if (beginTransaction()) {
				if (!t()) {
					cancelTransaction();
				}
				return endTransaction();
			}
		}
		return false;
	}

public: // Object CRUD
	virtual bool createObject(const Scheme &, data::Value &data) = 0;
	virtual bool saveObject(const Scheme &, uint64_t oid, const data::Value &newObject, const Vector<String> &fields) = 0;
	virtual data::Value patchObject(const Scheme &, uint64_t oid, const data::Value &data) = 0;

	virtual bool removeObject(const Scheme &, uint64_t oid) = 0;
	virtual data::Value getObject(const Scheme &, uint64_t, bool forUpdate) = 0;
	virtual data::Value getObject(const Scheme &, const String &, bool forUpdate) = 0;
	virtual bool init(Server &serv, const Map<String, const Scheme *> &) = 0;

	virtual data::Value selectObjects(const Scheme &, const Query &) = 0;
	virtual size_t countObjects(const Scheme &, const Query &) = 0;


public: // Object properties CRUD
	virtual data::Value getProperty(const Scheme &, uint64_t oid, const Field &) = 0;
	virtual data::Value getProperty(const Scheme &, const data::Value &, const Field &) = 0;

	virtual data::Value setProperty(const Scheme &, uint64_t oid, const Field &, data::Value &&) = 0;
	virtual data::Value setProperty(const Scheme &, const data::Value &, const Field &, data::Value &&) = 0;

	virtual bool clearProperty(const Scheme &, uint64_t oid, const Field &, data::Value && = data::Value()) = 0;
	virtual bool clearProperty(const Scheme &, const data::Value &, const Field &, data::Value && = data::Value()) = 0;

	virtual data::Value appendProperty(const Scheme &, uint64_t oid, const Field &, data::Value &&) = 0;
	virtual data::Value appendProperty(const Scheme &, const data::Value &, const Field &, data::Value &&) = 0;


public: // session support
	virtual data::Value getSessionData(const Bytes &) = 0;
	virtual bool setSessionData(const Bytes &, const data::Value &, TimeInterval) = 0;
	virtual bool clearSessionData(const Bytes &) = 0;

public: // Key-Value storage
	virtual bool setData(const String &, const data::Value &, TimeInterval = config::getKeyValueStorageTime()) = 0;
	virtual data::Value getData(const String &) = 0;
	virtual bool clearData(const String &) = 0;

	virtual User * authorizeUser(const Scheme &, const String &name, const String &password) = 0;

	virtual void broadcast(const Bytes &) = 0;
	virtual void broadcast(const data::Value &val) {
		broadcast(data::write(val, data::EncodeFormat::Cbor));
	}

	virtual Resource *makeResource(ResourceType, QueryList &&, const Field *) = 0;

public: // resource requests
	virtual Vector<int64_t> performQueryListForIds(const QueryList &, size_t count = maxOf<size_t>()) = 0;
	virtual data::Value performQueryList(const QueryList &, size_t count = maxOf<size_t>(), bool forUpdate = false, const Field * = nullptr) = 0;

protected:
	virtual bool beginTransaction() = 0;
	virtual bool endTransaction() = 0;

	virtual void cancelTransaction() { transactionStatus = TransactionStatus::Rollback; }
	virtual bool isInTransaction() const { return transactionStatus != TransactionStatus::None; }
	virtual TransactionStatus getTransactionStatus() const { return transactionStatus; }

    TransactionStatus transactionStatus = TransactionStatus::None;
};

NS_SA_EXT_END(storage)

#endif /* SERENITY_SRC_STORAGE_STORAGEADAPTER_H_ */
