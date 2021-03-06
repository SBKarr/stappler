/**
Copyright (c) 2019 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef STELLATOR_REQUEST_STCONNECTION_H_
#define STELLATOR_REQUEST_STCONNECTION_H_

#include "STServer.h"

namespace stellator {

class Connection : public mem::AllocBase {
public:
	struct Config;

	Connection();
	Connection(Config *);
	Connection & operator =(Config *);

	Connection(Connection &&);
	Connection & operator =(Connection &&);

	Connection(const Connection &);
	Connection & operator =(const Connection &);

public:
	bool isSecureConnection() const;

	mem::StringView getClientIp() const;

	mem::StringView getLocalIp() const;
	mem::StringView getLocalHost() const;

	bool isAborted() const;
	bool isKeepAlive() const;
    int getKeepAlivesCount() const;

	Server server() const;
	mem::pool_t *getPool() const;

protected:
	Config *_config = nullptr;
};

}

#endif /* STELLATOR_REQUEST_STCONNECTION_H_ */
