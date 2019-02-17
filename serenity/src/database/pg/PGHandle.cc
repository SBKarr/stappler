// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2017-2018 Roman Katuntsev <sbkarr@stappler.org>

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

#include "Define.h"
#include "PGHandle.h"
#include "ResourceTemplates.h"
#include "StorageScheme.h"
#include "User.h"

NS_SA_EXT_BEGIN(pg)

class PgQueryInterface : public sql::QueryInterface {
public:
	using Binder = sql::Binder;

	size_t push(String &&val) {
		params.emplace_back(Bytes());
		params.back().assign_strong((uint8_t *)val.data(), val.size() + 1);
		binary.emplace_back(false);
		return params.size();
	}

	size_t push(const StringView &val) {
		params.emplace_back(Bytes());
		params.back().assign_strong((uint8_t *)val.data(), val.size() + 1);
		binary.emplace_back(false);
		return params.size();
	}

	size_t push(Bytes &&val) {
		params.emplace_back(std::move(val));
		binary.emplace_back(true);
		return params.size();
	}
	size_t push(StringStream &query, const data::Value &val, bool force) {
		if (!force) {
			switch (val.getType()) {
			case data::Value::Type::EMPTY:
				query << "NULL";
				break;
			case data::Value::Type::BOOLEAN:
				if (val.asBool()) {
					query << "TRUE";
				} else {
					query << "FALSE";
				}
				break;
			case data::Value::Type::INTEGER:
				query << val.asInteger();
				break;
			case data::Value::Type::DOUBLE:
				if (isnan(val.asDouble())) {
					query << "NaN";
				} else if (val.asDouble() == std::numeric_limits<double>::infinity()) {
					query << "-Infinity";
				} else if (-val.asDouble() == std::numeric_limits<double>::infinity()) {
					query << "Infinity";
				} else {
					query << val.asDouble();
				}
				break;
			case data::Value::Type::CHARSTRING:
				params.emplace_back(Bytes());
				params.back().assign_strong((uint8_t *)val.getString().data(), val.size() + 1);
				binary.emplace_back(false);
				query << "$" << params.size() << "::text";
				break;
			case data::Value::Type::BYTESTRING:
				params.emplace_back(val.asBytes());
				binary.emplace_back(true);
				query << "$" << params.size() << "::bytea";
				break;
			case data::Value::Type::ARRAY:
			case data::Value::Type::DICTIONARY:
				params.emplace_back(data::write(val, data::EncodeFormat::Cbor));
				binary.emplace_back(true);
				query << "$" << params.size() << "::bytea";
				break;
			default: break;
			}
		} else {
			params.emplace_back(data::write(val, data::EncodeFormat::Cbor));
			binary.emplace_back(true);
			query << "$" << params.size() << "::bytea";
		}
		return params.size();
	}


	virtual void bindInt(Binder &, StringStream &query, int64_t val) override {
		query << val;
	}
	virtual void bindUInt(Binder &, StringStream &query, uint64_t val) override {
		query << val;
	}
	virtual void bindString(Binder &, StringStream &query, const String &val) override {
		if (auto num = push(String(val))) {
			query << "$" << num << "::text";
		}
	}
	virtual void bindMoveString(Binder &, StringStream &query, String &&val) override {
		if (auto num = push(move(val))) {
			query << "$" << num << "::text";
		}
	}
	virtual void bindStringView(Binder &, StringStream &query, const StringView &val) override {
		if (auto num = push(val.str())) {
			query << "$" << num << "::text";
		}
	}
	virtual void bindBytes(Binder &, StringStream &query, const Bytes &val) override {
		if (auto num = push(Bytes(val))) {
			query << "$" << num << "::bytea";
		}
	}
	virtual void bindMoveBytes(Binder &, StringStream &query, Bytes &&val) override {
		if (auto num = push(move(val))) {
			query << "$" << num << "::bytea";
		}
	}
	virtual void bindCoderSource(Binder &, StringStream &query, const CoderSource &val) override {
		if (auto num = push(Bytes(val.data(), val.data() + val.size()))) {
			query << "$" << num << "::bytea";
		}
	}
	virtual void bindValue(Binder &, StringStream &query, const data::Value &val) override {
		push(query, val, false);
	}
	virtual void bindDataField(Binder &, StringStream &query, const Binder::DataField &f) override {
		push(query, f.data, f.force);
	}
	virtual void bindTypeString(Binder &, StringStream &query, const Binder::TypeString &type) override {
		if (auto num = push(type.str)) {
			query << "$" << num << "::" << type.type;
		}
	}

