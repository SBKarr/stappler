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

#include "Define.h"
#include "ResourceHandler.h"
#include "Resource.h"
#include "StorageResolver.h"

NS_SA_BEGIN

static Vector<StringView> parsePath(const String &path) {
	auto delim = "/";
	if (!path.empty() && path.front() == ':') {
		delim = ":";
	}
	Vector<StringView> pathVec;
	string::split(path, delim, [&] (const StringView &v) {
		pathVec.emplace_back(v);
	});
	while (!pathVec.empty() && pathVec.back().empty()) {
		pathVec.pop_back();
	}

	if (!pathVec.empty()) {
		std::reverse(pathVec.begin(), pathVec.end());
		while (pathVec.back().empty()) {
			pathVec.pop_back();
		}
	}

	return pathVec;
}

static bool getSelectResource(storage::Resolver *resv, Vector<StringView> &path, bool &isSingleObject) {
	if (path.size() < 2) {
		messages::error("ResourceResolver", "invalid 'select' query");
		return false;
	}

	auto field = resv->getScheme()->getField(path.back());
	if (!field || !field->isIndexed()) {
		messages::error("ResourceResolver", "invalid 'select' query");
		return false;
	}
	path.pop_back();

	StringView cmpStr(path.back()); path.pop_back();

	db::Comparation cmp = db::Comparation::Equal;
	size_t valuesRequired = 1;

	if (cmpStr == "lt") {
		cmp = db::Comparation::LessThen;
	} else if (cmpStr == "le") {
		cmp = db::Comparation::LessOrEqual;
	} else if (cmpStr == "eq") {
		cmp = db::Comparation::Equal;
		if (field->hasFlag(db::Flags::Unique) || field->getTransform() == db::Transform::Alias) {
			isSingleObject = true;
		}
	} else if (cmpStr == "neq") {
		cmp = db::Comparation::NotEqual;
	} else if (cmpStr == "ge") {
		cmp = db::Comparation::GreatherOrEqual;
	} else if (cmpStr == "gt") {
		cmp = db::Comparation::GreatherThen;
	} else if (cmpStr == "bw") {
		cmp = db::Comparation::BetweenValues;
		valuesRequired = 2;
	} else if (cmpStr == "be") {
		cmp = db::Comparation::BetweenEquals;
		valuesRequired = 2;
	} else if (cmpStr == "nbw") {
		cmp = db::Comparation::NotBetweenValues;
		valuesRequired = 2;
	} else if (cmpStr == "nbe") {
		cmp = db::Comparation::NotBetweenEquals;
		valuesRequired = 2;
	} else {
		if (field->hasFlag(db::Flags::Unique) || field->getTransform() == db::Transform::Alias) {
			isSingleObject = true;
		}
		if (field->getType() == db::Type::Text) {
			return resv->selectByQuery(db::Query::Select(field->getName(), cmp, cmpStr));
		} else if (field->getType() == db::Type::Boolean) {
			if (valid::validateNumber(cmpStr)) {
				return resv->selectByQuery(db::Query::Select(field->getName(), cmp, cmpStr.readInteger().get(), 0));
			} else if (cmpStr == "t" || cmpStr == "true") {
				return resv->selectByQuery(db::Query::Select(field->getName(), cmp, data::Value(true), data::Value(false)));
			} else if (cmpStr == "f" || cmpStr == "false") {
				return resv->selectByQuery(db::Query::Select(field->getName(), cmp, data::Value(false), data::Value(false)));
			}
		} else if (valid::validateNumber(cmpStr) && db::checkIfComparationIsValid(field->getType(), cmp)) {
			return resv->selectByQuery(db::Query::Select(field->getName(), cmp, cmpStr.readInteger().get(), 0));
		} else {
			messages::error("ResourceResolver", "invalid 'select' query");
			return false;
		}
	}

	if (path.size() < valuesRequired || !db::checkIfComparationIsValid(field->getType(), cmp)) {
		messages::error("ResourceResolver", "invalid 'select' query");
		return false;
	}

	if (valuesRequired == 1) {
		StringView value(std::move(path.back())); path.pop_back();
		if (field->getType() == db::Type::Text) {
			return resv->selectByQuery(db::Query::Select(field->getName(), cmp, value));
		} else if (valid::validateNumber(value)) {
			return resv->selectByQuery(db::Query::Select(field->getName(), cmp, value.readInteger().get(), 0));
		} else {
			messages::error("ResourceResolver", "invalid 'select' query");
			return false;
		}
	}

	if (valuesRequired == 2) {
		StringView value1(std::move(path.back())); path.pop_back();
		StringView value2(std::move(path.back())); path.pop_back();
		if (valid::validateNumber(value1) && valid::validateNumber(value2)) {
			return resv->selectByQuery(db::Query::Select(field->getName(), cmp, value1.readInteger().get(), value2.readInteger().get()));
		}
	}

	return false;
}

