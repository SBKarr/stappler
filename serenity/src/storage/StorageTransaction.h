/**
Copyright (c) 2018 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef SERENITY_SRC_STORAGE_STORAGETRANSACTION_H_
#define SERENITY_SRC_STORAGE_STORAGETRANSACTION_H_

#include "StorageField.h"
#include "StorageQuery.h"
#include "InputFilter.h"

NS_SA_EXT_BEGIN(storage)

enum class AccessRoleId {
	Admin = 0,
	Operator = 1,
	Nobody = 2,
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
	UserDefined12,
	System,
	Max = 16,
};

class Transaction : public AllocPool {
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
		Delta,
		DeltaView,
		RemoveFromView,
		AddToView,
		Max,
	};

	static Op getTransactionOp(Action);

	static Transaction acquire(const Adapter &);
	static Transaction acquire();

	void setRole(AccessRoleId) const;
	AccessRoleId getRole() const;

	const data::Value &setValue(const StringView &, data::Value &&);
	const data::Value &getValue(const StringView &) const;

	const data::Value &setObject(int64_t, data::Value &&) const;
	const data::Value &getObject(int64_t) const;

	void setStatus(int);
	int getStatus() const;

	void setAdapter(const Adapter &);
	const Adapter &getAdapter() const;

	operator bool () const { return _data != nullptr; }

	const data::Value &acquireObject(const Scheme &, uint64_t oid) const;

public: // adapter interface
	template <typename Callback>
	bool perform(Callback && cb) const;

	template <typename Callback>
	bool performAsSystem(Callback && cb) const;

	bool isInTransaction() const;
	TransactionStatus getTransactionStatus() const;

	// returns Array with zero or more Dictionaries with object data or Null value
	data::Value select(Worker &, const Query &) const;

	size_t count(Worker &, const Query &) const;

	bool remove(Worker &t, uint64_t oid) const;

	data::Value create(Worker &, data::Value &data) const;
	data::Value save(Worker &, uint64_t oid, const data::Value &newObject, const Vector<String> &fields) const;
	data::Value patch(Worker &, uint64_t oid, const data::Value &data) const;

	data::Value field(Action, Worker &, uint64_t oid, const Field &, data::Value && = data::Value()) const;
	data::Value field(Action, Worker &, const data::Value &, const Field &, data::Value && = data::Value()) const;

	bool removeFromView(const Scheme &, const FieldView &, uint64_t oid, const data::Value &obj) const;
	bool addToView(const Scheme &, const FieldView &, uint64_t oid, const data::Value &obj, const data::Value &viewObj) const;

	int64_t getDeltaValue(const Scheme &); // scheme-based delta
	int64_t getDeltaValue(const Scheme &, const FieldView &, uint64_t); // view-based delta

	Vector<int64_t> performQueryListForIds(const QueryList &, size_t count = maxOf<size_t>());
	data::Value performQueryList(const QueryList &, size_t count = maxOf<size_t>(), bool forUpdate = false, const Field * = nullptr);

protected:
	bool beginTransaction() const;
	bool endTransaction() const;
	void cancelTransaction() const;

	void clearObjectStorage() const;

	bool processReturnObject(const Scheme &, data::Value &) const;
	bool processReturnField(const Scheme &, const data::Value &obj, const Field &, data::Value &) const;

	bool isOpAllowed(const Scheme &, Op, const Field * = nullptr) const;

	struct Data : AllocPool {
		Adapter adapter;
		Request request;
		mutable Map<int64_t, data::Value> objects;
		Map<String, data::Value> data;
		mutable AccessRoleId role = AccessRoleId::Nobody;
		int status = 0;

		Data(const Adapter &, const Request & = Request());
	};

	Transaction(Data *);

	Data *_data = nullptr;
};

template <typename Callback>
inline bool Transaction::perform(Callback && cb) const {
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

struct AccessRole : public AllocPool {
	static AccessRole Default();
	static AccessRole Admin();

	std::bitset<toInt(Transaction::Op::Max)> operations;

	Function<bool(Worker &, const Query &)> onSelect;
	Function<bool(Worker &, const Query &)> onCount;

	Function<bool(Worker &, data::Value &obj)> onCreate;
	Function<bool(Worker &, const data::Value &, data::Value &obj, Vector<String> &fields)> onSave;
	Function<bool(Worker &, const data::Value &)> onRemove;

	Function<bool(Action, Worker &, const data::Value &, const Field &, data::Value &)> onField;

	Function<bool(const Scheme &, data::Value &)> onReturn;
	Function<bool(const Scheme &, const Field &, data::Value &)> onReturnField;
};

NS_SA_EXT_END(storage)

#endif /* SERENITY_SRC_STORAGE_STORAGETRANSACTION_H_ */
