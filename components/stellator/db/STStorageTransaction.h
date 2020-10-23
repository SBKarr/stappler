/**
Copyright (c) 2018-2019 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef STELLATOR_DB_STSTORAGETRANSACTION_H_
#define STELLATOR_DB_STSTORAGETRANSACTION_H_

#include "STStorageAdapter.h"
#include "STStorageField.h"
#include "STStorageQueryList.h"

NS_DB_BEGIN

enum class AccessRoleId {
	Nobody = 0,
	Authorized = 1,
	UserDefined1,
	UserDefined2,
	UserDefined3,
	UserDefined4,
	UserDefined5,
	UserDefined6,
	UserDefined7,
	UserDefined8,
	UserDefined9,
	UserDefined10,
	UserDefined11,
	Admin,
	System,
	Default,
	Max = 16,
};

class Transaction : public mem::AllocBase {
public:
	enum Op {
		None = 0,
		Id,
		Select,
		Count,
		Remove,
		Create,
		Save,
		Patch,
		FieldGet,
		FieldSet,
		FieldAppend,
		FieldClear,
		FieldCount,
		Delta,
		DeltaView,
		RemoveFromView,
		AddToView,
		Max,
	};

	struct Data : AllocPool {
		Adapter adapter;
		mem::pool_t * pool;
		mem::Map<mem::String, mem::Value> data;
		int status = 0;

		mutable uint32_t refCount = 1;
		mutable mem::Map<int64_t, mem::Value> objects;
		mutable AccessRoleId role = AccessRoleId::Nobody;
		mutable mem::Map<mem::pool_t *, uint32_t> pools;

		Data(const Adapter &, stappler::memory::pool_t * = nullptr);
	};

	static Op getTransactionOp(Action);

	static Transaction acquire(const Adapter &);
	static Transaction acquire();

	static Transaction acquireIfExists();
	static Transaction acquireIfExists(stappler::memory::pool_t *);

	Transaction(nullptr_t);

	void setRole(AccessRoleId) const;
	AccessRoleId getRole() const;

	void setStatus(int);
	int getStatus() const;

	const mem::Value &setValue(const mem::StringView &, mem::Value &&);
	const mem::Value &getValue(const mem::StringView &) const;

	const mem::Value &setObject(int64_t, mem::Value &&) const;
	const mem::Value &getObject(int64_t) const;

	void setAdapter(const Adapter &);
	const Adapter &getAdapter() const;

	operator bool () const { return _data != nullptr && _data->adapter; }

	const mem::Value &acquireObject(const Scheme &, uint64_t oid) const;

public: // adapter interface
	template <typename Callback>
	bool perform(Callback && cb) const;

	template <typename Callback>
	bool performAsSystem(Callback && cb) const;

	bool isInTransaction() const;
	TransactionStatus getTransactionStatus() const;

	// returns Array with zero or more Dictionaries with object data or Null value
	mem::Value select(Worker &, const Query &) const;

	size_t count(Worker &, const Query &) const;

	bool remove(Worker &t, uint64_t oid) const;

	mem::Value create(Worker &, mem::Value &data) const;
	mem::Value save(Worker &, uint64_t oid, const mem::Value &newObject, const mem::Vector<mem::String> &fields) const;
	mem::Value patch(Worker &, uint64_t oid, mem::Value &data) const;

	mem::Value field(Action, Worker &, uint64_t oid, const Field &, mem::Value && = mem::Value()) const;
	mem::Value field(Action, Worker &, const mem::Value &, const Field &, mem::Value && = mem::Value()) const;

	bool removeFromView(const Scheme &, const FieldView &, uint64_t oid, const mem::Value &obj) const;
	bool addToView(const Scheme &, const FieldView &, uint64_t oid, const mem::Value &obj, const mem::Value &viewObj) const;

	int64_t getDeltaValue(const Scheme &); // scheme-based delta
	int64_t getDeltaValue(const Scheme &, const FieldView &, uint64_t); // view-based delta

	mem::Vector<int64_t> performQueryListForIds(const QueryList &, size_t count = stappler::maxOf<size_t>());
	mem::Value performQueryList(const QueryList &, size_t count = stappler::maxOf<size_t>(), bool forUpdate = false);
	mem::Value performQueryListField(const QueryList &, const Field &);

	void scheduleAutoField(const Scheme &, const Field &, uint64_t id) const;

protected:
	struct TransactionGuard {
		TransactionGuard(const Transaction &t) : _t(&t) { _t->retain(); }
		~TransactionGuard() { _t->release(); }

		const Transaction *_t;
	};

	friend struct TransactionGuard;
	friend class Worker;

	bool beginTransaction() const;
	bool endTransaction() const;
	void cancelTransaction() const;

	void clearObjectStorage() const;

	bool processReturnObject(const Scheme &, mem::Value &) const;
	bool processReturnField(const Scheme &, const mem::Value &obj, const Field &, mem::Value &) const;

	bool isOpAllowed(const Scheme &, Op, const Field * = nullptr) const;

	void retain() const;
	void release() const;

	Transaction(Data *);

	Data *_data = nullptr;
};

template <typename Callback>
inline bool Transaction::perform(Callback && cb) const {
	TransactionGuard g(*this);

	if (isInTransaction()) {
		if (!cb()) {
			cancelTransaction();
		} else {
			return true;
		}
	} else {
		if (beginTransaction()) {
			if (!cb()) {
				cancelTransaction();
			}
			return endTransaction();
		}
	}
	return false;
}

template <typename Callback>
inline bool Transaction::performAsSystem(Callback && cb) const {
	auto tmpRole = getRole();
	setRole(AccessRoleId::System);
	auto ret = perform(std::forward<Callback>(cb));
	setRole(tmpRole);
	return ret;
}

struct AccessRole : public mem::AllocBase {
	using OnSelect = stappler::ValueWrapper<mem::Function<bool(Worker &, const Query &)>, class OnSelectTag>;
	using OnCount = stappler::ValueWrapper<mem::Function<bool(Worker &, const Query &)>, class OnCountTag>;
	using OnCreate = stappler::ValueWrapper<mem::Function<bool(Worker &, mem::Value &obj)>, class OnCreateTag>;
	using OnPatch = stappler::ValueWrapper<mem::Function<bool(Worker &, int64_t id, mem::Value &obj)>, class OnPatchTag>;
	using OnSave = stappler::ValueWrapper<mem::Function<bool(Worker &, const mem::Value &, mem::Value &obj, mem::Vector<mem::String> &fields)>, class OnSaveTag>;
	using OnRemove = stappler::ValueWrapper<mem::Function<bool(Worker &, const mem::Value &)>, class OnRemoveTag>;
	using OnField = stappler::ValueWrapper<mem::Function<bool(Action, Worker &, const mem::Value &, const Field &, mem::Value &)>, class OnFieldTag>;
	using OnReturn = stappler::ValueWrapper<mem::Function<bool(const Scheme &, mem::Value &)>, class OnReturnTag>;
	using OnReturnField = stappler::ValueWrapper<mem::Function<bool(const Scheme &, const Field &, mem::Value &)>, class OnReturnFieldTag>;

	template <typename ... Args>
	static AccessRole Empty(Args && ... args);

	template <typename ... Args>
	static AccessRole Default(Args && ... args);

	template <typename ... Args>
	static AccessRole Admin(Args && ... args);

	template <typename T, typename ... Args>
	AccessRole &define(T &&, Args && ... args);

	AccessRole &define();
	AccessRole &define(AccessRoleId);
	AccessRole &define(Transaction::Op);
	AccessRole &define(OnSelect &&);
	AccessRole &define(OnCount &&);
	AccessRole &define(OnCreate &&);
	AccessRole &define(OnPatch &&);
	AccessRole &define(OnSave &&);
	AccessRole &define(OnRemove &&);
	AccessRole &define(OnField &&);
	AccessRole &define(OnReturn &&);
	AccessRole &define(OnReturnField &&);

	std::bitset<stappler::toInt(AccessRoleId::Max)> users;
	std::bitset<stappler::toInt(Transaction::Op::Max)> operations;

	mem::Function<bool(Worker &, const Query &)> onSelect;
	mem::Function<bool(Worker &, const Query &)> onCount;

	mem::Function<bool(Worker &, mem::Value &obj)> onCreate;
	mem::Function<bool(Worker &, int64_t id, mem::Value &obj)> onPatch;
	mem::Function<bool(Worker &, const mem::Value &, mem::Value &obj, mem::Vector<mem::String> &fields)> onSave;
	mem::Function<bool(Worker &, const mem::Value &)> onRemove;

	mem::Function<bool(Action, Worker &, const mem::Value &, const Field &, mem::Value &)> onField;

	mem::Function<bool(const Scheme &, mem::Value &)> onReturn;
	mem::Function<bool(const Scheme &, const Field &, mem::Value &)> onReturnField;
};

template <typename T, typename ... Args>
inline AccessRole &AccessRole::define(T &&v, Args && ... args) {
	define(std::forward<T>(v));
	define(std::forward<Args>(args)...);
	return *this;
}

template <typename ... Args>
AccessRole AccessRole::Empty(Args && ... args) {
	AccessRole ret;
	ret.define(std::forward<Args>(args)...);
	return ret;
}

template <typename ... Args>
AccessRole AccessRole::Default(Args && ... args) {
	AccessRole ret;

	ret.operations.set(Transaction::Op::Id);
	ret.operations.set(Transaction::Op::Select);
	ret.operations.set(Transaction::Op::Count);
	ret.operations.set(Transaction::Op::Delta);
	ret.operations.set(Transaction::Op::DeltaView);
	ret.operations.set(Transaction::Op::FieldGet);
	ret.operations.set(Transaction::Op::FieldCount);

	ret.define(std::forward<Args>(args)...);

	return ret;
}

template <typename ... Args>
AccessRole AccessRole::Admin(Args && ... args) {
	AccessRole ret;

	ret.operations.set(Transaction::Op::Id);
	ret.operations.set(Transaction::Op::Select);
	ret.operations.set(Transaction::Op::Count);
	ret.operations.set(Transaction::Op::Delta);
	ret.operations.set(Transaction::Op::DeltaView);
	ret.operations.set(Transaction::Op::FieldGet);

	ret.operations.set(Transaction::Op::Remove);
	ret.operations.set(Transaction::Op::Create);
	ret.operations.set(Transaction::Op::Save);
	ret.operations.set(Transaction::Op::Patch);
	ret.operations.set(Transaction::Op::FieldSet);
	ret.operations.set(Transaction::Op::FieldAppend);
	ret.operations.set(Transaction::Op::FieldClear);
	ret.operations.set(Transaction::Op::FieldCount);
	ret.operations.set(Transaction::Op::RemoveFromView);
	ret.operations.set(Transaction::Op::AddToView);

	ret.define(std::forward<Args>(args)...);

	return ret;
}

NS_DB_END

#endif /* STELLATOR_DB_STSTORAGETRANSACTION_H_ */
