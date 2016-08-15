/*
 * Couchbase.h
 *
 *  Created on: 14 февр. 2016 г.
 *      Author: sbkarr
 */

#ifndef SERENITY_SRC_COUCHBASE_COUCHBASE_H_
#define SERENITY_SRC_COUCHBASE_COUCHBASE_H_

#include "CouchbaseConnection.h"
#include "Server.h"

#ifndef NOCB

NS_SA_EXT_BEGIN(couchbase)

class Handle : public AllocPool {
public:
	Handle();
	Handle(const serenity::Server &);
	Handle(const serenity::Request &);
	Handle(const serenity::Connection &);

	~Handle();

	Handle(Handle &&h);
	Handle& operator=(Handle &&h);

	Handle(const Handle &h) = delete;
	Handle& operator=(const Handle &h) = delete;

	operator bool() const { return connection != nullptr; }

	template <typename ... Args>
	inline void perform(Args && ... args) {
		if (connection) {
			connection->perform(std::forward<Args>(args)...);
		}
	}

protected:
	void acquire(const Server &);
	void release();

	Server serv;
	Connection *connection;
};

NS_SA_EXT_END(couchbase)

#endif // NOCB

#endif /* SERENITY_SRC_COUCHBASE_COUCHBASE_H_ */
