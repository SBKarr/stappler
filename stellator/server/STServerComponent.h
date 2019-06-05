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

#ifndef STELLATOR_SERVER_STSERVERCOMPONENT_H_
#define STELLATOR_SERVER_STSERVERCOMPONENT_H_

#include "STDefine.h"

namespace stellator {

class ServerComponent : public mem::AllocBase {
public:
	struct Command {
		mem::StringView name;
		mem::StringView desc;
		mem::StringView help;
		mem::Function<mem::Value(const mem::StringView &)> callback;
	};

	ServerComponent(Server &serv, const mem::StringView &name, const mem::Value &dict);
	virtual ~ServerComponent() { }

	virtual void onChildInit(Server &);
	virtual void onStorageInit(Server &, const db::Adapter &);
	virtual void onStorageTransaction(db::Transaction &);

	const mem::Value & getConfig() const { return _config; }
	mem::StringView getName() const { return _name; }
	mem::StringView getVersion() const { return _version; }

	const db::Scheme * exportScheme(const db::Scheme &);

	void addCommand(const mem::StringView &name, mem::Function<mem::Value(const mem::StringView &)> &&,
			const mem::StringView &desc = mem::StringView(), const mem::StringView &help = mem::StringView());
	const Command *getCommand(const mem::StringView &name) const;

	const mem::Map<mem::String, Command> &getCommands() const;

	template <typename Value>
	void exportValues(Value &&val) {
		exportScheme(val);
	}

	template <typename Value, typename ... Args>
	void exportValues(Value &&val, Args && ... args) {
		exportValues(forward<Value>(val));
		exportValues(forward<Args>(args)...);
	}

protected:
	mem::Map<mem::String, Command> _commands;

	Server _server;
	mem::String _name;
	mem::String _version;
	mem::Value _config;
};

}

#endif /* STELLATOR_SERVER_STSERVERCOMPONENT_H_ */
