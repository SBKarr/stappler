// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2017-2019 Roman Katuntsev <sbkarr@stappler.org>

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

#include "STPqHandle.h"

NS_DB_PQ_BEGIN

static std::mutex s_logMutex;

static mem::String pg_numeric_to_string(stappler::BytesViewNetwork r) {
	using NumericDigit = int16_t;
	static constexpr auto DEC_DIGITS = 4;
	static constexpr auto NUMERIC_NEG = 0x4000;

	int d;
	NumericDigit dig;
	NumericDigit d1;

	uint16_t ndigits = uint16_t(r.readUnsigned16());
	int16_t weight = int16_t(r.readUnsigned16());
	int16_t sign = int16_t(r.readUnsigned16());
	int16_t dscale = int16_t(r.readUnsigned16());

	mem::Vector<int16_t> digits;
	for (uint16_t i = 0; i < ndigits && !r.empty(); i++) {
		digits.emplace_back(r.readUnsigned16());
	}

	int i = (weight + 1) * DEC_DIGITS;
	if (i <= 0) {
		i = 1;
	}

	mem::String str; str.reserve(i + dscale + DEC_DIGITS + 2);

	if (sign == NUMERIC_NEG) { str.push_back('-'); }
	if (weight < 0) {
		d = weight + 1;
		str.push_back('0');
	} else {
		for (d = 0; d <= weight; d++) {
			dig = (d < ndigits) ? digits[d] : 0;

			bool putit = (d > 0);
			d1 = dig / 1000;
			dig -= d1 * 1000;
			putit |= (d1 > 0);
			if (putit) { str.push_back(d1 + '0'); }
			d1 = dig / 100;
			dig -= d1 * 100;
			putit |= (d1 > 0);
			if (putit) { str.push_back(d1 + '0'); }
			d1 = dig / 10;
			dig -= d1 * 10;
			putit |= (d1 > 0);
			if (putit) { str.push_back(d1 + '0'); }
			str.push_back(dig + '0');
		}
	}

	if (dscale > 0) {
		str.push_back('.');
		for (i = 0; i < dscale; d++, i += DEC_DIGITS) {
			dig = (d >= 0 && d < ndigits) ? digits[d] : 0;

			d1 = dig / 1000;
			dig -= d1 * 1000;
			str.push_back(d1 + '0');
			d1 = dig / 100;
			dig -= d1 * 100;
			str.push_back(d1 + '0');
			d1 = dig / 10;
			dig -= d1 * 10;
			str.push_back(d1 + '0');
			str.push_back(dig + '0');
		}
	}

	return str;
}

class PgQueryInterface : public db::QueryInterface {
public:
	using Binder = db::Binder;

	size_t push(mem::String &&val) {
		params.emplace_back(mem::Bytes());
		params.back().assign_strong((uint8_t *)val.data(), val.size() + 1);
		binary.emplace_back(false);
		return params.size();
	}

	size_t push(const mem::StringView &val) {
		params.emplace_back(mem::Bytes());
		params.back().assign_strong((uint8_t *)val.data(), val.size() + 1);
		binary.emplace_back(false);
		return params.size();
	}

	size_t push(mem::Bytes &&val) {
		params.emplace_back(std::move(val));
		binary.emplace_back(true);
		return params.size();
	}
	size_t push(mem::StringStream &query, const mem::Value &val, bool force, bool compress = false) {
		if (!force || val.getType() == mem::Value::Type::EMPTY) {
			switch (val.getType()) {
			case mem::Value::Type::EMPTY:
				query << "NULL";
				break;
			case mem::Value::Type::BOOLEAN:
				if (val.asBool()) {
					query << "TRUE";
				} else {
					query << "FALSE";
				}
				break;
			case mem::Value::Type::INTEGER:
				query << val.asInteger();
				break;
			case mem::Value::Type::DOUBLE:
				if (std::isnan(val.asDouble())) {
					query << "NaN";
				} else if (val.asDouble() == std::numeric_limits<double>::infinity()) {
					query << "-Infinity";
				} else if (-val.asDouble() == std::numeric_limits<double>::infinity()) {
					query << "Infinity";
				} else {
					query << std::setprecision(std::numeric_limits<double>::max_digits10 + 1) << val.asDouble();
				}
				break;
			case mem::Value::Type::CHARSTRING:
				params.emplace_back(mem::Bytes());
				params.back().assign_strong((uint8_t *)val.getString().data(), val.size() + 1);
				binary.emplace_back(false);
				query << "$" << params.size() << "::text";
				break;
			case mem::Value::Type::BYTESTRING:
				params.emplace_back(val.asBytes());
				binary.emplace_back(true);
				query << "$" << params.size() << "::bytea";
				break;
			case mem::Value::Type::ARRAY:
			case mem::Value::Type::DICTIONARY:
				params.emplace_back(mem::writeData(val, mem::EncodeFormat(mem::EncodeFormat::Cbor,
						compress ? mem::EncodeFormat::LZ4HCCompression : mem::EncodeFormat::DefaultCompress)));
				binary.emplace_back(true);
				query << "$" << params.size() << "::bytea";
				break;
			default: break;
			}
		} else {
			params.emplace_back(mem::writeData(val, mem::EncodeFormat(mem::EncodeFormat::Cbor,
					compress ? mem::EncodeFormat::LZ4HCCompression : mem::EncodeFormat::DefaultCompress)));
			binary.emplace_back(true);
			query << "$" << params.size() << "::bytea";
		}
		return params.size();
	}


