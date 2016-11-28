/*
   Copyright 2013 Roman "SBKarr" Katuntsev, LLC St.Appler

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

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

