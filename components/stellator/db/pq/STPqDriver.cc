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

#include "STPqDriver.h"

#if SPAPR
#include "apr_dbd.h"
#include "mod_dbd.h"
#include <postgresql/libpq-fe.h>

struct apr_dbd_transaction_t {
    int mode;
    int errnum;
    apr_dbd_t *handle;
};

struct apr_dbd_t {
    PGconn *conn;
    apr_dbd_transaction_t *trans;
};

NS_DB_PQ_BEGIN

Driver *Driver::open(const mem::StringView &path) {
	static Driver s_tmp = Driver(path);
	return &s_tmp;
}

Driver::Handle Driver::connect(const char * const *keywords, const char * const *values, int expand_dbname) const {
	auto p = mem::pool::create(mem::pool::acquire());

	ConnStatusType status = CONNECTION_OK;
	PGconn * ret = PQconnectdbParams(keywords, values, expand_dbname);
	if (ret) {
		status = PQstatus(ret);
	}
	if (ret && status == CONNECTION_OK) {
		apr_dbd_t *dbd = (apr_dbd_t *)mem::pool::palloc(p, sizeof(apr_dbd_t));
		dbd->conn = ret;
		dbd->trans = (apr_dbd_transaction_t *)mem::pool::palloc(p, sizeof(apr_dbd_transaction_t));
		dbd->trans->mode = 0;
		dbd->trans->errnum = CONNECTION_OK;
		dbd->trans->handle = dbd;

		ap_dbd_t *db = (ap_dbd_t *)mem::pool::palloc(p, sizeof(ap_dbd_t));
		db->handle = dbd;
		db->driver = (const apr_dbd_driver_t *)_handle;
		db->pool = p;
		db->prepared = nullptr;

		return Driver::Handle(db);
	}
	return Driver::Handle(nullptr);
}

void Driver::finish(Handle h) const {
	if (auto conn = (PGconn *)getConnection(h).get()) {
		PQfinish(conn);
	}
	ap_dbd_t *db = (ap_dbd_t *)h.get();
	if (db && db->pool) {
		mem::pool::destroy(db->pool);
	}
}

Driver::Connection Driver::getConnection(Handle _h) const {
	ap_dbd_t *h = (ap_dbd_t *)_h.get();
	if (strcmp(apr_dbd_name(h->driver), "pgsql") == 0) {
		return Driver::Connection(apr_dbd_native_handle(h->driver, h->handle));
	}

	return Driver::Connection(nullptr);
}

bool Driver::isValid(Handle handle) const {
	auto conn = getConnection(handle);
	if (conn.get()) {
		return isValid(conn);
	}
	return false;
}

bool Driver::isValid(Connection conn) const {
	if (PQstatus((const PGconn *)conn.get()) != CONNECTION_OK) {
		PQreset((PGconn *)conn.get());
		if (PQstatus((const PGconn *)conn.get()) != CONNECTION_OK) {
			return false;
		}
	}
	return true;
}

Driver::TransactionStatus Driver::getTransactionStatus(Connection conn) const {
	auto ret = PQtransactionStatus((const PGconn *)conn.get());
	switch (ret) {
	case PQTRANS_IDLE: return TransactionStatus::Idle; break;
	case PQTRANS_ACTIVE: return TransactionStatus::Active; break;
	case PQTRANS_INTRANS: return TransactionStatus::InTrans; break;
	case PQTRANS_INERROR: return TransactionStatus::InError; break;
	case PQTRANS_UNKNOWN: return TransactionStatus::Unknown; break;
	default: break;
	}
	return TransactionStatus::Unknown;
}

Driver::Status Driver::getStatus(Result res) const {
	auto err = PQresultStatus((PGresult *)res.get());
	switch (err) {
	case PGRES_EMPTY_QUERY: return Driver::Status::Empty; break;
	case PGRES_COMMAND_OK: return Driver::Status::CommandOk; break;
	case PGRES_TUPLES_OK: return Driver::Status::TuplesOk; break;
	case PGRES_COPY_OUT: return Driver::Status::CopyOut; break;
	case PGRES_COPY_IN: return Driver::Status::CopyIn; break;
	case PGRES_BAD_RESPONSE: return Driver::Status::BadResponse; break;
	case PGRES_NONFATAL_ERROR: return Driver::Status::NonfatalError; break;
	case PGRES_FATAL_ERROR: return Driver::Status::FatalError; break;
	case PGRES_COPY_BOTH: return Driver::Status::CopyBoth; break;
	case PGRES_SINGLE_TUPLE: return Driver::Status::SingleTuple; break;
	default: break;
	}
	return Driver::Status::Empty;
}

