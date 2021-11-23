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

#include <dlfcn.h>

NS_DB_PQ_BEGIN

constexpr static auto LIST_DB_TYPES = "SELECT oid, typname, typcategory FROM pg_type WHERE typcategory = 'B'"
		" OR typcategory = 'D' OR typcategory = 'I' OR typcategory = 'N' OR typcategory = 'S' OR typcategory = 'U';";

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

struct pgNotify {
	char	   *relname;		/* notification condition name */
	int			be_pid;			/* process ID of notifying server process */
	char	   *extra;			/* notification parameter */
	/* Fields below here are private to libpq; apps should not use 'em */
	pgNotify *next;		/* list link */
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
	using PQsendQueryType = int (*) (void *conn, const char *query);
	using PQstatusType = ConnStatusType (*) (void *conn);
	using PQerrorMessageType = char * (*) (const void *conn);
	using PQresetType = void (*) (void *conn);
	using PQtransactionStatusType = PGTransactionStatusType (*) (void *conn);
	using PQsetnonblockingType = int (*) (void *conn, int arg);
	using PQsocketType = int (*) (const void *conn);
	using PQconsumeInputType = int (*) (void *conn);
	using PQnotifiesType = pgNotify *(*) (void *conn);
	using PQfreememType = void (*) (void *ptr);
	using PQisBusyType = int (*) (void *conn);
	using PQgetResultType = void *(*) (void *conn);
	using PQsetNoticeProcessorType = void (*) (void *conn, PQnoticeProcessor, void *);

	DriverSym(mem::StringView name, void *d) : name(name), ptr(d) {
		this->PQresultStatus = DriverSym::PQresultStatusType(dlsym(d, "PQresultStatus"));
		this->PQconnectdbParams = DriverSym::PQconnectdbParamsType(dlsym(d, "PQconnectdbParams"));
		this->PQfinish = DriverSym::PQfinishType(dlsym(d, "PQfinish"));
		this->PQfformat = DriverSym::PQfformatType(dlsym(d, "PQfformat"));
		this->PQgetisnull = DriverSym::PQgetisnullType(dlsym(d, "PQgetisnull"));
		this->PQgetvalue = DriverSym::PQgetvalueType(dlsym(d, "PQgetvalue"));
		this->PQgetlength = DriverSym::PQgetlengthType(dlsym(d, "PQgetlength"));
		this->PQfname = DriverSym::PQfnameType(dlsym(d, "PQfname"));
		this->PQftype = DriverSym::PQftypeType(dlsym(d, "PQftype"));
		this->PQntuples = DriverSym::PQntuplesType(dlsym(d, "PQntuples"));
		this->PQnfields = DriverSym::PQnfieldsType(dlsym(d, "PQnfields"));
		this->PQcmdTuples = DriverSym::PQcmdTuplesType(dlsym(d, "PQcmdTuples"));
		this->PQresStatus = DriverSym::PQresStatusType(dlsym(d, "PQresStatus"));
		this->PQresultErrorMessage = DriverSym::PQresultErrorMessageType(dlsym(d, "PQresultErrorMessage"));
		this->PQclear = DriverSym::PQclearType(dlsym(d, "PQclear"));
		this->PQexec = DriverSym::PQexecType(dlsym(d, "PQexec"));
		this->PQexecParams = DriverSym::PQexecParamsType(dlsym(d, "PQexecParams"));
		this->PQsendQuery = DriverSym::PQsendQueryType(dlsym(d, "PQsendQuery"));
		this->PQstatus = DriverSym::PQstatusType(dlsym(d, "PQstatus"));
		this->PQerrorMessage = DriverSym::PQerrorMessageType(dlsym(d, "PQerrorMessage"));
		this->PQreset = DriverSym::PQresetType(dlsym(d, "PQreset"));
		this->PQtransactionStatus = DriverSym::PQtransactionStatusType(dlsym(d, "PQtransactionStatus"));
		this->PQsetnonblocking = DriverSym::PQsetnonblockingType(dlsym(d, "PQsetnonblocking"));
		this->PQsocket = DriverSym::PQsocketType(dlsym(d, "PQsocket"));
		this->PQconsumeInput = DriverSym::PQconsumeInputType(dlsym(d, "PQconsumeInput"));
		this->PQnotifies = DriverSym::PQnotifiesType(dlsym(d, "PQnotifies"));
		this->PQfreemem = DriverSym::PQfreememType(dlsym(d, "PQfreemem"));
		this->PQisBusy = DriverSym::PQisBusyType(dlsym(d, "PQisBusy"));
		this->PQgetResult = DriverSym::PQgetResultType(dlsym(d, "PQgetResult"));
		this->PQsetNoticeProcessor = DriverSym::PQsetNoticeProcessorType(dlsym(d, "PQsetNoticeProcessor"));
	}