	virtual void bindFullText(Binder &, StringStream &query, const Binder::FullTextField &d) override {
		if (!d.data || !d.data.isArray() || d.data.size() == 0) {
			query << "NULL";
		} else {
			bool first = true;
			for (auto &it : d.data.asArray()) {
				auto &data = it.getString(0);
				auto lang = storage::FullTextData::Language(it.getInteger(1));
				auto rank = it.getInteger(2);

				if (!data.empty()) {
					if (!first) { query << " || "; } else { first = false; }

					auto dataIdx = push(data);

					query << " setweight(to_tsvector('" << storage::FullTextData::getLanguageString(lang) << "', $" << dataIdx << "::text), '" << char('A' + char(rank)) << "')";
				}
			}
		}
	}

	virtual void bindFullTextRank(Binder &, StringStream &query, const Binder::FullTextRank &d) override {
		int normalizationValue = 0;
		if (d.field->hasFlag(storage::Flags::TsNormalize_DocLength)) {
			normalizationValue |= 2;
		} else if (d.field->hasFlag(storage::Flags::TsNormalize_DocLengthLog)) {
			normalizationValue |= 1;
		} else if (d.field->hasFlag(storage::Flags::TsNormalize_UniqueWordsCount)) {
			normalizationValue |= 8;
		} else if (d.field->hasFlag(storage::Flags::TsNormalize_UniqueWordsCountLog)) {
			normalizationValue |= 16;
		}
		query << " ts_rank(" << d.scheme << ".\"" << d.field->getName() << "\" , __ts_query_" << d.field->getName() << ", " << normalizationValue << ")";
	}

	virtual void bindFullTextData(Binder &, StringStream &query, const storage::FullTextData &d) override {
		auto idx = push(String(d.buffer));
		query  << " websearch_to_tsquery('" << d.getLanguageString() << "', $" << idx << "::text)";
	}

	virtual void clear() override {
		params.clear();
		binary.clear();
	}

public:
	Vector<Bytes> params;
	Vector<bool> binary;
};

class PgResultInterface : public sql::ResultInterface {
public:
	inline static constexpr bool pgsql_is_success(ExecStatusType x) {
		return (x == PGRES_EMPTY_QUERY) || (x == PGRES_COMMAND_OK) || (x == PGRES_TUPLES_OK) || (x == PGRES_SINGLE_TUPLE);
	}

	PgResultInterface(PGresult *res) : result(res) {
		err = result ? PQresultStatus(result) : PGRES_FATAL_ERROR;
	}
	virtual ~PgResultInterface() {
		clear();
	}

	virtual bool isNull(size_t row, size_t field) override {
		return PQgetisnull(result, row, field);
	}

