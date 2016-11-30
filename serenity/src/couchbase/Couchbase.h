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
