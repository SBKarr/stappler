/**
Copyright (c) 2016-2018 Roman Katuntsev <sbkarr@stappler.org>

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
#include "Tools.h"
#include "User.h"
#include "Output.h"
#include "StorageScheme.h"
#include "StorageAdapter.h"
#include "Resource.h"
#include "PGHandle.h"

NS_SA_EXT_BEGIN(tools)

struct SocketCommand;

class ShellSocketHandler : public websocket::Handler, ReaderClassBase<char> {
public:
	enum class ShellMode {
		Plain,
		Html,
	};

	ShellSocketHandler(Manager *m, const Request &req, User *user);

	void setShellMode(ShellMode m) { _mode = m; }
	ShellMode getShellMode() const { return _mode; }

	User *getUser() const { return _user; }
	const Vector<SocketCommand *> &getCommands() const { return _cmds; }

	bool onCommand(StringView &r);

	virtual void onBegin() override;

	// Data frame was recieved from network
	virtual bool onFrame(FrameType t, const Bytes &b) override;

	// Message was recieved from broadcast
	virtual bool onMessage(const data::Value &val) override;

	void sendCmd(const StringView &v);
	void sendError(const String &str);
	void sendData(const data::Value & data);

protected:
	Vector<SocketCommand *> _cmds;
	ShellMode _mode = ShellMode::Plain;
	User *_user;
};

struct SocketCommand : AllocBase {
	SocketCommand(const String &str) : name(str) { }
	virtual ~SocketCommand() { }
	virtual bool run(ShellSocketHandler &h, StringView &r) = 0;
	virtual const String desc() const = 0;
	virtual const String help() const = 0;

	String name;
};

struct ModeCmd : SocketCommand {
	ModeCmd() : SocketCommand("mode") { }

	virtual bool run(ShellSocketHandler &h, StringView &r) override {
		if (!r.empty()) {
			if (r.is("plain")) {
				h.setShellMode(ShellSocketHandler::ShellMode::Plain);
				h.send("You are now in plain text mode");
			} else if (r.is("html")) {
				h.setShellMode(ShellSocketHandler::ShellMode::Html);
				h.send("You are now in <font color=\"#3f51b5\">HTML</font> mode");
			}
		} else {
			if (h.getShellMode() == ShellSocketHandler::ShellMode::Plain) {
				h.send("Plain text mode");
			} else {
				h.send("<font color=\"#3f51b5\">HTML</font> mode");
			}
		}
		return true;
	}

	virtual const String desc() const {
		return "plain|html - Switch socket output mode"_weak;
	}
	virtual const String help() const {
		return "plain|html - Switch socket output mode"_weak;
	}
};

struct DebugCmd : SocketCommand {
	DebugCmd() : SocketCommand("debug") { }

	virtual bool run(ShellSocketHandler &h, StringView &r) override {
		if (!r.empty()) {
			if (r.is("on")) {
				messages::setDebugEnabled(true);
				h.send("Try to enable debug mode, wait a second, until all servers receive your message");
			} else if (r.is("off")) {
				messages::setDebugEnabled(false);
				h.send("Try to disable debug mode, wait a second, until all servers receive your message");
			}
		} else {
			if (messages::isDebugEnabled()) {
				h.send("Debug mode: On");
			} else {
				h.send("Debug mode: Off");
			}
		}
		return true;
	}

	virtual const String desc() const {
		return "on|off - Switch server debug mode"_weak;
	}
	virtual const String help() const {
		return "on|off - Switch server debug mode"_weak;
	}
};

struct ListCmd : SocketCommand {
	ListCmd() : SocketCommand("meta") { }

	virtual bool run(ShellSocketHandler &h, StringView &r) override {
		if (r.empty()) {
			data::Value ret;
			auto &schemes = h.request().server().getSchemes();
			for (auto &it : schemes) {
				ret.addString(it.first);
			}
			h.sendData(ret);
		} else if (r == "all") {
			data::Value ret;
			auto &schemes = h.request().server().getSchemes();
			for (auto &it : schemes) {
				auto &val = ret.emplace(it.first);
				auto &fields = it.second->getFields();
				for (auto &fit : fields) {
					val.setValue(fit.second.getTypeDesc(), fit.first);
				}
			}
			h.sendData(ret);
		} else {
			data::Value ret;
			auto cmd = r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
			auto scheme =  h.request().server().getScheme(cmd.str());
			if (scheme) {
				auto &fields = scheme->getFields();
				for (auto &fit : fields) {
					ret.setValue(fit.second.getTypeDesc(), fit.first);
				}
				h.sendData(ret);
			}
		}
		return true;
	}

	virtual const String desc() const {
		return "all|<name> - Get information about data scheme"_weak;
	}
	virtual const String help() const {
		return "all|<name> - Get information about data scheme"_weak;
	}
};

struct ResourceCmd : SocketCommand {
	ResourceCmd(const String &str) : SocketCommand(str) { }

	const storage::Scheme *acquireScheme(ShellSocketHandler &h, const StringView &scheme) {
		return h.request().server().getScheme(scheme.str());
	}

	Resource *acquireResource(ShellSocketHandler &h, const StringView &scheme, const StringView &path,
			const StringView &resolve, const data::Value &val = data::Value()) {
		if (!scheme.empty()) {
			auto s =  acquireScheme(h, scheme);
			if (s) {
				Resource *r = Resource::resolve(h.storage(), *s, path.empty()?String("/"):path.str());
				if (r) {
					r->setUser(h.getUser());
					if (!resolve.empty()) {
						if (resolve.front() == '(') {
							r->applyQuery(data::read(resolve));
						} else {
							r->setResolveOptions(data::Value(resolve.str()));
						}
					}
					if (!val.empty()) {
						r->applyQuery(val);
					}
					r->prepare();
					return r;
				} else {
					h.sendError(toString("Fail to resolve resource \"", path, "\" for scheme ", scheme));
				}
			} else {
				h.sendError(toString("No such scheme: ", scheme));
			}
		} else {
			h.sendError(toString("Scheme is not defined"));
		}
		return nullptr;
	}
};

struct GetCmd : ResourceCmd {
	GetCmd() : ResourceCmd("get") { }

	virtual bool run(ShellSocketHandler &h, StringView &r) override {
		auto schemeName = r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();

		StringView path("/");
		if (r.is('/')) {
			path = r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
			r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		}

		auto resolve = r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		if (auto r = acquireResource(h, schemeName, path, resolve)) {
			auto data = r->getResultObject();
			h.sendData(data);
		}
		return true;
	}

	virtual const String desc() const {
		return "<scheme> <path> <resolve> - Get data from scheme"_weak;
	}
	virtual const String help() const {
		return "<scheme> <path> <resolve> - Get data from scheme"_weak;
	}
};

struct HistoryCmd : ResourceCmd {
	HistoryCmd() : ResourceCmd("history") { }

	virtual bool run(ShellSocketHandler &h, StringView &r) override {
		auto schemeName = r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();

		int64_t time = 0;
		auto testTime = r.readInteger();
		if (testTime > 0) {
			time = testTime;
		}

		r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();

		StringView field; uint64_t tag = 0;
		if (!r.empty()) {
			field = r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
			r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
			tag = r.readInteger();
		}

		if (auto s = acquireScheme(h, schemeName)) {
			if (auto a = dynamic_cast<pg::Handle *>(h.storage())) {
				if (field.empty()) {
					h.sendData(a->getHistory(*s, Time::microseconds(time), true));
				} else if (auto f = s->getField(field)) {
					if (f->getType() == storage::Type::View) {
						h.sendData(a->getHistory(*static_cast<const storage::FieldView *>(f->getSlot()), s, tag, Time::microseconds(time), true));
					}
				}
				return true;
			}
		}

		h.sendError(toString("Scheme is not defined"));
		return false;
	}

	virtual const String desc() const {
		return "<scheme> <time> [<field> <tag>] - Changelog for scheme or view"_weak;
	}
	virtual const String help() const {
		return "<scheme> <time> [<field> <tag>] - Changelog for scheme or view"_weak;
	}
};

struct DeltaCmd : ResourceCmd {
	DeltaCmd() : ResourceCmd("delta") { }

	virtual bool run(ShellSocketHandler &h, StringView &r) override {
		auto schemeName = r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();

		int64_t time = 0;
		auto testTime = r.readInteger();
		if (testTime > 0) {
			time = testTime;
		}

		r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();

		StringView field; uint64_t tag = 0;
		if (!r.empty()) {
			field = r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
			r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
			tag = r.readInteger();
		}

		if (auto s = acquireScheme(h, schemeName)) {
			if (auto a = dynamic_cast<pg::Handle *>(h.storage())) {
				if (field.empty()) {
					h.sendData(a->getDeltaData(*s, Time::microseconds(time)));
				} else if (auto f = s->getField(field)) {
					if (f->getType() == storage::Type::View) {
						h.sendData(a->getDeltaData(*s, *static_cast<const storage::FieldView *>(f->getSlot()), Time::microseconds(time), tag));
					}
				}
				return true;
			}
		}

		h.sendError(toString("Scheme is not defined"));
		return false;
	}

	virtual const String desc() const {
		return "<scheme> <time> [<field> <tag>] - Delta for scheme"_weak;
	}
	virtual const String help() const {
		return "<scheme> <time> [<field> <tag>] - Delta for scheme"_weak;
	}
};

struct MultiCmd : ResourceCmd {
	MultiCmd() : ResourceCmd("multi") { }

	virtual bool run(ShellSocketHandler &h, StringView &r) override {
		r.skipUntil<StringView::Chars<'('>>();
		if (r.is('(')) {
			data::Value result;
			data::Value requests = data::read(r);
			if (requests.isDictionary()) {
				for (auto &it : requests.asDict()) {
					StringView path(it.first);
					StringView scheme = path.readUntil<StringView::Chars<'/'>>();
					if (path.is('/')) {
						++ path;
					}

					if (auto r = acquireResource(h, scheme, path, StringView(), it.second)) {
						result.setValue(r->getResultObject(), it.first);
					}
				}
			}
			h.sendData(result);
		}

		return true;
	}

	virtual const String desc() const {
		return "<request> - perform multi-request"_weak;
	}
	virtual const String help() const {
		return "<request> - perform multi-request"_weak;
	}
};

struct CreateCmd : ResourceCmd {
	CreateCmd() : ResourceCmd("create") { }

	virtual bool run(ShellSocketHandler &h, StringView &r) override {
		auto schemeName = r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();

		StringView path("/");
		if (r.is('/')) {
			path = r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
			r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		}

		data::Value patch = (r.is('{') || r.is('[') || r.is('(')) ? data::read(r) : Url::parseDataArgs(r, 1_KiB);
		if (auto r = acquireResource(h, schemeName, path, StringView())) {
			apr::array<InputFile> f;
			if (r->prepareCreate()) {
				auto ret = r->createObject(patch, f);
				h.sendData(ret);
				return true;
			} else {
				h.sendError(toString("Action for scheme ", schemeName, " is forbidden for ", h.getUser()->getName()));
			}
		}

		h.sendData(patch);
		h.sendError("Fail to create object with data:");

		return true;
	}

	virtual const String desc() const {
		return "<scheme> <path> <data> - Create object for scheme"_weak;
	}
	virtual const String help() const {
		return "<scheme> <path> <data> - Create object for scheme"_weak;
	}
};

struct UpdateCmd : ResourceCmd {
	UpdateCmd() : ResourceCmd("update") { }

	virtual bool run(ShellSocketHandler &h, StringView &r) override {
		auto schemeName = r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();

		StringView path("/");
		if (r.is('/')) {
			path = r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
			r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		}

		data::Value patch = (r.is('{') || r.is('[') || r.is('(')) ? data::read(r) : Url::parseDataArgs(r, 1_KiB);
		if (auto r = acquireResource(h, schemeName, path, StringView())) {
			apr::array<InputFile> f;
			if (r->prepareUpdate()) {
				auto ret = r->updateObject(patch, f);
				h.sendData(ret);
				return true;
			} else {
				h.sendError(toString("Action for scheme ", schemeName, " is forbidden for ", h.getUser()->getName()));
			}
		}

		h.sendData(patch);
		h.sendError("Fail to update object with data:");

		return true;
	}

	virtual const String desc() const {
		return "<scheme> <path> <data> - Update object for scheme"_weak;
	}
	virtual const String help() const {
		return "<scheme> <path> <data>  - Update object for scheme"_weak;
	}
};

struct UploadCmd : ResourceCmd {
	UploadCmd() : ResourceCmd("upload") { }

	virtual bool run(ShellSocketHandler &h, StringView &r) override {
		auto schemeName = r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();

		StringView path("/");
		if (r.is('/')) {
			path = r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
			r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		}

		if (auto r = acquireResource(h, schemeName, path, StringView())) {
			if (r->prepareCreate()) {
				Bytes bkey = valid::makeRandomBytes(8);
				String key = base16::encode(bkey);

				data::Value token {
					pair("scheme", data::Value(schemeName)),
					pair("path", data::Value(path)),
					pair("resolve", data::Value("")),
					pair("user", data::Value(h.getUser()->getObjectId())),
				};

				h.storage()->setData(key, token, TimeInterval::seconds(5));

				StringStream str;
				str << ":upload:" << data::Value{
					pair("url", data::Value(toString(h.request().getUri(), "/upload/", key))),
					pair("name", data::Value("content")),
				};
				h.send(str.str());
				return true;
			}
		}

		h.sendError("Fail to prepare upload");
		return true;
	}

	virtual const String desc() const {
		return "<scheme> <path> - Upload file for scheme resource"_weak;
	}
	virtual const String help() const {
		return "<scheme> <path> - Update file for scheme resource"_weak;
	}
};

struct AppendCmd : ResourceCmd {
	AppendCmd() : ResourceCmd("append") { }

	virtual bool run(ShellSocketHandler &h, StringView &r) override {
		auto schemeName = r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();

		StringView path("/");
		if (r.is('/')) {
			path = r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
			r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		}

		data::Value patch = (r.is('{') || r.is('[') || r.is('(')) ? data::read(r) : Url::parseDataArgs(r, 1_KiB);
		if (auto r = acquireResource(h, schemeName, path, StringView())) {
			if (r->prepareAppend()) {
				auto ret = r->appendObject(patch);
				h.sendData(ret);
				return true;
			} else {
				h.sendError(toString("Action for scheme ", schemeName, " is forbidden for ", h.getUser()->getName()));
			}
		}

		h.sendData(patch);
		h.sendError("Fail to update object with data:");

		return true;
	}

	virtual const String desc() const {
		return "<scheme> <path> <data> - Append object for scheme"_weak;
	}
	virtual const String help() const {
		return "<scheme> <path> <data> - Append object for scheme"_weak;
	}
};

struct DeleteCmd : ResourceCmd {
	DeleteCmd() : ResourceCmd("delete") { }

	virtual bool run(ShellSocketHandler &h, StringView &r) override {
		auto schemeName = r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();

		StringView path("/");
		if (r.is('/')) {
			path = r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
			r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		}

		if (auto r = acquireResource(h, schemeName, path, StringView())) {
			if (r->removeObject()) {
				h.sendData(data::Value(true));
				return true;
			} else {
				h.sendError(toString("Action for scheme ", schemeName, " is forbidden for ", h.getUser()->getName()));
			}
		}

		h.sendError("Fail to delete object");

		return true;
	}

	virtual const String desc() const {
		return "<scheme> <path> - Delete object for scheme"_weak;
	}
	virtual const String help() const {
		return "<scheme> <path> - Delete object for scheme"_weak;
	}
};

struct HandlersCmd : SocketCommand {
	HandlersCmd() : SocketCommand("handlers") { }

	virtual bool run(ShellSocketHandler &h, StringView &r) override {
		auto serv = h.request().server();
		auto &hdl = serv.getRequestHandlers();

		data::Value ret;
		for (auto &it : hdl) {
			auto &v = ret.emplace(it.first);
			if (!it.second.data.isNull()) {
				v.setValue(it.second.data, "data");
			}
			if (it.second.scheme) {
				v.setString(it.second.scheme->getName(), "scheme");
			}
			if (!it.second.component.empty()) {
				v.setString(it.second.component, "component");
			}
			if (it.first.back() == '/') {
				v.setBool(true, "forSubPaths");
			}
		}

		h.sendData(ret);
		return true;
	}

	virtual const String desc() const {
		return " - Information about registered handlers"_weak;
	}
	virtual const String help() const {
		return " - Information about registered handlers"_weak;
	}
};

struct CloseCmd : SocketCommand {
	CloseCmd() : SocketCommand("exit") { }

	virtual bool run(ShellSocketHandler &h, StringView &r) override {
		h.send("Connection will be closed by your request");
		return false;
	}

	virtual const String desc() const {
		return " - close current connection"_weak;
	}
	virtual const String help() const {
		return " - close current connection"_weak;
	}
};

struct EchoCmd : SocketCommand {
	EchoCmd() : SocketCommand("echo") { }

	virtual bool run(ShellSocketHandler &h, StringView &r) override {
		if (!r.empty()) { h.send(r.str()); }
		return true;
	}

	virtual const String desc() const {
		return "<message> - display message in current terminal"_weak;
	}
	virtual const String help() const {
		return "<message> - display message in current terminal"_weak;
	}
};

struct ParseCmd : SocketCommand {
	ParseCmd() : SocketCommand("parse") { }

	virtual bool run(ShellSocketHandler &h, StringView &r) override {
		data::Value patch = (r.is('{') || r.is('[') || r.is('(')) ? data::read(r) : Url::parseDataArgs(r, 1_KiB);
		h.sendData(patch);
		return true;
	}

	virtual const String desc() const {
		return "<message> - parse message as object changeset"_weak;
	}
	virtual const String help() const {
		return "<message> - parse message as object changeset"_weak;
	}
};

struct MsgCmd : SocketCommand {
	MsgCmd() : SocketCommand("msg") { }

	virtual bool run(ShellSocketHandler &h, StringView &r) override {
		h.sendBroadcast(data::Value{
			std::make_pair("user", data::Value(h.getUser()->getName())),
			std::make_pair("event", data::Value("message")),
			std::make_pair("message", data::Value(r.str())),
		});
		return true;
	}

	virtual const String desc() const {
		return "<message> - display message in all opened terminals"_weak;
	}
	virtual const String help() const {
		return "<message> - display message in all opened terminals"_weak;
	}
};

struct CountCmd : SocketCommand {
	CountCmd() : SocketCommand("count") { }

	virtual bool run(ShellSocketHandler &h, StringView &r) override {
		StringStream resp;
		resp << "Users on socket: " << h.manager()->size();
		h.send(resp.weak());
		return true;
	}

	virtual const String desc() const {
		return " - display number of opened terminals"_weak;
	}
	virtual const String help() const {
		return " - display number of opened terminals"_weak;
	}
};

struct HelpCmd : SocketCommand {
	HelpCmd() : SocketCommand("help") { }

	virtual bool run(ShellSocketHandler &h, StringView &r) override {
		auto & cmds = h.getCommands();
		StringStream stream;
		if (r.empty()) {
			for (auto &it : cmds) {
				stream << "  - " << it->name << " " << it->desc() << "\n";
			}
		} else {
			for (auto &it : cmds) {
				if (r == it->name) {
					stream << "  - " << it->name << " " << it->desc() << "\n" << it->help();
				}
			}
		}
		h.send(stream.weak());
		return true;
	}

	virtual const String desc() const {
		return "<>|<cmd> - display command list or information about command"_weak;
	}
	virtual const String help() const {
		return "<>|<cmd> - display command list or information about command"_weak;
	}
};

ShellSocketHandler::ShellSocketHandler(Manager *m, const Request &req, User *user) : Handler(m, req, 600_sec), _user(user) {
	sendBroadcast(data::Value{
		std::make_pair("user", data::Value(_user->getName())),
		std::make_pair("event", data::Value("enter")),
	});

	_cmds.push_back(new ListCmd());
	_cmds.push_back(new HandlersCmd());
	_cmds.push_back(new HistoryCmd());
	_cmds.push_back(new DeltaCmd());
	_cmds.push_back(new GetCmd());
	_cmds.push_back(new MultiCmd());
	_cmds.push_back(new CreateCmd());
	_cmds.push_back(new UpdateCmd());
	_cmds.push_back(new AppendCmd());
	_cmds.push_back(new UploadCmd());
	_cmds.push_back(new DeleteCmd());
	//_cmds.push_back(new ModeCmd());
	_cmds.push_back(new DebugCmd());
	_cmds.push_back(new CloseCmd());
	_cmds.push_back(new EchoCmd());
	_cmds.push_back(new ParseCmd());
	_cmds.push_back(new MsgCmd());
	_cmds.push_back(new CountCmd());
	_cmds.push_back(new HelpCmd());
}

bool ShellSocketHandler::onCommand(StringView &r) {
	auto cmd = r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
	r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();

	for (auto &it : _cmds) {
		if (cmd == it->name) {
			return it->run(*this, r);
		}
	}

	sendError("No such command, use 'help' to list available commands");
	return true;
}

void ShellSocketHandler::onBegin() {
	StringStream resp;
	resp << "Serenity: Welcome. " << _user->getName() << "!";
	send(resp.weak());
}

bool ShellSocketHandler::onFrame(FrameType t, const Bytes &b) {
	if (t == FrameType::Text) {
		StringView r((const char *)b.data(), b.size());
		sendCmd(r);
		return onCommand(r);
	}
	return true;
}

bool ShellSocketHandler::onMessage(const data::Value &val) {
	if (val.isBool("message")) {
		sendData(val);
	} else  if (val.isString("event")) {
		auto &ev = val.getString("event");
		if (ev == "enter") {
			StringStream resp;
			resp << "User " << val.getString("user") << " connected.";
			send(resp.weak());
		} else if (ev == "message") {
			StringStream resp;
			resp << "- [" << val.getString("user") << "] " << val.getString("message");
			send(resp.weak());
		}
	}

	return true;
}

void ShellSocketHandler::sendCmd(const StringView &v) {
	StringStream stream;
	stream << _user->getName() << "@" << _request.getHostname() << ": " << v;
	send(stream.weak());
}

void ShellSocketHandler::sendError(const String &str) {
	StringStream stream;
	switch(_mode) {
	case ShellMode::Plain: stream << "Error: " << str << "\n"; break;
	case ShellMode::Html: stream << "<span class=\"error\">Error:</span> " << str; break;
	};
	send(stream.weak());
}

void ShellSocketHandler::sendData(const data::Value & data) {
	StringStream stream;
	switch(_mode) {
	case ShellMode::Plain: data::write(stream, data, data::EncodeFormat::Json); break;
	case ShellMode::Html: stream << "<p>"; output::formatJsonAsHtml(stream, data); stream << "</p>"; break;
	};
	send(stream.weak());
}


ShellSocket::Handler * ShellSocket::onAccept(const Request &req) {
	Request rctx(req);
	if (!req.isSecureConnection() && req.getUseragentIp() != "127.0.0.1" && req.getUseragentIp() != "::1") {
		rctx.setStatus(HTTP_FORBIDDEN);
		return nullptr;
	}

	auto &data = rctx.getParsedQueryArgs();
	if (data.isString("name") && data.isString("passwd")) {
		auto &name = data.getString("name");
		auto &passwd = data.getString("passwd");

		if (auto a = rctx.storage()) {
			User * user = User::get(a, name, passwd);
			if (!user || !user->isAdmin()) {
				rctx.setStatus(HTTP_FORBIDDEN);
			} else {
				rctx.setUser(user);
				return new ShellSocketHandler(this, req, user);
			}
		}
	}
	return nullptr;
}

bool ShellSocket::onBroadcast(const data::Value &) {
	return true;
}

int ShellGui::onPostReadRequest(Request &rctx) {
	if (rctx.getMethod() == Request::Get) {
		if (_subPath.empty()) {
			rctx.setContentType("text/html;charset=UTF-8");
			rctx << VirtualFile::get("/html/shell.html");
			return DONE;
		} else {
			return HTTP_NOT_FOUND;
		}
	} else if (rctx.getMethod() == Request::Post) {
		StringView path(_subPath);
		if (!path.is("/upload/")) {
			return HTTP_NOT_IMPLEMENTED;
		} else {
			path += "/upload/"_len;
			auto storage = rctx.storage();
			if (storage && !path.empty()) {
				auto data = storage->getData(path.str());
				if (data) {
					storage->clearData(path.str());
				} else {
					return HTTP_NOT_FOUND;
				}

				auto scheme = rctx.server().getScheme(data.getString("scheme"));
				auto path = data.getString("path");
				auto resolve = data.getString("resolve");
				auto user = User::get(storage, data.getInteger("user"));
				if (scheme && !path.empty() && user) {
					if (Resource *r = Resource::resolve(storage, *scheme, path)) {
						r->setUser(user);
						if (!resolve.empty()) {
							r->setResolveOptions(data::Value(resolve));
						}
						if (r->prepareCreate()) {
							_resource = r;
							return DECLINED;
						}
					}
				}
			}
		}
		return HTTP_NOT_FOUND;
	}
	return DECLINED;
}

void ShellGui::onInsertFilter(Request &rctx) {
	if (!_resource) {
		return;
	}

	rctx.setRequiredData(InputConfig::Require::Files);
	rctx.setMaxRequestSize(_resource->getMaxRequestSize());
	rctx.setMaxVarSize(_resource->getMaxVarSize());
	rctx.setMaxFileSize(_resource->getMaxFileSize());

	if (rctx.getMethod() == Request::Put || rctx.getMethod() == Request::Post) {
		auto ex = InputFilter::insert(rctx);
		if (ex != InputFilter::Exception::None) {
			if (ex == InputFilter::Exception::TooLarge) {
				rctx.setStatus(HTTP_REQUEST_ENTITY_TOO_LARGE);
			} else if (ex == InputFilter::Exception::Unrecognized) {
				rctx.setStatus(HTTP_UNSUPPORTED_MEDIA_TYPE);
			}
		}
	}
}

int ShellGui::onHandler(Request &) {
	return OK;
}

void ShellGui ::onFilterComplete(InputFilter *filter) {
	Request rctx(filter->getRequest());
	data::Value data;
	if (_resource) {
		data.setValue(_resource->createObject(filter->getData(), filter->getFiles()), "result");
		data.setBool(true, "OK");
	} else {
		data.setBool(false, "OK");
	}

	data.setInteger(apr_time_now(), "date");
#if DEBUG
	auto &debug = _request.getDebugMessages();
	if (!debug.empty()) {
		data.setArray(debug, "debug");
	}
#endif
	auto &error = _request.getErrorMessages();
	if (!error.empty()) {
		data.setArray(error, "errors");
	}

	_request.writeData(data, false);
}

NS_SA_EXT_END(tools)