	virtual void bindInt(db::Binder &, mem::StringStream &query, int64_t val) override {
		query << val;
	}
	virtual void bindUInt(db::Binder &, mem::StringStream &query, uint64_t val) override {
		query << val;
	}
	virtual void bindDouble(db::Binder &, mem::StringStream &query, double val) override {
		query << std::setprecision(std::numeric_limits<double>::max_digits10 + 1) << val;
	}
	virtual void bindString(db::Binder &, mem::StringStream &query, const mem::String &val) override {
		if (auto num = push(mem::String(val))) {
			query << "$" << num << "::text";
		}
	}
	virtual void bindMoveString(db::Binder &, mem::StringStream &query, mem::String &&val) override {
		if (auto num = push(std::move(val))) {
			query << "$" << num << "::text";
		}
	}
	virtual void bindStringView(db::Binder &, mem::StringStream &query, const mem::StringView &val) override {
		if (auto num = push(val)) {
			query << "$" << num << "::text";
		}
	}
	virtual void bindBytes(db::Binder &, mem::StringStream &query, const mem::Bytes &val) override {
		if (auto num = push(mem::Bytes(val))) {
			query << "$" << num << "::bytea";
		}
	}
	virtual void bindMoveBytes(db::Binder &, mem::StringStream &query, mem::Bytes &&val) override {
		if (auto num = push(std::move(val))) {
			query << "$" << num << "::bytea";
		}
	}
	virtual void bindCoderSource(db::Binder &, mem::StringStream &query, const stappler::CoderSource &val) override {
		if (auto num = push(mem::Bytes(val.data(), val.data() + val.size()))) {
			query << "$" << num << "::bytea";
		}
	}
	virtual void bindValue(db::Binder &, mem::StringStream &query, const mem::Value &val) override {
		push(query, val, false);
	}
	virtual void bindDataField(db::Binder &, mem::StringStream &query, const db::Binder::DataField &f) override {
		if (f.field && f.field->getType() == db::Type::Custom) {
			if (!f.field->getSlot<db::FieldCustom>()->writeToStorage(*this, query, f.data)) {
				query << "NULL";
			}
		} else {
			push(query, f.data, f.force, f.compress);
		}
	}
	virtual void bindTypeString(db::Binder &, mem::StringStream &query, const db::Binder::TypeString &type) override {
		if (auto num = push(type.str)) {
			query << "$" << num << "::" << type.type;
		}
	}

	virtual void bindFullText(db::Binder &, mem::StringStream &query, const db::Binder::FullTextField &d) override {
		if (!d.data || !d.data.isArray() || d.data.size() == 0) {
			query << "NULL";
		} else {
			bool first = true;
			for (auto &it : d.data.asArray()) {
				auto &data = it.getString(0);
				auto lang = stappler::search::Language(it.getInteger(1));
				auto rank = stappler::search::SearchData::Rank(it.getInteger(2));
				auto type = stappler::search::SearchData::Type(it.getInteger(3));

				if (!data.empty()) {
					if (!first) { query << " || "; } else { first = false; }
					auto dataIdx = push(data);
					if (type == stappler::search::SearchData::Parse) {
						if (rank == stappler::search::SearchData::Unknown) {
							query << " to_tsvector('" << stappler::search::getLanguageName(lang) << "', $" << dataIdx << "::text)";
						} else {
							query << " setweight(to_tsvector('" << stappler::search::getLanguageName(lang) << "', $" << dataIdx << "::text), '" << char('A' + char(rank)) << "')";
						}
					} else {
						query << " $" << dataIdx << "::tsvector";
					}
				} else {
					query << " NULL";
				}
			}
		}
	}

