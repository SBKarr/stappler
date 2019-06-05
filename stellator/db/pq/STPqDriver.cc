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

NS_DB_PQ_BEGIN

Driver *Driver::open(const mem::StringView &path) {
	static Driver s_tmp = Driver(mem::StringView());
	return &s_tmp;
}

Driver::Connection Driver::getConnection(Handle _h) const {
	ap_dbd_t *h = (ap_dbd_t *)_h.get();
	if (strcmp(apr_dbd_name(h->driver), "pgsql") == 0) {
		return Driver::Connection(apr_dbd_native_handle(h->driver, h->handle));
	}

	return Driver::Connection(nullptr);
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
	PQclear((PGresult *)res.get());
}

Driver::Result Driver::exec(Connection conn, const char *query) {
	return Driver::Result(PQexec((PGconn *)conn.get(), query));
}

Driver::Result Driver::exec(Connection conn, const char *command, int nParams, const char *const *paramValues,
		const int *paramLengths, const int *paramFormats, int resultFormat) {
	return Driver::Result(PQexecParams((PGconn *)conn.get(), command, nParams, nullptr, paramValues, paramLengths, paramFormats, resultFormat));
}

Driver::Driver(const mem::StringView &) { }

NS_DB_PQ_END
#endif
