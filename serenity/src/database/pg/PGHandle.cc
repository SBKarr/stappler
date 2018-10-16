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
#include "PGHandleTypes.h"
#include "PGQuery.h"
#include "ResourceTemplates.h"
#include "StorageScheme.h"
#include "User.h"

NS_SA_EXT_BEGIN(pg)

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

	ExecParamData(const ExecQuery &query) {
		auto size = query.getParams().size();

		if (size > 64) {
			valuesVec.reserve(size);
			sizesVec.reserve(size);
			formatsVec.reserve(size);

			for (size_t i = 0; i < size; ++ i) {
				const Bytes &d = query.getParams().at(i);
				bool bin = query.getBinaryVec().at(i);
				valuesVec.emplace_back((const char *)d.data());
				sizesVec.emplace_back(int(d.size()));
				formatsVec.emplace_back(bin);
			}

			paramValues = valuesVec.data();
			paramLengths = sizesVec.data();
			paramFormats = formatsVec.data();
		} else {
			for (size_t i = 0; i < size; ++ i) {
				const Bytes &d = query.getParams().at(i);
				bool bin = query.getBinaryVec().at(i);
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
			transactionStatus = TransactionStatus::Commit;
			return true;
		}
		break;
	case TransactionLevel::RepeatableRead:
		if (performSimpleQuery("BEGIN ISOLATION LEVEL REPEATABLE READ"_weak)) {
			setVariables();
			level = TransactionLevel::RepeatableRead;
			transactionStatus = TransactionStatus::Commit;
			return true;
		}
		break;
	case TransactionLevel::Serialized:
		if (performSimpleQuery("BEGIN ISOLATION LEVEL SERIALIZABLE"_weak)) {
			setVariables();
			level = TransactionLevel::Serialized;
			transactionStatus = TransactionStatus::Commit;
			return true;
		}
		break;
	default:
		break;
	}
	return false;
}

void Handle::cancelTransaction_pg() {
	transactionStatus = TransactionStatus::Rollback;
}