	~DriverSym() { }

	operator bool () const {
		void **begin = (void **)&this->PQconnectdbParams;
		void **end = (void **)&this->PQsetNoticeProcessor + 1;
		while (begin != end) {
			if (*begin == nullptr) {
				return false;
			}
			++ begin;
		}
		return true;
	}

	mem::StringView name;
	void *ptr = nullptr;
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
	PQsendQueryType PQsendQuery = nullptr;
	PQstatusType PQstatus = nullptr;
	PQerrorMessageType PQerrorMessage = nullptr;
	PQresetType PQreset = nullptr;
	PQtransactionStatusType PQtransactionStatus = nullptr;
	PQsetnonblockingType PQsetnonblocking = nullptr;
	PQsocketType PQsocket = nullptr;
	PQconsumeInputType PQconsumeInput = nullptr;
	PQnotifiesType PQnotifies = nullptr;
	PQfreememType PQfreemem = nullptr;
	PQisBusyType PQisBusy = nullptr;
	PQgetResultType PQgetResult = nullptr;
	PQsetNoticeProcessorType PQsetNoticeProcessor = nullptr;
	uint32_t refCount = 1;
};

struct DriverHandle {
	void *conn;
	const Driver *driver;
	void *padding;
	mem::pool_t *pool;
};

static std::mutex s_driverMutex;

struct PgDriverLibStorage {
	std::mutex s_driverMutex;
	std::map<std::string, DriverSym> s_driverLibs;

	static PgDriverLibStorage *getInstance();

	DriverSym *openLib(mem::StringView lib) {
		std::unique_lock<std::mutex> lock(s_driverMutex);

		auto target = lib.str<stappler::memory::StandartInterface>();
		auto it = s_driverLibs.find(target);
		if (it != s_driverLibs.end()) {
			++ it->second.refCount;
			return &it->second;
		}

		if (auto d = dlopen(target.data(), RTLD_LAZY)) {
			DriverSym syms(target, d);
			if (syms) {
				auto ret = s_driverLibs.emplace(target, std::move(syms)).first;
				ret->second.name = ret->first;
				return &ret->second;
			} else {
				dlclose(d);
			}
		}
		return nullptr;
	}

	void closeLib(DriverSym *sym) {
		std::unique_lock<std::mutex> lock(s_driverMutex);
		if (sym->refCount == 1) {
			s_driverLibs.erase(sym->name.str<stappler::memory::StandartInterface>());
		} else {
			-- sym->refCount;
		}
	}
};

static PgDriverLibStorage *s_libStorage;

PgDriverLibStorage *PgDriverLibStorage::getInstance() {
	if (!s_libStorage) {
		s_libStorage = new PgDriverLibStorage;
	}
	return s_libStorage;
}

void Driver_noticeMessage(void *arg, const char *message) {
	// std::cout << "Notice: " << message << "\n";
	// Silence libpq notices
}

static void Driver_insert_sorted(mem::Vector<mem::Pair<uint32_t, Interface::StorageType>> & vec, uint32_t oid, Interface::StorageType type) {
	auto it = std::upper_bound(vec.begin(), vec.end(), oid, [] (uint32_t l, const mem::Pair<uint32_t, Interface::StorageType> &r) -> bool {
		return l < r.first;
	});
	vec.emplace(it, oid, type);
}

