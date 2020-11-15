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
		if (auto num = push(val.str())) {
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

	virtual void clear() override {
		params.clear();
		binary.clear();
	}

public:
	mem::Vector<mem::Bytes> params;
	mem::Vector<bool> binary;
};

class PgResultInterface : public db::ResultInterface {
public:
	inline static constexpr bool pgsql_is_success(Driver::Status x) {
		return (x == Driver::Status::Empty) || (x == Driver::Status::CommandOk) || (x == Driver::Status::TuplesOk) || (x == Driver::Status::SingleTuple);
	}

	PgResultInterface(const Handle *h, Driver *d, Driver::Result res) : handle(h), driver(d), result(res) {
		err = result.get() ? driver->getStatus(result) : Driver::Status::FatalError;
	}
	virtual ~PgResultInterface() {
		clear();
	}

	virtual bool isBinaryFormat(size_t field) const override {
		return driver->isBinaryFormat(result, field) != 0;
	}

	virtual bool isNull(size_t row, size_t field) override {
		return driver->isNull(result, row, field);
	}

	virtual mem::StringView toString(size_t row, size_t field) override {
		return mem::StringView(driver->getValue(result, row, field), driver->getLength(result, row, field));
	}
	virtual mem::BytesView toBytes(size_t row, size_t field) override {
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
	virtual int64_t toInteger(size_t row, size_t field) override {
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
	virtual double toDouble(size_t row, size_t field) override {
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
	virtual bool toBool(size_t row, size_t field) override {
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
	virtual mem::Value toTypedData(size_t row, size_t field) override {
		auto t = driver->getType(result, field);
		auto s = handle->getTypeById(t);
		switch (s) {
		case Interface::StorageType::Unknown:
			std::cout << "Unknown type conversion: " << handle->getTypeNameById(t) << "\n";
			messages::error("DB", "Unknown type conversion", mem::Value(handle->getTypeNameById(t)));
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
	virtual int64_t toId() override {
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
	virtual mem::StringView getFieldName(size_t field) override {
		auto ptr = driver->getName(result, field);
		if (ptr) {
			return mem::StringView(ptr);
		}
		return mem::StringView();
	}
	virtual bool isSuccess() const override {
		return result.get() && pgsql_is_success(err);
	}
	virtual size_t getRowsCount() const override {
		return driver->getNTuples(result);
	}
	virtual size_t getFieldsCount() const override {
		return driver->getNFields(result);
	}
	virtual size_t getAffectedRows() const override {
		return driver->getCmdTuples(result);
	}
	virtual mem::Value getInfo() const override {
		return mem::Value({
			stappler::pair("error", mem::Value(stappler::toInt(err))),
			stappler::pair("status", mem::Value(driver->getStatusMessage(err))),
			stappler::pair("desc", mem::Value(result.get() ? driver->getResultErrorMessage(result) : "Fatal database error")),
		});
	}
	virtual void clear() {
		if (result.get()) {
			driver->clearResult(result);
	        result = Driver::Result(nullptr);
		}
	}

	Driver::Status getError() const {
		return err;
	}

public:
	const Handle *handle = nullptr;
	Driver *driver = nullptr;
	Driver::Result result = Driver::Result(nullptr);
	Driver::Status err = Driver::Status::Empty;
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

Handle::Handle(Driver *d, Driver::Handle h) : driver(d), handle(h) {
	if (h.get()) {
		auto c = d->getConnection(h);
		if (c.get()) {
			conn = c;

			performSimpleSelect("SELECT current_database();", [&] (db::sql::Result &qResult) {
				if (!qResult.empty()) {
					dbName = qResult.front().toString(0).pdup();
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

void Handle::setStorageTypeMap(const mem::Vector<mem::Pair<uint32_t, Interface::StorageType>> *vec) {
	storageTypes = vec;
}
void Handle::setCustomTypeMap(const mem::Vector<mem::Pair<uint32_t, mem::String>> *vec) {
	customTypes = vec;
}

Interface::StorageType Handle::getTypeById(uint32_t oid) const {
	if (storageTypes) {
		auto it = std::lower_bound(storageTypes->begin(), storageTypes->end(), oid, [] (const mem::Pair<uint32_t, Interface::StorageType> &l, uint32_t r) -> bool {
			return l.first < r;
		});
		if (it != storageTypes->end() && it->first == oid) {
			return it->second;
		}
	}
	return Interface::StorageType::Unknown;
}

mem::StringView Handle::getTypeNameById(uint32_t oid) const {
	if (customTypes) {
		auto it = std::lower_bound(customTypes->begin(), customTypes->end(), oid, [] (const mem::Pair<uint32_t, mem::String> &l, uint32_t r) -> bool {
			return l.first < r;
		});
		if (it != customTypes->end() && it->first == oid) {
			return it->second;
		}
	}
	return mem::StringView();
}

void Handle::close() {
	conn = Driver::Connection(nullptr);
}

void Handle::makeQuery(const stappler::Callback<void(sql::SqlQuery &)> &cb) {
	PgQueryInterface interface;
	db::sql::SqlQuery query(&interface);
	cb(query);
}

bool Handle::selectQuery(const sql::SqlQuery &query, const stappler::Callback<void(sql::Result &)> &cb) {
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
	PgResultInterface res(this, driver, driver->exec(conn, query.getQuery().weak().data(), queryInterface->params.size(),
			data.paramValues, data.paramLengths, data.paramFormats, 1));
	if (!res.isSuccess()) {
		auto info = res.getInfo();
#if DEBUG
		std::cout << query.getQuery().weak() << "\n";
		std::cout << mem::EncodeFormat::Pretty << info << "\n";
		info.setString(query.getQuery().str(), "query");
#endif
		messages::debug("Database", "Fail to perform query", std::move(info));
		messages::error("Database", "Fail to perform query");
		cancelTransaction_pg();
	}

	lastError = res.getError();

	db::sql::Result ret(&res);
	cb(ret);
	return res.isSuccess();
}

bool Handle::performSimpleQuery(const mem::StringView &query) {
	if (getTransactionStatus() == db::TransactionStatus::Rollback) {
		return false;
	}

	PgResultInterface res(this, driver, driver->exec(conn, query.data()));
	lastError = res.getError();
	if (!res.isSuccess()) {
		auto info = res.getInfo();
		s_logMutex.lock();
		std::cout << query << "\n";
		std::cout << info << "\n";
		s_logMutex.unlock();
		cancelTransaction_pg();
	}
	return res.isSuccess();
}

bool Handle::performSimpleSelect(const mem::StringView &query, const stappler::Callback<void(sql::Result &)> &cb) {
	if (getTransactionStatus() == db::TransactionStatus::Rollback) {
		return false;
	}

	PgResultInterface res(this, driver, driver->exec(conn, query.data()));
	lastError = res.getError();

	if (res.isSuccess()) {
		db::sql::Result ret(&res);
		cb(ret);
		return true;
	} else {
		auto info = res.getInfo();
		s_logMutex.lock();
		std::cout << query << "\n";
		std::cout << info << "\n";
		s_logMutex.unlock();
		cancelTransaction_pg();
	}

	return false;
}

bool Handle::isSuccess() const {
	return PgResultInterface::pgsql_is_success(lastError);
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
