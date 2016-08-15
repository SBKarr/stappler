/*
 * ResourceResolver.cpp
 *
 *  Created on: 6 марта 2016 г.
 *      Author: sbkarr
 */

#include "Define.h"
#include "ResourceHandler.h"
#include "Resource.h"
#include "StorageResolver.h"
#include "StorageScheme.h"

NS_SA_BEGIN

static Vector<String> parsePath(const String &path) {
	auto pathVec = string::split(path, "/");
	while (!pathVec.empty() && pathVec.back().empty()) {
		pathVec.pop_back();
	}

	if (!pathVec.empty()) {
		std::reverse(pathVec.begin(), pathVec.end());
		while (pathVec.back().empty()) {
			pathVec.pop_back();
		}
	}

	return std::move(pathVec);
}

static bool getSelectResource(storage::Resolver *resv, Vector<String> &path, bool &isSingleObject) {
	if (path.size() < 2) {
		messages::error("ResourceResolver", "invalid 'select' query");
		return false;
	}

	auto field = resv->getSchemeField(path.back());
	if (!field || !field->isIndexed()) {
		messages::error("ResourceResolver", "invalid 'select' query");
		return false;
	}
	path.pop_back();

	String cmpStr(std::move(path.back())); path.pop_back();

	storage::Comparation cmp = storage::Comparation::Equal;
	size_t valuesRequired = 1;

	if (cmpStr == "lt") {
		cmp = storage::Comparation::LessThen;
	} else if (cmpStr == "le") {
		cmp = storage::Comparation::LessOrEqual;
	} else if (cmpStr == "eq") {
		cmp = storage::Comparation::Equal;
		if (field->hasFlag(storage::Flags::Unique) || field->getTransform() == storage::Transform::Alias) {
			isSingleObject = true;
		}
	} else if (cmpStr == "neq") {
		cmp = storage::Comparation::NotEqual;
	} else if (cmpStr == "ge") {
		cmp = storage::Comparation::GreatherOrEqual;
	} else if (cmpStr == "gt") {
		cmp = storage::Comparation::GreatherThen;
	} else if (cmpStr == "bw") {
		cmp = storage::Comparation::BetweenValues;
		valuesRequired = 2;
	} else if (cmpStr == "be") {
		cmp = storage::Comparation::BetweenEquals;
		valuesRequired = 2;
	} else if (cmpStr == "nbw") {
		cmp = storage::Comparation::NotBetweenValues;
		valuesRequired = 2;
	} else if (cmpStr == "nbe") {
		cmp = storage::Comparation::NotBetweenEquals;
		valuesRequired = 2;
	} else {
		if (field->hasFlag(storage::Flags::Unique) || field->getTransform() == storage::Transform::Alias) {
			isSingleObject = true;
		}
		if (field->getType() == storage::Type::Text) {
			return resv->selectByQuery(storage::Query::Select(field->getName(), cmp, cmpStr));
		} else if (field->getType() == storage::Type::Boolean) {
			if (valid::validateNumber(cmpStr)) {
				return resv->selectByQuery(storage::Query::Select(field->getName(), cmp, (uint64_t)apr_strtoi64(cmpStr.c_str(), nullptr, 10), 0));
			} else if (cmpStr == "t" || cmpStr == "true") {
				return resv->selectByQuery(storage::Query::Select(field->getName(), cmp, 1, 0));
			} else if (cmpStr == "f" || cmpStr == "false") {
				return resv->selectByQuery(storage::Query::Select(field->getName(), cmp, 0, 0));
			}
		} else if (valid::validateNumber(cmpStr) && storage::checkIfComparationIsValid(field->getType(), cmp)) {
			return resv->selectByQuery(storage::Query::Select(field->getName(), cmp, (uint64_t)apr_strtoi64(cmpStr.c_str(), nullptr, 10), 0));
		} else {
			messages::error("ResourceResolver", "invalid 'select' query");
			return false;
		}
	}

	if (path.size() < valuesRequired || !storage::checkIfComparationIsValid(field->getType(), cmp)) {
		messages::error("ResourceResolver", "invalid 'select' query");
		return false;
	}

	if (valuesRequired == 1) {
		String value(std::move(path.back())); path.pop_back();
		if (field->getType() == storage::Type::Text) {
			return resv->selectByQuery(storage::Query::Select(field->getName(), cmp, value));
		} else if (valid::validateNumber(value)) {
			return resv->selectByQuery(storage::Query::Select(field->getName(), cmp, (uint64_t)apr_strtoi64(value.c_str(), nullptr, 10), 0));
		} else {
			messages::error("ResourceResolver", "invalid 'select' query");
			return false;
		}
	}

	if (valuesRequired == 2) {
		String value1(std::move(path.back())); path.pop_back();
		String value2(std::move(path.back())); path.pop_back();
		if (valid::validateNumber(value1) && valid::validateNumber(value2)) {
			return resv->selectByQuery(storage::Query::Select(field->getName(), cmp,
					(uint64_t)apr_strtoi64(value1.c_str(), nullptr, 10),
					(uint64_t)apr_strtoi64(value2.c_str(), nullptr, 10)));
		}
	}

	return false;
}

