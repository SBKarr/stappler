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

#ifndef STELLATOR_DB_STSTORAGEINTERFACE_H_
#define STELLATOR_DB_STSTORAGEINTERFACE_H_

#include "STStorageAuth.h"

NS_DB_BEGIN

class Interface : public mem::AllocBase {
public:
	struct Config {
		mem::String name;
		const Scheme *fileScheme;
	};

	virtual ~Interface() { }

public: // key-value storage
	virtual bool set(const stappler::CoderSource &, const mem::Value &, stappler::TimeInterval) = 0;
	virtual mem::Value get(const stappler::CoderSource &) = 0;
	virtual bool clear(const stappler::CoderSource &) = 0;

public: // resource requests
	virtual mem::Vector<int64_t> performQueryListForIds(const QueryList &, size_t count) = 0;
	virtual mem::Value performQueryList(const QueryList &, size_t count, bool forUpdate, const Field *) = 0;

public:
	virtual bool init(const Config &serv, const mem::Map<mem::String, const Scheme *> &) = 0;

	virtual mem::Value select(Worker &, const Query &) = 0;

	virtual mem::Value create(Worker &, const mem::Value &) = 0;
	virtual mem::Value save(Worker &, uint64_t oid, const mem::Value &obj, const mem::Vector<mem::String> &fields) = 0;
	virtual mem::Value patch(Worker &, uint64_t oid, const mem::Value &patch) = 0;

	virtual bool remove(Worker &, uint64_t oid) = 0;

	virtual size_t count(Worker &, const Query &) = 0;

public:
	virtual mem::Value field(Action, Worker &, uint64_t oid, const Field &, mem::Value &&) = 0;
	virtual mem::Value field(Action, Worker &, const mem::Value &, const Field &, mem::Value &&) = 0;

	virtual bool addToView(const FieldView &, const Scheme *, uint64_t oid, const mem::Value &) = 0;
	virtual bool removeFromView(const FieldView &, const Scheme *, uint64_t oid) = 0;

	virtual mem::Vector<int64_t> getReferenceParents(const Scheme &, uint64_t oid, const Scheme *, const Field *) = 0;

public: // others
	virtual bool beginTransaction() = 0;
	virtual bool endTransaction() = 0;

	virtual User * authorizeUser(const Auth &, const mem::StringView &name, const mem::StringView &password) = 0;

	virtual void broadcast(const mem::Bytes &) = 0;

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
		const db::Field *field;
		const mem::Value &data;
		bool force = false;
	};

	struct FullTextField {
		const mem::Value &data;
	};

	struct FullTextRank {
		mem::StringView scheme;
		const Field *field;
		mem::StringView query;
	};

	struct TypeString {
		mem::StringView str;
		mem::StringView type;

		template <typename Str, typename Type>
		TypeString(Str && str, Type && type)
		: str(str), type(type) { }
	};

	void setInterface(QueryInterface *);
	QueryInterface * getInterface() const;

	void writeBind(mem::StringStream &, int64_t);
	void writeBind(mem::StringStream &, uint64_t);
	void writeBind(mem::StringStream &, double);
	void writeBind(mem::StringStream &query, stappler::Time val);
	void writeBind(mem::StringStream &query, stappler::TimeInterval val);
	void writeBind(mem::StringStream &, const mem::String &);
	void writeBind(mem::StringStream &, mem::String &&);
	void writeBind(mem::StringStream &, const mem::StringView &);
	void writeBind(mem::StringStream &, const mem::Bytes &);
	void writeBind(mem::StringStream &, mem::Bytes &&);
	void writeBind(mem::StringStream &, const stappler::CoderSource &);
	void writeBind(mem::StringStream &, const mem::Value &);
	void writeBind(mem::StringStream &, const DataField &);
	void writeBind(mem::StringStream &, const TypeString &);
	void writeBind(mem::StringStream &, const FullTextField &);
	void writeBind(mem::StringStream &, const FullTextRank &);
	void writeBind(mem::StringStream &, const FullTextData &);

	void clear();

protected:
	QueryInterface *_iface = nullptr;
};

class QueryInterface {
public:
	virtual ~QueryInterface() = default;

	virtual void bindInt(Binder &, mem::StringStream &, int64_t) = 0;
	virtual void bindUInt(Binder &, mem::StringStream &, uint64_t) = 0;
	virtual void bindDouble(Binder &, mem::StringStream &, double) = 0;
	virtual void bindString(Binder &, mem::StringStream &, const mem::String &) = 0;
	virtual void bindMoveString(Binder &, mem::StringStream &, mem::String &&) = 0;
	virtual void bindStringView(Binder &, mem::StringStream &, const mem::StringView &) = 0;
	virtual void bindBytes(Binder &, mem::StringStream &, const mem::Bytes &) = 0;
	virtual void bindMoveBytes(Binder &, mem::StringStream &, mem::Bytes &&) = 0;
	virtual void bindCoderSource(Binder &, mem::StringStream &, const stappler::CoderSource &) = 0;
	virtual void bindValue(Binder &, mem::StringStream &, const mem::Value &) = 0;
	virtual void bindDataField(Binder &, mem::StringStream &, const Binder::DataField &) = 0;
	virtual void bindTypeString(Binder &, mem::StringStream &, const Binder::TypeString &) = 0;
	virtual void bindFullText(Binder &, mem::StringStream &, const Binder::FullTextField &) = 0;
	virtual void bindFullTextRank(Binder &, mem::StringStream &, const Binder::FullTextRank &) = 0;
	virtual void bindFullTextData(Binder &, mem::StringStream &, const FullTextData &) = 0;

	virtual void clear() = 0;
};

class ResultInterface {
public:
	virtual ~ResultInterface() = default;

	virtual bool isBinaryFormat(size_t field) const = 0;

	virtual bool isNull(size_t row, size_t field) = 0;

	virtual mem::StringView toString(size_t row, size_t field) = 0;
	virtual stappler::DataReader<stappler::ByteOrder::Host> toBytes(size_t row, size_t field) = 0;

	virtual int64_t toInteger(size_t row, size_t field) = 0;
	virtual double toDouble(size_t row, size_t field) = 0;
	virtual bool toBool(size_t row, size_t field) = 0;

	virtual int64_t toId() = 0;

	virtual mem::StringView getFieldName(size_t field) = 0;

	virtual bool isSuccess() const = 0;
	virtual size_t getRowsCount() const = 0;
	virtual size_t getFieldsCount() const = 0;
	virtual size_t getAffectedRows() const = 0;

	virtual mem::Value getInfo() const = 0;
	virtual void clear() = 0;
};

NS_DB_END

#endif /* STELLATOR_DB_STSTORAGEINTERFACE_H_ */
