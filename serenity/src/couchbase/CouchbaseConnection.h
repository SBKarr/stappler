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

#ifndef SERENITY_SRC_COUCHBASE_COUCHBASECONNECTION_H_
#define SERENITY_SRC_COUCHBASE_COUCHBASECONNECTION_H_

#include "CouchbaseCommand.h"
#include "CouchbaseConfig.h"

#ifndef NOCB

NS_SA_EXT_BEGIN(couchbase)

class Connection : public AllocPool {
public:
	// creates Couchbase connection, bounded to current context pool
	static Connection *create(apr_pool_t *, const Config &);

	~Connection();
	void release();
	bool shouldBeInvalidated() const;

	const char *getServerVersion();
	const char *getVersion();
	const apr::string &getHost() const { return _host; }
	const apr::string &getBucket() const { return _bucket; }

	template <typename ... Args>
	void perform(Args && ... args) {
		prepare(std::forward<Args>(args)...);
		run();
	}

	template <typename T, typename ... Args>
	void prepare(T && t, Args && ... args) {
		addCmd(t.cmd, t.rec);
		prepare(std::forward<Args>(args)...);
	}

	void prepare() { }

	/*bool perform(cb::Command *cmd);
	void perform(Array *arr);
	void perform(Vector<cb::Command *> *vec);

	void perform_va(cb::Command *cmd, ...) __attribute__((sentinel));
	void perform_va(Array *arr, ...) __attribute__((sentinel));
	void perform_va(Vector<cb::Command *> *set, ...) __attribute__((sentinel));*/

	void onError(lcb_error_t err, const apr::string &txt);

protected:
	Connection(apr_pool_t *, const Config &, lcb_t);

	void addCmd(GetCmd &cmd, GetRec &rec);
	void addCmd(StoreCmd &cmd, StoreRec &rec);
	void addCmd(MathCmd &cmd, MathRec &rec);
	void addCmd(ObserveCmd &cmd, ObserveRec &rec);
	void addCmd(TouchCmd &cmd, TouchRec &rec);
	void addCmd(UnlockCmd &cmd, UnlockRec &rec);
	void addCmd(RemoveCmd &cmd, RemoveRec &rec);
	void addCmd(VersionCmd &cmd, VersionRec &rec);
	void addCmd(FlushCmd &cmd, FlushRec &rec);

	void run();

	apr_pool_t *_pool = nullptr;
	lcb_error_t _lastError = LCB_SUCCESS;
	lcb_t _instance = nullptr;
	apr::string _host;
	apr::string _bucket;

	/*void prepare(cb::Command *cmd);
	void prepare(Array *arr);
	void prepare(Vector<cb::Command *> *list);*/
};

NS_SA_EXT_END(couchbase)

#endif

#endif /* SERENITY_SRC_COUCHBASE_COUCHBASECONNECTION_H_ */
