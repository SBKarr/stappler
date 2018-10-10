/**
 Copyright (c) 2018 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef SRC_SPUGCACHE_H_
#define SRC_SPUGCACHE_H_

#include "SPug.h"
#include "SPugTemplate.h"

NS_SP_EXT_BEGIN(pug)

class FileRef : public AtomicRef {
public:
	static Rc<FileRef> read(const FilePath &path, Template::Options opts = Template::Options::getDefault(),
			const Function<void(const StringView &)> & = nullptr);

	static Rc<FileRef> read(String && content, bool isTemplate, Template::Options opts = Template::Options::getDefault(),
			const Function<void(const StringView &)> & = nullptr);

	const String &getContent() const;
	const Template *getTemplate() const;
	time_t getMtime() const;

	FileRef(memory::pool_t *, const FilePath &path, Template::Options opts, const Function<void(const StringView &)> &cb);
	FileRef(memory::pool_t *, String && content, bool isTemplate, Template::Options opts, const Function<void(const StringView &)> &cb);

	virtual ~FileRef();

protected:
	time_t _mtime = 0;
	memory::pool_t *_pool = nullptr;
	String _content;
	Template * _template = nullptr;
};

class Cache : public AllocBase {
public:
	using RunCallback = Function<bool(Context &, const Template &)>;
	using Options = Template::Options;

	Cache(Template::Options opts = Template::Options::getDefault(), const Function<void(const StringView &)> &err = nullptr);

	void update(apr_pool_t *);

	bool runTemplate(const StringView &, const RunCallback &, std::ostream &);

	void addFile(const StringView &);
	void addContent(const StringView &key, String &&);
	void addTemplate(const StringView &key, String &&);

	Rc<FileRef> get(const StringView &key) const;

protected:
	Rc<FileRef> acquireTemplate(const StringView &, bool readOnly);
	Rc<FileRef> openTemplate(const StringView &);

	void onError(const StringView &);

	memory::pool_t *_pool = nullptr;
	Mutex _mutex;
	Map<String, Rc<FileRef>> _templates;
	Template::Options _opts;
	Function<void(const StringView &)> _errorCallback;
};

NS_SP_EXT_END(pug)

#endif /* SRC_SPUGCACHE_H_ */
