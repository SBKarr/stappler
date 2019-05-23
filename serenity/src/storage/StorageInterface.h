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

#ifndef SERENITY_SRC_STORAGE_STORAGEINTERFACE_H_
#define SERENITY_SRC_STORAGE_STORAGEINTERFACE_H_

#include "StorageAuth.h"

NS_SA_EXT_BEGIN(storage)

class Interface : public AllocPool {
public:
	struct Config {
		String name;
		const Scheme *fileScheme;
	};

	virtual ~Interface() { }

public: // key-value storage
	virtual bool set(const CoderSource &, const data::Value &, TimeInterval) = 0;
	virtual data::Value get(const CoderSource &) = 0;
	virtual bool clear(const CoderSource &) = 0;

public: // resource requests
	virtual Vector<int64_t> performQueryListForIds(const QueryList &, size_t count) = 0;
	virtual data::Value performQueryList(const QueryList &, size_t count, bool forUpdate, const Field *) = 0;

public:
	virtual bool init(const Config &serv, const Map<String, const Scheme *> &) = 0;

	virtual data::Value select(Worker &, const Query &) = 0;

	virtual data::Value create(Worker &, const data::Value &) = 0;
	virtual data::Value save(Worker &, uint64_t oid, const data::Value &obj, const Vector<String> &fields) = 0;
	virtual data::Value patch(Worker &, uint64_t oid, const data::Value &patch) = 0;

	virtual bool remove(Worker &, uint64_t oid) = 0;

	virtual size_t count(Worker &, const Query &) = 0;

public:
	virtual data::Value field(Action, Worker &, uint64_t oid, const Field &, data::Value &&) = 0;
	virtual data::Value field(Action, Worker &, const data::Value &, const Field &, data::Value &&) = 0;

	virtual bool addToView(const FieldView &, const Scheme *, uint64_t oid, const data::Value &) = 0;
	virtual bool removeFromView(const FieldView &, const Scheme *, uint64_t oid) = 0;

	virtual Vector<int64_t> getReferenceParents(const Scheme &, uint64_t oid, const Scheme *, const Field *) = 0;

public: // others
	virtual bool beginTransaction() = 0;
	virtual bool endTransaction() = 0;

	virtual User * authorizeUser(const Auth &, const StringView &name, const StringView &password) = 0;

	virtual void broadcast(const Bytes &) = 0;

	virtual int64_t getDeltaValue(const Scheme &) = 0;
	virtual int64_t getDeltaValue(const Scheme &, const FieldView &, uint64_t) = 0;

public:
	void cancelTransaction() { transactionStatus = TransactionStatus::Rollback; }
	bool isInTransaction() const { return transactionStatus != TransactionStatus::None; }
	TransactionStatus getTransactionStatus() const { return transactionStatus; }

protected:
    TransactionStatus transactionStatus = TransactionStatus::None;
};

class Binder {
public:
	struct DataField {
		const storage::Field *field;
		const data::Value &data;
		bool force = false;
	};

	struct FullTextField {
		const data::Value &data;
	};

	struct FullTextRank {
		StringView scheme;
		const Field *field;
	};

	struct TypeString {
		StringView str;
		StringView type;

		template <typename Str, typename Type>
		TypeString(Str && str, Type && type)
		: str(str), type(type) { }
	};

	void setInterface(QueryInterface *);
	QueryInterface * getInterface() const;

	void writeBind(StringStream &, int64_t);
	void writeBind(StringStream &, uint64_t);
	void writeBind(StringStream &, const String &);
	void writeBind(StringStream &, String &&);
	void writeBind(StringStream &, const StringView &);
	void writeBind(StringStream &, const Bytes &);
	void writeBind(StringStream &, Bytes &&);
	void writeBind(StringStream &, const CoderSource &);
	void writeBind(StringStream &, const data::Value &);
	void writeBind(StringStream &, const DataField &);
	void writeBind(StringStream &, const TypeString &);
	void writeBind(StringStream &, const FullTextField &);
	void writeBind(StringStream &, const FullTextRank &);
	void writeBind(StringStream &, const FullTextData &);

	void clear();

protected:
	QueryInterface *_iface = nullptr;
};

class QueryInterface {
public:
	virtual ~QueryInterface() = default;

	virtual void bindInt(Binder &, StringStream &, int64_t) = 0;
	virtual void bindUInt(Binder &, StringStream &, uint64_t) = 0;
	virtual void bindString(Binder &, StringStream &, const String &) = 0;
	virtual void bindMoveString(Binder &, StringStream &, String &&) = 0;
	virtual void bindStringView(Binder &, StringStream &, const StringView &) = 0;
	virtual void bindBytes(Binder &, StringStream &, const Bytes &) = 0;
	virtual void bindMoveBytes(Binder &, StringStream &, Bytes &&) = 0;
	virtual void bindCoderSource(Binder &, StringStream &, const CoderSource &) = 0;
	virtual void bindValue(Binder &, StringStream &, const data::Value &) = 0;
	virtual void bindDataField(Binder &, StringStream &, const Binder::DataField &) = 0;
	virtual void bindTypeString(Binder &, StringStream &, const Binder::TypeString &) = 0;
	virtual void bindFullText(Binder &, StringStream &, const Binder::FullTextField &) = 0;
	virtual void bindFullTextRank(Binder &, StringStream &, const Binder::FullTextRank &) = 0;
	virtual void bindFullTextData(Binder &, StringStream &, const FullTextData &) = 0;

	virtual void clear() = 0;
};

class ResultInterface {
public:
	virtual ~ResultInterface() = default;

	virtual bool isBinaryFormat(size_t field) const = 0;

	virtual bool isNull(size_t row, size_t field) = 0;

	virtual StringView toString(size_t row, size_t field) = 0;
	virtual DataReader<ByteOrder::Host> toBytes(size_t row, size_t field) = 0;

	virtual int64_t toInteger(size_t row, size_t field) = 0;
	virtual double toDouble(size_t row, size_t field) = 0;
	virtual bool toBool(size_t row, size_t field) = 0;

	virtual int64_t toId() = 0;

	virtual StringView getFieldName(size_t field) = 0;

	virtual bool isSuccess() const = 0;
	virtual size_t getRowsCount() const = 0;
	virtual size_t getFieldsCount() const = 0;
	virtual size_t getAffectedRows() const = 0;

	virtual data::Value getInfo() const = 0;
	virtual void clear() = 0;
};

NS_SA_EXT_END(storage)

#endif /* SERENITY_SRC_STORAGE_STORAGEINTERFACE_H_ */
