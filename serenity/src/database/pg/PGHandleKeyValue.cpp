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
		query << "SELECT last_value FROM __broadcasts_id_seq;";
		maxId = selectId(query);
	} else {
		query << "SELECT id, date, msg FROM __broadcasts WHERE id > " << value << ";";
		Result ret(select(query));
		for (auto it : ret) {
			if (it.size() >= 3) {
				auto msgId = it.front().readInteger();
				Bytes msgData(it.toBytes(2));
				if (!msgData.empty()) {
					if (msgId > maxId) {
						maxId = msgId;
					}
					serv.onBroadcast(msgData);
				}
			}
		}

		query.clear();
	}
	return maxId;
}

void Handle::broadcast(const Bytes &bytes) {
	if (transaction == TransactionStatus::None) {
		ExecQuery query;
		query << "INSERT INTO __broadcasts (date, msg) VALUES (" << apr_time_now() << ",";
		query.write(Bytes(bytes));
		query << ");";
		perform(query);
	} else {
		_bcasts.emplace_back(apr_time_now(), bytes);
	}
}

void Handle::setMinNextOid(uint64_t oid) {
	StringStream query;
	query << "SELECT setval('__objects___oid_seq', " << oid << ");";
	performSimpleQuery(query.weak());
}

User * Handle::authorizeUser(Scheme *s, const String &iname, const String &password) {
	ExecQuery query;
	query << "WITH u AS (SELECT * FROM " << s->getName() << " WHERE ";
	String name = iname;
	if (valid::validateEmail(name)) {
		query << "email=";
	} else {
		query << "name=";
	}
	query.write(std::move(name));

	query << "), l AS (SELECT count(*) AS failed_count FROM __login, u WHERE success=FALSE AND date>"
			<< (Time::now() - config::getMaxAuthTime()).toSeconds() << " AND \"user\"=u.__oid) "
			<< "SELECT l.failed_count, u.* FROM u, l;";

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

	data::Value d(res.decode(*s));
	if (d.size() != 1) {
		return nullptr;
	}

	auto &ud = d.getValue(0);
	auto &passwd = ud.getBytes("password");

	auto & fields = s->getFields();
	auto it = s->getFields().find("password");
	if (it != fields.end() && it->second.getTransform() == storage::Transform::Password) {
		auto f = static_cast<const storage::FieldPassword *>(it->second.getSlot());
		User *ret = nullptr;

		query.clear();
		query << "INSERT INTO __login (\"user\", name, password, date, success, addr, host, path) VALUES ("
				<< ud.getInteger("__oid") << ", ";
		query.write(String(name));
		query << ", ";
		query.write(Bytes(passwd));
		query << ", " << Time::now().toSeconds() << ", ";

		if (valid::validatePassord(password, passwd, f->salt)) {
			ret = new User(std::move(ud), s);
			query << "TRUE, ";
		} else {
			query << "FALSE, ";
			messages::error("Auth", "Login attempts", data::Value(config::getMaxLoginFailure() - count - 1));
		}
		auto req = apr::pool::request();
		if (req) {
			query.write(String(req->useragent_ip));
			query << ", ";
			query.write(String(req->hostname));
			query << ", ";
			query.write(String(req->uri));
		} else {
			query << "NULL, NULL, NULL";
		}
		query << ");";
		perform(query);
		return ret;
	}

	return nullptr;
}

data::Value Handle::getData(const String &key) {
	Bytes bkey; bkey.resize(key.size() + 4);
	memcpy(bkey.data(), "kvs:", 4);
	memcpy(bkey.data() + 4, key.data(), key.size());

	ExecQuery query("SELECT data FROM __sessions WHERE name=");
	query.write(std::move(bkey));
	query << ";";

	auto res = select(query);
	if (res.nrows() == 1) {
		return data::read(res.front().toBytes(0));
	}
	return data::Value();
}

bool Handle::setData(const String &key, const data::Value &data, TimeInterval maxage) {
	Bytes bkey; bkey.resize(key.size() + 4);
	memcpy(bkey.data(), "kvs:", 4);
	memcpy(bkey.data() + 4, key.data(), key.size());

	ExecQuery query("INSERT INTO __sessions  (name, mtime, maxage, data) VALUES (");
	query.write(std::move(bkey));
	query << ", " << Time::now().toSeconds() << ", " << maxage.toSeconds() << ", ";
	query.write(data::write(data, data::EncodeFormat::Cbor));
	query << ") ON CONFLICT (name) DO UPDATE SET mtime=EXCLUDED.mtime, maxage=EXCLUDED.maxage, data=EXCLUDED.data;";

	return perform(query) != maxOf<size_t>();
}

bool Handle::clearData(const String &key) {
	Bytes bkey; bkey.resize(key.size() + 4);
	memcpy(bkey.data(), "kvs:", 4);
	memcpy(bkey.data() + 4, key.data(), key.size());

	ExecQuery query("DELETE FROM __sessions WHERE name=");
	query.write(std::move(bkey));
	query << ";";

	return perform(query) == 1; // one row should be affected
}

data::Value Handle::getSessionData(const Bytes &key) {
	ExecQuery query("SELECT data FROM __sessions WHERE name=");
	query.write(Bytes(key));

	auto res = select(query);
	if (res.nrows() == 1) {
		return data::read(res.front().toBytes(0));
	}
	return data::Value();
}

bool Handle::setSessionData(const Bytes &key, const data::Value &data, TimeInterval maxage) {
	ExecQuery query("INSERT INTO __sessions  (name, mtime, maxage, data) VALUES (");
	query.write(Bytes(key));
	query << ", " << Time::now().toSeconds() << ", " << maxage.toSeconds() << ", ";
	query.write(data::write(data, data::EncodeFormat::Cbor));
	query << ") ON CONFLICT (name) DO UPDATE SET mtime=EXCLUDED.mtime, maxage=EXCLUDED.maxage, data=EXCLUDED.data;";

	return perform(query) != maxOf<size_t>();
}

bool Handle::clearSessionData(const Bytes &key) {
	ExecQuery query("DELETE FROM __sessions WHERE name=");
	query.write(Bytes(key));
	query << ";";

	return perform(query) == 1; // one row should be affected
}

NS_SA_EXT_END(pg)