static bool getOrderResource(storage::Resolver *resv, Vector<String> &path) {
	if (path.size() < 1) {
		messages::error("ResourceResolver", "invalid 'order' query");
		return false;
	}

	auto field = resv->getSchemeField(path.back());
	if (!field || !field->isIndexed()) {
		messages::error("ResourceResolver", "invalid 'order' query");
		return false;
	}
	path.pop_back();

	storage::Ordering ord = storage::Ordering::Ascending;
	if (!path.empty()) {
		if (path.back() == "asc" ) {
			ord = storage::Ordering::Ascending;
			path.pop_back();
		} else if (path.back() == "desc") {
			ord = storage::Ordering::Descending;
			path.pop_back();
		}
	}

	if (!path.empty()) {
		auto &n = path.back();
		if (valid::validateNumber(n)) {
			if (resv->order(field->getName(), ord)) {
				return resv->limit((uint64_t)apr_strtoi64(n.c_str(), nullptr, 10));
			} else {
				return false;
			}
		}
	}

	return resv->order(field->getName(), ord);
}

static bool getLimitResource(storage::Resolver *resv, Vector<String> &path, bool &isSingleObject) {
	if (path.size() < 1) {
		messages::error("ResourceResolver", "invalid 'limit' query");
		return false;
	}

	String value(std::move(path.back())); path.pop_back();
	if (valid::validateNumber(value)) {
		auto val = (size_t)apr_strtoi64(value.c_str(), nullptr, 10);
		if (val == 1) {
			isSingleObject = true;
		}
		return resv->limit(val);
	} else {
		messages::error("ResourceResolver", "invalid 'limit' query");
		return false;
	}
}

static bool getOffsetResource(storage::Resolver *resv, Vector<String> &path) {
	if (path.size() < 1) {
		messages::error("ResourceResolver", "invalid 'offset' query");
		return false;
	}

	String value(std::move(path.back())); path.pop_back();
	if (valid::validateNumber(value)) {
		return resv->offset((size_t)apr_strtoi64(value.c_str(), nullptr, 10));
	} else {
		messages::error("ResourceResolver", "invalid 'offset' query");
		return false;
	}
}

