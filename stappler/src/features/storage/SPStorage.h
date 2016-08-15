
#ifndef stappler_storage_SPStorage
#define stappler_storage_SPStorage

#include "SPDefine.h"

/**
 * storage module
 *
 * Storage is asynchronous database concept, back-ended by sqlite
 * all operations on storage (by key-value interface of Schemes) are
 * conditionally-asynchronous
 *
 * "Conditionally" means, that on base storage thread (named "SqlStorageThread")
 * all operations will be performed immediately. Callbacks will be called directly
 * from operation function call.
 *
 * On all other threads all actions will be scheduled on storage thread, callbacks
 * will be called on main thread after operations are completed.
 *
 * This concept allow you to extend Storage functionality, write your own logic,
 * backended by this system, that will work on storage thread without environment
 * overhead.
 */

NS_SP_EXT_BEGIN(storage)

Handle *defaultStorage();
Handle *create(const std::string &name, const std::string &filePath);

Thread & thread(Handle * = nullptr);

typedef std::function<void(const std::string &key, data::Value &value)> KeyDataCallback;
typedef std::function<void(const std::string &key)> KeyCallback;

void get(const std::string &key, KeyDataCallback callback, Handle * = nullptr);
void set(const std::string &key, const data::Value &value, KeyDataCallback callback = nullptr, Handle * = nullptr);
void set(const std::string &key, data::Value &&value, KeyDataCallback callback = nullptr, Handle * = nullptr);
void remove(const std::string &key, KeyCallback callback = nullptr, Handle * = nullptr);

NS_SP_EXT_END(storage)

#endif
