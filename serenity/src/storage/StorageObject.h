/*
 * StorageObject.h
 *
 *  Created on: 24 февр. 2016 г.
 *      Author: sbkarr
 */

#ifndef SERENITY_SRC_STORAGE_STORAGEOBJECT_H_
#define SERENITY_SRC_STORAGE_STORAGEOBJECT_H_

#include "Define.h"
#include "SPDataWrapper.h"

NS_SA_EXT_BEGIN(storage)

class Object : public data::Wrapper {
public:
	Object(data::Value &&, Scheme *);

	Scheme *getScheme() const;
	uint64_t getObjectId() const;

	void lockProperty(const String &prop);
	void unlockProperty(const String &prop);
	bool isPropertyLocked(const String &prop) const;

	bool isFieldProtected(const String &) const;

	auto begin() { return Wrapper::begin<Object>(this); }
	auto end() { return Wrapper::end<Object>(this); }

	auto begin() const { return Wrapper::begin<Object>(this); }
	auto end() const { return Wrapper::end<Object>(this); }

	bool save(Adapter *, bool force = false);

protected:
	friend class Scheme;

	uint64_t _oid;
	Set<String> _locked;
	Scheme *_scheme;
};

NS_SA_EXT_END(storage)

#endif /* SERENITY_SRC_STORAGE_STORAGEOBJECT_H_ */
