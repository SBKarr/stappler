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

#ifndef STELLATOR_SERVER_STSERVER_H_
#define STELLATOR_SERVER_STSERVER_H_

#include "STDefine.h"

namespace stellator {

class Server : public mem::AllocBase {
public:
	using HandlerCallback = mem::Function<RequestHandler *()>;

	struct Config;

	static void * merge(void *base, void *add);

	Server();
	Server(Config *);
	Server & operator =(Config *);

	Server(Server &&);
	Server & operator =(Server &&);

	Server(const Server &);
	Server & operator =(const Server &);

	void onServerInit();
	void onChildInit();
	void onHeartBeat(mem::pool_t *);
	void onBroadcast(const mem::Value &);
	void onBroadcast(const mem::BytesView &);

	void onStorageTransaction(db::Transaction &);

	void setForceHttps();
	void addProtectedLocation(const mem::StringView &);

	template <typename Component = ServerComponent>
	auto getComponent(const mem::StringView &) const -> Component *;

	template <typename Component>
	auto getComponent() const -> Component *;

	template <typename Component>
	auto addComponent(Component *) -> Component *;

	const mem::Map<mem::String, ServerComponent *> &getComponents() const;

	int onRequest(Request &);
	void addPreRequest(mem::Function<int(Request &)> &&);

	void addHandler(const mem::String &, const HandlerCallback &, const mem::Value & = mem::Value());
	void addHandler(std::initializer_list<mem::String>, const HandlerCallback &, const mem::Value & = mem::Value());
	void addHandler(const mem::String &, const HandlerMap *);
	void addHandler(std::initializer_list<mem::String>, const HandlerMap *);

	void addResourceHandler(const mem::String &, const db::Scheme &);
	void addResourceHandler(const mem::String &, const db::Scheme &, const mem::Value &);
	void addMultiResourceHandler(const mem::String &, std::initializer_list<stappler::Pair<const mem::String, const db::Scheme *>> &&);

	void addWebsocket(const mem::String &, websocket::Manager *);

	const db::Scheme * exportScheme(const db::Scheme &);

	const db::Scheme * getScheme(const mem::StringView &) const;
	const db::Scheme * getFileScheme() const;
	const db::Scheme * getUserScheme() const;
	const db::Scheme * getErrorScheme() const;

	const db::Scheme * defineUserScheme(std::initializer_list<db::Field> il);

	db::Scheme * getMutable(const db::Scheme *) const;

	mem::String getResourcePath(const db::Scheme &) const;

	struct ResourceScheme {
		mem::String path;
		mem::Value data;
	};

	struct RequestScheme {
		mem::String component;
		HandlerCallback callback;
		mem::Value data;
		const db::Scheme *scheme;
		const HandlerMap *map;
	};

	const mem::Map<mem::StringView, const db::Scheme *> &getSchemes() const;
	const mem::Map<const db::Scheme *, ResourceScheme> &getResources() const;
	const mem::Map<mem::String, RequestScheme> &getRequestHandlers() const;

	void reportError(const mem::Value &);

	bool performTask(Task *task, bool performFirst = false) const;
	bool scheduleTask(Task *task, mem::TimeInterval) const;

	void performWithStorage(const mem::Callback<void(const db::Transaction &)> &cb, bool openNewConnecton = false) const;

public: // httpd server info
	mem::StringView getDefaultName() const;
    mem::StringView getServerScheme() const;
    mem::StringView getServerAdmin() const;
    mem::StringView getServerHostname() const;
    mem::StringView getDocumentRoot() const;

    uint16_t getServerPort() const;

    mem::TimeInterval getTimeout() const;
    mem::TimeInterval getKeepAliveTimeout() const;

	int getMaxKeepAlives() const;
	bool isUsingKeepAlive() const;

	mem::StringView getServerPath() const;

    int getMaxRequestLineSize() const;
    int getMaxHeaderSize() const;
    int getMaxHeaders() const;

	operator bool () const { return _config != nullptr; }

	mem::StringView getSessionKey() const;
	mem::StringView getSessionName() const;
	mem::TimeInterval getSessionMaxAge() const;
	bool isSessionSecure() const;

	void *server() const { return (void *)_config; }
	void *getConfig() const { return (void *)_config; }
	mem::pool_t *getPool() const;

	stappler::pug::Cache *getPugCache() const;

	void setSessionKeys(mem::StringView pub, mem::StringView priv) const;
	mem::StringView getSessionPublicKey() const;
	mem::StringView getSessionPrivateKey() const;
	mem::BytesView getServerSecret() const;

	Server next() const;

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

	void addComponentWithName(const mem::StringView &, ServerComponent *);

	ServerComponent *getServerComponent(const mem::StringView &name) const;
	ServerComponent *getServerComponent(std::type_index name) const;

	void runErrorReportTask(const Request &, const mem::Vector<mem::Value> &);

	Config *_config = nullptr;
};

template <typename Component>
inline auto Server::getComponent(const mem::StringView &name) const -> Component * {
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

}

#endif /* STELLATOR_SERVER_STSERVER_H_ */
