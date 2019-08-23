/**
Copyright (c) 2016-2019 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef STELLATOR_DB_STSTORAGEADAPTER_H_
#define STELLATOR_DB_STSTORAGEADAPTER_H_

#include "STStorageInterface.h"

NS_DB_BEGIN

class Adapter final : public mem::AllocBase {
public:
	static Adapter FromContext();

	Adapter(Interface *);

	Adapter(const Adapter &);
	Adapter& operator=(const Adapter &);

	operator bool () const { return _interface != nullptr; }

	Interface *interface() const;

public: // key-value storage
	bool set(const stappler::CoderSource &, const mem::Value &, stappler::TimeInterval = config::getKeyValueStorageTime());
	mem::Value get(const stappler::CoderSource &);
	bool clear(const stappler::CoderSource &);

public:
	bool init(const Interface::Config &cfg, const mem::Map<mem::String, const Scheme *> &);

	User * authorizeUser(const Auth &, const mem::StringView &name, const mem::StringView &password) const;

	void broadcast(const mem::Bytes &);
	void broadcast(const mem::Value &val);

	template <typename Callback>
	bool performInTransaction(Callback && t);

	mem::Vector<int64_t> getReferenceParents(const Scheme &, uint64_t oid, const Scheme *, const Field *) const;

protected:
	friend class Transaction;

	int64_t getDeltaValue(const Scheme &); // scheme-based delta
	int64_t getDeltaValue(const Scheme &, const FieldView &, uint64_t); // view-based delta

	mem::Vector<int64_t> performQueryListForIds(const QueryList &, size_t count = stappler::maxOf<size_t>()) const;
	mem::Value performQueryList(const QueryList &, size_t count = stappler::maxOf<size_t>(), bool forUpdate = false) const;

	mem::Value select(Worker &, const Query &) const;

	mem::Value create(Worker &, const mem::Value &) const;
	mem::Value save(Worker &, uint64_t oid, const mem::Value &obj, const mem::Vector<mem::String> &fields) const;
	mem::Value patch(Worker &, uint64_t oid, const mem::Value &patch) const;

	bool remove(Worker &, uint64_t oid) const;

	size_t count(Worker &, const Query &) const;

	void scheduleAutoField(const Scheme &, const Field &, uint64_t id);

protected:
	mem::Value field(Action, Worker &, uint64_t oid, const Field &, mem::Value && = mem::Value()) const;
	mem::Value field(Action, Worker &, const mem::Value &, const Field &, mem::Value && = mem::Value()) const;

	bool addToView(const FieldView &, const Scheme *, uint64_t oid, const mem::Value &) const;
	bool removeFromView(const FieldView &, const Scheme *, uint64_t oid) const;

	bool beginTransaction() const;
	bool endTransaction() const;

	void cancelTransaction() const;
	bool isInTransaction() const;
	TransactionStatus getTransactionStatus() const;

	void runAutoFields(const Transaction &t, const mem::Vector<uint64_t> &vec, const Scheme &, const Field &);

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

NS_DB_END

#endif /* STELLATOR_DB_STSTORAGEADAPTER_H_ */
