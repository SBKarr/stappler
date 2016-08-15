/*
 * ToolsAdminShell.cpp
 *
 *  Created on: 20 апр. 2016 г.
 *      Author: sbkarr
 */

#include "Define.h"
#include "Tools.h"
#include "User.h"
#include "Output.h"
#include "StorageScheme.h"
#include "Resource.h"

NS_SA_EXT_BEGIN(tools)

class SchellSocketHandler : public websocket::Handler, ReaderClassBase<CharReaderBase> {
public:
	enum class ShellMode {
		Plain,
		Html,
	};

	SchellSocketHandler(Manager *m, const Request &req, User *user) : Handler(m, req, 600_sec), _user(user) {
		sendBroadcast(data::Value{
			std::make_pair("user", data::Value(_user->getName())),
			std::make_pair("event", data::Value("enter")),
		});
	}

	bool onModeCommand(CharReaderBase &r) {
		if (!r.empty()) {
			if (r.is("plain")) {
				_mode = ShellMode::Plain;
				send("You are now in plain text mode");
			} else if (r.is("html")) {
				_mode = ShellMode::Html;
				send("You are now in <font color=\"#3f51b5\">HTML</font> mode");
			}
		} else {
			if (_mode == ShellMode::Plain) {
				send("Plain text mode");
			} else {
				send("<font color=\"#3f51b5\">HTML</font> mode");
			}
		}
		return true;
	}

	bool onDebugCommand(CharReaderBase &r) {
		if (!r.empty()) {
			if (r.is("on")) {
				messages::setDebugEnabled(true);
				send("Try to enable debug mode, wait a second, until all servers receive your message");
			} else if (r.is("off")) {
				messages::setDebugEnabled(false);
				send("Try to disable debug mode, wait a second, until all servers receive your message");
			}
		} else {
			if (messages::isDebugEnabled()) {
				send("Debug mode: On");
			} else {
				send("Debug mode: Off");
			}
		}
		return true;
	}

	bool onListCommand(CharReaderBase &r) {
		if (r.empty()) {
			data::Value ret;
			auto &schemes = _request.server().getSchemes();
			for (auto &it : schemes) {
				ret.addString(it.first);
			}
			sendData(ret);
		} else if (r == "all") {
			data::Value ret;
			auto &schemes = _request.server().getSchemes();
			for (auto &it : schemes) {
				auto &val = ret.emplace(it.first);
				auto &fields = it.second->getFields();
				for (auto &fit : fields) {
					val.setValue(fit.second.getTypeDesc(), fit.first);
				}
			}
			sendData(ret);
		} else {
			data::Value ret;
			auto cmd = r.readUntil<Group<GroupId::WhiteSpace>>();
			auto scheme =  _request.server().getScheme(cmd.str());
			if (scheme) {
				auto &fields = scheme->getFields();
				for (auto &fit : fields) {
					ret.setValue(fit.second.getTypeDesc(), fit.first);
				}
				sendData(ret);
			}
		}
		return true;
	}

	bool onGetCommand(CharReaderBase &r) {
		auto schemeName = r.readUntil<Group<GroupId::WhiteSpace>>();
		r.skipChars<Group<GroupId::WhiteSpace>>();
		auto path = r.readUntil<Group<GroupId::WhiteSpace>>();
		r.skipChars<Group<GroupId::WhiteSpace>>();
		auto resolve = r.readUntil<Group<GroupId::WhiteSpace>>();

		if (!schemeName.empty() && !path.empty()) {
			data::Value ret;
			auto scheme =  _request.server().getScheme(schemeName.str());
			if (scheme) {
				Resource *r = Resource::resolve(storage(), scheme, path.str());
				if (r) {
					r->setUser(_user);
					if (!resolve.empty()) {
						r->setResolveOptions(data::Value(resolve.str()));
					}

					auto data = r->getResultObject();
					sendData(data);
				} else {
					data::Value val;
					val.setString("Fail to get resource", "error");
					val.setString(path.str(), "path");
					val.setString(schemeName.str(), "scheme");
					sendData(val);

				}
			}

		}

		return true;
	}

	bool onCommand(CharReaderBase &r) {
		auto cmd = r.readUntil<Group<GroupId::WhiteSpace>>();
		r.skipChars<Group<GroupId::WhiteSpace>>();
		if (cmd == "echo") {
			if (!r.empty()) { send(r.str()); }

		} else if (cmd == "close") {
			send("Connection will be closed by your request");
			return false;

		} else if (cmd == "mode") {
			return onModeCommand(r);

		} else if (cmd == "debug") {
			return onDebugCommand(r);

		} else if (cmd == "list") {
			return onListCommand(r);

		} else if (cmd == "get") {
			return onGetCommand(r);

		} else if (cmd == "msg") {
			sendBroadcast(data::Value{
				std::make_pair("user", data::Value(_user->getName())),
				std::make_pair("event", data::Value("message")),
				std::make_pair("message", data::Value(r.str())),
			});
		} else if (cmd == "count") {
			apr::ostringstream resp;
			resp << "Users on socket: " << _manager->size();
			send(resp.weak());
		}
		return true;
	}

