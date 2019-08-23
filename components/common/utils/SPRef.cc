// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2017-2018 Roman Katuntsev <sbkarr@stappler.org>

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

#include "SPCommon.h"
#include "SPRef.h"
#include "SPLog.h"
#include "SPTime.h"

NS_SP_BEGIN

namespace memleak {

static std::mutex s_mutex;
static Map<Ref *, Time> s_map;

void store(Ref *ptr) {
	s_mutex.lock();
	s_map.emplace(ptr, Time::now());
	s_mutex.unlock();
}
void release(Ref *ptr) {
	s_mutex.lock();
	s_map.erase(ptr);
	s_mutex.unlock();
}
void check(const std::function<void(Ref *, Time)> &cb) {
	s_mutex.lock();
	for (auto &it : s_map) {
		cb(it.first, it.second);
	}
	s_mutex.unlock();
}
size_t count() {
	size_t ret = 0;
	s_mutex.lock();
	ret = s_map.size();
	s_mutex.unlock();
	return ret;
}

}

NS_SP_END