static bool getSearchResource(storage::Resolver *resv, Vector<StringView> &path, bool &isSingleObject) {
	if (path.size() < 1) {
		messages::error("ResourceResolver", "invalid 'search' query");
		return false;
	}

	auto field = resv->getScheme()->getField(path.back());
	if (!field || field->getType() != db::Type::FullTextView) {
		messages::error("ResourceResolver", "invalid 'search' query");
		return false;
	}
	path.pop_back();

	return resv->searchByField(field);
}

static bool getOrderResource(storage::Resolver *resv, Vector<StringView> &path) {
	if (path.size() < 1) {
		messages::error("ResourceResolver", "invalid 'order' query");
		return false;
	}

	auto field = resv->getScheme()->getField(path.back());
	if (!field || !field->isIndexed()) {
		messages::error("ResourceResolver", "invalid 'order' query");
		return false;
	}
	path.pop_back();

	db::Ordering ord = db::Ordering::Ascending;
	if (!path.empty()) {
		if (path.back() == "asc" ) {
			ord = db::Ordering::Ascending;
			path.pop_back();
		} else if (path.back() == "desc") {
			ord = db::Ordering::Descending;
			path.pop_back();
		}
	}

	if (!path.empty()) {
		auto &n = path.back();
		if (valid::validateNumber(n)) {
			if (resv->order(field->getName(), ord)) {
				auto ret = resv->limit(n.readInteger().get());
				path.pop_back();
				return ret;
			} else {
				return false;
			}
		}
	}

	return resv->order(field->getName(), ord);
}

static bool getOrderResource(storage::Resolver *resv, Vector<StringView> &path, const StringView &fieldName, db::Ordering ord) {
	auto field = resv->getScheme()->getField(fieldName);
	if (!field || !field->isIndexed()) {
		messages::error("ResourceResolver", "invalid 'order' query");
		return false;
	}

	if (!path.empty()) {
		auto &n = path.back();
		if (valid::validateNumber(n)) {
			if (resv->order(field->getName(), ord)) {
				auto ret = resv->limit(n.readInteger().get());
				path.pop_back();
				return ret;
			} else {
				return false;
			}
		}
	}

	return resv->order(field->getName(), ord);
}

static bool getLimitResource(storage::Resolver *resv, Vector<StringView> &path, bool &isSingleObject) {
	if (path.size() < 1) {
		messages::error("ResourceResolver", "invalid 'limit' query");
		return false;
	}

	StringView value(path.back()); path.pop_back();
	if (valid::validateNumber(value)) {
		auto val = value.readInteger().get();
		if (val == 1) {
			isSingleObject = true;
		}
		return resv->limit(val);
	} else {
		messages::error("ResourceResolver", "invalid 'limit' query");
		return false;
	}
}

static bool getOffsetResource(storage::Resolver *resv, Vector<StringView> &path) {
	if (path.size() < 1) {
		messages::error("ResourceResolver", "invalid 'offset' query");
		return false;
	}

	StringView value(std::move(path.back())); path.pop_back();
	if (valid::validateNumber(value)) {
		return resv->offset(value.readInteger().get());
	} else {
		messages::error("ResourceResolver", "invalid 'offset' query");
		return false;
	}
}

static bool getFirstResource(storage::Resolver *resv, Vector<StringView> &path, bool &isSingleObject) {
	if (path.size() < 1) {
		messages::error("ResourceResolver", "invalid 'first' query");
		return false;
	}

	auto field = resv->getScheme()->getField(path.back());
	if (!field || !field->isIndexed()) {
		messages::error("ResourceResolver", "invalid 'first' query");
		return false;
	}
	path.pop_back();

	if (!path.empty()) {
		if (valid::validateNumber(path.back())) {
			size_t val = path.back().readInteger().get();
			path.pop_back();

			if (val == 1) {
				isSingleObject = true;
			}
			return resv->first(field->getName(), val);
		}
	}

	isSingleObject = true;
	return resv->first(field->getName(), 1);
}

static bool getLastResource(storage::Resolver *resv, Vector<StringView> &path, bool &isSingleObject) {
	if (path.size() < 1) {
		messages::error("ResourceResolver", "invalid 'last' query");
		return false;
	}

	auto field = resv->getScheme()->getField(path.back());
	if (!field || !field->isIndexed()) {
		messages::error("ResourceResolver", "invalid 'last' query");
		return false;
	}
	path.pop_back();

	if (!path.empty()) {
		if (valid::validateNumber(path.back())) {
			size_t val = path.back().readInteger().get();
			path.pop_back();

			if (val == 1) {
				isSingleObject = true;
			}
			return resv->last(field->getName(), val);
		}
	}

	isSingleObject = true;
	return resv->last(field->getName(), 1);
}

