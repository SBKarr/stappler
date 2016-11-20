/*
 * Resource.h
 *
 *  Created on: 6 марта 2016 г.
 *      Author: sbkarr
 */

#ifndef SERENITY_SRC_RESOURCE_RESOURCE_H_
#define SERENITY_SRC_RESOURCE_RESOURCE_H_

#include "StorageQuery.h"
#include "AccessControl.h"

NS_SA_BEGIN

enum class ResolveOptions {
	None = 0,
	Files = 2,
	Sets = 4,
	Objects = 8,
	Arrays = 16,
	Ids = 32,
};

SP_DEFINE_ENUM_AS_MASK(ResolveOptions);

class Resource : public AllocBase {
public:
	using Scheme = storage::Scheme;
	using Object = storage::Object;
	using File = storage::File;
	using Query = storage::Query;

	using Permission = AccessControl::Permission;
	using Action = AccessControl::Action;

	static Resource *resolve(storage::Adapter *a, storage::Scheme *scheme, const String &path, const data::TransformMap * = nullptr);
	static Resource *resolve(storage::Adapter *a, storage::Scheme *scheme, const String &path, data::Value & sub, const data::TransformMap * = nullptr);

	/* PathVec should be inverted (so, first selectors should be last in vector */
	static Resource *resolve(storage::Adapter *a, storage::Scheme *scheme, Vector<String> &path);

	virtual ~Resource() { }
	Resource(Scheme *, storage::Adapter *);

	Scheme *getScheme() const;
	int getStatus() const;

	void setTransform(const data::TransformMap *);
	const data::TransformMap *getTransform() const;

	void setAccessControl(AccessControl *);
	void setUser(User *);
	void setFilterData(const data::Value &);

	void setResolveOptions(ResolveOptions opts);
	void setResolveOptions(const data::Value & opts);
	void setResolveDepth(size_t size);
	void setPagination(size_t from, size_t count = maxOf<size_t>());

public: // common interface
	virtual bool prepareUpdate();
	virtual bool prepareCreate();
	virtual bool prepareAppend();
	virtual bool removeObject();
	virtual data::Value updateObject(data::Value &, apr::array<InputFile> &);
	virtual data::Value createObject(data::Value &, apr::array<InputFile> &);
	virtual data::Value appendObject(data::Value &);

	virtual data::Value getResultObject();
	virtual void resolve(Scheme *, data::Value &); // called to apply resolve rules to object

public:
	size_t getMaxRequestSize() const;
	size_t getMaxVarSize() const;
	size_t getMaxFileSize() const;

protected:
	void resolveSet(Scheme *, int64_t, const storage::Field &, Scheme *next, data::Value &);
	void resolveObject(Scheme *, int64_t, const storage::Field &, Scheme *next, data::Value &);
	void resolveFile(Scheme *, int64_t, const storage::Field &, data::Value &);
	void resolveArray(Scheme *, int64_t, const storage::Field &, data::Value &);
	void resolveExtra(const apr::map<String, storage::Field> &fields, data::Value &it);

	void resolveResult(Scheme *, data::Value &, size_t depth);

	Permission isSchemeAllowed(Scheme *, AccessControl::Action) const;
	bool isObjectAllowed(Scheme *, AccessControl::Action, data::Value &) const;
	bool isObjectAllowed(Scheme *, AccessControl::Action, data::Value &, data::Value &) const;
	bool isObjectAllowed(Scheme *, AccessControl::Action, data::Value &, data::Value &, apr::array<InputFile> &) const;

protected:
	ResolveOptions resolveOptionForString(const String &str);

	int _status = HTTP_OK;
	User *_user = nullptr;
	storage::Scheme *_scheme = nullptr;
	AccessControl *_access = nullptr;
	const data::TransformMap *_transform = nullptr;
	ResolveOptions _resolveOptions = ResolveOptions::None;
	size_t _resolveDepth = 0;
	size_t _pageFrom = 0, _pageCount = maxOf<size_t>();
	Set<int64_t> _resolveObjects;
	Permission _perms = Permission::Restrict;
	storage::Adapter *_adapter = nullptr;
	data::Value _filterData;
};

NS_SA_END

#endif /* SERENITY_SRC_RESOURCE_RESOURCE_H_ */