static void Driver_insert_sorted(mem::Vector<mem::Pair<uint32_t, mem::String>> & vec, uint32_t oid, mem::StringView type) {
	auto it = std::upper_bound(vec.begin(), vec.end(), oid, [] (uint32_t l, const mem::Pair<uint32_t, mem::String> &r) -> bool {
		return l < r.first;
	});
	vec.emplace(it, oid, type.str<mem::Interface>());
}

bool Driver::init(Handle handle, const mem::Vector<mem::StringView> &dbs) {
	if (_init) {
		return true;
	}

	auto conn = getConnection(handle);
	mem::Vector<mem::StringView> toCreate(dbs);
	if (!dbs.empty()) {
		auto res = exec(conn, "SELECT datname FROM pg_database;");

		for (size_t i = 0; i < getNTuples(res); ++ i) {
			auto name = mem::StringView(getValue(res, i, 0), getLength(res, i, 0));
			auto it = std::find(toCreate.begin(), toCreate.end(), name);
			if (it != toCreate.end()) {
				toCreate.erase(it);
			}
		}

		clearResult(res);

		if (!toCreate.empty()) {
			for (auto &it : toCreate) {
				mem::StringStream query;
				query << "CREATE DATABASE " << it << ";";
				auto q = query.weak().data();
				auto res = exec(conn, q);
				clearResult(res);
			}
		}
	}

	ResultInterface result(this, exec(conn, LIST_DB_TYPES));

	db::sql::Result res(&result);
	mem::pool::push(_storageTypes.get_allocator());
	for (auto it : res) {
		auto tid = it.toInteger(0);
		auto tname = it.at(1);
		if (tname == "bool") {
			Driver_insert_sorted(_storageTypes, uint32_t(tid), Interface::StorageType::Bool);
		} else if (tname == "bytea") {
			Driver_insert_sorted(_storageTypes, uint32_t(tid), Interface::StorageType::Bytes);
		} else if (tname == "char") {
			Driver_insert_sorted(_storageTypes, uint32_t(tid), Interface::StorageType::Char);
		} else if (tname == "int8") {
			Driver_insert_sorted(_storageTypes, uint32_t(tid), Interface::StorageType::Int8);
		} else if (tname == "int4") {
			Driver_insert_sorted(_storageTypes, uint32_t(tid), Interface::StorageType::Int4);
		} else if (tname == "int2") {
			Driver_insert_sorted(_storageTypes, uint32_t(tid), Interface::StorageType::Int2);
		} else if (tname == "float4") {
			Driver_insert_sorted(_storageTypes, uint32_t(tid), Interface::StorageType::Float4);
		} else if (tname == "float8") {
			Driver_insert_sorted(_storageTypes, uint32_t(tid), Interface::StorageType::Float8);
		} else if (tname == "varchar") {
			Driver_insert_sorted(_storageTypes, uint32_t(tid), Interface::StorageType::VarChar);
		} else if (tname == "text") {
			Driver_insert_sorted(_storageTypes, uint32_t(tid), Interface::StorageType::Text);
		} else if (tname == "numeric") {
			Driver_insert_sorted(_storageTypes, uint32_t(tid), Interface::StorageType::Numeric);
		} else if (tname == "tsvector") {
			Driver_insert_sorted(_storageTypes, uint32_t(tid), Interface::StorageType::TsVector);
		} else {
			Driver_insert_sorted(_customTypes, uint32_t(tid), tname);
		}
	}
	mem::pool::pop();

	_init = true;
	return true;
}

