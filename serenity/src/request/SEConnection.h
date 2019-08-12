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

#ifndef SERENITY_SRC_REQUEST_SECONNECTION_H_
#define SERENITY_SRC_REQUEST_SECONNECTION_H_

#include "Server.h"

NS_SA_BEGIN

class Connection : public AllocPool {
public:
	Connection();
	Connection(conn_rec *);
	Connection & operator =(conn_rec *);

	Connection(Connection &&);
	Connection & operator =(Connection &&);

	Connection(const Connection &);
	Connection & operator =(const Connection &);

	conn_rec *connection() const { return _conn; }
	operator conn_rec *() const { return _conn; }
	operator bool () const { return _conn != nullptr; }

public:
	bool isSecureConnection() const;

	apr::weak_string getClientIp() const;

	apr::weak_string getLocalIp() const;
	apr::weak_string getLocalHost() const;

	bool isAborted() const;
	bool isKeepAlive() const;
    int getKeepAlivesCount() const;

	Server server() const;
	apr_pool_t *pool() const;

protected:
	struct Config;

	conn_rec *_conn = nullptr;
	Config *_config = nullptr;
};

NS_SA_END

#endif /* SERENITY_SRC_REQUEST_CONNECTION_H_ */