	virtual void onBegin() override {
		apr::ostringstream resp;
		resp << "Welcome. " << _user->getName() << "!";
		send(resp.weak());
	}

	// Data frame was recieved from network
	virtual bool onFrame(FrameType t, const Bytes &b) override {
		if (t == FrameType::Text) {
			CharReaderBase r((const char *)b.data(), b.size());
			return onCommand(r);
		}
		return true;
	}

	// Message was recieved from broadcast
	virtual bool onMessage(const data::Value &val) override {
		if (val.isBool("message")) {
			sendData(val);
		} else  if (val.isString("event")) {
			auto &ev = val.getString("event");
			if (ev == "enter") {
				apr::ostringstream resp;
				resp << "User " << val.getString("user") << " connected.";
				send(resp.weak());
			} else if (ev == "message") {
				apr::ostringstream resp;
				resp << "- [" << val.getString("user") << "] " << val.getString("message");
				send(resp.weak());
			}
		}

		return true;
	}

	void sendData(const data::Value & data) {
		apr::ostringstream stream;
		switch(_mode) {
		case ShellMode::Plain: data::write(stream, data, data::EncodeFormat::Json); break;
		case ShellMode::Html: output::formatJsonAsHtml(stream, data); break;
		};
		send(stream.weak());
	}

protected:
	ShellMode _mode = ShellMode::Plain;
	User *_user;
};

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
				return new SchellSocketHandler(this, req, user);
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
		rctx << R"ShellGui(<!DOCTYPE html>
<html><head>
	<title>Serenity Shell</title>
	<link rel="stylesheet" href="/__server/virtual/css/style.css" />
	<link rel="stylesheet" href="/__server/virtual/css/kawaiJson.css" />
	<script src="/__server/virtual/js/kawaiJson.js"></script>
	<script src="/__server/virtual/js/shellHistory.js"></script>
	<script type="text/javascript">
		var ws;
		var wsaddress = ((window.location.protocol == "https:") ? "wss:" : "ws:") + "//" + window.location.host + window.location.pathname
		if ((typeof(WebSocket) == 'undefined') && (typeof(MozWebSocket) != 'undefined')) {
			WebSocket = MozWebSocket;
		}
		function send(input) { if (ws) { ws.send(input.value); input.value = ""; } }
		function init() {
			document.getElementById("main").style.visibility = "hidden";
			ShellHistory(document.getElementById("input"), send);
		}

		function connect(form) {
			var nameEl = document.getElementById("ws_name");
			var passwdEl = document.getElementById("ws_passwd");

			var name = nameEl.value;
			var passwd = passwdEl.value;

			nameEl.value = "";
			passwdEl.value = "";

			ws = new WebSocket(wsaddress + "?name=" + name + "&passwd=" + passwd);
			ws.onopen = function(event) {
				document.getElementById("login").style.visibility = "hidden";
				document.getElementById("main").style.visibility = "visible";
				document.getElementById("connected").innerHTML = "Connected to WebSocket server";
				document.getElementById("output").innerHTML = "";
			};
			ws.onmessage = function(event) {
				var output = document.getElementById("output");
				if (event.data[0] == '{' || event.data[0] == '[') {
					var node = document.createElement("p");
					var json = JSON.parse(event.data);
					KawaiJson(node, json);
					output.insertBefore(node, output.firstChild);
				} else {
					output.innerHTML = "<p>" + event.data + "</p>" + output.innerHTML;
				}
			};
			ws.onerror = function(event) { console.log(event); };
			ws.onclose = function(event) {
				ws = null;
				console.log(event);
				document.getElementById("login").style.visibility = "visible";
				document.getElementById("main").style.visibility = "hidden";
          		document.getElementById("connected").innerHTML = "Connection Closed";
			}
			return false;
		}

	</script>
</head><body onload="init();">
	<h4>Serenity admin shell</h4>
	<form id="login" onsubmit="return connect(this);">
		<input id="ws_name" type="text" name="name" placeholder="User name" value="" size="24" />
		<input id="ws_passwd" type="password" name="passwd" placeholder="Password" value="" size="24" />
		<input type="submit" />
	</form>
	<div id="connected">Not Connected</div>
	<div id="main" style="visibility:hidden">
		Enter Message: <input id="input" type="text" name="message" value="" size="80"/><br/>
		Server says... <div id="output"></div>
	</div>
</body></html>
)ShellGui";
		rctx.setContentType("text/html;charset=UTF-8");
		return DONE;
	}
	return DECLINED;
}
NS_SA_EXT_END(tools)
