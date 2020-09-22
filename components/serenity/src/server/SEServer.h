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

#ifndef SERENITY_SRC_SERVER_SESERVER_H_
#define SERENITY_SRC_SERVER_SESERVER_H_

#include "Define.h"

NS_SA_BEGIN

class Server : public AllocPool {
public:
	using HandlerCallback = Function<RequestHandler *()>;

	static void * merge(void *base, void *add);

	Server();
	Server(server_rec *);
	Server & operator =(server_rec *);

	Server(Server &&);
	Server & operator =(Server &&);

	Server(const Server &);
	Server & operator =(const Server &);

	void onChildInit();
	void initHeartBeat(apr_pool_t *, int);
	void onHeartBeat(apr_pool_t *);
	void checkBroadcasts();
	void onBroadcast(const data::Value &);
	void onBroadcast(const BytesView &);
	int onRequest(Request &);

	void onStorageTransaction(storage::Transaction &);

	void setSourceRoot(StringView file);
	void addHanderSource(StringView w);
	void addAllow(StringView);
	void setSessionParams(StringView w);
	void setServerKey(StringView w);
	void setWebHookParams(StringView w);
	void setForceHttps();
	void setProtectedList(StringView w);

	void addProtectedLocation(const StringView &);

	template <typename Component = ServerComponent>
	auto getComponent(const StringView &) const -> Component *;

	template <typename Component>
	auto getComponent() const -> Component *;

	template <typename Component>
	auto addComponent(Component *) -> Component *;

	const Map<String, ServerComponent *> &getComponents() const;

	void addPreRequest(Function<int(Request &)> &&);

	void addHandler(const String &, const HandlerCallback &, const data::Value & = data::Value::Null);
	void addHandler(std::initializer_list<String>, const HandlerCallback &, const data::Value & = data::Value::Null);
	void addHandler(const String &, const HandlerMap *);
	void addHandler(std::initializer_list<String>, const HandlerMap *);

	void addResourceHandler(const String &, const storage::Scheme &);
	void addResourceHandler(const String &, const storage::Scheme &, const data::Value &val);
	void addMultiResourceHandler(const String &, std::initializer_list<Pair<const String, const storage::Scheme *>> &&);

	void addWebsocket(const String &, websocket::Manager *);

	const storage::Scheme * exportScheme(const storage::Scheme &);

	const storage::Scheme * getScheme(const StringView &) const;
	const storage::Scheme * getFileScheme() const;
	const storage::Scheme * getUserScheme() const;
	const storage::Scheme * getErrorScheme() const;

	const storage::Scheme * defineUserScheme(std::initializer_list<storage::Field> il);

	db::Scheme * getMutable(const db::Scheme *) const;

	String getResourcePath(const storage::Scheme &) const;

	struct ResourceScheme {
		String path;
		data::Value data;
	};

	struct RequestScheme {
		String component;
		HandlerCallback callback;
		data::Value data;
		const storage::Scheme *scheme = nullptr;
		const HandlerMap *map = nullptr;
	};

	const Map<String, const storage::Scheme *> &getSchemes() const;
	const Map<const storage::Scheme *, ResourceScheme> &getResources() const;
	const Map<String, RequestScheme> &getRequestHandlers() const;

	void reportError(const data::Value &);

	bool performTask(Task *task, bool performFirst = false) const;
	bool scheduleTask(Task *task, TimeInterval) const;

	void performWithStorage(const Callback<void(db::Transaction &)> &cb) const;

public: // httpd server info
	apr::weak_string getDefaultName() const;

	bool isVirtual() const;

    apr_port_t getServerPort() const;
    apr::weak_string getServerScheme() const;
    apr::weak_string getServerAdmin() const;
    apr::weak_string getServerHostname() const;
    apr::weak_string getDocumentRoot() const;

    apr_interval_time_t getTimeout() const;
	apr_interval_time_t getKeepAliveTimeout() const;

	int getMaxKeepAlives() const;
	bool isUsingKeepAlive() const;

	apr::weak_string getServerPath() const;

    int getMaxRequestLineSize() const;
    int getMaxHeaderSize() const;
    int getMaxHeaders() const;

    server_rec *server() const { return _server; }
	operator server_rec *() const { return _server; }
	operator bool () const { return _server != nullptr; }

	mem::pool_t *getPool() const { return _server->process->pconf; }

	Server next() const { return Server(_server->next); }

	const String &getSessionKey() const;
	const String &getSessionName() const;
	apr_time_t getSessionMaxAge() const;
	bool isSessionSecure() const;

	void *getConfig() const { return (void *)_config; }

	tpl::Cache *getTemplateCache() const;
	pug::Cache *getPugCache() const;

	void setSessionKeys(StringView pub, StringView priv) const;
	StringView getSessionPublicKey() const;
	StringView getSessionPrivateKey() const;
	BytesView getServerSecret() const;

	bool isSecureAuthAllowed(const Request &rctx) const;

	struct Config;

public: // compression
	enum EtagMode {
		AddSuffix,
		NoChange,
		Remove
	};

	// brotli compression configuration
	// based on mod_brotli defaults
	struct CompressionConfig {
		bool enabled = true;
	    int quality = 5;
	    int lgwin = 18;
	    int lgblock = 0;
	    EtagMode etag_mode = EtagMode::NoChange;
	    const char *note_input_name = nullptr;
	    const char *note_output_name = nullptr;
	    const char *note_ratio_name = nullptr;
	};

	CompressionConfig *getCompressionConfig() const;

protected:
	void processReports();

	void addComponentWithName(const StringView &, ServerComponent *);

	ServerComponent *getServerComponent(const StringView &name) const;
	ServerComponent *getServerComponent(std::type_index name) const;

	void runErrorReportTask(request_rec *, const Vector<data::Value> &);

	server_rec *_server = nullptr;
	Config *_config = nullptr;
};

template <typename Component>
inline auto Server::getComponent(const StringView &name) const -> Component * {
	return dynamic_cast<Component *>(getServerComponent(name));
}

template <typename Component>
inline auto Server::getComponent() const -> Component * {
	if (auto c = getServerComponent(std::type_index(typeid(Component)))) {
		return static_cast<Component *>(c);
	}

	auto &cmp = getComponents();
	for (auto &it : cmp) {
		if (auto c = dynamic_cast<Component *>(it.second)) {
			return c;
		}
	}

	return nullptr;
}

template <typename Component>
auto Server::addComponent(Component *c) -> Component * {
	addComponentWithName(c->getName(), c);
	return c;
}

NS_SA_END

#endif	/* SERENITY_SRC_SERVER_SESERVER_H_ */

