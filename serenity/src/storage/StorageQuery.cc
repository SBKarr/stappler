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
#include "StorageQuery.h"
#include "StorageScheme.h"
#include "StorageFile.h"

NS_SA_EXT_BEGIN(storage)

static const Field *getFieldFormMap(const Map<String, Field> &fields, const String &name) {
	auto it = fields.find(name);
	if (it != fields.end()) {
		return &it->second;
	}
	return nullptr;
}

static const Query::FieldsVec *getFieldsVec(const Query::FieldsVec *vec, const String &name) {
	if (vec) {
		for (auto &it : *vec) {
			if (it.name == name) {
				return it.fields.empty() ? nullptr : &it.fields;
			}
		}
	}
	return nullptr;
}

static void QueryFieldResolver_resolveByName(Set<const Field *> &ret, const Map<String, Field> &fields, const String &name) {
	if (!name.empty() && name.front() == '$') {
		auto res = Query::decodeResolve(name);
		switch (res) {
		case query::Resolve::Files:
			for (auto &it : fields) {
				if (it.second.isFile()) {
					ret.emplace(&it.second);
				}
			}
			break;
		case query::Resolve::Sets:
			for (auto &it : fields) {
				if (it.second.getType() == Type::Set) {
					ret.emplace(&it.second);
				}
			}
			break;
		case query::Resolve::Objects:
			for (auto &it : fields) {
				if (it.second.getType() == Type::Object) {
					ret.emplace(&it.second);
				}
			}
			break;
		case query::Resolve::Arrays:
			for (auto &it : fields) {
				if (it.second.getType() == Type::Array) {
					ret.emplace(&it.second);
				}
			}
			break;
		case query::Resolve::Basics:
			for (auto &it : fields) {
				if (it.second.isSimpleLayout() && !it.second.isDataLayout()) {
					ret.emplace(&it.second);
				}
			}
			break;
		case query::Resolve::Defaults:
			for (auto &it : fields) {
				if (it.second.isSimpleLayout()) {
					ret.emplace(&it.second);
				}
			}
			break;
		case query::Resolve::All:
			for (auto &it : fields) {
				ret.emplace(&it.second);
			}
			break;
		case query::Resolve::Ids:
			for (auto &it : fields) {
				if (it.second.isFile() || it.second.getType() == Type::Object) {
					ret.emplace(&it.second);
				}
			}
			break;
		default: break;
		}
	} else {
		if (auto field = getFieldFormMap(fields, name)) {
			ret.emplace(field);
		}
	}
}

QueryFieldResolver::QueryFieldResolver() : root(nullptr) { }

QueryFieldResolver::QueryFieldResolver(const Scheme &scheme, const Query &query, const Vector<String> &extraFields) {
	root = new Data{&scheme, &scheme.getFields(), &query.getIncludeFields(), &query.getExcludeFields()};
	doResolve(root, extraFields, 0, query.getResolveDepth());
}

const Field *QueryFieldResolver::getField(const String &name) const {
	if (root && root->fields) {
		auto it = root->fields->find(name);
		if (it != root->fields->end()) {
			return &it->second;
		}
	}
	return nullptr;
}

const Scheme *QueryFieldResolver::getScheme() const {
	if (root) {
		return root->scheme;
	}
	return nullptr;
}
const Map<String, Field> *QueryFieldResolver::getFields() const {
	if (root) {
		return root->fields;
	}
	return nullptr;
}

QueryFieldResolver::Meta QueryFieldResolver::getMeta() const {
	return root ? root->meta : Meta::None;
}

const Set<const Field *> &QueryFieldResolver::getResolves() const {
	return root->resolved;
}

const Query::FieldsVec *QueryFieldResolver::getIncludeVec() const {
	if (root) {
		return root->include;
	}
	return nullptr;
}

const Query::FieldsVec *QueryFieldResolver::getExcludeVec() const {
	if (root) {
		return root->exclude;
	}
	return nullptr;
}

QueryFieldResolver QueryFieldResolver::next(const String &f) const {
	if (root) {
		auto it = root->next.find(f);
		if (it != root->next.end()) {
			return QueryFieldResolver(&it->second);
		}
	}
	return QueryFieldResolver();
}

QueryFieldResolver::operator bool () const {
	return root && root->scheme != nullptr && root->fields != nullptr;
}

