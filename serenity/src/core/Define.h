/**
Copyright (c) 2016-2018 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef SADEFINES_H
#define	SADEFINES_H

#include "Config.h"

#include "SPAprArray.h"
#include "SPAprTable.h"
#include "SPAprUri.h"
#include "SPAprUuid.h"
#include "SPAprMutex.h"

#include "SPRef.h"
#include "SPData.h"

#include <typeindex>

NS_SA_BEGIN

enum class ResourceType {
	ResourceList,
	ReferenceSet,
	Object,
	Set,
	View,
	File,
	Array
};

struct InputConfig {
	enum class Require {
		None = 0,
		Data = 1,
		Files = 2,
		Body = 4,
		FilesAsData = 8,
	};

	InputConfig() = default;
	InputConfig(const InputConfig &) = default;
	InputConfig & operator=(const InputConfig &) = default;

	InputConfig(InputConfig &&) = default;
	InputConfig & operator=(InputConfig &&) = default;

	Require required = Require::None;
	size_t maxRequestSize = config::getMaxRequestSize();
	size_t maxVarSize = config::getMaxVarSize();
	size_t maxFileSize = config::getMaxFileSize();

	TimeInterval updateTime = config::getInputUpdateTime();
	float updateFrequency = config::getInputUpdateFrequency();
};

SP_DEFINE_ENUM_AS_MASK(InputConfig::Require);

constexpr apr_time_t operator"" _apr_sec ( unsigned long long int val ) { return val * 1000000; }
constexpr apr_time_t operator"" _apr_msec ( unsigned long long int val ) { return val * 1000; }
constexpr apr_time_t operator"" _apr_mksec ( unsigned long long int val ) { return val; }

class SharedObject : public AtomicRef {
public:
	template <typename Type, typename ... Args>
	static Rc<Type> create(memory::pool_t *, Args && ...);

	virtual bool init() { return true; }

protected:
	SharedObject() { }

	void setPool(memory::MemPool &&p) {
		_pool = move(p);
	}

	memory::MemPool _pool;
};

template <typename Type, typename ... Args>
auto SharedObject::create(memory::pool_t *pool, Args && ... args) -> Rc<Type> {
	memory::MemPool p(pool);
	return apr::pool::perform([&] {
		auto pRet = new Type();
		pRet->setPool(move(p));
	    if (pRet->init(std::forward<Args>(args)...)) {
	    	auto ret = Rc<Type>(pRet);
	    	pRet->release();
	    	return ret;
		} else {
			delete pRet;
			return Rc<Type>(nullptr);
		}
	}, p.pool());
}

template <typename Type, typename ... Args>
inline auto construct(memory::pool_t *pool, Args && ... args) -> Rc<Type> {
	return SharedObject::create<Type>(pool, std::forward<Args>(args)...);
}

NS_SA_END

#endif	/* SADEFINES_H */
