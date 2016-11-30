// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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

#include "Define.h"
#include "DatabaseResolver.h"
#include "Resource.h"
#include "StorageScheme.h"
#include "StorageFile.h"
#include "SPFilesystem.h"

NS_SA_EXT_BEGIN(database)

data::Value Resolver::Subquery::encode() {
	data::Value ret;
	ret.setString(scheme->getName(), "scheme");
	if (oid != 0) {
		ret.setInteger(oid, "oid");
	}
	if (!alias.empty()) {
		ret.setString(alias, "alias");
	}
	if (all) {
		ret.setBool(all, "all");
	}
	if (subquery) {
		ret.setValue(subquery->encode(), "subquery");
	}
	if (!subqueryField.empty()) {
		ret.setString(subqueryField, "subqueryField");
	}
	return ret;
}

Resolver::Resolver(Handle *h, Scheme *scheme, const data::TransformMap *map)
: storage::Resolver(scheme, map), _handle(h) {
	_type = Objects;
}
Resolver::~Resolver() { }

bool Resolver::selectById(uint64_t oid) {
	if (_type == Objects) {
		if (_subquery) {
			_subquery->oid = oid;
			return true;
		} else {
			_subquery = new Subquery(_scheme);
			_subquery->oid = oid;
			return true;
		}
	}
	messages::error("ResourceResolver", "Invalid 'select by id', invalid resource type");
	return false;
}

bool Resolver::selectByAlias(const String &str) {
	if (_type == Objects) {
		if (_subquery) {
			_subquery->alias = str;
			return true;
		} else {
			_subquery = new Subquery(_scheme);
			_subquery->alias = str;
			return true;
		}
	}
	messages::error("ResourceResolver", "Invalid 'select by alias', invalid resource type");
	return false;
}
bool Resolver::selectByQuery(Query::Select &&q) {
	if (_type == Objects) {
		if (_subquery) {
			if (_subquery->alias.empty() && _subquery->oid == 0) {
				_subquery->where.emplace_back(std::move(q));
				return true;
			} else {
				messages::error("ResourceResolver", "Invalid 'select by query', objects already selected");
				return false;
			}
		} else {
			_subquery = new Subquery(_scheme);
			_subquery->where.emplace_back(std::move(q));
			return true;
		}
	}
	messages::error("ResourceResolver", "Invalid 'select by query', invalid resource type");
	return false;
}

bool Resolver::order(const String &f, storage::Ordering o) {
	if (_type == Objects) {
		if (_subquery) {
			if (_subquery->orderField.empty()) {
				_subquery->orderField = f;
				_subquery->ordering = o;
				return true;
			}
			messages::error("ResourceResolver", "Invalid 'order', already specified");
			return false;
		} else {
			_subquery = new Subquery(_scheme);
			_subquery->orderField = f;
			_subquery->ordering = o;
			return true;
		}
	}
	messages::error("ResourceResolver", "Invalid 'order', invalid resource type");
	return false;
}

bool Resolver::first(const String &f, size_t v) {
	if (_type == Objects) {
		if (_subquery) {
			if (_subquery->orderField.empty() && _subquery->limit > v && _subquery->offset == 0) {
				_subquery->orderField = f;
				_subquery->ordering = storage::Ordering::Ascending;
				_subquery->limit = v;
				return true;
			}
			messages::error("ResourceResolver", "Invalid 'first', ordering, limit or offset already specified");
			return false;
		} else {
			_subquery = new Subquery(_scheme);
			_subquery->orderField = f;
			_subquery->ordering = storage::Ordering::Ascending;
			_subquery->limit = v;
			return true;
		}
	}
	messages::error("ResourceResolver", "Invalid 'first', invalid resource type");
	return false;
}
bool Resolver::last(const String &f, size_t v) {
	if (_type == Objects) {
		if (_subquery) {
			if (_subquery->orderField.empty() && _subquery->limit > v && _subquery->offset == 0) {
				_subquery->orderField = f;
				_subquery->ordering = storage::Ordering::Descending;
				_subquery->limit = v;
				return true;
			}
			messages::error("ResourceResolver", "Invalid 'last', ordering, limit or offset already specified");
			return false;
		} else {
			_subquery = new Subquery(_scheme);
			_subquery->orderField = f;
			_subquery->ordering = storage::Ordering::Descending;
			_subquery->limit = v;
			return true;
		}
	}
	messages::error("ResourceResolver", "Invalid 'last', invalid resource type");
	return false;
}