QueryFieldResolver::QueryFieldResolver(Data *data) : root(data) { }

void QueryFieldResolver::doResolve(Data *data, const Vector<String> &extra, uint16_t depth, uint16_t max) {
	if (!data->fields) {
		return;
	}

	if (data->include && !data->include->empty()) {
		for (const Query::Field &it : *data->include) {
			if (it.name == "$meta") {
				for (auto &f_it : it.fields) {
					if (f_it.name == "time") {
						data->meta |= Meta::Time;
					} else if (f_it.name == "action") {
						data->meta |= Meta::Action;
					} else if (f_it.name == "view") {
						data->meta |= Meta::View;
					}
				}
			} else {
				QueryFieldResolver_resolveByName(data->resolved, *data->fields, it.name);
			}
		}
	}

	if (!data->include || data->include->empty() || (data->include->size() == 1 && data->include->front().name == "$meta")) {
		for (auto &it : *data->fields) {
			if (it.second.isSimpleLayout()) {
				data->resolved.emplace(&it.second);
			}
		}
	}

	if (!extra.empty()) {
		for (auto &it : extra) {
			QueryFieldResolver_resolveByName(data->resolved, *data->fields, it);
		}
	}

	if (data->exclude) {
		for (const Query::Field &it : *data->exclude) {
			if (it.fields.empty()) {
				if (auto field = getFieldFormMap(*data->fields, it.name)) {
					data->resolved.erase(field);
				}
			}
		}
	}

	for (const Field *it : data->resolved) {
		const Scheme *scheme = nullptr;
		const Map<String, Field> *fields = nullptr;

		if (auto s = it->getForeignScheme()) {
			scheme = s;
			fields = &s->getFields();
		} else if (it->getType() == Type::Extra) {
			scheme = data->scheme;
			fields = &static_cast<const FieldExtra *>(it->getSlot())->fields;
		} else if (it->isFile()) {
			scheme = storage::File::getScheme();
			fields = &scheme->getFields();
		}
		if (scheme && fields && depth < max) {
			auto n_it = data->next.emplace(it->getName(), Data{scheme, fields, getFieldsVec(data->include, it->getName()), getFieldsVec(data->exclude, it->getName())}).first;
			doResolve(&n_it->second, extra, depth + 1, max);
		}
	}
}

const Set<const Field *> &QueryList::Item::getQueryFields() const {
	return fields.getResolves();
}

QueryList::QueryList(const Scheme *scheme) {
	queries.reserve(4);
	queries.emplace_back(Item{scheme});
}

bool QueryList::selectById(const Scheme *scheme, uint64_t id) {
	Item &b = queries.back();
	if (b.scheme == scheme && b.query.getSelectIds().empty() && b.query.getSelectAlias().empty()) {
		b.query.select(id);
		return true;
	}
	return false;
}

bool QueryList::selectByName(const Scheme *scheme, const String &f) {
	Item &b = queries.back();
	if (b.scheme == scheme && b.query.getSelectIds().empty() && b.query.getSelectAlias().empty()) {
		b.query.select(f);
		return true;
	}
	return false;
}

bool QueryList::selectByQuery(const Scheme *scheme, Query::Select &&f) {
	Item &b = queries.back();
	if (b.scheme == scheme && (b.query.empty() || !b.query.getSelectList().empty())) {
		b.query.select(move(f));
		return true;
	}
	return false;
}

bool QueryList::order(const Scheme *scheme, const String &f, Ordering o) {
	Item &b = queries.back();
	if (b.scheme == scheme && b.query.getOrderField().empty()) {
		b.query.order(f, o);
		return true;
	}

	return false;
}
bool QueryList::first(const Scheme *scheme, const String &f, size_t v) {
	Item &b = queries.back();
	if (b.scheme == scheme && b.query.getOrderField().empty() && b.query.getLimitValue() > v && b.query.getOffsetValue() == 0) {
		b.query.order(f, Ordering::Ascending);
		b.query.limit(v, 0);
		return true;
	}

	return false;
}
bool QueryList::last(const Scheme *scheme, const String &f, size_t v) {
	Item &b = queries.back();
	if (b.scheme == scheme && b.query.getOrderField().empty() && b.query.getLimitValue() > v && b.query.getOffsetValue() == 0) {
		b.query.order(f, Ordering::Descending);
		b.query.limit(v, 0);
		return true;
	}

	return false;
}
bool QueryList::limit(const Scheme *scheme, size_t limit) {
	Item &b = queries.back();
	if (b.scheme == scheme && b.query.getLimitValue() > limit) {
		b.query.limit(limit);
		return true;
	}

	return false;
}
bool QueryList::offset(const Scheme *scheme, size_t offset) {
	Item &b = queries.back();
	if (b.scheme == scheme && b.query.getOffsetValue() == 0) {
		b.query.offset(offset);
		return true;
	}

	return false;
}