void Driver::performWithStorage(Handle handle, const mem::Callback<void(const db::Adapter &)> &cb) const {
	auto targetPool = mem::pool::acquire();

	db::pq::Handle h(this, handle);
	db::Adapter storage(&h);
	mem::pool::userdata_set((void *)&h, config::getStorageInterfaceKey(), nullptr, targetPool);

	cb(storage);

	auto stack = stappler::memory::pool::get<db::Transaction::Stack>(targetPool, config::getTransactionStackKey());
	if (stack) {
		for (auto &it : stack->stack) {
			if (it->adapter == storage) {
				it->adapter = db::Adapter(nullptr);
				messages::error("Root", "Incomplete transaction found");
			}
		}
	}
	mem::pool::userdata_set((void *)nullptr, storage.getTransactionKey().data(), nullptr, targetPool);
	mem::pool::userdata_set((void *)nullptr, config::getStorageInterfaceKey(), nullptr, targetPool);
}

Interface *Driver::acquireInterface(Handle handle, mem::pool_t *pool) const {
	Interface *ret = nullptr;
	mem::pool::push(pool);
	ret = new (pool) db::pq::Handle(this, handle);
	mem::pool::pop();
	return ret;
}

Driver::Handle Driver::connect(const mem::Map<mem::StringView, mem::StringView> &params) const {
	auto p = mem::pool::create(mem::pool::acquire());
	Driver::Handle rec;
	mem::pool::push(p);
	do {
		mem::Vector<const char *> keywords; keywords.reserve(params.size());
		mem::Vector<const char *> values; values.reserve(params.size());

		for (auto &it : params) {
			if (it.first == "host"
					|| it.first == "hostaddr"
					|| it.first == "port"
					|| it.first == "dbname"
					|| it.first == "user"
					|| it.first == "password"
					|| it.first == "passfile"
					|| it.first == "channel_binding"
					|| it.first == "connect_timeout"
					|| it.first == "client_encoding"
					|| it.first == "options"
					|| it.first == "application_name"
					|| it.first == "fallback_application_name"
					|| it.first == "keepalives"
					|| it.first == "keepalives_idle"
					|| it.first == "keepalives_interval"
					|| it.first == "keepalives_count"
					|| it.first == "tcp_user_timeout"
					|| it.first == "replication"
					|| it.first == "gssencmode"
					|| it.first == "sslmode"
					|| it.first == "requiressl"
					|| it.first == "sslcompression"
					|| it.first == "sslcert"
					|| it.first == "sslkey"
					|| it.first == "sslpassword"
					|| it.first == "sslrootcert"
					|| it.first == "sslcrl"
					|| it.first == "requirepeer"
					|| it.first == "ssl_min_protocol_version"
					|| it.first == "ssl_max_protocol_version"
					|| it.first == "krbsrvname"
					|| it.first == "gsslib"
					|| it.first == "service"
					|| it.first == "target_session_attrs") {
				keywords.emplace_back(it.first.data());
				values.emplace_back(it.second.data());
			} else if (it.first != "driver" && it.first == "nmin" && it.first == "nkeep"
					&& it.first == "nmax" && it.first == "exptime" && it.first == "persistent") {
				std::cout << "[pq::Driver] unknown connection parameter: " << it.first << "=" << it.second << "\n";
			}
		}

		keywords.emplace_back(nullptr);
		values.emplace_back(nullptr);

		rec = doConnect(keywords.data(), values.data(), 0);
	} while (0);
	mem::pool::pop();

	if (!rec.get()) {
		mem::pool::destroy(p);
	}
	return rec;
}

void Driver::finish(Handle h) const {
	auto db = (DriverHandle *)h.get();
	if (db && db->pool) {
		mem::pool::destroy(db->pool);
	}
}

bool Driver::isValid(Handle handle) const {
	auto conn = getConnection(handle);
	if (conn.get()) {
		return isValid(conn);
	}
	return false;
}

bool Driver::isValid(Connection conn) const {
	if (_handle->PQstatus(conn.get()) != CONNECTION_OK) {
		_handle->PQreset(conn.get());
		if (_handle->PQstatus(conn.get()) != CONNECTION_OK) {
			return false;
		}
	}
	return true;
}

bool Driver::isIdle(Connection conn) const {
	return getTransactionStatus(conn) == TransactionStatus::Idle;
}