bool Driver::isBinaryFormat(Result res, size_t field) const {
	return PQfformat((PGresult *)res.get(), field) != 0;
}

bool Driver::isNull(Result res, size_t row, size_t field) const {
	return PQgetisnull((PGresult *)res.get(), row, field);
}

char *Driver::getValue(Result res, size_t row, size_t field) const {
	return PQgetvalue((PGresult *)res.get(), row, field);
}

size_t Driver::getLength(Result res, size_t row, size_t field) const {
	return size_t(PQgetlength((PGresult *)res.get(), row, field));
}

char *Driver::getName(Result res, size_t field) const {
	return PQfname((PGresult *)res.get(), field);
}

unsigned int Driver::getType(Result res, size_t field) const {
	return PQftype((PGresult *)res.get(), field);
}

size_t Driver::getNTuples(Result res) const {
	return size_t(PQntuples((PGresult *)res.get()));
}

size_t Driver::getNFields(Result res) const {
	return size_t(PQnfields((PGresult *)res.get()));
}

size_t Driver::getCmdTuples(Result res) const {
	return stappler::StringToNumber<size_t>(PQcmdTuples((PGresult *)res.get()));
}

char *Driver::getStatusMessage(Status st) const {
	switch (st) {
	case Status::Empty: return PQresStatus(PGRES_EMPTY_QUERY); break;
	case Status::CommandOk: return PQresStatus(PGRES_COMMAND_OK); break;
	case Status::TuplesOk: return PQresStatus(PGRES_TUPLES_OK); break;
	case Status::CopyOut: return PQresStatus(PGRES_COPY_OUT); break;
	case Status::CopyIn: return PQresStatus(PGRES_COPY_IN); break;
	case Status::BadResponse: return PQresStatus(PGRES_BAD_RESPONSE); break;
	case Status::NonfatalError: return PQresStatus(PGRES_NONFATAL_ERROR); break;
	case Status::FatalError: return PQresStatus(PGRES_FATAL_ERROR); break;
	case Status::CopyBoth: return PQresStatus(PGRES_COPY_BOTH); break;
	case Status::SingleTuple: return PQresStatus(PGRES_SINGLE_TUPLE); break;
	}
	return nullptr;
}

char *Driver::getResultErrorMessage(Result res) const {
	return PQresultErrorMessage((PGresult *)res.get());
}

void Driver::clearResult(Result res) const {
	if (_dbCtrl) {
		_dbCtrl(true);
	}
	PQclear((PGresult *)res.get());
}

Driver::Result Driver::exec(Connection conn, const char *query) {
	if (_dbCtrl) {
		_dbCtrl(false);
	}
	return Driver::Result(PQexec((PGconn *)conn.get(), query));
}

Driver::Result Driver::exec(Connection conn, const char *command, int nParams, const char *const *paramValues,
		const int *paramLengths, const int *paramFormats, int resultFormat) {
	if (_dbCtrl) {
		_dbCtrl(false);
	}
	return Driver::Result(PQexecParams((PGconn *)conn.get(), command, nParams, nullptr, paramValues, paramLengths, paramFormats, resultFormat));
}

Driver::~Driver() { }

Driver::Driver(const mem::StringView &name) {
	const apr_dbd_driver_t *driver = nullptr;
	if (apr_dbd_get_driver(mem::pool::acquire(), name.data(), &driver) == APR_SUCCESS) {
		_driverName = name;
		_handle = (void *)driver;
	}
}

void Driver::release() { }

void Driver::setDbCtrl(mem::Function<void(bool)> &&cb) {
	_dbCtrl = std::move(cb);
}

NS_DB_PQ_END
#elif STELLATOR
#include <dlfcn.h>

NS_DB_PQ_BEGIN

enum ConnStatusType {
	CONNECTION_OK,
	CONNECTION_BAD,
};

