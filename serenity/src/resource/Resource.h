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

#ifndef SERENITY_SRC_RESOURCE_RESOURCE_H_
#define SERENITY_SRC_RESOURCE_RESOURCE_H_

#include "StorageQuery.h"
#include "AccessControl.h"

NS_SA_BEGIN

using ResolveOptions = query::Resolve;

class Resource : public AllocBase {
public:
	using Adapter = storage::Adapter;
	using Scheme = storage::Scheme;
	using Field = storage::Field;
	using Object = storage::Object;
	using File = storage::File;
	using Query = storage::Query;
	using QueryList = storage::QueryList;

	using Permission = AccessControl::Permission;
	using Action = AccessControl::Action;

	using QueryFieldResolver = storage::QueryFieldResolver;

	static Resource *resolve(storage::Adapter *a, const storage::Scheme &scheme, const String &path, const data::TransformMap * = nullptr);
	static Resource *resolve(storage::Adapter *a, const storage::Scheme &scheme, const String &path, data::Value & sub, const data::TransformMap * = nullptr);

	/* PathVec should be inverted (so, first selectors should be last in vector */
	static Resource *resolve(storage::Adapter *a, const storage::Scheme &scheme, Vector<String> &path);

	virtual ~Resource() { }
	Resource(ResourceType, Adapter *, QueryList &&);

	ResourceType getType() const;
	const Scheme &getScheme() const;
	int getStatus() const;

	bool isDeltaApplicable() const;
	bool hasDelta() const;

	void setQueryDelta(Time);
	Time getQueryDelta() const;

	Time getSourceDelta() const;

	void setTransform(const data::TransformMap *);
	const data::TransformMap *getTransform() const;

	void setAccessControl(const AccessControl *);
	void setUser(User *);
	void setFilterData(const data::Value &);

	void setResolveOptions(const data::Value & opts);
	void setResolveDepth(size_t size);

	void setPageFrom(size_t);
	void setPageCount(size_t);

	void applyQuery(const data::Value &);

	void prepare();

public: // common interface
	virtual bool prepareUpdate();
	virtual bool prepareCreate();
	virtual bool prepareAppend();
	virtual bool removeObject();
	virtual data::Value updateObject(data::Value &, apr::array<InputFile> &);
	virtual data::Value createObject(data::Value &, apr::array<InputFile> &);
	virtual data::Value appendObject(data::Value &);

	virtual data::Value getResultObject();
	virtual void resolve(const Scheme &, data::Value &); // called to apply resolve rules to object

public:
	size_t getMaxRequestSize() const;
	size_t getMaxVarSize() const;
	size_t getMaxFileSize() const;

protected:
	void encodeFiles(data::Value &, apr::array<InputFile> &);

	void resolveSet(const QueryFieldResolver &, int64_t, const storage::Field &, data::Value &);
	void resolveObject(const QueryFieldResolver &, int64_t, const storage::Field &, data::Value &);
	void resolveArray(const QueryFieldResolver &, int64_t, const storage::Field &, data::Value &);
	void resolveFile(const QueryFieldResolver &, int64_t, const storage::Field &, data::Value &);

	int64_t processResolveResult(const QueryFieldResolver &res, const Set<const Field *> &, data::Value &obj);

	void resolveResult(const QueryFieldResolver &res, data::Value &obj, uint16_t depth, uint16_t max);
	void resolveResult(const QueryList &, data::Value &);

	Permission isSchemeAllowed(const Scheme &, AccessControl::Action) const;
	bool isObjectAllowed(const Scheme &, AccessControl::Action, data::Value &) const;
	bool isObjectAllowed(const Scheme &, AccessControl::Action, data::Value &, data::Value &) const;

protected:
	virtual const storage::Scheme &getRequestScheme() const;
	void resolveOptionForString(const String &str);

	Time _delta;
	ResourceType _type = ResourceType::Object;
	int _status = HTTP_OK;

	storage::Adapter *_adapter = nullptr;
	QueryList _queries;

	User *_user = nullptr;
	const AccessControl *_access = nullptr;
	const data::TransformMap *_transform = nullptr;
	Set<int64_t> _resolveObjects;
	Permission _perms = Permission::Restrict;
	data::Value _filterData;
	Vector<String> _extraResolves;
	ResolveOptions _resolve = ResolveOptions::None;
};

NS_SA_END

#endif /* SERENITY_SRC_RESOURCE_RESOURCE_H_ */