int Driver::listenForNotifications(Handle handle) const {
	auto conn = getConnection(handle).get();

	auto query = mem::toString("LISTEN ", config::getSerenityBroadcastChannelName(), ";");
	int querySent = _handle->PQsendQuery(conn, query.data());
	if (querySent == 0) {
		std::cout << "[Postgres]: " << _handle->PQerrorMessage(conn) << "\n";
		return -1;
	}

	if (_handle->PQsetnonblocking(conn, 1) == -1) {
		std::cout << "[Postgres]: " << _handle->PQerrorMessage(conn) << "\n";
		return -1;
	} else {
		return _handle->PQsocket(conn);
	}
}

bool Driver::consumeNotifications(Handle handle, const mem::Callback<void(mem::StringView)> &cb) const {
	auto conn = getConnection(handle).get();

	auto connStatusType = _handle->PQstatus(conn);
	if (connStatusType == CONNECTION_BAD) {
		return false;
	}

	int rc = _handle->PQconsumeInput(conn);
	if (rc == 0) {
		std::cout << "[Postgres]: " << _handle->PQerrorMessage(conn) << "\n";
		return false;
	}
	pgNotify *notify;
	while ((notify = _handle->PQnotifies(conn)) != NULL) {
		cb(notify->relname);
		_handle->PQfreemem(notify);
	}
	if (_handle->PQisBusy(conn) == 0) {
		void *result;
		while ((result = _handle->PQgetResult(conn)) != NULL) {
			_handle->PQclear(result);
		}
	}
	return true;
}

Driver::TransactionStatus Driver::getTransactionStatus(Connection conn) const {
	auto ret = _handle->PQtransactionStatus(conn.get());
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
	auto err = _handle->PQresultStatus(res.get());
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
	return _handle->PQfformat(res.get(), field) != 0;
}

bool Driver::isNull(Result res, size_t row, size_t field) const {
	return _handle->PQgetisnull(res.get(), row, field);
}

char *Driver::getValue(Result res, size_t row, size_t field) const {
	return _handle->PQgetvalue(res.get(), row, field);
}

size_t Driver::getLength(Result res, size_t row, size_t field) const {
	return size_t(_handle->PQgetlength(res.get(), row, field));
}

char *Driver::getName(Result res, size_t field) const {
	return _handle->PQfname(res.get(), field);
}

unsigned int Driver::getType(Result res, size_t field) const {
	return _handle->PQftype(res.get(), field);
}

size_t Driver::getNTuples(Result res) const {
	return size_t(_handle->PQntuples(res.get()));
}

size_t Driver::getNFields(Result res) const {
	return size_t(_handle->PQnfields(res.get()));
}

size_t Driver::getCmdTuples(Result res) const {
	return stappler::StringToNumber<size_t>(_handle->PQcmdTuples(res.get()));
}

char *Driver::getStatusMessage(Status st) const {
	switch (st) {
	case Status::Empty: return _handle->PQresStatus(PGRES_EMPTY_QUERY); break;
	case Status::CommandOk: return _handle->PQresStatus(PGRES_COMMAND_OK); break;
	case Status::TuplesOk: return _handle->PQresStatus(PGRES_TUPLES_OK); break;
	case Status::CopyOut: return _handle->PQresStatus(PGRES_COPY_OUT); break;
	case Status::CopyIn: return _handle->PQresStatus(PGRES_COPY_IN); break;
	case Status::BadResponse: return _handle->PQresStatus(PGRES_BAD_RESPONSE); break;
	case Status::NonfatalError: return _handle->PQresStatus(PGRES_NONFATAL_ERROR); break;
	case Status::FatalError: return _handle->PQresStatus(PGRES_FATAL_ERROR); break;
	case Status::CopyBoth: return _handle->PQresStatus(PGRES_COPY_BOTH); break;
	case Status::SingleTuple: return _handle->PQresStatus(PGRES_SINGLE_TUPLE); break;
	}
	return nullptr;
}

char *Driver::getResultErrorMessage(Result res) const {
	return _handle->PQresultErrorMessage(res.get());
}

