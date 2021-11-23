/**
Copyright (c) 2019 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef COMMON_DB_PQ_SPDBPQDRIVER_H_
#define COMMON_DB_PQ_SPDBPQDRIVER_H_

#include "STSqlDriver.h"

NS_DB_PQ_BEGIN

struct DriverSym;

class Driver : public sql::Driver {
public:
	enum class Status {
		Empty = 0,
		CommandOk,
		TuplesOk,
		CopyOut,
		CopyIn,
		BadResponse,
		NonfatalError,
		FatalError,
		CopyBoth,
		SingleTuple,
	};

	enum class TransactionStatus {
		Idle,
		Active,
		InTrans,
		InError,
		Unknown
	};

	static Driver *open(mem::StringView path = mem::StringView(), const void *external = nullptr);

	virtual ~Driver();

	virtual bool init(Handle handle, const mem::Vector<mem::StringView> &) override;

	virtual void performWithStorage(Handle handle, const mem::Callback<void(const db::Adapter &)> &cb) const;
	virtual Interface *acquireInterface(Handle handle, mem::pool_t *) const;

	virtual Handle connect(const mem::Map<mem::StringView, mem::StringView> &) const;
	virtual void finish(Handle) const;

	virtual Connection getConnection(Handle h) const;

	virtual bool isValid(Handle) const;
	virtual bool isValid(Connection) const;
	virtual bool isIdle(Connection) const;

	virtual int listenForNotifications(Handle) const override;
	virtual bool consumeNotifications(Handle, const mem::Callback<void(mem::StringView)> &) const override;

	virtual bool isNotificationsSupported() const override { return true; }

	TransactionStatus getTransactionStatus(Connection) const;

	Status getStatus(Result res) const;

	bool isBinaryFormat(Result res, size_t field) const;
	bool isNull(Result res, size_t row, size_t field) const;
	char *getValue(Result res, size_t row, size_t field) const;
	size_t getLength(Result res, size_t row, size_t field) const;

	char *getName(Result res, size_t field) const;
	unsigned int getType(Result res, size_t field) const;

	size_t getNTuples(Result res) const;
	size_t getNFields(Result res) const;
	size_t getCmdTuples(Result res) const;

	char *getStatusMessage(Status) const;
	char *getResultErrorMessage(Result res) const;

	void clearResult(Result res) const;

	Result exec(Connection conn, const char *query) const;
	Result exec(Connection conn, const char *command, int nParams, const char *const *paramValues,
			const int *paramLengths, const int *paramFormats, int resultFormat) const;

	operator bool () const { return _handle != nullptr; }

	Interface::StorageType getTypeById(uint32_t) const;
	mem::StringView getTypeNameById(uint32_t) const;

protected:
	Driver(mem::StringView, const void *external);

	Handle doConnect(const char * const *keywords, const char * const *values, int expand_dbname) const;

	bool _init = false;
	mem::StringView _driverPath;

	mem::Vector<mem::Pair<uint32_t, db::Interface::StorageType>> _storageTypes;
	mem::Vector<mem::Pair<uint32_t, mem::String>> _customTypes;

	DriverSym *_handle = nullptr;
	const void *_external = nullptr;
};

class ResultInterface : public db::ResultInterface {
public:
	inline static constexpr bool pgsql_is_success(Driver::Status x) {
		return (x == Driver::Status::Empty) || (x == Driver::Status::CommandOk) || (x == Driver::Status::TuplesOk) || (x == Driver::Status::SingleTuple);
	}

	ResultInterface(const Driver *d, Driver::Result res);

	virtual ~ResultInterface();
	virtual bool isBinaryFormat(size_t field) const override;
	virtual bool isNull(size_t row, size_t field) override;
	virtual mem::StringView toString(size_t row, size_t field) override;
	virtual mem::BytesView toBytes(size_t row, size_t field) override;
	virtual int64_t toInteger(size_t row, size_t field) override;
	virtual double toDouble(size_t row, size_t field) override;
	virtual bool toBool(size_t row, size_t field) override;
	virtual mem::Value toTypedData(size_t row, size_t field) override;
	virtual int64_t toId() override;
	virtual mem::StringView getFieldName(size_t field) override;
	virtual bool isSuccess() const override;
	virtual size_t getRowsCount() const override;
	virtual size_t getFieldsCount() const override;
	virtual size_t getAffectedRows() const override;
	virtual mem::Value getInfo() const override;
	virtual void clear() override;
	Driver::Status getError() const;

public:
	const Driver *driver = nullptr;
	Driver::Result result = Driver::Result(nullptr);
	Driver::Status err = Driver::Status::Empty;
};

NS_DB_PQ_END

#endif /* COMMON_DB_PQ_SPDBPQDRIVER_H_ */
