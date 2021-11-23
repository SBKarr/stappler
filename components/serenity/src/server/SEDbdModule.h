/**
 Copyright (c) 2021 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef COMPONENTS_SERENITY_SRC_SERVER_SEDBDMODULE_H_
#define COMPONENTS_SERENITY_SRC_SERVER_SEDBDMODULE_H_

#include "Root.h"

NS_SA_BEGIN

class DbdModule : public AllocBase {
public:
	struct Config {
		int nmin = 1;
		int nkeep = 2;
		int nmax = 10;
		int64_t exptime = 300;
		bool persistent = true;
	};

	static DbdModule *create(mem::pool_t *root, Map<StringView, StringView> *params);
	static void destroy(DbdModule *);

	db::sql::Driver::Handle openConnection(apr_pool_t *);
	void closeConnection(db::sql::Driver::Handle);

	void close();

	mem::pool_t *getPool() const { return _pool; }
	bool isDestroyed() const { return _destroyed; }
	db::sql::Driver *getDriver() const { return _dbDriver; }
	Map<StringView, StringView> *getParams() const { return _dbParams; }

protected:
	DbdModule(mem::pool_t *root, const Config &, db::sql::Driver *, Map<StringView, StringView> *params);

	mem::pool_t *_pool = nullptr;
	Config _config;
	Map<StringView, StringView> *_dbParams = nullptr;
	bool _destroyed = true;
	apr_reslist_t *_reslist = nullptr;
	db::sql::Driver *_dbDriver = nullptr;

	Vector<const char *> _keywords;
	Vector<const char *> _values;
};

NS_SA_END

#endif /* COMPONENTS_SERENITY_SRC_SERVER_SEDBDMODULE_H_ */