bool Handle::endTransaction_pg() {
	switch (transactionStatus) {
	case TransactionStatus::Commit:
		transactionStatus = TransactionStatus::None;
		if (performSimpleQuery("COMMIT"_weak)) {
			finalizeBroadcast();
			return true;
		}
		break;
	case TransactionStatus::Rollback:
		transactionStatus = TransactionStatus::None;
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

void Handle::finalizeBroadcast() {
	if (!_bcasts.empty()) {
		ExecQuery query;
		auto vals = query.insert("__broadcasts").fields("date", "msg").values();
		for (auto &it : _bcasts) {
			vals.values(it.first, move(it.second));
		}
		query.finalize();
		perform(query);
		_bcasts.clear();
	}
}

Result Handle::select(const ExecQuery &query) {
	if (!conn || getTransactionStatus() == TransactionStatus::Rollback) {
		return Result();
	}

	ExecParamData data(query);
	Result res(PQexecParams(conn, query.getQuery().weak().data(), query.getParams().size(), nullptr,
			data.paramValues, data.paramLengths, data.paramFormats, 1));
	if (!res || !res.success()) {
		std::cout << res.info() << "\n";
		messages::debug("Database", "Fail mto perform query", res.info());
		messages::error("Database", "Fail to perform query");
	}

	lastError = res.getError();
	return res;
}

int64_t Handle::selectId(const ExecQuery &query) {
	if (getTransactionStatus() == TransactionStatus::Rollback) {
		return 0;
	}
	Result res(select(query));
	if (!res.empty()) {
		return res.readId();
	}
	return 0;
}
size_t Handle::perform(const ExecQuery &query) {
	if (getTransactionStatus() == TransactionStatus::Rollback) {
		return maxOf<size_t>();
	}
	Result res(select(query));
	if (res.success()) {
		return res.getAffectedRows();
	}
	return maxOf<size_t>();
}

data::Value Handle::select(const Scheme &scheme, const ExecQuery &query) {
	auto result = select(query);
	if (result) {
		return result.decode(scheme);
	}
	return data::Value();
}

data::Value Handle::select(const Field &field, const ExecQuery &query) {
	auto result = select(query);
	if (result) {
		return result.decode(field);
	}
	return data::Value();
}

static bool Handle_convertViewDelta(data::Value &it) {
	auto vid_it = it.asDict().find("__vid");
	auto d_it = it.asDict().find("__delta");
	if (vid_it != it.asDict().end() && d_it != it.asDict().end()) {
		if (vid_it->second.getInteger()) {
			d_it->second.setString("update", "action");
			it.asDict().erase(vid_it);
		} else {
			d_it->second.setString("delete", "action");
			auto dict_it = it.asDict().begin();
			while (dict_it != it.asDict().end()) {
				if (dict_it->first != "__oid" && dict_it->first != "__delta") {
					dict_it = it.asDict().erase(dict_it);
				} else {
					++ dict_it;
				}
			}
			return false;
		}
	}
	return true;
}

static void Handle_mergeViews(data::Value &objs, data::Value &vals) {
	for (auto &it : objs.asArray()) {
		if (!Handle_convertViewDelta(it)) {
			continue;
		}

		if (auto oid = it.getInteger("__oid")) {
			auto v_it = std::lower_bound(vals.asArray().begin(), vals.asArray().end(), oid,
					[&] (const data::Value &l, int64_t r) -> bool {
				return (l.isInteger() ? l.getInteger() : l.getInteger("__oid")) < r;
			});
			if (v_it != vals.asArray().end()) {
				auto objId = v_it->getInteger("__oid");
				if (objId == oid) {
					v_it->erase("__oid");
					if (it.hasValue("__views")) {
						it.getValue("__views").addValue(move(*v_it));
					} else {
						it.emplace("__views").addValue(move(*v_it));
					}
					v_it->setInteger(oid);
				}
			}
		}
	}
}

void Handle::select(data::Value &objs, const Field &field, const ExecQuery &query) {
	auto result = select(query);
	if (result) {
		auto vals = result.decode(field);
		if (!vals.isArray()) {
			for (auto &it : objs.asArray()) {
				Handle_convertViewDelta(it);
			}
		} else if (vals.isArray() && objs.isArray()) {
			Handle_mergeViews(objs, vals);
		}
	}
}

bool Handle::performSimpleQuery(const String &query) {
	if (getTransactionStatus() == TransactionStatus::Rollback) {
		return false;
	}

	Result res(PQexec(conn, query.data()));
	lastError = res.getError();
	return res.success();
}

Result Handle::performSimpleSelect(const String &query) {
	if (getTransactionStatus() == TransactionStatus::Rollback) {
		return Result();
	}

	Result res(PQexec(conn, query.data()));
	lastError = res.getError();
	return res;
}

Resource *Handle::makeResource(ResourceType type, QueryList &&list, const Field *f) {
	switch (type) {
	case ResourceType::ResourceList: return new ResourceReslist(this, std::move(list));  break;
	case ResourceType::ReferenceSet: return new ResourceRefSet(this, std::move(list)); break;
	case ResourceType::ObjectField: return new ResourceFieldObject(this, std::move(list)); break;
	case ResourceType::Object: return new ResourceObject(this, std::move(list)); break;
	case ResourceType::Set: return new ResourceSet(this, std::move(list)); break;
	case ResourceType::View: return new ResourceView(this, std::move(list)); break;
	case ResourceType::File: return new ResourceFile(this, std::move(list), f); break;
	case ResourceType::Array: return new ResourceArray(this, std::move(list), f); break;
	}
	return nullptr;
}

data::Value Handle::getHistory(const Scheme &scheme, const Time &time, bool resolveUsers) {
	data::Value ret;
	if (!scheme.hasDelta()) {
		return ret;
	}

	ExecQuery q;
	q.select().from(TableRec::getNameForDelta(scheme)).where("time", Comparation::GreatherThen, time.toMicroseconds())
			.order(Ordering::Descending, "time").finalize();

	auto res = select(q);
	for (auto it : res) {
		auto &d = ret.emplace();
		for (size_t i = 0; i < it.size(); ++ i) {
			auto name = res.name(i);
			if (name == "action") {
				switch (DeltaAction(it.toInteger(i))) {
				case DeltaAction::Create: d.setString("create", "action"); break;
				case DeltaAction::Update: d.setString("update", "action"); break;
				case DeltaAction::Delete: d.setString("delete", "action"); break;
				case DeltaAction::Append: d.setString("append", "action"); break;
				case DeltaAction::Erase:d.setString("erase", "action");  break;
				default: break;
				}
			} else if (name == "time") {
				d.setString(Time::microseconds(it.toInteger(i)).toHttp(), "http-date");
				d.setInteger(it.toInteger(i), "time");
			} else if (name == "user" && resolveUsers) {
				if (auto u = User::get(this, it.toInteger(i))) {
					auto &ud = d.emplace("user");
					ud.setInteger(u->getObjectId(), "id");
					ud.setString(u->getName(), "name");
				} else {
					d.setInteger(it.toInteger(i), name.str());
				}
			} else if (name != "id") {
				d.setInteger(it.toInteger(i), name.str());
			}
		}
	}

	return ret;
}

data::Value Handle::getHistory(const FieldView &view, const Scheme *scheme, uint64_t tag, const Time &time, bool resolveUsers) {
	data::Value ret;
	if (!view.delta) {
		return ret;
	}

	Result res;
	if (scheme) {
		String name = toString(scheme->getName(), "_f_", view.name, "_delta");

		ExecQuery q;
		q.select().from(name).where("time", Comparation::GreatherThen, time.toMicroseconds())
				.where(Operator::And, "tag", Comparation::Equal, tag)
				.order(Ordering::Descending, "time").finalize();

		res = select(q);
	}

	for (auto it : res) {
		auto &d = ret.emplace();
		for (size_t i = 0; i < it.size(); ++ i) {
			auto name = res.name(i);
			if (name == "tag") {
				d.setInteger(it.toInteger(i), "tag");
			} else if (name == "time") {
				d.setString(Time::microseconds(it.toInteger(i)).toHttp(), "http-date");
				d.setInteger(it.toInteger(i), "time");
			} else if (name == "user" && resolveUsers) {
				if (auto u = User::get(this, it.toInteger(i))) {
					auto &ud = d.emplace("user");
					ud.setInteger(u->getObjectId(), "id");
					ud.setString(u->getName(), "name");
				} else {
					d.setInteger(it.toInteger(i), name.str());
				}
			} else if (name != "id") {
				d.setInteger(it.toInteger(i), name.str());
			}
		}
	}

	return ret;
}

data::Value Handle::getDeltaData(const Scheme &scheme, const Time &time) {
	if (scheme.hasDelta()) {
		ExecQuery q;
		q.writeQueryDelta(scheme, time, Set<const Field *>(), false);
		q.finalize();

		return select(scheme, q);
	}
	return data::Value();
}

int64_t Handle::getDeltaValue(const Scheme &scheme) {
	if (scheme.hasDelta()) {
		ExecQuery q;
		q.select().aggregate("max", ExecQuery::Field("d", "time"))
				.from(ExecQuery::Field(TableRec::getNameForDelta(scheme.getName())).as("d")).finalize();
		if (auto res = select(q)) {
			return res.at(0).toInteger(0);
		}
	}
	return 0;
}

int64_t Handle::getDeltaValue(const Scheme &scheme, const FieldView &view, uint64_t tag) {
	if (view.delta) {
		String deltaName = toString(scheme.getName(), "_f_", view.name, "_delta");
		ExecQuery q;
		q.select().aggregate("max", ExecQuery::Field("d", "time"))
				.from(ExecQuery::Field(deltaName).as("d")).where("tag", Comparation::Equal, tag).finalize();
		if (auto res = select(q)) {
			return res.at(0).toInteger(0);
		}
	}
	return 0;
}

NS_SA_EXT_END(pg)