	virtual void bindFullTextRank(db::Binder &, mem::StringStream &query, const db::Binder::FullTextRank &d) override {
		int normalizationValue = 0;
		if (d.field->hasFlag(db::Flags::TsNormalize_DocLength)) {
			normalizationValue |= 2;
		} else if (d.field->hasFlag(db::Flags::TsNormalize_DocLengthLog)) {
			normalizationValue |= 1;
		} else if (d.field->hasFlag(db::Flags::TsNormalize_UniqueWordsCount)) {
			normalizationValue |= 8;
		} else if (d.field->hasFlag(db::Flags::TsNormalize_UniqueWordsCountLog)) {
			normalizationValue |= 16;
		}
		query << " ts_rank(" << d.scheme << ".\"" << d.field->getName() << "\", " << d.query << ", " << normalizationValue << ")";
	}

	virtual void bindFullTextData(db::Binder &, mem::StringStream &query, const db::FullTextData &d) override {
		auto idx = push(mem::String(d.buffer));
		switch (d.type) {
		case db::FullTextData::Parse:
			query  << " websearch_to_tsquery('" << d.getLanguage() << "', $" << idx << "::text)";
			break;
		case db::FullTextData::Cast:
			query  << " to_tsquery('" << d.getLanguage() << "', $" << idx << "::text)";
			break;
		case db::FullTextData::ForceCast:
			query  << " $" << idx << "::tsquery ";
			break;
		}
	}

	virtual void bindIntVector(Binder &, mem::StringStream &query, const mem::Vector<int64_t> &vec) override {
		query << "(";
		bool start = true;
		for (auto &it : vec) {
			if (start) { start = false; } else { query << ","; }
			query << it;
		}
		query << ")";
	}

	virtual void bindDoubleVector(Binder &b, mem::StringStream &query, const mem::Vector<double> &vec) override {
		query << "(";
		bool start = true;
		for (auto &it : vec) {
			if (start) { start = false; } else { query << ","; }
			bindDouble(b, query, it);
		}
		query << ")";
	}

	virtual void bindStringVector(Binder &b, mem::StringStream &query, const mem::Vector<mem::StringView> &vec) override {
		query << "(";
		bool start = true;
		for (auto &it : vec) {
			if (start) { start = false; } else { query << ","; }
			bindStringView(b, query, it);
		}
		query << ")";
	}

	virtual void clear() override {
		params.clear();
		binary.clear();
	}

public:
	mem::Vector<mem::Bytes> params;
	mem::Vector<bool> binary;
};


struct ExecParamData {
	std::array<const char *, 64> values;
	std::array<int, 64> sizes;
	std::array<int, 64> formats;

	mem::Vector<const char *> valuesVec;
	mem::Vector<int> sizesVec;
	mem::Vector<int> formatsVec;

	const char *const * paramValues = nullptr;
	const int *paramLengths = nullptr;
	const int *paramFormats = nullptr;