void Driver::clearResult(Result res) const {
	if (_dbCtrl) {
		_dbCtrl(true);
	}
	_handle->PQclear(res.get());
}

Driver::Result Driver::exec(Connection conn, const char *query) const {
	if (_dbCtrl) {
		_dbCtrl(false);
	}
	return Driver::Result(_handle->PQexec(conn.get(), query));
}

Driver::Result Driver::exec(Connection conn, const char *command, int nParams, const char *const *paramValues,
		const int *paramLengths, const int *paramFormats, int resultFormat) const {
	if (_dbCtrl) {
		_dbCtrl(false);
	}
	return Driver::Result(_handle->PQexecParams(conn.get(), command, nParams, nullptr, paramValues, paramLengths, paramFormats, resultFormat));
}

Interface::StorageType Driver::getTypeById(uint32_t oid) const {
	auto it = std::lower_bound(_storageTypes.begin(), _storageTypes.end(), oid, [] (const mem::Pair<uint32_t, Interface::StorageType> &l, uint32_t r) -> bool {
		return l.first < r;
	});
	if (it != _storageTypes.end() && it->first == oid) {
		return it->second;
	}
	return Interface::StorageType::Unknown;
}

mem::StringView Driver::getTypeNameById(uint32_t oid) const {
	auto it = std::lower_bound(_customTypes.begin(), _customTypes.end(), oid, [] (const mem::Pair<uint32_t, mem::String> &l, uint32_t r) -> bool {
		return l.first < r;
	});
	if (it != _customTypes.end() && it->first == oid) {
		return it->second;
	}
	return mem::StringView();
}

Driver::~Driver() { }

Driver *Driver::open(mem::StringView path, const void *external) {
	auto ret = new (mem::pool::acquire()) Driver(path, external);
	if (ret->_handle) {
		return ret;
	}
	return nullptr;
}

Driver::Driver(mem::StringView path, const void *external) : _external(external) {
	mem::StringView name;
	if (path.empty() || path == "pgsql") {
		name = mem::StringView("libpq.so");
	}

	if (auto l = PgDriverLibStorage::getInstance()->openLib(name)) {
		_handle = l;

		mem::pool::cleanup_register(mem::pool::acquire(), [this] {
			PgDriverLibStorage::getInstance()->closeLib(_handle);
			_handle = nullptr;
		});
	}
}

ResultInterface::ResultInterface(const Driver *d, Driver::Result res) : driver(d), result(res) {
	err = result.get() ? driver->getStatus(result) : Driver::Status::FatalError;
}

ResultInterface::~ResultInterface() {
	clear();
}

bool ResultInterface::isBinaryFormat(size_t field) const {
	return driver->isBinaryFormat(result, field) != 0;
}

bool ResultInterface::isNull(size_t row, size_t field) {
	return driver->isNull(result, row, field);
}

mem::StringView ResultInterface::toString(size_t row, size_t field) {
	return mem::StringView(driver->getValue(result, row, field), driver->getLength(result, row, field));
}