bool Resolver::limit(size_t limit) {
	if (_type == Objects) {
		if (_subquery) {
			if (_subquery->limit == maxOf<size_t>()) {
				_subquery->limit = limit;
				return true;
			}
			messages::error("ResourceResolver", "Invalid 'limit', already specified");
			return false;
		} else {
			_subquery = new Subquery(_scheme);
			_subquery->limit = limit;
			return true;
		}
	}
	messages::error("ResourceResolver", "Invalid 'limit', invalid resource type");
	return false;
}
bool Resolver::offset(size_t offset) {
	if (_type == Objects) {
		if (_subquery) {
			if (_subquery->offset == 0) {
				_subquery->offset = offset;
				return true;
			}
			messages::error("ResourceResolver", "Invalid 'offset', already specified");
			return false;
		} else {
			_subquery = new Subquery(_scheme);
			_subquery->offset = offset;
			return true;
		}
	}
	messages::error("ResourceResolver", "Invalid 'offset', invalid resource type");
	return false;
}

bool Resolver::getObject(const Field *f) {
	if (_type == Objects) {
		auto fo = static_cast<const storage::FieldObject *>(f->getSlot());
		auto scheme = fo->scheme;
		auto sq = new Subquery(scheme);
		sq->subquery = _subquery;

		_scheme = scheme;
		_subquery = sq;
		return true;
	}
	messages::error("ResourceResolver", "Invalid 'getObject', invalid resource type");
	return false;
}

bool Resolver::getSet(const Field *f) {
	if (_type == Objects) {
		auto fo = static_cast<const storage::FieldObject *>(f->getSlot());
		auto scheme = fo->scheme;
		auto sq = new Subquery(scheme);
		sq->subquery = _subquery;
		sq->subqueryField = f->getName();

		_scheme = scheme;
		_subquery = sq;
		return true;
	}
	messages::error("ResourceResolver", "Invalid 'getSet', invalid resource type");
	return false;
}

bool Resolver::getField(const String &str, const Field *f) {
	if ((_type == Objects) && f->getType() == storage::Type::Array) {
		_resource = new ResourceArray(_scheme, _handle, _subquery, f, str);
		_type = Array;
		return true;
	}

	if (_type == Objects && (f->getType() == storage::Type::File || f->getType() == storage::Type::Image)) {
		_resource = new ResourceFile(_scheme, _handle, _subquery, f, str);
		_type = File;
		return true;
	}
	messages::error("ResourceResolver", "Invalid 'getField', invalid resource type");
	return false;
}

bool Resolver::getAll() {
	if (_type == Objects && _subquery && !_subquery->all) {
		_subquery->all = true;
		return true;
	} else if (_type == Objects && !_subquery && !_all) {
		_all = true;
		return true;
	}
	messages::error("ResourceResolver", "Invalid 'getAll', invalid resource type or already enabled");
	return false;
}

Resource *Resolver::getResult() {
	if (_type == Objects) {
		if (!_subquery) {
			// reslist
			_subquery = new Subquery(_scheme);
			_subquery->all = _all;
			return new ResourceReslist(_scheme, _handle, _subquery);
		} else {
			// check for reference set
			if (!_subquery->subqueryField.empty() && _subquery->subquery && _subquery->subquery->scheme) {
				auto f = _subquery->subquery->scheme->getField(_subquery->subqueryField);
				if (f->isReference() && !_subquery->all) {
					return new ResourceRefSet(_scheme, _handle, _subquery);
				}
			}
			if (_subquery->oid > 0 || !_subquery->alias.empty() || _subquery->limit == 0) {
				return new ResourceObject(_scheme, _handle, _subquery);
			}
		}
		return new ResourceSet(_scheme, _handle, _subquery);
	}
	return _resource;
}

