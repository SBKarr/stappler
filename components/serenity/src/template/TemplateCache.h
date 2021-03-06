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

#ifndef SERENITY_SRC_TEMPLATE_TEMPLATECACHE_H_
#define SERENITY_SRC_TEMPLATE_TEMPLATECACHE_H_

#include "TemplateFile.h"

NS_SA_EXT_BEGIN(tpl)

class FileRef : public AllocPool {
public:
	FileRef(mem::pool_t *, const String &, int watch, int wId);

	void retain(Request &);
	void release();

	int regenerate(int notify);

	const File &getFile() const;
	time_t getMtime() const;
	int getWatch() const { return _watch; }

protected:
	int _watch = -1;
	mem::pool_t * _pool = nullptr;
	File _file;
	std::atomic<uint32_t> _refCount;
};


class Cache : public AllocPool {
public:
	using RunCallback = Function<void(Exec &, Request &)>;

	Cache();
	~Cache();

	int getNotify() const { return _inotify; }

	void update(int, bool regenerate);
	void update(apr_pool_t *);

	bool isNotifyAvailable();

	void runTemplate(const String &, Request &, const RunCallback &);

protected:
	FileRef *acquireTemplate(const String &, Request &);
	FileRef *openTemplate(const String &, int wId);

	int _inotify = -1;
	bool _inotifyAvailable = false;
	Map<int, String> _watches;

	apr_pool_t *_pool = nullptr;
	apr::mutex _mutex;
	Map<String, FileRef *> _templates;
};

NS_SA_EXT_END(tpl)

#endif /* SERENITY_SRC_TEMPLATE_TEMPLATECACHE_H_ */
