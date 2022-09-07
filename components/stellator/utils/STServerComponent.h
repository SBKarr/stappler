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

#ifndef STELLATOR_UTILS_STSERVERCOMPONENT_H_
#define STELLATOR_UTILS_STSERVERCOMPONENT_H_

#include "Request.h"

NS_SA_ST_BEGIN

class ServerComponent : public mem::AllocBase {
public:
	using Symbol = ServerComponent * (*) (Server &serv, const mem::String &name, const mem::Value &dict);
	using Scheme = db::Scheme;

	using CommandCallback = mem::Function<bool(mem::StringView, const mem::Callback<void(const mem::Value &)> &)>;

	struct Loader {
		mem::StringView name;
		Symbol loader;

		Loader(const mem::StringView &, Symbol);
	};

	struct Command {
		mem::String name;
		mem::String desc;
		mem::String help;
		CommandCallback callback;
	};

	ServerComponent(Server &serv, const mem::StringView &name, const mem::Value &dict);
	virtual ~ServerComponent() { }

	virtual void onChildInit(Server &);
	virtual void onStorageInit(Server &, const db::Adapter &);
	virtual void onStorageTransaction(db::Transaction &);
	virtual void onHeartbeat(Server &);

	const mem::Value & getConfig() const { return _config; }
	mem::StringView getName() const { return _name; }
	mem::StringView getVersion() const { return _version; }

	const db::Scheme * exportScheme(const db::Scheme &);

	void addCommand(const mem::StringView &name, mem::Function<mem::Value(const mem::StringView &)> &&,
			const mem::StringView &desc = mem::StringView(), const mem::StringView &help = mem::StringView());
	void addOutputCommand(const mem::StringView &name, CommandCallback &&,
			const mem::StringView &desc = mem::StringView(), const mem::StringView &help = mem::StringView());
	const Command *getCommand(const mem::StringView &name) const;

	const mem::Map<mem::String, Command> &getCommands() const;

	template <typename Value>
	void exportValues(Value &&val) {
		exportScheme(val);
	}

	template <typename Value, typename ... Args>
	void exportValues(Value &&val, Args && ... args) {
		exportValues(std::forward<Value>(val));
		exportValues(std::forward<Args>(args)...);
	}

	Server getServer() const { return _server; }

protected:
	mem::Map<mem::String, Command> _commands;

	Server _server;
	mem::String _name;
	mem::String _version;
	mem::Value _config;
};

NS_SA_ST_END

#endif /* STELLATOR_UTILS_STSERVERCOMPONENT_H_ */