mem::BytesView ResultInterface::toBytes(size_t row, size_t field) {
	if (isBinaryFormat(field)) {
		return mem::BytesView((uint8_t *)driver->getValue(result, row, field), driver->getLength(result, row, field));
	} else {
		auto val = driver->getValue(result, row, field);
		auto len = driver->getLength(result, row, field);
		if (len > 2 && memcmp(val, "\\x", 2) == 0) {
			auto d = new mem::Bytes(stappler::base16::decode<mem::Interface>(stappler::CoderSource(val + 2, len - 2)));
			return mem::BytesView(*d);
		}
		return mem::BytesView((uint8_t *)val, len);
	}
}
int64_t ResultInterface::toInteger(size_t row, size_t field) {
	if (isBinaryFormat(field)) {
		stappler::BytesViewNetwork r((const uint8_t *)driver->getValue(result, row, field), driver->getLength(result, row, field));
		switch (r.size()) {
		case 1: return r.readUnsigned(); break;
		case 2: return r.readUnsigned16(); break;
		case 4: return r.readUnsigned32(); break;
		case 8: return r.readUnsigned64(); break;
		default: break;
		}
		return 0;
	} else {
		auto val = driver->getValue(result, row, field);
		return stappler::StringToNumber<int64_t>(val, nullptr, 0);
	}
}
double ResultInterface::toDouble(size_t row, size_t field) {
	if (isBinaryFormat(field)) {
		stappler::BytesViewNetwork r((const uint8_t *)driver->getValue(result, row, field), driver->getLength(result, row, field));
		switch (r.size()) {
		case 2: return r.readFloat16(); break;
		case 4: return r.readFloat32(); break;
		case 8: return r.readFloat64(); break;
		default: break;
		}
		return 0;
	} else {
		auto val = driver->getValue(result, row, field);
		return stappler::StringToNumber<double>(val, nullptr, 0);
	}
}
bool ResultInterface::toBool(size_t row, size_t field) {
	auto val = driver->getValue(result, row, field);
	if (!isBinaryFormat(field)) {
		if (val) {
			if (*val == 'T' || *val == 't' || *val == 'y') {
				return true;
			}
		}
		return false;
	} else {
		return val && *val != 0;
	}
}
mem::Value ResultInterface::toTypedData(size_t row, size_t field) {
	auto t = driver->getType(result, field);
	auto s = driver->getTypeById(t);
	switch (s) {
	case Interface::StorageType::Unknown:
		messages::error("DB", "Unknown type conversion", mem::Value(driver->getTypeNameById(t)));
		return mem::Value();
		break;
	case Interface::StorageType::TsVector:
		return mem::Value();
		break;
	case Interface::StorageType::Bool:
		return mem::Value(toBool(row, field));
		break;
	case Interface::StorageType::Char:
		break;
	case Interface::StorageType::Float4:
	case Interface::StorageType::Float8:
		return mem::Value(toDouble(row, field));
		break;
	case Interface::StorageType::Int2:
	case Interface::StorageType::Int4:
	case Interface::StorageType::Int8:
		return mem::Value(toInteger(row, field));
		break;
	case Interface::StorageType::Text:
	case Interface::StorageType::VarChar:
		return mem::Value(toString(row, field));
		break;
	case Interface::StorageType::Numeric: {
		stappler::BytesViewNetwork r((const uint8_t *)driver->getValue(result, row, field), driver->getLength(result, row, field));
		auto str = pg_numeric_to_string(r);

		auto v = mem::StringView(str).readDouble();
		if (v.valid()) {
			return mem::Value(v.get());
		} else {
			return mem::Value(str);
		}
		break;
	}
	case Interface::StorageType::Bytes:
		return mem::Value(toBytes(row, field));
		break;
	}
	return mem::Value();
}
int64_t ResultInterface::toId() {
	if (isBinaryFormat(0)) {
		stappler::BytesViewNetwork r((const uint8_t *)driver->getValue(result, 0, 0), driver->getLength(result, 0, 0));
		switch (r.size()) {
		case 1: return int64_t(r.readUnsigned()); break;
		case 2: return int64_t(r.readUnsigned16()); break;
		case 4: return int64_t(r.readUnsigned32()); break;
		case 8: return int64_t(r.readUnsigned64()); break;
		default: break;
		}
		return 0;
	} else {
		auto val = driver->getValue(result, 0, 0);
		return stappler::StringToNumber<int64_t>(val, nullptr, 0);
	}
}
mem::StringView ResultInterface::getFieldName(size_t field) {
	auto ptr = driver->getName(result, field);
	if (ptr) {
		return mem::StringView(ptr);
	}
	return mem::StringView();
}
bool ResultInterface::isSuccess() const {
	return result.get() && pgsql_is_success(err);
}
size_t ResultInterface::getRowsCount() const {
	return driver->getNTuples(result);
}
size_t ResultInterface::getFieldsCount() const {
	return driver->getNFields(result);
}
size_t ResultInterface::getAffectedRows() const {
	return driver->getCmdTuples(result);
}
mem::Value ResultInterface::getInfo() const {
	return mem::Value({
		stappler::pair("error", mem::Value(stappler::toInt(err))),
		stappler::pair("status", mem::Value(driver->getStatusMessage(err))),
		stappler::pair("desc", mem::Value(result.get() ? driver->getResultErrorMessage(result) : "Fatal database error")),
	});
}
void ResultInterface::clear() {
	if (result.get()) {
		driver->clearResult(result);
		result = Driver::Result(nullptr);
	}
}