	virtual StringView toString(size_t row, size_t field) override {
		return StringView(PQgetvalue(result, row, field), size_t(PQgetlength(result, row, field)));
	}
	virtual DataReader<ByteOrder::Host> toBytes(size_t row, size_t field) override {
		if (PQfformat(result, field) == 0) {
			auto val = PQgetvalue(result, row, field);
			auto len = PQgetlength(result, row, field);
			if (len > 2 && memcmp(val, "\\x", 2) == 0) {
				auto d = new Bytes(base16::decode(CoderSource(val + 2, len - 2)));
				return DataReader<ByteOrder::Host>(*d);
			}
			return DataReader<ByteOrder::Host>((uint8_t *)val, len);
		} else {
			return DataReader<ByteOrder::Host>((uint8_t *)PQgetvalue(result, row, field), size_t(PQgetlength(result, row, field)));
		}
	}
	virtual int64_t toInteger(size_t row, size_t field) override {
		if (PQfformat(result, field) == 0) {
			auto val = PQgetvalue(result, row, field);
			return StringToNumber<int64_t>(val, nullptr);
		} else {
			DataReader<ByteOrder::Network> r((const uint8_t *)PQgetvalue(result, row, field), size_t(PQgetlength(result, row, field)));
			switch (r.size()) {
			case 1: return r.readUnsigned(); break;
			case 2: return r.readUnsigned16(); break;
			case 4: return r.readUnsigned32(); break;
			case 8: return r.readUnsigned64(); break;
			default: break;
			}
			return 0;
		}
	}
	virtual double toDouble(size_t row, size_t field) override {
		if (PQfformat(result, field) == 0) {
			auto val = PQgetvalue(result, row, field);
			return StringToNumber<int64_t>(val, nullptr);
		} else {
			DataReader<ByteOrder::Network> r((const uint8_t *)PQgetvalue(result, row, field), size_t(PQgetlength(result, row, field)));
			switch (r.size()) {
			case 2: return r.readFloat16(); break;
			case 4: return r.readFloat32(); break;
			case 8: return r.readFloat64(); break;
			default: break;
			}
			return 0;
		}
	}
	virtual bool toBool(size_t row, size_t field) override {
		auto val = PQgetvalue(result, row, field);
		if (PQfformat(result, field) == 0) {
			if (val && (*val == 'T' || *val == 'y')) {
				return true;
			}
			return false;
		} else {
			return val && *val != 0;
		}
	}
	virtual int64_t toId() override {
		if (PQfformat(result, 0) == 0) {
			auto val = PQgetvalue(result, 0, 0);
			return StringToNumber<int64_t>(val, nullptr);
		} else {
			DataReader<ByteOrder::Network> r((const uint8_t *)PQgetvalue(result, 0, 0), size_t(PQgetlength(result, 0, 0)));
			switch (r.size()) {
			case 1: return int64_t(r.readUnsigned()); break;
			case 2: return int64_t(r.readUnsigned16()); break;
			case 4: return int64_t(r.readUnsigned32()); break;
			case 8: return int64_t(r.readUnsigned64()); break;
			default: break;
			}
			return 0;
		}
	}
	virtual StringView getFieldName(size_t field) override {
		auto ptr = PQfname(result, field);
		if (ptr) {
			return StringView(ptr);
		}
		return StringView();
	}
	virtual bool isSuccess() const override {
		return result && pgsql_is_success(err);
	}
	virtual size_t getRowsCount() const override {
		return size_t(PQntuples(result));
	}
	virtual size_t getFieldsCount() const override {
		return size_t(PQnfields(result));
	}
	virtual size_t getAffectedRows() const override {
		return StringToNumber<size_t>(PQcmdTuples(result));
	}
	virtual data::Value getInfo() const override {
		return data::Value {
			pair("error", data::Value(err)),
			pair("status", data::Value(PQresStatus(err))),
			pair("desc", data::Value(result ? PQresultErrorMessage(result) : "Fatal database error")),
		};
	}
	virtual void clear() {
		if (result) {
	        PQclear(result);
	        result = nullptr;
		}
	}

	ExecStatusType getError() const {
		return err;
	}

public:
	PGresult *result = nullptr;
	ExecStatusType err = PGRES_EMPTY_QUERY;
};

struct ExecParamData {
	std::array<const char *, 64> values;
	std::array<int, 64> sizes;
	std::array<int, 64> formats;

	Vector<const char *> valuesVec;
	Vector<int> sizesVec;
	Vector<int> formatsVec;

	const char *const * paramValues = nullptr;
	const int *paramLengths = nullptr;
	const int *paramFormats = nullptr;

	ExecParamData(const sql::SqlQuery &query) {
		auto queryInterface = static_cast<PgQueryInterface *>(query.getInterface());

		auto size = queryInterface->params.size();

		if (size > 64) {
			valuesVec.reserve(size);
			sizesVec.reserve(size);
			formatsVec.reserve(size);

			for (size_t i = 0; i < size; ++ i) {
				const Bytes &d = queryInterface->params.at(i);
				bool bin = queryInterface->binary.at(i);
				valuesVec.emplace_back((const char *)d.data());
				sizesVec.emplace_back(int(d.size()));
				formatsVec.emplace_back(bin);
			}

			paramValues = valuesVec.data();
			paramLengths = sizesVec.data();
			paramFormats = formatsVec.data();
		} else {
			for (size_t i = 0; i < size; ++ i) {
				const Bytes &d = queryInterface->params.at(i);
				bool bin = queryInterface->binary.at(i);
				values[i] = (const char *)d.data();
				sizes[i] = int(d.size());
				formats[i] = bin;
			}

			paramValues = values.data();
			paramLengths = sizes.data();
			paramFormats = formats.data();
		}
	}
};

Handle::Handle(apr_pool_t *p, ap_dbd_t *h) : pool(p), handle(h) {
	if (strcmp(apr_dbd_name(handle->driver), "pgsql") == 0) {
		conn = (PGconn *)apr_dbd_native_handle(handle->driver, handle->handle);
	}
}
Handle::Handle(Handle &&h) : pool(h.pool), handle(h.handle), conn(h.conn) {
	h.pool = nullptr;
	h.handle = nullptr;
	h.conn = nullptr;
}

Handle & Handle::operator=(Handle &&h) {
	pool = h.pool;
	handle = h.handle;
	conn = h.conn;
	h.pool = nullptr;
	h.handle = nullptr;
	h.conn = nullptr;
	return *this;
}