enum ExecStatusType {
	PGRES_EMPTY_QUERY = 0,
	PGRES_COMMAND_OK,
	PGRES_TUPLES_OK,
	PGRES_COPY_OUT,
	PGRES_COPY_IN,
	PGRES_BAD_RESPONSE,
	PGRES_NONFATAL_ERROR,
	PGRES_FATAL_ERROR,
	PGRES_COPY_BOTH,
	PGRES_SINGLE_TUPLE
};

enum PGTransactionStatusType {
	PQTRANS_IDLE,
	PQTRANS_ACTIVE,
	PQTRANS_INTRANS,
	PQTRANS_INERROR,
	PQTRANS_UNKNOWN
};

struct DriverSym : mem::AllocBase {
	using PQnoticeProcessor = void (*) (void *arg, const char *message);

	using PQresultStatusType = ExecStatusType (*) (const void *res);
	using PQconnectdbParamsType = void * (*) (const char * const *keywords, const char * const *values, int expand_dbname);
	using PQfinishType = void (*) (void *conn);
	using PQfformatType = int (*) (const void *res, int field_num);
	using PQgetisnullType = int	(*) (const void *res, int tup_num, int field_num);
	using PQgetvalueType = char *(*) (const void *res, int tup_num, int field_num);
	using PQgetlengthType = int	(*) (const void *res, int tup_num, int field_num);
	using PQfnameType = char *(*) (const void *res, int field_num);
	using PQftypeType = unsigned int (*) (const void *res, int field_num);
	using PQntuplesType = int (*) (const void *res);
	using PQnfieldsType = int (*) (const void *res);
	using PQcmdTuplesType = char *(*) (void *res);
	using PQresStatusType = char *(*) (ExecStatusType status);
	using PQresultErrorMessageType = char *(*) (const void *res);
	using PQclearType = void (*) (void *res);
	using PQexecType = void *(*) (void *conn, const char *query);
	using PQexecParamsType = void *(*) (void *conn, const char *command, int nParams, const void *paramTypes,
			const char *const *paramValues, const int *paramLengths, const int *paramFormats, int resultFormat);

	using PQstatusType = ConnStatusType (*) (void *conn);
	using PQtransactionStatusType = PGTransactionStatusType (*) (void *conn);
	using PQsetNoticeProcessorType = void (*) (void *conn, PQnoticeProcessor, void *);

	DriverSym(void *p) : ptr(p) { }

	void *ptr;

	PQconnectdbParamsType PQconnectdbParams = nullptr;
	PQfinishType PQfinish = nullptr;
	PQresultStatusType PQresultStatus = nullptr;
	PQfformatType PQfformat = nullptr;
	PQgetisnullType PQgetisnull = nullptr;
	PQgetvalueType PQgetvalue = nullptr;
	PQgetlengthType PQgetlength = nullptr;
	PQfnameType PQfname = nullptr;
	PQftypeType PQftype = nullptr;
	PQntuplesType PQntuples = nullptr;
	PQnfieldsType PQnfields = nullptr;
	PQcmdTuplesType PQcmdTuples = nullptr;
	PQresStatusType PQresStatus = nullptr;
	PQresultErrorMessageType PQresultErrorMessage = nullptr;
	PQclearType PQclear = nullptr;
	PQexecType PQexec = nullptr;
	PQexecParamsType PQexecParams = nullptr;
	PQstatusType PQstatus = nullptr;
	PQtransactionStatusType PQtransactionStatus = nullptr;
	PQsetNoticeProcessorType PQsetNoticeProcessor = nullptr;
};

Driver *Driver::open(const mem::StringView &path) {
	auto ret = new (mem::pool::acquire()) Driver(path);
	if (ret->_handle) {
		return ret;
	}
	return nullptr;
}

void Driver_noticeMessage(void *arg, const char *message) {
	// std::cout << "Notice: " << message << "\n";
	// Silence libpq notices
}

Driver::Handle Driver::connect(const char * const *keywords, const char * const *values, int expand_dbname) const{
	auto ret = (((DriverSym *)_handle)->PQconnectdbParams(keywords, values, expand_dbname));
	if (ret && ((DriverSym *)_handle)->PQstatus(ret) == CONNECTION_OK) {
		((DriverSym *)_handle)->PQsetNoticeProcessor(ret, Driver_noticeMessage, (void *)this);
		return Driver::Handle(ret);
	}
	return Driver::Handle(nullptr);
}