Driver::Status ResultInterface::getError() const {
	return err;
}

NS_DB_PQ_END

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

Driver::Handle Driver::doConnect(const char * const *keywords, const char * const *values, int expand_dbname) const {
	auto p = mem::pool::acquire();
	if (_external) {
		ConnStatusType status = CONNECTION_OK;
		PGconn * ret = (PGconn *)_handle->PQconnectdbParams(keywords, values, expand_dbname);
		if (ret) {
			status = _handle->PQstatus(ret);
			if (status != CONNECTION_OK) {
				_handle->PQfinish(ret);
				return Driver::Handle(nullptr);
			}
			_handle->PQsetNoticeProcessor(ret, Driver_noticeMessage, (void *)this);
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
			db->driver = (const apr_dbd_driver_t *)_external;
			db->pool = p;
			db->prepared = nullptr;

			mem::pool::cleanup_register(p, [h = (DriverSym *)_handle, ret] {
				h->PQfinish(ret);
			});

			return Driver::Handle(db);
		}
	} else {
		auto h = (DriverHandle *)mem::pool::palloc(p, sizeof(DriverHandle));
		h->pool = p;
		h->driver = this;
		h->conn = _handle->PQconnectdbParams(keywords, values, expand_dbname);

		if (h->conn) {
			if (_handle->PQstatus(h->conn) != CONNECTION_OK) {
				_handle->PQfinish(h->conn);
				return Driver::Handle(nullptr);
			}
			_handle->PQsetNoticeProcessor(h->conn, Driver_noticeMessage, (void *)this);

			mem::pool::cleanup_register(p, [h = (DriverSym *)_handle, ret = h->conn] {
				h->PQfinish(ret);
			});

			return Driver::Handle(h);
		}
	}
	return Driver::Handle(nullptr);
}

Driver::Connection Driver::getConnection(Handle _h) const {
	if (_external) {
		auto h = (ap_dbd_t *)_h.get();
		if (strcmp(apr_dbd_name(h->driver), "pgsql") == 0) {
			return Driver::Connection(apr_dbd_native_handle(h->driver, h->handle));
		}
	} else {
		auto h = (DriverHandle *)_h.get();
		return Driver::Connection(h->conn);
	}

	return Driver::Connection(nullptr);
}

NS_DB_PQ_END

#elif STELLATOR

NS_DB_PQ_BEGIN

Driver::Handle Driver::doConnect(const char * const *keywords, const char * const *values, int expand_dbname) const {
	auto p = mem::pool::acquire();
	auto h = (DriverHandle *)mem::pool::palloc(p, sizeof(DriverHandle));
	h->pool = p;
	h->driver = this;
	h->conn = (_handle->PQconnectdbParams(keywords, values, expand_dbname));

	if (h->conn) {
		if (_handle->PQstatus(h->conn) != CONNECTION_OK) {
			_handle->PQfinish(h->conn);
			return Driver::Handle(nullptr);
		}
		_handle->PQsetNoticeProcessor(h->conn, Driver_noticeMessage, (void *)this);

		mem::pool::cleanup_register(p, [h = (DriverSym *)_handle, ret = h->conn] {
			h->PQfinish(ret);
		});

		return Driver::Handle(h);
	}
	return Driver::Handle(nullptr);
}

Driver::Connection Driver::getConnection(Handle _h) const {
	auto h = (DriverHandle *)_h.get();
	return Driver::Connection(h->conn);
}

NS_DB_PQ_END
#endif
