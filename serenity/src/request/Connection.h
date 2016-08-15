/*
 * Connection.h
 *
 *  Created on: 25 дек. 2015 г.
 *      Author: sbkarr
 */

#ifndef SERENITY_SRC_REQUEST_CONNECTION_H_
#define SERENITY_SRC_REQUEST_CONNECTION_H_

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
