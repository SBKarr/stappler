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

#ifndef SERENITY_SRC_STORAGE_STORAGERESOLVER_H_
#define SERENITY_SRC_STORAGE_STORAGERESOLVER_H_

#include "Request.h"

NS_SA_EXT_BEGIN(storage)

class Resolver : public AllocPool {
public:
	Resolver(Scheme *scheme, const data::TransformMap *map) : _scheme(scheme), _transform(map) { }
	virtual ~Resolver() { }

	virtual bool selectById(uint64_t) = 0; // objects/id123
	virtual bool selectByAlias(const String &) = 0; // objects/named-alias
	virtual bool selectByQuery(Query::Select &&) = 0; // objects/select/counter/2 or objects/select/counter/bw/10/20

	virtual bool order(const String &f, Ordering o) = 0; // objects/order/counter/desc
	virtual bool first(const String &f, size_t v) = 0; // objects/first/10
	virtual bool last(const String &f, size_t v) = 0; // objects/last/10
	virtual bool limit(size_t limit) = 0; // objects/order/counter/desc/10
	virtual bool offset(size_t offset) = 0; // objects/order/counter/desc/10

	virtual bool getObject(const Field *) = 0; // objects/id123/owner
	virtual bool getSet(const Field *) = 0; // objects/id123/childs
	virtual bool getField(const String &, const Field *) = 0; // objects/id123/image
	virtual bool getAll() = 0; // objects/id123/childs/all

	virtual Resource *getResult() = 0;

	virtual const Field *getSchemeField(const String &name) const;
	virtual Scheme *getScheme() const;

protected:
	Scheme *_scheme;
	const data::TransformMap *_transform;
};

NS_SA_EXT_END(storage)

#endif /* SERENITY_SRC_STORAGE_STORAGERESOLVER_H_ */