static bool getFirstResource(storage::Resolver *resv, Vector<String> &path, bool &isSingleObject) {
	if (path.size() < 1) {
		messages::error("ResourceResolver", "invalid 'first' query");
		return false;
	}

	auto field = resv->getSchemeField(path.back());
	if (!field || !field->isIndexed()) {
		messages::error("ResourceResolver", "invalid 'first' query");
		return false;
	}
	path.pop_back();

	if (!path.empty()) {
		if (valid::validateNumber(path.back())) {
			size_t val = (size_t)apr_strtoi64(path.back().c_str(), nullptr, 10);
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

static bool getLastResource(storage::Resolver *resv, Vector<String> &path, bool &isSingleObject) {
	if (path.size() < 1) {
		messages::error("ResourceResolver", "invalid 'last' query");
		return false;
	}

	auto field = resv->getSchemeField(path.back());
	if (!field || !field->isIndexed()) {
		messages::error("ResourceResolver", "invalid 'last' query");
		return false;
	}
	path.pop_back();

	if (!path.empty()) {
		if (valid::validateNumber(path.back())) {
			size_t val = (size_t)apr_strtoi64(path.back().c_str(), nullptr, 10);
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

static Resource *parseResource(storage::Resolver *resv, Vector<String> &path) {
	bool isSingleObject = false;
	while (!path.empty()) {
		String filter(std::move(path.back()));
		path.pop_back();

		if (!isSingleObject) {
			if (filter.compare(0, 2, "id") == 0 && !isSingleObject) {
				if (uint64_t id = (uint64_t) apr_atoi64(filter.data() + 2)) {
					if (!resv->selectById(id)) {
						return nullptr;
					}
					isSingleObject = true;
				} else {
					return nullptr;
				}
			} else if (filter.compare(0, 6, "named-") == 0 && !isSingleObject) {
				String name = filter.substr(6);
				if (valid::validateIdentifier(name)) {
					if (!resv->selectByAlias(name)) {
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
			} else if (filter == "order") {
				if (!getOrderResource(resv, path)) {
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
			} else {
				messages::error("ResourceResolver", "Invalid query", data::Value(filter));
				return nullptr;
			}
		} else {
			if (filter == "offset" && !path.empty() && valid::validateNumber(path.back())) {
				if (resv->offset((size_t)apr_strtoi64(path.back().c_str(), nullptr, 10))) {
					path.pop_back();
					continue;
				}
			}
			auto f = resv->getSchemeField(filter);
			if (!f) {
				messages::error("ResourceResolver", "No such field", data::Value(filter));
				return nullptr;
			}

			auto type = f->getType();
			if (type == storage::Type::File || type == storage::Type::Image || type == storage::Type::Array) {
				isSingleObject = true;
				if (!resv->getField(filter, f)) {
					return nullptr;
				} else {
					return resv->getResult();
				}
			} else if (type == storage::Type::Object) {
				isSingleObject = true;
				if (!resv->getObject(f)) {
					return nullptr;
				}
			} else if (type == storage::Type::Set) {
				isSingleObject = false;
				if (!resv->getSet(f)) {
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

static Resource *getResolvedResource(storage::Resolver *resv, Vector<String> &path) {
	if (path.empty()) {
		return resv->getResult();
	}
	return parseResource(resv, path);
}

Resource *Resource::resolve(storage::Adapter *a, storage::Scheme *scheme, const String &path, const data::TransformMap *map) {
	data::Value tmp;
	return resolve(a, scheme, path, tmp, map);
}

Resource *Resource::resolve(storage::Adapter *a, storage::Scheme *scheme, const String &path, data::Value & sub, const data::TransformMap *map) {
	auto pathVec = parsePath(path);
	auto resolver = a->createResolver(scheme, map);

	if (sub.isDictionary() && !sub.empty()) {
		for (auto &it : sub.asDict()) {
			if (auto f = resolver->getSchemeField(it.first)) {
				if (f->isIndexed()) {
					switch (f->getType()) {
					case storage::Type::Integer:
					case storage::Type::Boolean:
					case storage::Type::Object:
						resolver->selectByQuery(
								storage::Query::Select(it.first, storage::Comparation::Equal, it.second.getInteger(), 0));
						break;
					case storage::Type::Text:
						resolver->selectByQuery(
								storage::Query::Select(it.first, storage::Comparation::Equal, it.second.getString()));
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
			resolver->selectById(id);
		}
	} else if (sub.isString()) {
		auto & str = sub.getString();
		if (!str.empty()) {
			resolver->selectByAlias(str);
		}
	}

	return getResolvedResource(resolver, pathVec);
}

NS_SA_END
