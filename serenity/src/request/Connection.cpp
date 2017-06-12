// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2016 Roman Katuntsev <sbkarr@stappler.org>

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
#include "Connection.h"
#include "Root.h"

APLOG_USE_MODULE(serenity);

NS_SA_BEGIN

struct Connection::Config : public AllocPool {
	static Config *get(conn_rec *c) {
		if (!c) { return nullptr; }
		auto cfg = (Config *)ap_get_module_config(c->conn_config, &serenity_module);
		if (cfg) { return cfg; }

		return apr::pool::perform([&] () -> Config * {
			return new Config(c);
		}, c);
	}

	Config(conn_rec *c)  {
		ap_set_module_config(c->conn_config, &serenity_module, this);
	}

	const apr::vector<apr::string> &getPath() {
		return _path;
	}

	const data::Value &getData() {
		return _data;
	}

	apr::vector<apr::string> _path;
	data::Value _data;
};

Connection::Connection() : _conn(nullptr), _config(nullptr) { }
Connection::Connection(conn_rec *c) : _conn(c), _config(Config::get(c)) { }
Connection & Connection::operator =(conn_rec *c) {
	_conn = c;
	_config = Config::get(c);
	return *this;
}

Connection::Connection(Connection &&other) : _conn(other._conn), _config(other._config) { }
Connection & Connection::operator =(Connection &&other) {
	_conn = other._conn;
	_config = other._config;
	return *this;
}

Connection::Connection(const Connection &other) : _conn(other._conn), _config(other._config) { }
Connection & Connection::operator =(const Connection &other) {
	_conn = other._conn;
	_config = other._config;
	return *this;
}

bool Connection::isSecureConnection() const {
	return Root::getInstance()->isSecureConnection(_conn);
}

apr::weak_string Connection::getClientIp() const {
	return apr::string::make_weak(_conn->client_ip);
}

apr::weak_string Connection::getLocalIp() const {
	return apr::string::make_weak(_conn->local_ip);
}
apr::weak_string Connection::getLocalHost() const {
	return apr::string::make_weak(_conn->local_host);
}

bool Connection::isAborted() const {
	return _conn->aborted;
}
bool Connection::isKeepAlive() const {
	return _conn->keepalive == AP_CONN_KEEPALIVE;
}
int Connection::getKeepAlivesCount() const {
	return _conn->keepalives;
}

Server Connection::server() const {
	return Server(_conn->base_server);
}

apr_pool_t *Connection::pool() const {
	return _conn->pool;
}

NS_SA_END
