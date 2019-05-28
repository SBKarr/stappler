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

#ifndef COMMON_DB_SPDBSTORAGE_H_
#define COMMON_DB_SPDBSTORAGE_H_

#include "SPCommon.h"
#include "SPData.h"
#include "SPMemUserData.h"
#include "SPDBStorageConfig.h"

#include <bitset>

NS_DB_BEGIN

namespace mem {

using StringView = stappler::StringView;
using Interface = stappler::memory::PoolInterface;

using AllocBase = stappler::memory::AllocPool;

using String = stappler::memory::string;
using WideString = stappler::memory::u16string;
using Bytes = stappler::memory::vector<uint8_t>;

using StringStream = stappler::memory::ostringstream;
using OutputStream = std::ostream;

template <typename T>
using Vector = stappler::memory::vector<T>;

template <typename K, typename V, typename Compare = std::less<void>>
using Map = stappler::memory::map<K, V, Compare>;

template <typename T, typename Compare = std::less<void>>
using Set = stappler::memory::set<T, Compare>;

template <typename T>
using Function = stappler::memory::function<T>;

using Value = stappler::data::ValueTemplate<stappler::memory::PoolInterface>;
using Array = Value::ArrayType;
using Dictionary = Value::DictionaryType;
using EncodeFormat = stappler::data::EncodeFormat;

inline auto writeData(const Value &data, EncodeFormat fmt = EncodeFormat()) -> Bytes {
	return stappler::data::EncodeTraits<stappler::memory::PoolInterface>::write(data, fmt);
}

inline bool writeData(std::ostream &stream, const Value &data, EncodeFormat fmt = EncodeFormat()) {
	return stappler::data::EncodeTraits<stappler::memory::PoolInterface>::write(stream, data, fmt);
}

template <typename T>
inline mem::String toString(const T &t) {
	return stappler::string::ToStringTraits<Interface>::toString(t);
}

template <typename T>
inline void toStringStream(mem::StringStream &stream, T value) {
	stream << value;
}

template <typename T, typename... Args>
inline void toStringStream(mem::StringStream &stream, T value, Args && ... args) {
	stream << value;
	mem::toStringStream(stream, std::forward<Args>(args)...);
}

template <typename T, typename... Args>
inline mem::String toString(T t, Args && ... args) {
	mem::StringStream stream;
	mem::toStringStream(stream, t);
	mem::toStringStream(stream, std::forward<Args>(args)...);
    return stream.str();
}

};

class InputFile;

class Adapter;
class Transaction;
class Worker;

class Query;
class Interface;
class Binder;
class QueryInterface;
class ResultInterface;

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

template <typename ... Args>
void error(Args && ...args) {
	_addError(std::forward<Args>(args)...);
}

template <typename ... Args>
void debug(Args && ...args) {
	_addDebug(std::forward<Args>(args)...);
}

}

NS_DB_END

#endif /* COMMON_DB_SPDBSTORAGE_H_ */
