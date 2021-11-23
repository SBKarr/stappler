/**
Copyright (c) 2016-2021 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef SERENITY_SRC_WEBSOCKET_WEBSOCKET_H_
#define SERENITY_SRC_WEBSOCKET_WEBSOCKET_H_

#include "Request.h"
#include "Connection.h"
#include "SPBuffer.h"
#include "WebSocketConnection.h"

NS_SA_EXT_BEGIN(websocket)

constexpr auto WEBSOCKET_FILTER = "Serenity::WebsocketFilter";
constexpr auto WEBSOCKET_FILTER_IN = "Serenity::WebsocketConnectionInput";
constexpr auto WEBSOCKET_FILTER_OUT = "Serenity::WebsocketConnectionOutput";

class Manager : public AllocPool {
public:
	using Handler = websocket::Handler;

	static apr_status_t filterFunc(ap_filter_t *f, apr_bucket_brigade *bb);
	static int filterInit(ap_filter_t *f);
	static void filterRegister();

	Manager(Server);
	virtual ~Manager();

	virtual Handler * onAccept(const Request &, mem::pool_t *);
	virtual bool onBroadcast(const data::Value &);

	size_t size() const;

	void receiveBroadcast(const data::Value &);
	int accept(Request &);

	void run(Handler *);

	const Server &server() const { return _server; }

protected:
	void addHandler(Handler *);
	void removeHandler(Handler *);

	apr_pool_t *_pool;
	apr::mutex _mutex;
	std::atomic<size_t> _count;
	Vector<Handler *> _handlers;
	Server _server;
};

class Handler : public AllocPool {
public:
	using Manager = websocket::Manager;
	using FrameType = websocket::FrameType;

	Handler(Manager *m, mem::pool_t *p, StringView url,
			TimeInterval ttl = config::getDefaultWebsocketTtl(),
			size_t max = config::getDefaultWebsocketMax());
	virtual ~Handler();

	// Client just connected
	virtual void onBegin() { }

	// Data frame was recieved from network
	virtual bool onFrame(FrameType, const Bytes &);

	// Message was recieved from broadcast
	virtual bool onMessage(const data::Value &);

	// Client is about disconnected
	// You can not send any frames in this call, because 'close' frame was already sent
	virtual void onEnd() { }

	// Send system-wide broadcast, that can be received by any other websocket with same servername and url
	// This socket also receive this broadcast
	void sendBroadcast(data::Value &&) const;

	void setEncodeFormat(const data::EncodeFormat &);

	bool send(StringView);
	bool send(BytesView);
	bool send(const data::Value &);

	// get default storage adapter, that binds to current call context
	Manager *manager() const { return _manager; }
	Connection *connection() const { return _conn; }
	mem::pool_t *pool() const;

	StringView getUrl() const { return _url; }
	TimeInterval getTtl() const { return _ttl; }
	size_t getMaxInputFrameSize() const { return _maxInputFrameSize; }

	bool isEnabled() const { return _conn && _conn->isEnabled(); }

	void sendPendingNotifications(apr_pool_t *);

	void performWithStorage(const Callback<void(const db::Transaction &)> &cb) const;

	bool performAsync(const Callback<void(Task &)> &cb) const;

protected:
	friend class websocket::Manager;
	friend class websocket::Connection;

	void setConnection(Connection *);
	virtual void receiveBroadcast(const data::Value &);
	bool processBroadcasts();

	mem::pool_t *_pool = nullptr;
	Manager *_manager = nullptr;

	data::EncodeFormat _format = data::EncodeFormat::Json;
	StringView _url;
	TimeInterval _ttl;
	size_t _maxInputFrameSize = config::getDefaultWebsocketMax();

	apr::mutex _broadcastMutex;
	apr_pool_t *_broadcastsPool = nullptr;
	Vector<data::Value> *_broadcastsMessages = nullptr;

	Connection *_conn = nullptr;
};

NS_SA_EXT_END(websocket)

#endif /* SERENITY_SRC_WEBSOCKET_WEBSOCKET_H_ */
