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

#include "STSqlDriver.h"
#include "STPqDriver.h"

NS_DB_SQL_BEGIN

Driver *Driver::open(mem::pool_t *pool, mem::StringView path, const void *external) {
	Driver *ret = nullptr;
	mem::pool::push(pool);
	if (path == "pgsql") {
		ret = pq::Driver::open(mem::StringView(), external);
	} else if (path.starts_with("pgsql:")) {
		path += "pgsql:"_len;
		ret = pq::Driver::open(path, external);
	}
	mem::pool::pop();
	return ret;
}

Driver::~Driver() { }

void Driver::setDbCtrl(mem::Function<void(bool)> &&fn) {
	_dbCtrl = std::move(fn);
}

NS_DB_SQL_END
