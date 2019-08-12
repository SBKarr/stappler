// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2016-2019 Roman Katuntsev <sbkarr@stappler.org>

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
#include "ServerComponent.h"

NS_SA_BEGIN

ServerComponent::Loader::Loader(const mem::StringView &str, Symbol s) : name(str), loader(s) { }

ServerComponent::ServerComponent(Server &serv, const String &name, const data::Value &dict)
: _server(serv), _name(name), _version("1.0"), _config(dict) {
	if (_config.isString("version")) {
		_version = _config.getString("version");
	}
}

void ServerComponent::onChildInit(Server &serv) { }

void ServerComponent::onStorageInit(Server &, const storage::Adapter &) { }

void ServerComponent::onStorageTransaction(storage::Transaction &) { }

const storage::Scheme * ServerComponent::exportScheme(const storage::Scheme &scheme) {
	return _server.exportScheme(scheme);
}

void ServerComponent::addCommand(const StringView &name, Function<data::Value(const StringView &)> &&cb, const StringView &desc, const StringView &help) {
	_commands.emplace(name.str(), Command{name, desc, help, move(cb)});
}

const ServerComponent::Command *ServerComponent::getCommand(const StringView &name) const {
	auto it = _commands.find(name);
	if (it != _commands.end()) {
		return &it->second;
	}
	return nullptr;
}

const Map<String, ServerComponent::Command> &ServerComponent::getCommands() const {
	return _commands;
}

NS_SA_END
