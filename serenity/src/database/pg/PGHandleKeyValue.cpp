// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2017 Roman Katuntsev <sbkarr@stappler.org>

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
#include "PGQuery.h"
#include "StorageFile.h"
#include "StorageScheme.h"
#include "User.h"
#include "Server.h"

NS_SA_EXT_BEGIN(pg)

void Handle::makeSessionsCleanup() {
	StringStream query;
	query << "DELETE FROM __sessions WHERE (mtime + maxage + 10) < " << Time::now().toSeconds() << ";";
	performSimpleQuery(query.weak());

	query.clear();
	query << "DELETE FROM __removed RETURNING __oid;";
	auto res = performSimpleSelect(query.weak());
	if (!res.empty()) {
		query.clear();
		query << "SELECT obj.__oid AS id, obj.tableoid::regclass AS t FROM __objects obj WHERE ";

		bool first = true;
		for (auto it : res) {
			if (first) { first = false; } else { query << " OR "; }
			query << "obj.__oid=" << it.at(0);
		}
		query << ";";
		res = performSimpleSelect(query.weak());
		if (!res.empty()) {
			query.clear();
			query << "DELETE FROM __files WHERE ";
			first = true;
			// check for files to remove
			for (auto it : res) {
				if (it.back() == "__files") {
					auto fileId = it.front().readInteger();
					filesystem::remove(storage::File::getFilesystemPath(fileId));

					if (first) { first = false; } else { query << " OR "; }
					query << " __oid=" << fileId;
				}
			}
			query << ";";
			performSimpleQuery(query.weak());

			// other processing can be added for other schemes
		}
	}

	query.clear();
	query << "DELETE FROM __broadcasts WHERE date < " << (Time::now() - TimeInterval::seconds(10)).toMicroseconds() << ";";
	performSimpleQuery(query.weak());
}

int64_t Handle::processBroadcasts(Server &serv, int64_t value) {
	ExecQuery query;
	int64_t maxId = value;
	if (value <= 0) {
		query.select("last_value").from("__broadcasts_id_seq").finalize();
		maxId = selectId(query);
	} else {
		query.select("id", "date", "msg").from("__broadcasts")
				.where("id", Comparation::GreatherThen, value).finalize();
		Result ret(select(query));
		for (auto it : ret) {
			if (it.size() >= 3) {
				auto msgId = it.toInteger(0);
				Bytes msgData(it.toBytes(2));
				if (!msgData.empty()) {
					if (msgId > maxId) {
						maxId = msgId;
					}
					serv.onBroadcast(msgData);
				}
			}
		}
	}
	return maxId;
}

void Handle::broadcast(const Bytes &bytes) {
	if (getTransactionStatus() == TransactionStatus::None) {
		ExecQuery query;
		query.insert("__broadcasts").fields("date", "msg").values(apr_time_now(), Bytes(bytes)).finalize();
		perform(query);
	} else {
		_bcasts.emplace_back(apr_time_now(), bytes);
	}
}

