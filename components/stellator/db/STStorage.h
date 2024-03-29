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

#ifndef STELLATOR_DB_STSTORAGE_H_
#define STELLATOR_DB_STSTORAGE_H_

#include "SPCommon.h"
#include "SPData.h"
#include "SPMemUserData.h"
#include "SPMemory.h"
#include "STStorageConfig.h"
#include <bitset>

NS_DB_BEGIN

using nullptr_t = std::nullptr_t;

namespace mem {

using namespace stappler::mem_pool;

};

class StorageRoot;
class InputFile;

class Adapter;
class Transaction;
class Worker;

class Query;
class Interface;
class Binder;
class QueryInterface;
class ResultCursor;

class Scheme;
class Field;
class Object;
class User;

struct FieldText;
struct FieldPassword;
struct FieldExtra;
struct FieldFile;
struct FieldImage;
struct FieldObject;
struct FieldArray;
struct FieldView;
struct FieldFullTextView;
struct FieldCustom;

namespace internals {

struct RequestData {
	bool exists = false;
	mem::StringView address;
	mem::StringView hostname;
	mem::StringView uri;

	operator bool() { return exists; }
};

Adapter getAdapterFromContext();

void scheduleAyncDbTask(const stappler::Callback<mem::Function<void(const Transaction &)>(stappler::memory::pool_t *)> &setupCb);

bool isAdministrative();

mem::String getDocuemntRoot();

const Scheme *getFileScheme();
const Scheme *getUserScheme();

InputFile *getFileFromContext(int64_t);

RequestData getRequestData();
int64_t getUserIdFromContext();

}

namespace messages {

/* Serenity cross-server messaging interface
 *
 * error(Tag, Name, Data = nullptr)
 *  - send information about error to current output interface,
 *  if debug interface is active, it will receive this message
 *
 * debug(Tag, Name, Data = nullptr)
 *  - send debug information to current output interface and
 *  connected debug interface
 *
 * broadcast(Message)
 *  - send cross-server broadcast with message
 *
 *  Broadcast format:
 *  (message) ->  { "message": true, "level": "debug"/"error", "data": Data }
 *   - used for system errors, warnings and debug, received by admin shell
 *
 *  (websocket) -> { "server": "SERVER_NAME", "url": "WEBSOCKET_URL", "data": Data }
 *   - used for communication between websockets, received by all websockets with the same server id and url
 *
 *  (option) -> { "system": true, "option": "OPTION_NAME", "OPTION_NAME": NewValue }
 *   - used for system-wide option switch, received only by system
 *
 *   NOTE: All broadcast messages coded as CBOR, so, BYTESTRING type is safe
 */

bool isDebugEnabled();
void setDebugEnabled(bool);

void _addErrorMessage(mem::Value &&);
void _addDebugMessage(mem::Value &&);

void broadcast(const mem::Value &);
void broadcast(const mem::Bytes &);

template <typename Source, typename Text>
void _addError(Source &&source, Text &&text) {
	_addErrorMessage(mem::Value{
		std::make_pair("source", mem::Value(std::forward<Source>(source))),
		std::make_pair("text", mem::Value(std::forward<Text>(text)))
	});
}

template <typename Source, typename Text>
void _addError(Source &&source, Text &&text, mem::Value &&d) {
	_addErrorMessage(mem::Value{
		std::make_pair("source", mem::Value(std::forward<Source>(source))),
		std::make_pair("text", mem::Value(std::forward<Text>(text))),
		std::make_pair("data", std::move(d))
	});
}

template <typename Source, typename Text>
void _addDebug(Source &&source, Text &&text) {
	_addDebugMessage(mem::Value{
		std::make_pair("source", mem::Value(std::forward<Source>(source))),
		std::make_pair("text", mem::Value(std::forward<Text>(text)))
	});
}

template <typename Source, typename Text>
void _addDebug(Source &&source, Text &&text, mem::Value &&d) {
	_addDebugMessage(mem::Value{
		std::make_pair("source", mem::Value(std::forward<Source>(source))),
		std::make_pair("text", mem::Value(std::forward<Text>(text))),
		std::make_pair("data", std::move(d))
	});
}

template <typename Source, typename Text>
void _addLocal(Source &&source, Text &&text) {
	broadcast(mem::Value{
		std::make_pair("local", mem::Value(true)),
		std::make_pair("message", mem::Value(true)),
		std::make_pair("data", mem::Value({
			std::make_pair("source", mem::Value(std::forward<Source>(source))),
			std::make_pair("text", mem::Value(std::forward<Text>(text)))
		})),
	});
}

template <typename Source, typename Text>
void _addLocal(Source &&source, Text &&text, mem::Value &&d) {
	broadcast(mem::Value{
		std::make_pair("local", mem::Value(true)),
		std::make_pair("message", mem::Value(true)),
		std::make_pair("data", mem::Value({
			std::make_pair("source", mem::Value(std::forward<Source>(source))),
			std::make_pair("text", mem::Value(std::forward<Text>(text))),
			std::make_pair("data", std::move(d))
		})),
	});
}

template <typename ... Args>
void error(Args && ...args) {
	_addError(std::forward<Args>(args)...);
}

template <typename ... Args>
void debug(Args && ...args) {
	_addDebug(std::forward<Args>(args)...);
}

template <typename ... Args>
void local(Args && ...args) {
	_addLocal(std::forward<Args>(args)...);
}

}