Handle::operator bool() const {
	return pool != nullptr && conn != nullptr;
}

void Handle::makeQuery(const Callback<void(sql::SqlQuery &)> &cb) {
	PgQueryInterface interface;
	sql::SqlQuery query(&interface);
	cb(query);
}

bool Handle::selectQuery(const sql::SqlQuery &query, const Callback<void(sql::Result &)> &cb) {
	if (!conn || getTransactionStatus() == storage::TransactionStatus::Rollback) {
		return false;
	}

	auto queryInterface = static_cast<PgQueryInterface *>(query.getInterface());

	ExecParamData data(query);
	PgResultInterface res(PQexecParams(conn, query.getQuery().weak().data(), queryInterface->params.size(), nullptr,
			data.paramValues, data.paramLengths, data.paramFormats, 1));
	if (!res.isSuccess()) {
		auto info = res.getInfo();
#if DEBUG
		std::cout << query.getQuery().weak() << "\n";
		std::cout << data::EncodeFormat::Pretty << info << "\n";
#endif
		messages::debug("Database", "Fail to perform query", move(info));
		messages::error("Database", "Fail to perform query");
		cancelTransaction_pg();
	}

	lastError = res.getError();

	sql::Result ret(&res);
	cb(ret);
	return res.isSuccess();
}

bool Handle::performSimpleQuery(const StringView &query) {
	if (getTransactionStatus() == storage::TransactionStatus::Rollback) {
		return false;
	}

	PgResultInterface res(PQexec(conn, query.data()));
	lastError = res.getError();
	if (!res.isSuccess()) {
		auto info = res.getInfo();
		std::cout << query << "\n";
		std::cout << info << "\n";
		cancelTransaction_pg();
	}
	return res.isSuccess();
}

bool Handle::performSimpleSelect(const StringView &query, const Callback<void(sql::Result &)> &cb) {
	if (getTransactionStatus() == storage::TransactionStatus::Rollback) {
		return false;
	}

	PgResultInterface res(PQexec(conn, query.data()));
	lastError = res.getError();

	if (res.isSuccess()) {
		sql::Result ret(&res);
		cb(ret);
	} else {
		auto info = res.getInfo();
		std::cout << query << "\n";
		std::cout << info << "\n";
		cancelTransaction_pg();
	}

	return res.isSuccess();
}

bool Handle::beginTransaction_pg(TransactionLevel l) {
	int64_t userId = 0;
	int64_t now = Time::now().toMicros();
	if (auto req = apr::pool::request()) {
		userId = Request(req).getUserId();
	}

	auto setVariables = [&] {
		performSimpleQuery(toString("SET LOCAL serenity.\"user\" = ", userId, ";SET LOCAL serenity.\"now\" = ", now, ";"));
	};

	switch (l) {
	case TransactionLevel::ReadCommited:
		if (performSimpleQuery("BEGIN ISOLATION LEVEL READ COMMITTED"_weak)) {
			setVariables();
			level = TransactionLevel::ReadCommited;
			transactionStatus = storage::TransactionStatus::Commit;
			return true;
		}
		break;
	case TransactionLevel::RepeatableRead:
		if (performSimpleQuery("BEGIN ISOLATION LEVEL REPEATABLE READ"_weak)) {
			setVariables();
			level = TransactionLevel::RepeatableRead;
			transactionStatus = storage::TransactionStatus::Commit;
			return true;
		}
		break;
	case TransactionLevel::Serialized:
		if (performSimpleQuery("BEGIN ISOLATION LEVEL SERIALIZABLE"_weak)) {
			setVariables();
			level = TransactionLevel::Serialized;
			transactionStatus = storage::TransactionStatus::Commit;
			return true;
		}
		break;
	default:
		break;
	}
	return false;
}

void Handle::cancelTransaction_pg() {
	transactionStatus = storage::TransactionStatus::Rollback;
}

bool Handle::endTransaction_pg() {
	switch (transactionStatus) {
	case storage::TransactionStatus::Commit:
		transactionStatus = storage::TransactionStatus::None;
		if (performSimpleQuery("COMMIT"_weak)) {
			finalizeBroadcast();
			return true;
		}
		break;
	case storage::TransactionStatus::Rollback:
		transactionStatus = storage::TransactionStatus::None;
		if (performSimpleQuery("ROLLBACK"_weak)) {
			finalizeBroadcast();
			return false;
		}
		break;
	default:
		break;
	}
	return false;
}

NS_SA_EXT_END(pg)
