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

#ifndef SERENITY_SRC_RESOURCE_RESOURCETEMPLATES_H_
#define SERENITY_SRC_RESOURCE_RESOURCETEMPLATES_H_

#include "Resource.h"

NS_SA_BEGIN

class ResourceProperty : public Resource {
public:
	ResourceProperty(Adapter *h, QueryList &&q, const Field *prop);

	virtual bool removeObject() override;

protected:
	uint64_t getObjectId();
	data::Value getObject(bool forUpdate);

	const Field *_field = nullptr;
};

class ResourceFile : public ResourceProperty {
public:
	ResourceFile(Adapter *h, QueryList &&q, const Field *prop);

	virtual bool prepareUpdate() override;
	virtual bool prepareCreate() override;
	virtual data::Value updateObject(data::Value &, apr::array<InputFile> &f) override;
	virtual data::Value createObject(data::Value &val, apr::array<InputFile> &f) override;

	virtual data::Value getResultObject() override;

protected:
	data::Value getFileForObject(data::Value &object);
	data::Value getDatabaseObject();
};

class ResourceArray : public ResourceProperty {
public:
	ResourceArray(Adapter *h, QueryList &&q, const Field *prop);

	virtual bool prepareUpdate() override;
	virtual bool prepareCreate() override;
	virtual data::Value updateObject(data::Value &data, apr::array<InputFile> &) override;
	virtual data::Value createObject(data::Value &data, apr::array<InputFile> &) override;
	virtual data::Value getResultObject() override;

protected:
	data::Value getDatabaseObject();
	data::Value getArrayForObject(data::Value &object);
};

class ResourceObject : public Resource {
public:
	ResourceObject(Adapter *a, QueryList &&q);

	virtual bool prepareUpdate() override;
	virtual bool prepareCreate() override;
	virtual bool prepareAppend() override;
	virtual bool removeObject() override;
	virtual data::Value updateObject(data::Value &data, apr::array<InputFile> &) override;
	virtual data::Value getResultObject() override;

protected:
	data::Value performUpdate(const Vector<int64_t> &, data::Value &, apr::array<InputFile> &);

	data::Value processResultList(const QueryList &s, data::Value &ret);
	bool processResultObject(Permission p, const QueryList &s, data::Value &obj);
	data::Value getDatabaseObject();
	Vector<int64_t> getDatabaseId(const QueryList &q, size_t count = maxOf<size_t>());
};

class ResourceReslist : public ResourceObject {
public:
	ResourceReslist(Adapter *a, QueryList &&q);

	virtual bool prepareCreate() override;
	virtual data::Value createObject(data::Value &, apr::array<InputFile> &) override;

protected:
	data::Value performCreateObject(data::Value &data, apr::array<InputFile> &files, const data::Value &extra);
};

class ResourceSet : public ResourceReslist {
public:
	ResourceSet(Adapter *a, QueryList &&q);

	virtual bool prepareAppend() override;
	virtual data::Value createObject(data::Value &, apr::array<InputFile> &) override;
	virtual data::Value appendObject(data::Value &) override;
};

class ResourceRefSet : public ResourceSet {
public:
	ResourceRefSet(Adapter *a, QueryList &&q);

	virtual bool prepareUpdate() override;
	virtual bool prepareCreate() override;
	virtual bool prepareAppend() override;
	virtual bool removeObject() override;
	virtual data::Value updateObject(data::Value &, apr::array<InputFile> &) override;
	virtual data::Value createObject(data::Value &, apr::array<InputFile> &) override;
	virtual data::Value appendObject(data::Value &) override;

protected:
	int64_t getObjectId();
	data::Value getObjectValue();

	Vector<int64_t> prepareAppendList(int64_t id, const data::Value &, bool cleanup);

	bool doCleanup(int64_t, Permission p, const Vector<int64_t> &);

	data::Value doAppendObject(const data::Value &, bool cleanup);
	data::Value doAppendObjects(const data::Value &, bool cleanup);
	bool doAppendObjectsTransaction(data::Value &, const data::Value &, bool cleanup);

	bool isEmptyRequest();

	int64_t _objectId = 0;
	data::Value _objectValue;
	const Scheme *_sourceScheme = nullptr;
	const Field *_field = nullptr;
	Permission _refPerms = Permission::Restrict;
};

class ResourceFieldObject : public ResourceObject {
public:
	ResourceFieldObject(Adapter *a, QueryList &&q);

	virtual bool prepareUpdate() override;
	virtual bool prepareCreate() override;
	virtual bool prepareAppend() override;
	virtual bool removeObject() override;
	virtual data::Value updateObject(data::Value &, apr::array<InputFile> &) override;
	virtual data::Value createObject(data::Value &, apr::array<InputFile> &) override;
	virtual data::Value appendObject(data::Value &) override;

protected:
	int64_t getRootId();
	int64_t getObjectId();

	data::Value getRootObject(bool forUpdate);
	data::Value getTargetObject(bool forUpdate);

	bool doRemoveObject();
	data::Value doUpdateObject(data::Value &, apr::array<InputFile> &);
	data::Value doCreateObject(data::Value &, apr::array<InputFile> &);

	int64_t _objectId = 0;
	int64_t _rootId = 0;
	const Scheme *_sourceScheme = nullptr;
	const Field *_field = nullptr;
	Permission _refPerms = Permission::Restrict;
};

class ResourceView : public ResourceSet {
public:
	ResourceView(Adapter *h, QueryList &&q);

	virtual bool prepareUpdate() override;
	virtual bool prepareCreate() override;
	virtual bool prepareAppend() override;
	virtual bool removeObject() override;

	virtual data::Value updateObject(data::Value &data, apr::array<InputFile> &) override;
	virtual data::Value createObject(data::Value &data, apr::array<InputFile> &) override;

	virtual data::Value getResultObject() override;

protected:
	const Field *_field = nullptr;
};

NS_SA_END

#endif /* SERENITY_SRC_RESOURCE_RESOURCETEMPLATES_H_ */
