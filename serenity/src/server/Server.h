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

#ifndef SACONTEXT_H
#define	SACONTEXT_H

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
	void onHeartBeat();
	void onBroadcast(const data::Value &);
	void onBroadcast(const Bytes &);
	int onRequest(Request &);

	void setHandlerFile(const String &file);
	void setSourceRoot(const String &file);
	void addHanderSource(const String &w);
	void setSessionParams(const String &w);
	void setWebHookParams(const String &w);
	void setForceHttps();
	void setProtectedList(const StringView &w);

	void addProtectedLocation(const StringView &);

	const String &getHandlerFile() const;
	const String &getNamespace() const;

	template <typename Component = ServerComponent>
	auto getComponent(const StringView &) const -> Component *;
	void addComponent(const String &, ServerComponent *);

	const Map<String, ServerComponent *> &getComponents() const;

	void addPreRequest(Function<int(Request &)> &&);

	void addHandler(const String &, const HandlerCallback &, const data::Value & = data::Value::Null);
	void addHandler(std::initializer_list<String>, const HandlerCallback &, const data::Value & = data::Value::Null);

	void addResourceHandler(const String &, const storage::Scheme &,
			const data::TransformMap * = nullptr, const AccessControl * = nullptr, size_t priority = 0);

	void addResourceHandler(const String &, const storage::Scheme &, const data::Value &,
			const data::TransformMap * = nullptr, const AccessControl * = nullptr, size_t priority = 0);

	void addMultiResourceHandler(const String &, std::initializer_list<Pair<const String, const storage::Scheme *>> &&,
			const data::TransformMap * = nullptr, const AccessControl * = nullptr);

	void addWebsocket(const String &, websocket::Manager *);

	const storage::Scheme * exportScheme(const storage::Scheme &);

	const storage::Scheme * getScheme(const StringView &) const;
	const storage::Scheme * getFileScheme() const;
	const storage::Scheme * getUserScheme() const;

	const storage::Scheme * defineUserScheme(std::initializer_list<storage::Field> il);

	String getResourcePath(const storage::Scheme &) const;

	struct ResourceScheme {
		size_t priority;
		String path;
		data::Value data;
	};

	struct RequestScheme {
		String component;
		HandlerCallback callback;
		data::Value data;
		const storage::Scheme *scheme;
	};

	const Map<String, const storage::Scheme *> &getSchemes() const;
	const Map<const storage::Scheme *, ResourceScheme> &getResources() const;
	const Map<String, RequestScheme> &getRequestHandlers() const;

	void reportError(const data::Value &);

	bool performTask(Task *task, bool performFirst = false) const;
	bool scheduleTask(Task *task, TimeInterval) const;

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

	Server next() const { return Server(_server->next); }

	const String &getSessionKey() const;
	const String &getSessionName() const;
	apr_time_t getSessionMaxAge() const;
	bool isSessionSecure() const;

	void *getConfig() const { return (void *)_config; }

	tpl::Cache *getTemplateCache() const;

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
	ServerComponent *getServerComponent(const StringView &name) const;

	struct Config;

	server_rec *_server = nullptr;
	Config *_config = nullptr;
};

template <typename Component>
inline auto Server::getComponent(const StringView &name) const -> Component * {
	return dynamic_cast<Component *>(getServerComponent(name));
}

NS_SA_END

#endif	/* SACONTEXT_H */