void Driver::finish(Handle h) const {
	((DriverSym *)_handle)->PQfinish(h.get());
}

Driver::Connection Driver::getConnection(Handle _h) const {
	return Driver::Connection(_h.get());
}

bool Driver::isValid(Connection conn) const {
	return ((DriverSym *)_handle)->PQstatus(conn.get()) == CONNECTION_OK;
}

Driver::TransactionStatus Driver::getTransactionStatus(Connection conn) const {
	auto ret = ((DriverSym *)_handle)->PQtransactionStatus(conn.get());
	switch (ret) {
	case PQTRANS_IDLE: return TransactionStatus::Idle; break;
	case PQTRANS_ACTIVE: return TransactionStatus::Active; break;
	case PQTRANS_INTRANS: return TransactionStatus::InTrans; break;
	case PQTRANS_INERROR: return TransactionStatus::InError; break;
	case PQTRANS_UNKNOWN: return TransactionStatus::Unknown; break;
	}
	return TransactionStatus::Unknown;
}

Driver::Status Driver::getStatus(Result res) const {
	auto err = ((DriverSym *)_handle)->PQresultStatus(res.get());
	switch (err) {
	case PGRES_EMPTY_QUERY: return Driver::Status::Empty; break;
	case PGRES_COMMAND_OK: return Driver::Status::CommandOk; break;
	case PGRES_TUPLES_OK: return Driver::Status::TuplesOk; break;
	case PGRES_COPY_OUT: return Driver::Status::CopyOut; break;
	case PGRES_COPY_IN: return Driver::Status::CopyIn; break;
	case PGRES_BAD_RESPONSE: return Driver::Status::BadResponse; break;
	case PGRES_NONFATAL_ERROR: return Driver::Status::NonfatalError; break;
	case PGRES_FATAL_ERROR: return Driver::Status::FatalError; break;
	case PGRES_COPY_BOTH: return Driver::Status::CopyBoth; break;
	case PGRES_SINGLE_TUPLE: return Driver::Status::SingleTuple; break;
	default: break;
	}
	return Driver::Status::Empty;
}

bool Driver::isBinaryFormat(Result res, size_t field) const {
	return ((DriverSym *)_handle)->PQfformat(res.get(), field) != 0;
}

bool Driver::isNull(Result res, size_t row, size_t field) const {
	return ((DriverSym *)_handle)->PQgetisnull(res.get(), row, field);
}

char *Driver::getValue(Result res, size_t row, size_t field) const {
	return ((DriverSym *)_handle)->PQgetvalue(res.get(), row, field);
}

size_t Driver::getLength(Result res, size_t row, size_t field) const {
	return size_t(((DriverSym *)_handle)->PQgetlength(res.get(), row, field));
}

char *Driver::getName(Result res, size_t field) const {
	return ((DriverSym *)_handle)->PQfname(res.get(), field);
}

unsigned int Driver::getType(Result res, size_t field) const {
	return ((DriverSym *)_handle)->PQftype(res.get(), field);
}

size_t Driver::getNTuples(Result res) const {
	return size_t(((DriverSym *)_handle)->PQntuples(res.get()));
}

size_t Driver::getNFields(Result res) const {
	return size_t(((DriverSym *)_handle)->PQnfields(res.get()));
}

size_t Driver::getCmdTuples(Result res) const {
	return stappler::StringToNumber<size_t>(((DriverSym *)_handle)->PQcmdTuples(res.get()));
}

char *Driver::getStatusMessage(Status st) const {
	switch (st) {
	case Status::Empty: return ((DriverSym *)_handle)->PQresStatus(PGRES_EMPTY_QUERY); break;
	case Status::CommandOk: return ((DriverSym *)_handle)->PQresStatus(PGRES_COMMAND_OK); break;
	case Status::TuplesOk: return ((DriverSym *)_handle)->PQresStatus(PGRES_TUPLES_OK); break;
	case Status::CopyOut: return ((DriverSym *)_handle)->PQresStatus(PGRES_COPY_OUT); break;
	case Status::CopyIn: return ((DriverSym *)_handle)->PQresStatus(PGRES_COPY_IN); break;
	case Status::BadResponse: return ((DriverSym *)_handle)->PQresStatus(PGRES_BAD_RESPONSE); break;
	case Status::NonfatalError: return ((DriverSym *)_handle)->PQresStatus(PGRES_NONFATAL_ERROR); break;
	case Status::FatalError: return ((DriverSym *)_handle)->PQresStatus(PGRES_FATAL_ERROR); break;
	case Status::CopyBoth: return ((DriverSym *)_handle)->PQresStatus(PGRES_COPY_BOTH); break;
	case Status::SingleTuple: return ((DriverSym *)_handle)->PQresStatus(PGRES_SINGLE_TUPLE); break;
	}
	return nullptr;
}

