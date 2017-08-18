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
	Resolver(Adapter *a, const Scheme &scheme, const data::TransformMap *map);

	bool selectById(uint64_t); // objects/id123
	bool selectByAlias(const String &); // objects/named-alias
	bool selectByQuery(Query::Select &&); // objects/select/counter/2 or objects/select/counter/bw/10/20

	bool order(const String &f, Ordering o); // objects/order/counter/desc
	bool first(const String &f, size_t v); // objects/first/10
	bool last(const String &f, size_t v); // objects/last/10
	bool limit(size_t limit); // objects/order/counter/desc/10
	bool offset(size_t offset); // objects/order/counter/desc/10

	bool getObject(const Field *); // objects/id123/owner
	bool getSet(const Field *); // objects/id123/childs
	bool getField(const String &, const Field *); // objects/id123/image
	bool getAll(); // objects/id123/childs/all

	Resource *getResult();

	const Field *getSchemeField(const String &name) const;
	const Scheme *getScheme() const;

protected:
	enum InternalResourceType {
		Objects,
		File,
		Array,
	};

	bool _all = false;
	Adapter *_storage = nullptr;
	const Scheme *_scheme = nullptr;
	const data::TransformMap *_transform = nullptr;
	InternalResourceType _type = Objects;
	Resource *_resource = nullptr;
	QueryList _queries;
};

NS_SA_EXT_END(storage)

#endif /* SERENITY_SRC_STORAGE_STORAGERESOLVER_H_ */