bool QueryList::setAll() {
	Item &b = queries.back();
	if (!b.all) {
		b.all = true;
		return true;
	}
	return false;
}

bool QueryList::setField(const Scheme *scheme, const Field *field) {
	if (queries.size() < 4) {
		Item &prev = queries.back();
		prev.field = field;
		queries.emplace_back(Item{scheme, prev.scheme->getForeignLink(*prev.field)});
		return true;
	}
	return false;
}

bool QueryList::setProperty(const Field *field) {
	queries.back().query.include(Query::Field(String(field->getName())));
	return true;
}

bool QueryList::isAll() const {
	return queries.back().all;
}

bool QueryList::isRefSet() const {
	return (queries.size() > 1 && !queries.back().ref && !queries.back().all);
}

bool QueryList::isObject() const {
	const Query &b = queries.back().query;
	return b.getSelectIds().size() == 1 || !b.getSelectAlias().empty() || b.getLimitValue() == 1;
}
bool QueryList::isView() const {
	if (queries.size() > 1 && queries.at(queries.size() - 2).field) {
		return queries.at(queries.size() - 2).field->getType() == Type::View;
	} else if (!queries.empty() && queries.back().field) {
		return queries.back().field->getType() == Type::View;
	}
	return false;
}
bool QueryList::empty() const {
	return queries.size() == 1 && queries.front().query.empty();
}

bool QueryList::isDeltaApplicable() const {
	const QueryList::Item &item = getItems().back();
	if ((queries.size() == 1 || (isView() && queries.size() == 2 && queries.front().query.getSelectIds().size() == 1))
			&& !item.query.hasSelectName() && !item.query.hasSelectList()) {
		return true;
	}
	return false;
}

size_t QueryList::size() const {
	return queries.size();
}

const Scheme *QueryList::getPrimaryScheme() const {
	return queries.front().scheme;
}
const Scheme *QueryList::getSourceScheme() const {
	if (queries.size() >= 2) {
		return queries.at(queries.size() - 2).scheme;
	}
	return getPrimaryScheme();
}
const Scheme *QueryList::getScheme() const {
	return queries.back().scheme;
}

const Field *QueryList::getField() const {
	if (queries.size() >= 2) {
		return queries.at(queries.size() - 2).field;
	}
	return nullptr;
}

const Query &QueryList::getTopQuery() const {
	return queries.back().query;
}

const Vector<QueryList::Item> &QueryList::getItems() const {
	return queries;
}

void QueryList::decodeSelect(const Scheme &scheme, Query &q, const data::Value &val) {
	if (val.isInteger()) {
		q.select(val.asInteger());
	} else if (val.isString()) {
		q.select(val.getString());
	} else if (val.isArray() && val.size() > 0) {
		auto cb = [&] (const data::Value &iit) {
			if (iit.isArray() && iit.size() >= 3) {
				auto field = iit.getValue(0).asString();
				if (auto f = scheme.getField(field)) {
					if (f->isIndexed()) {
						auto cmp = iit.getValue(1).asString();
						auto d = query::decodeComparation(cmp);
						auto &val = iit.getValue(2);
						if (d.second && iit.size() >= 4) {
							auto &val2 = iit.getValue(4);
							q.select(field, d.first, data::Value(val), val2);
						} else {
							q.select(field, d.first, data::Value(val), data::Value());
						}
					} else {
						messages::error("QueryList", "Invalid field for select", data::Value(field));
					}
				} else {
					messages::error("QueryList", "Invalid field for select", data::Value(field));
				}
			}
		};

		if (val.getValue(0).isString()) {
			cb(val);
		} else if (val.getValue(0).isArray()) {
			for (auto &iit : val.asArray()) {
				cb(iit);
			}
		}
	}
}