char *Driver::getResultErrorMessage(Result res) const {
	return ((DriverSym *)_handle)->PQresultErrorMessage(res.get());
}

void Driver::clearResult(Result res) const {
	((DriverSym *)_handle)->PQclear(res.get());
}

Driver::Result Driver::exec(Connection conn, const char *query) {
	return Driver::Result(((DriverSym *)_handle)->PQexec(conn.get(), query));
}

Driver::Result Driver::exec(Connection conn, const char *command, int nParams, const char *const *paramValues,
		const int *paramLengths, const int *paramFormats, int resultFormat) {
	return Driver::Result(((DriverSym *)_handle)->PQexecParams(conn.get(), command, nParams, nullptr, paramValues, paramLengths, paramFormats, resultFormat));
}

Driver::~Driver() {
	release();
}

void Driver::release() {
	if (_handle) {
		dlclose(_handle);
	}
}

Driver::Driver(const mem::StringView &path) {
	if (auto d = dlopen(path.empty() ? "libpq.so" : path.data(), RTLD_LAZY)) {
		auto h = new DriverSym(d);

		h->PQresultStatus = DriverSym::PQresultStatusType(dlsym(d, "PQresultStatus"));
		h->PQconnectdbParams = DriverSym::PQconnectdbParamsType(dlsym(d, "PQconnectdbParams"));
		h->PQfinish = DriverSym::PQfinishType(dlsym(d, "PQfinish"));
		h->PQfformat = DriverSym::PQfformatType(dlsym(d, "PQfformat"));
		h->PQgetisnull = DriverSym::PQgetisnullType(dlsym(d, "PQgetisnull"));
		h->PQgetvalue = DriverSym::PQgetvalueType(dlsym(d, "PQgetvalue"));
		h->PQgetlength = DriverSym::PQgetlengthType(dlsym(d, "PQgetlength"));
		h->PQfname = DriverSym::PQfnameType(dlsym(d, "PQfname"));
		h->PQftype = DriverSym::PQftypeType(dlsym(d, "PQftype"));
		h->PQntuples = DriverSym::PQntuplesType(dlsym(d, "PQntuples"));
		h->PQnfields = DriverSym::PQnfieldsType(dlsym(d, "PQnfields"));
		h->PQcmdTuples = DriverSym::PQcmdTuplesType(dlsym(d, "PQcmdTuples"));
		h->PQresStatus = DriverSym::PQresStatusType(dlsym(d, "PQresStatus"));
		h->PQresultErrorMessage = DriverSym::PQresultErrorMessageType(dlsym(d, "PQresultErrorMessage"));
		h->PQclear = DriverSym::PQclearType(dlsym(d, "PQclear"));
		h->PQexec = DriverSym::PQexecType(dlsym(d, "PQexec"));
		h->PQexecParams = DriverSym::PQexecParamsType(dlsym(d, "PQexecParams"));
		h->PQstatus = DriverSym::PQstatusType(dlsym(d, "PQstatus"));
		h->PQtransactionStatus = DriverSym::PQtransactionStatusType(dlsym(d, "PQtransactionStatus"));
		h->PQsetNoticeProcessor = DriverSym::PQsetNoticeProcessorType(dlsym(d, "PQsetNoticeProcessor"));

		if (h->PQresultStatus && h->PQconnectdbParams && h->PQfinish && h->PQfformat && h->PQgetisnull && h->PQgetvalue && h->PQgetlength
				&& h->PQfname && h->PQftype && h->PQntuples && h->PQnfields && h->PQcmdTuples && h->PQresStatus && h->PQresultErrorMessage && h->PQclear
				&& h->PQexec && h->PQexecParams && h->PQstatus && h->PQtransactionStatus && h->PQsetNoticeProcessor) {
			_handle = h;
		}
	}
}

NS_DB_PQ_END
#endif