void Resolver::writeSubqueryContent(apr::ostringstream &stream, Handle *h, Scheme *scheme, Subquery *query) {
	auto currentScheme = (query?query->scheme:scheme);
	if (!query) {
		return;
	}

	bool first = true;
	if (query->subquery) {
		// check if we can optimize query
		bool written = false;
		auto squery = query->subquery;
		auto nscheme = query->subquery->scheme;
		if (nscheme) {
			auto nfield = nscheme->getField(query->subqueryField);
			if (nfield && !nfield->isReference()) {
				auto ref = static_cast<const storage::FieldObject *>(nfield->getSlot());
				auto other = nscheme->getForeignLink(ref);
				if (other) {
					if (squery->subquery == nullptr && squery->oid != 0) {
						stream << " WHERE \"" << other->getName() << "\"=" << squery->oid;
					} else {
						stream << ", (";
						writeSubquery(stream, h, squery->scheme, squery, "");
						stream << ") subquery WHERE "<< query->scheme->getName() << ".\"" << other->getName() << "\"=subquery.id";
					}
					written = true;
					first = false;
				}
			}
		}
		if (!written) {
			stream << ", (";
			writeSubquery(stream, h, query->scheme, query->subquery, query->subqueryField);
			stream << ") subquery WHERE " << query->scheme->getName() << ".__oid=subquery.id ";
			first = false;
		}
	}

	if (query->oid) {
		if (!first) { stream << " AND "; } else { stream << " WHERE "; first = false; }
		stream << query->scheme->getName() << ".__oid=" << query->oid;
	}
	if (!query->alias.empty()) {
		if (!first) { stream << " AND "; } else { stream << " WHERE "; first = false; }
		stream << "(";
		h->writeAliasRequest(stream, scheme, query->alias);
		stream << ")";
	}
	if (!query->where.empty()) {
		if (!first) { stream << " AND "; } else { stream << " WHERE "; first = false; }
		stream << "(";
		h->writeQueryRequest(stream, scheme, query->where);
		stream << ")";
	}
	if (!query->orderField.empty()) {
		stream << " ORDER BY " << currentScheme->getName() << ".\"" << query->orderField << "\" ";
		switch (query->ordering) {
		case storage::Ordering::Ascending: stream << "ASC"; break;
		case storage::Ordering::Descending: stream << "DESC NULLS LAST"; break;
		}
	}

	if (query->limit != maxOf<size_t>()) {
		stream << " LIMIT " << query->limit;
	}

	if (query->offset != 0) {
		stream << " OFFSET " << query->offset;
	}
}

void Resolver::writeSubquery(apr::ostringstream &stream, Handle *h, Scheme *scheme, Subquery *query, const String &field, bool idOnly) {
	if (field.empty()) {
		auto currentScheme = (query?query->scheme:scheme);
		if (idOnly) {
			stream << "SELECT " << currentScheme->getName() << ".__oid AS \"id\" FROM " << currentScheme->getName();
		} else {
			stream << "SELECT " << currentScheme->getName() << ".* FROM " << currentScheme->getName();
		}
		writeSubqueryContent(stream, h, scheme, query);
	} else {
		auto f = query->scheme->getField(field);
		if (f->isReference()) {
			stream << "SELECT list.\""<< scheme->getName() << "_id\" AS \"id\" FROM ";
			stream << query->scheme->getName() << "_f_" << field << " list";
			if (query->subquery == nullptr && query->oid != 0) {
				stream << " WHERE list." << query->scheme->getName() << "_id=" << query->oid;
			} else {
				stream << ", (";
				writeSubquery(stream, h, query->scheme, query, "");
				stream << ") subquery WHERE list." << query->scheme->getName() << "_id=subquery.id";
			}
		} else {
			auto ref = static_cast<const storage::FieldObject *>(f->getSlot());
			auto other = query->scheme->getForeignLink(ref);

			stream << "SELECT list.\"__oid\" AS \"id\" FROM ";
			stream << ref->scheme->getName() << " list";
			if (query->subquery == nullptr && query->oid != 0) {
				stream << " WHERE list.\"" << other->getName() << "\"=" << query->oid;
			} else {
				stream << ", (";
				writeSubquery(stream, h, query->scheme, query, "");
				stream << ") subquery WHERE list.\"" << other->getName() << "\"=subquery.id";
			}
		}
	}
}

void Resolver::writeFileSubquery(apr::ostringstream &stream, Handle *h, Scheme *scheme, Subquery *query, const String &field) {
	auto currentScheme = (query?query->scheme:scheme);
	stream << "SELECT " << currentScheme->getName() << ".\"" << field << "\" AS \"id\" FROM " << currentScheme->getName();
	writeSubqueryContent(stream, h, scheme, query);
}

NS_SA_EXT_END(database)
