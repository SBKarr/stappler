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

#include "STServerComponent.h"

namespace stellator {

ServerComponent::ServerComponent(Server &serv, const mem::StringView &name, const mem::Value &dict)
: _server(serv), _name(name.str<mem::Interface>()), _version("1.0"), _config(dict) {
	if (_config.isString("version")) {
		_version = _config.getString("version");
	}
}

void ServerComponent::onChildInit(Server &serv) { }

void ServerComponent::onStorageInit(Server &, const db::Adapter &) { }

void ServerComponent::onStorageTransaction(db::Transaction &) { }

void ServerComponent::onHeartbeat(Server &, db::Transaction &) { }

const db::Scheme * ServerComponent::exportScheme(const db::Scheme &scheme) {
	return _server.exportScheme(scheme);
}

void ServerComponent::addCommand(const mem::StringView &name, mem::Function<mem::Value(const mem::StringView &)> &&cb, const mem::StringView &desc, const mem::StringView &help) {
	_commands.emplace(name.str<mem::Interface>(), Command{name.str<mem::Interface>(), desc.str<mem::Interface>(), help.str<mem::Interface>(), move(cb)});
}

const ServerComponent::Command *ServerComponent::getCommand(const mem::StringView &name) const {
	auto it = _commands.find(name);
	if (it != _commands.end()) {
		return &it->second;
	}
	return nullptr;
}

const mem::Map<mem::String, ServerComponent::Command> &ServerComponent::getCommands() const {
	return _commands;
}

}