	ExecParamData(const db::sql::SqlQuery &query) {
		auto queryInterface = static_cast<PgQueryInterface *>(query.getInterface());

		auto size = queryInterface->params.size();

		if (size > 64) {
			valuesVec.reserve(size);
			sizesVec.reserve(size);
			formatsVec.reserve(size);

			for (size_t i = 0; i < size; ++ i) {
				const mem::Bytes &d = queryInterface->params.at(i);
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
				const mem::Bytes &d = queryInterface->params.at(i);
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

Handle::Handle(const Driver *d, Driver::Handle h) : driver(d), handle(h) {
	if (h.get()) {
		auto c = d->getConnection(h);
		if (c.get()) {
			conn = c;

			performSimpleSelect("SELECT current_database();", [&] (db::Result &qResult) {
				if (!qResult.empty()) {
					dbName = qResult.current().toString(0).pdup();
				}
			});
		}
	}
}

Handle::Handle(Handle &&h) : driver(h.driver), handle(h.handle), conn(h.conn), lastError(h.lastError), level(h.level) {
	h.conn = Driver::Connection(nullptr);
	h.driver = nullptr;
}

Handle & Handle::operator=(Handle &&h) {
	handle = h.handle;
	conn = h.conn;
	driver = h.driver;
	lastError = h.lastError;
	level = h.level;
	h.conn = Driver::Connection(nullptr);
	h.driver = nullptr;
	return *this;
}

Handle::operator bool() const {
	return conn.get() != nullptr;
}

Driver::Handle Handle::getHandle() const {
	return handle;
}

Driver::Connection Handle::getConnection() const {
	return conn;
}

void Handle::close() {
	conn = Driver::Connection(nullptr);
}

void Handle::makeQuery(const stappler::Callback<void(sql::SqlQuery &)> &cb) {
	PgQueryInterface interface;
	db::sql::SqlQuery query(&interface);
	cb(query);
}

bool Handle::selectQuery(const sql::SqlQuery &query, const stappler::Callback<bool(sql::Result &)> &cb,
		const mem::Callback<void(const mem::Value &)> &errCb) {
	if (!conn.get() || getTransactionStatus() == db::TransactionStatus::Rollback) {
		return false;
	}

	auto queryInterface = static_cast<PgQueryInterface *>(query.getInterface());

	if (messages::isDebugEnabled()) {
		if (!query.getTarget().starts_with("__")) {
			messages::local("Database-Query", query.getQuery().weak());
		}
	}

	ExecParamData data(query);
	ResultCursor res(driver, driver->exec(conn, query.getQuery().weak().data(), queryInterface->params.size(),
			data.paramValues, data.paramLengths, data.paramFormats, 1));
	if (!res.isSuccess()) {
		auto info = res.getInfo();
#if DEBUG
		std::cout << query.getQuery().weak() << "\n";
		std::cout << mem::EncodeFormat::Pretty << info << "\n";
		info.setString(query.getQuery().str(), "query");
#endif
		if (errCb) {
			errCb(info);
		}
		messages::debug("Database", "Fail to perform query", std::move(info));
		messages::error("Database", "Fail to perform query");
		cancelTransaction_pg();
	}

	lastError = res.getError();

	db::sql::Result ret(&res);
	return cb(ret);
}

bool Handle::performSimpleQuery(const mem::StringView &query, const mem::Callback<void(const mem::Value &)> &errCb) {
	if (getTransactionStatus() == db::TransactionStatus::Rollback) {
		return false;
	}

	ResultCursor res(driver, driver->exec(conn, query.data()));
	lastError = res.getError();
	if (!res.isSuccess()) {
		auto info = res.getInfo();
		if (errCb) {
			errCb(info);
		}
		s_logMutex.lock();
		std::cout << query << "\n";
		std::cout << info << "\n";
		s_logMutex.unlock();
		cancelTransaction_pg();
	}
	return res.isSuccess();
}

bool Handle::performSimpleSelect(const mem::StringView &query, const stappler::Callback<void(sql::Result &)> &cb,
		const mem::Callback<void(const mem::Value &)> &errCb) {
	if (getTransactionStatus() == db::TransactionStatus::Rollback) {
		return false;
	}

	ResultCursor res(driver, driver->exec(conn, query.data(), 0, nullptr, nullptr, nullptr, 1));
	lastError = res.getError();

	if (res.isSuccess()) {
		db::sql::Result ret(&res);
		cb(ret);
		return true;
	} else {
		auto info = res.getInfo();
		if (errCb) {
			errCb(info);
		}
		s_logMutex.lock();
		std::cout << query << "\n";
		std::cout << info << "\n";
		s_logMutex.unlock();
		cancelTransaction_pg();
	}

	return false;
}

bool Handle::isSuccess() const {
	return ResultCursor::pgsql_is_success(lastError);
}

bool Handle::beginTransaction_pg(TransactionLevel l) {
	int64_t userId = internals::getUserIdFromContext();
	int64_t now = stappler::Time::now().toMicros();

	auto setVariables = [&] {
		performSimpleQuery(mem::toString("SET LOCAL serenity.\"user\" = ", userId, ";SET LOCAL serenity.\"now\" = ", now, ";"));
	};

	switch (l) {
	case TransactionLevel::ReadCommited:
		if (performSimpleQuery("BEGIN ISOLATION LEVEL READ COMMITTED"_weak)) {
			setVariables();
			level = TransactionLevel::ReadCommited;
			transactionStatus = db::TransactionStatus::Commit;
			return true;
		}
		break;
	case TransactionLevel::RepeatableRead:
		if (performSimpleQuery("BEGIN ISOLATION LEVEL REPEATABLE READ"_weak)) {
			setVariables();
			level = TransactionLevel::RepeatableRead;
			transactionStatus = db::TransactionStatus::Commit;
			return true;
		}
		break;
	case TransactionLevel::Serialized:
		if (performSimpleQuery("BEGIN ISOLATION LEVEL SERIALIZABLE"_weak)) {
			setVariables();
			level = TransactionLevel::Serialized;
			transactionStatus = db::TransactionStatus::Commit;
			return true;
		}
		break;
	default:
		break;
	}
	return false;
}

void Handle::cancelTransaction_pg() {
	transactionStatus = db::TransactionStatus::Rollback;
}

bool Handle::endTransaction_pg() {
	switch (transactionStatus) {
	case db::TransactionStatus::Commit:
		transactionStatus = db::TransactionStatus::None;
		if (performSimpleQuery("COMMIT"_weak)) {
			finalizeBroadcast();
			return true;
		}
		break;
	case db::TransactionStatus::Rollback:
		transactionStatus = db::TransactionStatus::None;
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

NS_DB_PQ_END