// Default StorageRoot class is partially operational, requires customization for real systems
class StorageRoot {
public:
	virtual ~StorageRoot() { }

	virtual bool isDebugEnabled() const;
	virtual void setDebugEnabled(bool v);

	virtual void addErrorMessage(mem::Value &&data) const;
	virtual void addDebugMessage(mem::Value &&data) const;

	virtual void broadcast(const mem::Value &val);
	virtual void broadcast(const mem::Bytes &val);

	virtual Transaction acquireTransaction(const Adapter &adapter);
	virtual Adapter getAdapterFromContext();

	// Argument is a callback (function, that should not be stored), that return
	// 	 function (that can and will be stored), allocated from specified memory pool;
	// returned function should be called within async thread with usable db::Transaction to do its work
	virtual void scheduleAyncDbTask(
			const mem::Callback<mem::Function<void(const Transaction &)>(mem::pool_t *)> &setupCb);

	virtual bool isAdministrative() const;
	virtual mem::String getDocuemntRoot() const;
	virtual const Scheme *getFileScheme() const;
	virtual const Scheme *getUserScheme() const;
	virtual InputFile *getFileFromContext(int64_t) const;
	virtual internals::RequestData getRequestData() const;
	virtual int64_t getUserIdFromContext() const;

protected:
	virtual void onLocalBroadcast(const mem::Value &) { }
	virtual void onStorageTransaction(Transaction &) { }

	mutable std::mutex _debugMutex;
	std::atomic<bool> _debug = false;
};


struct InputConfig {
	enum class Require {
		None = 0,
		Data = 1,
		Files = 2,
		Body = 4,
		FilesAsData = 8,
	};

	InputConfig() = default;
	InputConfig(const InputConfig &) = default;
	InputConfig & operator=(const InputConfig &) = default;

	InputConfig(InputConfig &&) = default;
	InputConfig & operator=(InputConfig &&) = default;

	void updateLimits(const mem::Map<mem::String, Field> &vec);

	Require required = Require::None;
	size_t maxRequestSize = config::getMaxRequestSize();
	size_t maxVarSize = config::getMaxVarSize();
	size_t maxFileSize = config::getMaxFileSize();

	mem::TimeInterval updateTime = config::getInputUpdateTime();
	float updateFrequency = config::getInputUpdateFrequency();
};

SP_DEFINE_ENUM_AS_MASK(InputConfig::Require);

NS_DB_END

#endif /* STELLATOR_DB_STSTORAGE_H_ */
