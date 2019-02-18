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

#ifndef SERENITY_SRC_STORAGE_STORAGEADAPTER_H_
#define SERENITY_SRC_STORAGE_STORAGEADAPTER_H_

#include "StorageInterface.h"

NS_SA_EXT_BEGIN(storage)

class Adapter final : public AllocPool {
public:
	static Adapter FromContext();

	Adapter(Interface *);

	Adapter(const Adapter &);
	Adapter& operator=(const Adapter &);

	operator bool () const { return _interface != nullptr; }

	Interface *interface() const;

public: // key-value storage
	bool set(const CoderSource &, const data::Value &, TimeInterval = config::getKeyValueStorageTime());
	data::Value get(const CoderSource &);
	bool clear(const CoderSource &);

public: // resource requests
	Resource *makeResource(ResourceType, QueryList &&, const Field *);

public:
	bool init(Server &serv, const Map<String, const Scheme *> &);

	User * authorizeUser(const Auth &, const StringView &name, const StringView &password) const;

	void broadcast(const Bytes &);
	void broadcast(const data::Value &val);

	template <typename Callback>
	bool performInTransaction(Callback && t);

	Vector<int64_t> getReferenceParents(const Scheme &, uint64_t oid, const Scheme *, const Field *) const;

protected:
	friend class Transaction;

	int64_t getDeltaValue(const Scheme &); // scheme-based delta
	int64_t getDeltaValue(const Scheme &, const FieldView &, uint64_t); // view-based delta

	Vector<int64_t> performQueryListForIds(const QueryList &, size_t count = maxOf<size_t>()) const;
	data::Value performQueryList(const QueryList &, size_t count = maxOf<size_t>(), bool forUpdate = false, const Field * = nullptr) const;

	data::Value select(Worker &, const Query &) const;

	data::Value create(Worker &, const data::Value &) const;
	data::Value save(Worker &, uint64_t oid, const data::Value &obj, const Vector<String> &fields) const;
	data::Value patch(Worker &, uint64_t oid, const data::Value &patch) const;

	bool remove(Worker &, uint64_t oid) const;

	size_t count(Worker &, const Query &) const;

	void scheduleAutoField(const Scheme &, const Field &, uint64_t id);

protected:
	data::Value field(Action, Worker &, uint64_t oid, const Field &, data::Value && = data::Value()) const;
	data::Value field(Action, Worker &, const data::Value &, const Field &, data::Value && = data::Value()) const;

	bool addToView(const FieldView &, const Scheme *, uint64_t oid, const data::Value &) const;
	bool removeFromView(const FieldView &, const Scheme *, uint64_t oid) const;

	bool beginTransaction() const;
	bool endTransaction() const;

	void cancelTransaction() const;
	bool isInTransaction() const;
	TransactionStatus getTransactionStatus() const;

	void runAutoFields(const Transaction &t, const Vector<uint64_t> &vec, const Scheme &, const Field &);

protected:
	Interface *_interface;
};

template <typename Callback>
inline bool Adapter::performInTransaction(Callback && t) {
	if (isInTransaction()) {
		if (!t()) {
			cancelTransaction();
		} else {
			return true;
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

NS_SA_EXT_END(storage)

#endif /* SERENITY_SRC_STORAGE_STORAGEADAPTER_H_ */
