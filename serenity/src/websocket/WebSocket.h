/*
 * WebSocket.h
 *
 *  Created on: 17 апр. 2016 г.
 *      Author: sbkarr
 */

#ifndef SERENITY_SRC_WEBSOCKET_WEBSOCKET_H_
#define SERENITY_SRC_WEBSOCKET_WEBSOCKET_H_

#include "Request.h"
#include "Connection.h"
#include "SPBuffer.h"

NS_SA_EXT_BEGIN(websocket)

class Manager : public AllocPool {
public:
	using Handler = websocket::Handler;

	Manager();
	virtual ~Manager();

	virtual Handler * onAccept(const Request &);
	virtual bool onBroadcast(const data::Value &);

	size_t size() const;

	void receiveBroadcast(const data::Value &);
	int accept(Request &);

	void addHandler(Handler *);
	void removeHandler(Handler *);

protected:
	apr_thread_mutex_t *_mutex = nullptr;
	std::atomic<size_t> _count;
	Vector<Handler *> _handlers;
	apr_pool_t *_pool;
};

class Handler : public AllocPool {
public:
	using Manager = websocket::Manager;

	enum class FrameType : uint8_t {
		None,

		// User
		Text,
		Binary,

		// System
		Continue,
		Close,
		Ping,
		Pong,
	};

	enum class StatusCode : uint16_t {
		None = 0,
		Auto = 1,
		Ok = 1000,
		Away = 1001,
		ProtocolError = 1002,
		NotAcceptable = 1003,
		ExpectStatus = 1005,
		AbnormalClose = 1006,
		NotConsistent = 1007,
		PolicyViolated = 1008,
		TooLarge = 1009,
		NotNegotiated = 1010,
		UnexceptedCondition = 1011,
		SSLError = 1015,
	};

	Handler(Manager *m, const Request &,
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
	// This socket alse receive this broadcast
	void sendBroadcast(data::Value &&) const;

	void setStatusCode(StatusCode, const String & = String());
	void setEncodeFormat(const data::EncodeFormat &);

	bool send(const String &);
	bool send(const Bytes &);
	bool send(const data::Value &);

	// get default storage adapter, that binds to current call context
	storage::Adapter *storage() const;

protected:
	friend class websocket::Manager;

	void run();
	void receiveBroadcast(const data::Value &);

	bool onControlFrame(FrameType, const StackBuffer<128> &);

	bool processBroadcasts();
	bool processSocket(const apr_pollfd_t *fd);

	bool readSocket(const apr_pollfd_t *fd);
	bool writeSocket(const apr_pollfd_t *fd);

	// try to write into non-blocking socket
	bool writeToSocket(apr_bucket_brigade *pool, const uint8_t *bytes, size_t &count);
	bool trySend(FrameType, const uint8_t *bytes, size_t count);

	void cancel();
	StatusCode resolveStatus(StatusCode code);

	void pushNotificator(apr_pool_t *);

	struct Frame {
		bool fin; // fin value inside current frame
		FrameType type; // opcode from first frame
		Bytes buffer; // common data buffer
		size_t block; // size of completely written block when segmented
		size_t offset; // offset inside current frame
	};

	struct FrameReader {
		bool fin;
		bool masked;

		enum class Status : uint8_t {
			Head,
			Size16,
			Size64,
			Mask,
			Body,
			Control
		} status;

		enum class Error : uint8_t {
			None,
			NotInitialized, // error in reader initialization
			ExtraIsNotEmpty, // rsv 1-3 is not empty
			NotMasked, // input frame is not masked
			UnknownOpcode, // unknown opcode in frame
			InvalidSegment, // invalid FIN or OPCODE sequence in segmented frames
			InvalidSize, // frame (or sequence) is larger then max size
			InvalidAction, // Handler tries to perform invalid reading action
		} error;

		FrameType type;
		uint8_t extra;
		uint32_t mask;
		size_t size;
		size_t max; // absolute maximum (even for segmented frames)

		Frame frame;
		apr_pool_t *pool;
		StackBuffer<128> buffer;
		struct apr_bucket_alloc_t *bucket_alloc;
		apr_bucket_brigade *tmpbb;

		FrameReader(const Request &, apr_pool_t *p, size_t maxFrameSize);

		operator bool() const;

		size_t getRequiredBytes() const;
		uint8_t * prepare(size_t &len);
		bool save(uint8_t *, size_t nbytes);

		bool isFrameReady() const;
		bool isControlReady() const;
		void popFrame();

		bool updateState();
	};

	struct WriteSlot {
		apr_bucket_brigade *bb;
		apr_pool_t *pool;
		size_t size;
		size_t item;
		size_t offset;
		Vector<Bytes> storage;

		WriteSlot(apr_pool_t *p) : bb(nullptr), pool(p), size(0), item(0), offset(0), storage(p) { }
	};

	struct FrameWriter {
		apr_pool_t * pool;
		Vector<WriteSlot> frames;
		apr_bucket_brigade *tmpbb;

		FrameWriter(const Request &, apr_pool_t *);

		bool empty() const; // is write queue empty?
		size_t size() const; // size of write queue

		// if write queue is filled with many frames, you should not try to add another
		// may be, it's a user with slow connection
		//
		// instead, you can skip buffered frames in write queue, then add your new frame
		// or just wait for some time, when all frames will be sent to user

		size_t getReadyLength();
		uint8_t * getReadyBytes();
		apr_bucket_brigade *getReadyBrigade(const Request &);
		void drop(uint8_t *, size_t nbytes);

		// drop all frames after the one currently processed
		void skipBuffered();

		bool addFrame(FrameType, const uint8_t *buf, size_t len);
		bool addFrameOffset(const uint8_t *head, size_t nhead, const uint8_t *buf, size_t nbuf, size_t offset);

		// adds control frame immediately after current frame
		bool addControlFrame(FrameType, const String & = String());

		uint8_t *emplaceFrame(size_t, bool first = false, size_t offset = 0);
	};

	data::EncodeFormat _format = data::EncodeFormat::Json;

	apr_pollfd_t _pollfd;
	apr_pollset_t *_poll = nullptr;
	apr_socket_t *_socket = nullptr;
	Request _request;
	Connection _connection;
	Manager *_manager = nullptr;
	TimeInterval _ttl;
	bool _ended = false;
	bool _valid = false;
	bool _fdchanged = false;
	bool _ssl = false;

	FrameReader _reader;
	FrameWriter _writer;

	String _serverReason;
	StatusCode _clientCloseCode;
	StatusCode _serverCloseCode;

	apr_thread_mutex_t *_broadcastMutex = nullptr;
	apr_pool_t *_broadcastsPool = nullptr;
	Vector<data::Value> *_broadcastsMessages = nullptr;
};

NS_SA_EXT_END(websocket)

#endif /* SERENITY_SRC_WEBSOCKET_WEBSOCKET_H_ */
