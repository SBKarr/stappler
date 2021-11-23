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

#ifndef COMPONENTS_STELLATOR_DB_SQL_STSQLDRIVER_H_
#define COMPONENTS_STELLATOR_DB_SQL_STSQLDRIVER_H_

#include "STStorage.h"
#include "STStorageInterface.h"

NS_DB_SQL_BEGIN

class Driver : public mem::AllocBase {
public:
	using Handle = stappler::ValueWrapper<void *, class HandleClass>;
	using Result = stappler::ValueWrapper<void *, class ResultClass>;
	using Connection = stappler::ValueWrapper<void *, class ConnectionClass>;

	static Driver *open(mem::pool_t *, mem::StringView path = mem::StringView(), const void *external = nullptr);

	virtual ~Driver();

	mem::StringView getDriverName() const { return _driverPath; }

	virtual bool init(Handle handle, const mem::Vector<mem::StringView> &) = 0;

	virtual void performWithStorage(Handle handle, const mem::Callback<void(const db::Adapter &)> &cb) const = 0;
	virtual Interface *acquireInterface(Handle handle, mem::pool_t *) const = 0;

	virtual Handle connect(const mem::Map<mem::StringView, mem::StringView> &) const = 0;
	virtual void finish(Handle) const = 0;

	virtual Connection getConnection(Handle h) const = 0;

	virtual bool isValid(Handle) const = 0;
	virtual bool isValid(Connection) const = 0;
	virtual bool isIdle(Connection) const = 0;

	virtual int listenForNotifications(Handle) const { return -1; }
	virtual bool consumeNotifications(Handle, const mem::Callback<void(mem::StringView)> &) const { return true; }

	virtual bool isNotificationsSupported() const { return false; }

	void setDbCtrl(mem::Function<void(bool)> &&);

protected:
	mem::StringView _driverPath;
	mem::Function<void(bool)> _dbCtrl = nullptr;
};

NS_DB_SQL_END

#endif /* COMPONENTS_STELLATOR_DB_SQL_STSQLDRIVER_H_ */