void QueryList::decodeOrder(const Scheme &scheme, Query &q, const String &str, const data::Value &val) {
	String field;
	Ordering ord = Ordering::Ascending;
	size_t limit = maxOf<size_t>();
	size_t offset = 0;
	if (val.isArray() && val.size() > 0) {
		size_t target = 1;
		auto size = val.size();
		field = val.getValue(0).asString();
		if (str == "order") {
			if (size > target) {
				auto dir = val.getValue(target).asString();
				if (dir == "desc") {
					ord = Ordering::Descending;
				}
				++ target;
			}
		} else if (str == "last") {
			ord = Ordering::Descending;
			limit = 1;
		} else if (str == "first") {
			ord = Ordering::Ascending;
			limit = 1;
		}

		if (size > target) {
			limit = val.getInteger(target);
			++ target;
			if (size > target) {
				offset = val.getInteger(target);
				++ target;
			}
		}
	} else if (val.isString()) {
		field = val.asString();
		if (str == "last") {
			ord = Ordering::Descending;
			limit = 1;
		} else if (str == "first") {
			ord = Ordering::Ascending;
			limit = 1;
		}
	}
	if (!field.empty()) {
		if (auto f = scheme.getField(field)) {
			if (f->isIndexed()) {
				q.order(field, ord);
				if (limit != maxOf<size_t>() && !q.hasLimit()) {
					q.limit(limit);
				}
				if (offset != 0 && !q.hasOffset()) {
					q.offset(offset);
				}
				return;
			}
		}
	}
	messages::error("QueryList", "Invalid field for ordering", data::Value(field));
}

const Field *QueryList_getField(const Scheme &scheme, const Field *f, const String &name) {
	if (!f) {
		return scheme.getField(name);
	} else if (f->getType() == Type::Extra) {
		auto slot = static_cast<const FieldExtra *>(f->getSlot());
		auto it = slot->fields.find(name);
		if (it != slot->fields.end()) {
			return &it->second;
		}
	}
	return nullptr;
}

static uint16_t QueryList_emplaceItem(const Scheme &scheme, const Field *f, Vector<Query::Field> &dec, const String &name) {
	if (!name.empty() && name.front() == '$') {
		dec.emplace_back(String(name));
		return 1;
	}
	if (!f) {
		if (auto field = scheme.getField(name)) {
			dec.emplace_back(String(name));
			return (field->isFile() || field->getForeignScheme()) ? 1 : 0;
		}
	} else if (f->getType() == Type::Extra) {
		auto slot = static_cast<const FieldExtra *>(f->getSlot());
		if (slot->fields.find(name) != slot->fields.end()) {
			dec.emplace_back(String(name));
			return 0;
		}
	}
	if (!f) {
		messages::error("QueryList", toString("Invalid field name in 'include' for scheme ", scheme.getName()), data::Value(name));
	} else {
		messages::error("QueryList",
				toString("Invalid field name in 'include' for scheme ", scheme.getName(), " and field ", f->getName()), data::Value(name));
	}
	return 0;
}

static uint16_t QueryList_decodeIncludeItem(const Scheme &scheme, const Field *f, Vector<Query::Field> &dec, const data::Value &val) {
	uint16_t depth = 0;
	if (val.isString()) {
		return QueryList_emplaceItem(scheme, f, dec, val.getString());
	} else if (val.isArray()) {
		for (auto &iit : val.asArray()) {
			if (iit.isString()) {
				depth = std::max(depth, QueryList_emplaceItem(scheme, f, dec, iit.getString()));
			}
		}
	}
	return depth;
}

static void QueryList_decodeMeta(Vector<Query::Field> &dec, const data::Value &val) {
	if (val.isArray()) {
		for (auto &it : val.asArray()) {
			auto str = it.asString();
			if (!str.empty()) {
				dec.emplace_back(move(str));
			}
		}
	} else if (val.isDictionary()) {
		for (auto &it : val.asDict()) {
			dec.emplace_back(String(it.first));
			QueryList_decodeMeta(dec.back().fields, it.second);
		}
	} else if (val.isString()) {
		dec.emplace_back(val.asString());
	}
}

