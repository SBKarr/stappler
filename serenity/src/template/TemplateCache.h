/*
 * TemplateCache.h
 *
 *  Created on: 28 нояб. 2016 г.
 *      Author: sbkarr
 */

#ifndef SERENITY_SRC_TEMPLATE_TEMPLATECACHE_H_
#define SERENITY_SRC_TEMPLATE_TEMPLATECACHE_H_

#include "TemplateFile.h"

NS_SA_EXT_BEGIN(tpl)

class FileRef : public AllocPool {
public:
	FileRef(apr::MemPool &&, const String &);

	void retain(Request &);
	void release();

	const File &getFile() const;
	time_t getMtime() const;

protected:
	apr::MemPool _pool;
	File _file;
	std::atomic<uint32_t> _refCount;
};


class Cache : public AllocPool {
public:
	using RunCallback = Function<void(Exec &, Request &)>;

	Cache();

	void update();

	void runTemplate(const String &, Request &, const RunCallback &);

protected:
	FileRef *acquireTemplate(const String &, Request &);
	FileRef *openTemplate(const String &);

	apr_pool_t *_pool = nullptr;
	apr::mutex _mutex;
	Map<String, FileRef *> _templates;
};

NS_SA_EXT_END(tpl)

#endif /* SERENITY_SRC_TEMPLATE_TEMPLATECACHE_H_ */
