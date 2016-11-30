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

#ifndef SERENITY_SRC_SERVER_SERVERCOMPONENT_H_
#define SERENITY_SRC_SERVER_SERVERCOMPONENT_H_

#include "Server.h"
#include "AccessControl.h"

NS_SA_BEGIN

class ServerComponent : public AllocPool {
public:
	using Symbol = ServerComponent * (*) (Server &serv, const String &name, const data::Value &dict);
	using Scheme = storage::Scheme;

	ServerComponent(Server &serv, const String &name, const data::Value &dict);
	virtual ~ServerComponent() { }

	virtual void onChildInit(Server &);

	const data::Value & getConfig() const { return _config; }
	const String & getName() const { return _name; }

	storage::Scheme * exportScheme(storage::Scheme *);

protected:
	Server _server;
	String _name;
	data::Value _config;
};


NS_SA_END

#endif /* SERENITY_SRC_SERVER_SERVERCOMPONENT_H_ */
