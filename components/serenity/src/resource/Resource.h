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

#include "Define.h"
#include "SPAprArray.h"

NS_SA_BEGIN

using ResolveOptions = db::Resolve;

class Resource : public AllocBase {
public:
	using Adapter = db::Adapter;
	using Transaction = db::Transaction;
	using Scheme = db::Scheme;
	using Worker = db::Worker;
	using Field = db::Field;
	using Object = db::Object;
	using File = db::File;
	using Query = db::Query;
	using QueryList = db::QueryList;

	using QueryFieldResolver = db::QueryFieldResolver;

	static Resource *resolve(const Adapter &, const Scheme &scheme, const String &path);
	static Resource *resolve(const Adapter &, const Scheme &scheme, const String &path, data::Value & sub);

	/* PathVec should be inverted (so, first selectors should be last in vector */
	static Resource *resolve(const Adapter &, const Scheme &scheme, Vector<StringView> &path);

	virtual ~Resource();
	Resource(ResourceType, const Adapter &, QueryList &&);

	ResourceType getType() const;
	const Scheme &getScheme() const;
	int getStatus() const;

	bool isDeltaApplicable() const;
	bool hasDelta() const;

	void setQueryDelta(Time);
	Time getQueryDelta() const;

	Time getSourceDelta() const;

	void setUser(User *);
	void setFilterData(const data::Value &);

	void setResolveOptions(const data::Value & opts);
	void setResolveDepth(size_t size);

	void setPageFrom(size_t);
	void setPageCount(size_t);

	void applyQuery(const data::Value &);

	void prepare(QueryList::Flags = QueryList::None);

	const QueryList &getQueries() const;

public: // common interface
	virtual bool prepareUpdate();
	virtual bool prepareCreate();
	virtual bool prepareAppend();
	virtual bool removeObject();
	virtual data::Value updateObject(data::Value &, apr::array<db::InputFile> &);
	virtual data::Value createObject(data::Value &, apr::array<db::InputFile> &);
	virtual data::Value appendObject(data::Value &);

	virtual data::Value getResultObject();
	virtual void resolve(const Scheme &, data::Value &); // called to apply resolve rules to object

public:
	size_t getMaxRequestSize() const;
	size_t getMaxVarSize() const;
	size_t getMaxFileSize() const;

protected:
	void encodeFiles(data::Value &, apr::array<db::InputFile> &);

	void resolveSet(const QueryFieldResolver &, int64_t, const Field &, data::Value &);
	void resolveObject(const QueryFieldResolver &, int64_t, const Field &, data::Value &);
	void resolveArray(const QueryFieldResolver &, int64_t, const Field &, data::Value &);
	void resolveFile(const QueryFieldResolver &, int64_t, const Field &, data::Value &);

	int64_t processResolveResult(const QueryFieldResolver &res, const Set<const Field *> &, data::Value &obj);

	void resolveResult(const QueryFieldResolver &res, data::Value &obj, uint16_t depth, uint16_t max);
	void resolveResult(const QueryList &, data::Value &);

protected:
	virtual const Scheme &getRequestScheme() const;
	void resolveOptionForString(const String &str);

	Time _delta;
	ResourceType _type = ResourceType::Object;
	int _status = HTTP_OK;

	Transaction _transaction;
	QueryList _queries;

	User *_user = nullptr;
	Set<int64_t> _resolveObjects;
	data::Value _filterData;
	Vector<String> _extraResolves;
	ResolveOptions _resolve = ResolveOptions::None;
};

NS_SA_END

#endif /* SERENITY_SRC_RESOURCE_RESOURCE_H_ */
