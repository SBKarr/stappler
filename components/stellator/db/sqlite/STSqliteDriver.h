/**
 Copyright (c) 2021 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef COMPONENTS_STELLATOR_DB_SQLITE_STSQLITEDRIVER_H_
#define COMPONENTS_STELLATOR_DB_SQLITE_STSQLITEDRIVER_H_

#include "STSqlDriver.h"

NS_DB_SQLITE_BEGIN

class Driver : public sql::Driver {
public:
	static Driver *open(mem::StringView path = mem::StringView());

	virtual ~Driver();

	virtual bool init(Handle handle, const mem::Vector<mem::StringView> &) override;

	virtual void performWithStorage(Handle handle, const mem::Callback<void(const db::Adapter &)> &cb) const override;
	virtual Interface *acquireInterface(Handle handle, mem::pool_t *) const override;

	virtual Handle connect(const mem::Map<mem::StringView, mem::StringView> &) const override;
	virtual void finish(Handle) const override;

	virtual Connection getConnection(Handle h) const override;

	virtual bool isValid(Handle) const override;
	virtual bool isValid(Connection) const override;
	virtual bool isIdle(Connection) const override;

	virtual bool isNotificationsSupported() const override { return false; }

	mem::StringView getDbName(Handle) const;
	mem::Value getInfo(Connection, int) const;

	void setUserId(Handle, int64_t) const;

protected:
	Driver(mem::StringView);

	bool _init = false;
};

class ResultCursor : public db::ResultCursor {
public:
	static bool statusIsSuccess(int x);

	ResultCursor(const Driver *d, Driver::Connection, Driver::Result res, int);

	virtual ~ResultCursor();
	virtual bool isBinaryFormat(size_t field) const override;
	Interface::StorageType getType(size_t field) const;
	virtual bool isNull(size_t field) const override;
	virtual mem::StringView toString(size_t field) const override;
	virtual mem::BytesView toBytes(size_t field) const override;
	virtual int64_t toInteger(size_t field) const override;
	virtual double toDouble(size_t field) const override;
	virtual bool toBool(size_t field) const override;
	virtual mem::Value toTypedData(size_t field) const override;
	virtual int64_t toId() const override;
	virtual mem::StringView getFieldName(size_t field) const override;
	virtual bool isSuccess() const override;
	virtual bool isEmpty() const override;
	virtual bool isEnded() const override;
	virtual size_t getFieldsCount() const override;
	virtual size_t getAffectedRows() const override;
	virtual size_t getRowsHint() const override;
	virtual mem::Value getInfo() const override;
	virtual bool next() override;
	virtual void reset() override;
	virtual void clear() override;
	int getError() const;

public:
	const Driver *driver = nullptr;
	Driver::Connection conn = Driver::Connection(nullptr);
	Driver::Result result = Driver::Result(nullptr);
	int err = 0;
};

NS_DB_SQLITE_END

#endif /* COMPONENTS_STELLATOR_DB_SQLITE_STSQLITEDRIVER_H_ */
