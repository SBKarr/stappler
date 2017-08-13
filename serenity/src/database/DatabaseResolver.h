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

#ifndef SERENITY_SRC_DATABASE_DATABASERESOLVER_H_
#define SERENITY_SRC_DATABASE_DATABASERESOLVER_H_

#include "StorageResolver.h"
#include "DatabaseHandle.h"
#include "Resource.h"

NS_SA_EXT_BEGIN(database)

class Resolver : public storage::Resolver {
public:
	using Scheme = storage::Scheme;
	using Field = storage::Field;
	using Query = storage::Query;

	Resolver(Handle *, Scheme *scheme, const data::TransformMap *map);
	virtual ~Resolver();

	virtual bool selectById(uint64_t) override; // objects/id123
	virtual bool selectByAlias(const String &) override; // objects/named-alias
	virtual bool selectByQuery(Query::Select &&) override; // objects/select/counter/2 or objects/select/counter/bw/10/20

	virtual bool order(const String &f, storage::Ordering o) override; // objects/order/counter/desc
	virtual bool first(const String &f, size_t v) override; // objects/first/counter/10
	virtual bool last(const String &f, size_t v) override; // objects/last/counter/10
	virtual bool limit(size_t limit) override; // objects/order/counter/desc/limit/10/offset/5
	virtual bool offset(size_t offset) override; // objects/order/counter/desc/limit/10/offset/5

	virtual bool getObject(const Field *) override; // objects/id123/owner
	virtual bool getSet(const Field *) override; // objects/id123/childs
	virtual bool getField(const String &, const Field *) override; // objects/id123/image
	virtual bool getAll() override;

	virtual Resource *getResult() override;

protected:
	struct Subquery;
	class ResourceReslist;
	class ResourceObject;
	class ResourceSet;
	class ResourceRefSet;
	class ResourceArray;
	class ResourceFile;
	class ResourceProperty;

	static void writeSubqueryContent(apr::ostringstream &, Handle *h, Scheme *, Subquery *);
	static void writeSubquery(apr::ostringstream &, Handle *h, Scheme *, Subquery *, const String &f, bool idOnly = true);
	static void writeFileSubquery(apr::ostringstream &, Handle *h, Scheme *, Subquery *, const String &field);

	enum ResourceType {
		Objects,
		File,
		Array,
	};

	Handle *_handle = nullptr;

	bool _all = false;
	ResourceType _type = Objects;
	Subquery *_subquery = nullptr;
	Resource *_resource = nullptr;
};

struct Resolver::Subquery {
	using Ordering = storage::Ordering;

	Subquery(Scheme *s) : scheme(s) { }

	Scheme * scheme;

	Subquery *subquery = nullptr;
	String subqueryField;
	apr::vector<storage::Query::Select> where;
	uint64_t oid = 0;
	String alias;

	Ordering ordering = Ordering::Ascending;
	String orderField;

	size_t limit = maxOf<size_t>();
	size_t offset = 0;

	bool all = false;

	data::Value encode();
};

class Resolver::ResourceProperty : public Resource {
public:
	ResourceProperty(Scheme *s, Handle *h, Subquery *q, const Field *prop, const String &propName);

	virtual bool removeObject() override;

protected:
	uint64_t getObjectId();
	data::Value getObject(bool forUpdate);

	Handle *_handle = nullptr;
	Subquery *_query = nullptr;
	const Field *_field = nullptr;
	String _fieldName;
};

class Resolver::ResourceFile : public ResourceProperty {
public:
	ResourceFile(Scheme *s, Handle *h, Subquery *q, const Field *prop, const String &propName);

	virtual bool prepareUpdate() override;
	virtual bool prepareCreate() override;
	virtual data::Value updateObject(data::Value &, apr::array<InputFile> &f) override;
	virtual data::Value createObject(data::Value &val, apr::array<InputFile> &f) override;

	virtual data::Value getResultObject() override;

protected:
	data::Value getFileForObject(data::Value &object);
	data::Value getDatabaseObject();
};

class Resolver::ResourceArray : public ResourceProperty {
public:
	ResourceArray(Scheme *s, Handle *h, Subquery *q, const Field *prop, const String &propName);

	virtual bool prepareUpdate() override;
	virtual bool prepareCreate() override;
	virtual data::Value updateObject(data::Value &data, apr::array<InputFile> &) override;
	virtual data::Value createObject(data::Value &data, apr::array<InputFile> &) override;
	virtual data::Value getResultObject() override;

protected:
	data::Value getDatabaseObject();
	data::Value getArrayForObject(data::Value &object);
};

class Resolver::ResourceObject : public Resource {
public:
	ResourceObject(Scheme *s, Handle *h, Subquery *q);

	virtual bool prepareUpdate() override;
	virtual bool prepareCreate() override;
	virtual bool prepareAppend() override;
	virtual bool removeObject() override;
	virtual data::Value updateObject(data::Value &data, apr::array<InputFile> &) override;
	virtual data::Value getResultObject() override;

protected:
	data::Value performUpdate(const Vector<int64_t> &, data::Value &, apr::array<InputFile> &);

	data::Value processResultList(Scheme *s, data::Value &ret, bool resolve);
	bool processResultObject(Permission p, Scheme *s, data::Value &obj, bool resolve = false);
	data::Value getDatabaseObject();
	Vector<int64_t> getDatabaseId(Scheme *s, Subquery *q);

	Handle *_handle = nullptr;
	Subquery *_query = nullptr;
};

class Resolver::ResourceReslist : public ResourceObject {
public:
	ResourceReslist(Scheme *s, Handle *h, Subquery *q);

	virtual bool prepareCreate() override;
	virtual data::Value createObject(data::Value &, apr::array<InputFile> &) override;

protected:
	data::Value performCreateObject(data::Value &data, apr::array<InputFile> &files, const data::Value &extra);
};

class Resolver::ResourceSet : public ResourceReslist {
public:
	ResourceSet(Scheme *s, Handle *h, Subquery *q);

	virtual bool prepareAppend() override;
	virtual data::Value createObject(data::Value &, apr::array<InputFile> &) override;
	virtual data::Value appendObject(data::Value &) override;
};

class Resolver::ResourceRefSet : public ResourceSet {
public:
	ResourceRefSet(Scheme *s, Handle *h, Subquery *q, const Field *prop, const String &);

	virtual bool prepareUpdate() override;
	virtual bool prepareCreate() override;
	virtual bool prepareAppend() override;
	virtual bool removeObject() override;
	virtual data::Value updateObject(data::Value &, apr::array<InputFile> &) override;
	virtual data::Value createObject(data::Value &, apr::array<InputFile> &) override;
	virtual data::Value appendObject(data::Value &) override;

protected:
	uint64_t getObjectId();

	Vector<uint64_t> prepareAppendList(const data::Value &);

	data::Value doAppendObject(const data::Value &, bool cleanup);
	data::Value doAppendObjects(const data::Value &, bool cleanup);

	bool isEmptyRequest();

	virtual storage::Scheme *getRequestScheme() const;

	const Field *_field = nullptr;
	String _fieldName;
};

NS_SA_EXT_END(database)

#endif /* SERENITY_SRC_DATABASE_DATABASERESOLVER_H_ */