User * Handle::authorizeUser(const Scheme &s, const String &iname, const String &password) {
	ExecQuery query;
	query.with("u", [&] (ExecQuery::GenericQuery &q) {
		auto f = q.select().from(s.getName());
		String name = iname;
		if (valid::validateEmail(name)) {
			f.where("email", Comparation::Equal, std::move(name));
		} else {
			f.where("name", Comparation::Equal, std::move(name));
		}
	}).with("l", [&] (ExecQuery::GenericQuery &q) {
		q.select().count("failed_count").from("__login").innerJoinOn("u", [&] (ExecQuery::WhereBegin &w) {
			w.where(ExecQuery::Field( "__login", "user"), Comparation::Equal, ExecQuery::Field("u", "__oid"))
					.where(Operator::And, ExecQuery::Field( "__login", "success"), Comparation::Equal, data::Value(false))
					.where(Operator::And, ExecQuery::Field( "__login", "date"), Comparation::GreatherThen, uint64_t((Time::now() - config::getMaxAuthTime()).toSeconds()));
		});
	}).select().from("l", "u").finalize();
	auto res = select(query);
	if (res.nrows() != 1) {
		return nullptr;
	}

	auto count = res.front().toInteger(0);
	if (count >= config::getMaxLoginFailure()) {
		messages::error("Auth", "Autorization blocked", data::Value{
			pair("cooldown", data::Value((int64_t)config::getMaxAuthTime().toSeconds())),
			pair("failedAttempts", data::Value((int64_t)count)),
		});
		return nullptr;
	}

	data::Value d(res.decode(s));
	if (d.size() != 1) {
		return nullptr;
	}

	auto &ud = d.getValue(0);
	auto &passwd = ud.getBytes("password");

	auto & fields = s.getFields();
	auto it = s.getFields().find("password");
	if (it != fields.end() && it->second.getTransform() == storage::Transform::Password) {
		auto f = static_cast<const storage::FieldPassword *>(it->second.getSlot());
		User *ret = nullptr;
		bool success = false;
		auto req = apr::pool::request();
		if (valid::validatePassord(password, passwd, f->salt)) {
			ret = new User(std::move(ud), s);
			success = true;
		} else {
			messages::error("Auth", "Login attempts", data::Value(config::getMaxLoginFailure() - count - 1));
		}

		query.clear();
		query.insert("__login")
				.fields("user", "name", "password", "date", "success", "addr", "host", "path")
				.values(ud.getInteger("__oid"), iname, passwd, Time::now().toSeconds(), data::Value(success),
						ExecQuery::TypeString(req?req->useragent_ip:"NULL", "inet"), String(req?req->hostname:"NULL"), String(req?req->uri:"NULL"))
				.finalize();
		perform(query);
		return ret;
	}

	return nullptr;
}

data::Value Handle::getData(const String &key) {
	Bytes bkey; bkey.resize(key.size() + 4);
	memcpy(bkey.data(), "kvs:", 4);
	memcpy(bkey.data() + 4, key.data(), key.size());
	return getKVData(std::move(bkey));
}

bool Handle::setData(const String &key, const data::Value &data, TimeInterval maxage) {
	Bytes bkey; bkey.resize(key.size() + 4);
	memcpy(bkey.data(), "kvs:", 4);
	memcpy(bkey.data() + 4, key.data(), key.size());
	return setKVData(std::move(bkey), data, maxage);
}

bool Handle::clearData(const String &key) {
	Bytes bkey; bkey.resize(key.size() + 4);
	memcpy(bkey.data(), "kvs:", 4);
	memcpy(bkey.data() + 4, key.data(), key.size());
	return clearKVData(std::move(bkey));
}

data::Value Handle::getSessionData(const Bytes &key) {
	return getKVData(Bytes(key));
}

bool Handle::setSessionData(const Bytes &key, const data::Value &data, TimeInterval maxage) {
	return setKVData(Bytes(key), data, maxage);
}

bool Handle::clearSessionData(const Bytes &key) {
	return clearKVData(Bytes(key));
}

data::Value Handle::getKVData(Bytes &&key) {
	ExecQuery query;
	query.select("data").from("__sessions").where("name", Comparation::Equal, move(key)).finalize();
	auto res = select(query);
	if (res.nrows() == 1) {
		return data::read(res.front().toBytes(0));
	}
	return data::Value();
}
bool Handle::setKVData(Bytes &&key, const data::Value &data, TimeInterval maxage) {
	ExecQuery query;
	query.insert("__sessions").fields("name", "mtime", "maxage", "data")
			.values(move(key), Time::now().toSeconds(), maxage.toSeconds(), data::write(data, data::EncodeFormat::Cbor))
			.onConflict("name").doUpdate().excluded("mtime").excluded("maxage").excluded("data")
			.finalize();
	return perform(query) != maxOf<size_t>();
}
bool Handle::clearKVData(Bytes &&key) {
	ExecQuery query;
	query.remove("__sessions").where("name", Comparation::Equal, move(key)).finalize();
	return perform(query) == 1; // one row should be affected
}

NS_SA_EXT_END(pg)