static uint16_t QueryList_decodeInclude(const Scheme &scheme, const Field *f, Vector<Query::Field> &dec, const data::Value &val) {
	uint16_t depth = 0;
	if (val.isDictionary()) {
		for (auto &it : val.asDict()) {
			if (!it.first.empty()) {
				if (it.second.isBool() && it.second.asBool()) {
					QueryList_emplaceItem(scheme, f, dec, it.first);
				} else if (it.second.isArray() || it.second.isDictionary() || it.second.isString()) {
					if (it.first.front() == '$') {
						dec.emplace_back(String(it.first));
						QueryList_decodeMeta(dec.back().fields, it.second);
					} else if (auto target = QueryList_getField(scheme, f, it.first)) {
						if (auto ts = target->getForeignScheme()) {
							dec.emplace_back(String(it.first));
							depth = std::max(depth, QueryList_decodeInclude(*ts, nullptr, dec.back().fields, it.second));
						} else if (target->isFile()) {
							dec.emplace_back(String(it.first));
							depth = std::max(depth, QueryList_decodeInclude(*File::getScheme(), nullptr, dec.back().fields, it.second));
						} else {
							dec.emplace_back(String(it.first));
							depth = std::max(depth, QueryList_decodeInclude(scheme, target, dec.back().fields, it.second));
						}
					}
				}
			}
		}
		depth = (depth + 1);
	} else {
		depth = std::max(depth, QueryList_decodeIncludeItem(scheme, f, dec, val));
	}
	return depth;
}

bool QueryList::apply(const data::Value &val) {
	Item &item = queries.back();
	Query &q = item.query;
	const Scheme &scheme = *item.scheme;

	for (auto &it : val.asDict()) {
		if (it.first == "select") {
			decodeSelect(scheme, q, it.second);
		} else if (it.first == "order" || it.first == "last" || it.first == "first") {
			decodeOrder(scheme, q, it.first, it.second);
		} else if (it.first == "limit") {
			if (it.second.isInteger()) {
				q.limit(it.second.asInteger());
			}
		} else if (it.first == "offset") {
			if (it.second.isInteger()) {
				q.offset(it.second.asInteger());
			}
		} else if (it.first == "fields") {
			Vector<Query::Field> dec;
			q.depth(std::min(QueryList_decodeInclude(scheme, nullptr, dec, it.second), config::getResourceResolverMaxDepth()));
			for (auto &it : dec) {
				q.include(move(it));
			}
		} else if (it.first == "include") {
			Vector<Query::Field> dec;
			q.depth(std::min(QueryList_decodeInclude(scheme, nullptr, dec, it.second), config::getResourceResolverMaxDepth()));
			for (auto &it : dec) {
				q.include(move(it));
			}
		} else if (it.first == "exclude") {
			Vector<Query::Field> dec;
			q.depth(std::min(QueryList_decodeInclude(scheme, nullptr, dec, it.second), config::getResourceResolverMaxDepth()));
			for (auto &it : dec) {
				q.exclude(move(it));
			}
		} else if (it.first == "delta") {
			if (it.second.isString()) {
				q.delta(it.second.asString());
			} else if (it.second.isInteger()) {
				q.delta(it.second.asInteger());
			}
		} else if (it.first == "forUpdate") {
			q.forUpdate();
		}
	}

	return true;
}

void QueryList::resolve(const Vector<String> &vec) {
	Item &b = queries.back();
	b.fields = QueryFieldResolver(*b.scheme, b.query, vec);
}

uint16_t QueryList::getResolveDepth() const {
	return queries.back().query.getResolveDepth();
}

void QueryList::setResolveDepth(uint16_t d) {
	queries.back().query.depth(d);
}

void QueryList::setDelta(Time d) {
	queries.back().query.delta(d.toMicroseconds());
}

Time QueryList::getDelta() const {
	return Time::microseconds(queries.back().query.getDeltaToken());
}

const Query::FieldsVec &QueryList::getIncludeFields() const {
	return queries.back().query.getIncludeFields();
}
const Query::FieldsVec &QueryList::getExcludeFields() const {
	return queries.back().query.getExcludeFields();
}

QueryFieldResolver QueryList::getFields() const {
	return QueryFieldResolver(queries.back().fields);
}

NS_SA_EXT_END(storage)
