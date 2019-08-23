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

#ifndef LIBS_STAPPLER_FEATURES_STORAGE_SPSTORAGE_H
#define LIBS_STAPPLER_FEATURES_STORAGE_SPSTORAGE_H

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
Handle *create(const StringView &name, const StringView &filePath);

void finalize(Handle *);

Thread & thread(Handle * = nullptr);

using KeyDataCallback = Function<void(const StringView &key, data::Value && value)>;
using KeyCallback = Function<void(const StringView &key)>;

void get(const StringView &key, const data::DataCallback &callback, Handle * = nullptr);
void get(const StringView &key, const KeyDataCallback &callback, Handle * = nullptr);
void set(const StringView &key, const data::Value &value, const KeyDataCallback &callback = nullptr, Handle * = nullptr);
void set(const StringView &key, data::Value &&value, const KeyDataCallback &callback = nullptr, Handle * = nullptr);
void remove(const StringView &key, const KeyCallback &callback = nullptr, Handle * = nullptr);

// should be called only in storage thread
bool performWithTransaction(const Function<void()> &, Handle * = nullptr, bool unsafe = false);

NS_SP_EXT_END(storage)

#endif
