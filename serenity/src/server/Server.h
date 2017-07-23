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

	void setHandlerFile(const apr::string &file);
	void setSourceRoot(const apr::string &file);
	void addHanderSource(const apr::string &w);
	void setSessionParams(const apr::string &w);
	const apr::string &getHandlerFile() const;
	const apr::string &getNamespace() const;

	ServerComponent *getComponent(const String &) const;
	void addComponent(const String &, ServerComponent *);

	void addPreRequest(Function<int(Request &)> &&);

	void addHandler(const String &, const HandlerCallback &, const data::Value & = data::Value::Null);
	void addHandler(std::initializer_list<String>, const HandlerCallback &, const data::Value & = data::Value::Null);

	void addResourceHandler(const String &, storage::Scheme *, data::TransformMap * = nullptr, AccessControl * = nullptr, size_t priority = 0);
	void addResourceHandler(const String &, storage::Scheme *, const data::Value &, data::TransformMap * = nullptr, AccessControl * = nullptr, size_t priority = 0);

	void addWebsocket(const String &, websocket::Manager *);

	storage::Scheme * exportScheme(storage::Scheme *);

	storage::Scheme * getScheme(const String &) const;
	storage::Scheme * getFileScheme() const;
	storage::Scheme * getUserScheme() const;

	String getResourcePath(storage::Scheme *) const;

	struct ResourceScheme {
		String path;
		size_t priority;
		data::Value data;
	};

	const Map<String, storage::Scheme *> &getSchemes() const;
	const Map<storage::Scheme *, ResourceScheme> &getResources() const;
	const Map<String, std::pair<HandlerCallback, data::Value>> &getRequestHandlers() const;

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

	const apr::string &getSessionKey() const;
	const apr::string &getSessionName() const;
	apr_time_t getSessionMaxAge() const;
	bool isSessionSecure() const;

	void *getConfig() const { return (void *)_config; }

	tpl::Cache *getTemplateCache() const;
protected:
	struct Config;

#ifndef NOCB
	friend class couchbase::Handle;
	couchbase::Connection *acquireCouchbase();
	void releaseCouchbase(couchbase::Connection *cb);
#endif

	server_rec *_server = nullptr;
	Config *_config = nullptr;
};

NS_SA_END

#endif	/* SACONTEXT_H */