static Resource *parseResource(storage::Resolver *resv, Vector<StringView> &path) {
	bool isSingleObject = false;
	while (!path.empty()) {
		StringView filter(std::move(path.back()));
		path.pop_back();

		if (!isSingleObject) {
			if (filter.starts_with("id")) {
				filter += "id"_len;
				if (uint64_t id = filter.readInteger().get()) {
					if (!resv->selectById(id)) {
						return nullptr;
					}
					isSingleObject = true;
				} else {
					return nullptr;
				}
			} else if (filter.starts_with("named-")) {
				filter += "named-"_len;
				if (valid::validateIdentifier(filter)) {
					if (!resv->selectByAlias(filter)) {
						return nullptr;
					}
					isSingleObject = true;
				} else {
					return nullptr;
				}
			} else if (filter == "all") {
				if (!resv->getAll()) {
					return nullptr;
				}
				// do nothing
			} else if (filter == "select") {
				if (!getSelectResource(resv, path, isSingleObject)) {
					return nullptr;
				}
			} else if (filter == "search") {
				if (!getSearchResource(resv, path, isSingleObject)) {
					return nullptr;
				}
			} else if (filter == "order") {
				if (!getOrderResource(resv, path)) {
					return nullptr;
				}
			} else if (filter.size() > 2 && filter.front() == '+') {
				if (!getOrderResource(resv, path, filter.sub(1), db::Ordering::Ascending)) {
					return nullptr;
				}
			} else if (filter.size() > 2 && filter.front() == '-') {
				if (!getOrderResource(resv, path, filter.sub(1), db::Ordering::Descending)) {
					return nullptr;
				}
			} else if (filter == "limit") {
				if (!getLimitResource(resv, path, isSingleObject)) {
					return nullptr;
				}
			} else if (filter == "offset") {
				if (!getOffsetResource(resv, path)) {
					return nullptr;
				}
			} else if (filter == "first") {
				if (!getFirstResource(resv, path, isSingleObject)) {
					return nullptr;
				}
			} else if (filter == "last") {
				if (!getLastResource(resv, path, isSingleObject)) {
					return nullptr;
				}
			} else if (filter.size() > 2 && filter.front() == '+') {
				if (!getOrderResource(resv, path, filter.sub(1), db::Ordering::Ascending)) {
					return nullptr;
				}
			} else if (filter.size() > 2 && filter.front() == '-') {
				if (!getOrderResource(resv, path, filter.sub(1), db::Ordering::Descending)) {
					return nullptr;
				}
			} else {
				messages::error("ResourceResolver", "Invalid query", data::Value(filter));
				return nullptr;
			}
		} else {
			if (filter == "offset" && !path.empty() && valid::validateNumber(path.back())) {
				if (resv->offset(path.back().readInteger().get())) {
					path.pop_back();
					continue;
				}
			}
			auto f = resv->getScheme()->getField(filter);
			if (!f) {
				messages::error("ResourceResolver", "No such field", data::Value(filter));
				return nullptr;
			}

			auto type = f->getType();
			if (type == db::Type::File || type == db::Type::Image || type == db::Type::Array) {
				isSingleObject = true;
				if (!resv->getField(filter, f)) {
					return nullptr;
				} else {
					return resv->getResult();
				}
			} else if (type == db::Type::Object) {
				isSingleObject = true;
				if (!resv->getObject(f)) {
					return nullptr;
				}
			} else if (type == db::Type::Set) {
				isSingleObject = false;
				if (!resv->getSet(f)) {
					return nullptr;
				}
			} else if (type == db::Type::View) {
				isSingleObject = false;
				if (!resv->getView(f)) {
					return nullptr;
				}
			} else {
				messages::error("ResourceResolver", "Invalid query", data::Value(filter));
				return nullptr;
			}
		}
	}

	return resv->getResult();
}

static Resource *getResolvedResource(storage::Resolver *resv, Vector<StringView> &path) {
	if (path.empty()) {
		return resv->getResult();
	}
	return parseResource(resv, path);
}

Resource *Resource::resolve(const Adapter &a, const Scheme &scheme, const String &path) {
	data::Value tmp;
	return resolve(a, scheme, path, tmp);
}

Resource *Resource::resolve(const Adapter &a, const Scheme &scheme, const String &path, data::Value & sub) {
	auto pathVec = parsePath(path);

	storage::Resolver resolver(a, scheme);
	if (sub.isDictionary() && !sub.empty()) {
		for (auto &it : sub.asDict()) {
			if (auto f = resolver.getScheme()->getField(it.first)) {
				if (f->isIndexed()) {
					switch (f->getType()) {
					case db::Type::Integer:
					case db::Type::Boolean:
					case db::Type::Object:
						resolver.selectByQuery(
								db::Query::Select(it.first, db::Comparation::Equal, it.second.getInteger(), 0));
						break;
					case db::Type::Text:
						resolver.selectByQuery(
								db::Query::Select(it.first, db::Comparation::Equal, it.second.getString()));
						break;
					default:
						break;
					}
				}
			}
		}
	} else if (sub.isInteger()) {
		auto id = sub.getInteger();
		if (id) {
			resolver.selectById(id);
		}
	} else if (sub.isString()) {
		auto & str = sub.getString();
		if (!str.empty()) {
			resolver.selectByAlias(str);
		}
	}

	return getResolvedResource(&resolver, pathVec);
}

Resource *Resource::resolve(const Adapter &a, const storage::Scheme &scheme, Vector<StringView> &pathVec) {
	storage::Resolver resolver(a, scheme);
	return getResolvedResource(&resolver, pathVec);
}

NS_SA_END
