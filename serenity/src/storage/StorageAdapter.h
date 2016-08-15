/*
 * StorageAdapter.h
 *
 *  Created on: 29 февр. 2016 г.
 *      Author: sbkarr
 */

#ifndef SERENITY_SRC_STORAGE_STORAGEADAPTER_H_
#define SERENITY_SRC_STORAGE_STORAGEADAPTER_H_

#include "StorageQuery.h"

NS_SA_EXT_BEGIN(storage)

class Adapter : public AllocPool {
public:
	static Adapter *FromContext();

	virtual ~Adapter() { }


	// Object CRUD
	virtual bool createObject(Scheme *, data::Value &data) = 0;
	virtual bool saveObject(Scheme *, uint64_t oid, const data::Value &newObject, const Vector<String> &fields) = 0;
	virtual data::Value patchObject(Scheme *, uint64_t oid, const data::Value &data) = 0;

	virtual bool removeObject(Scheme *, uint64_t oid) = 0;
	virtual data::Value getObject(Scheme *, uint64_t, bool forUpdate) = 0;
	virtual data::Value getObject(Scheme *, const String &, bool forUpdate) = 0;
	virtual bool init(Server &serv, const Map<String, Scheme *> &) = 0;

	virtual data::Value selectObjects(Scheme *, const Query &) = 0;
	virtual size_t countObjects(Scheme *, const Query &) = 0;


	// Object properties CRUD
	virtual data::Value getProperty(Scheme *, uint64_t oid, const Field &) = 0;
	virtual data::Value getProperty(Scheme *, const data::Value &, const Field &) = 0;

	virtual data::Value setProperty(Scheme *, uint64_t oid, const Field &, data::Value &&) = 0;
	virtual data::Value setProperty(Scheme *, const data::Value &, const Field &, data::Value &&) = 0;

	virtual void clearProperty(Scheme *, uint64_t oid, const Field &) = 0;
	virtual void clearProperty(Scheme *, const data::Value &, const Field &) = 0;

	virtual data::Value appendProperty(Scheme *, uint64_t oid, const Field &, data::Value &&) = 0;
	virtual data::Value appendProperty(Scheme *, const data::Value &, const Field &, data::Value &&) = 0;


	// session support
	virtual data::Value getSessionData(const Bytes &) = 0;
	virtual bool setSessionData(const Bytes &, const data::Value &, TimeInterval) = 0;
	virtual bool clearSessionData(const Bytes &) = 0;

	// resource resolver
	virtual Resolver *createResolver(Scheme *, const data::TransformMap *map) = 0;

	virtual void setMinNextOid(uint64_t) = 0;

	virtual bool supportsAtomicPatches() const = 0;

	// Key-Value storage
	virtual bool setData(const String &, const data::Value &, TimeInterval = config::getKeyValueStorageTime()) = 0;
	virtual data::Value getData(const String &) = 0;
	virtual bool clearData(const String &) = 0;

	virtual User * authorizeUser(Scheme *, const String &name, const String &password) = 0;

	virtual void broadcast(const Bytes &) = 0;
	virtual void broadcast(const data::Value &val) {
		broadcast(data::write(val, data::EncodeFormat::Cbor));
	}
};

NS_SA_EXT_END(storage)

#endif /* SERENITY_SRC_STORAGE_STORAGEADAPTER_H_ */
