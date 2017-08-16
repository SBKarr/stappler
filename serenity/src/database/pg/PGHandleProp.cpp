// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2017 Roman Katuntsev <sbkarr@stappler.org>

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
#include "PGHandle.h"
#include "StorageScheme.h"
#include "StorageFile.h"

NS_SA_EXT_BEGIN(pg)

static void Handle_writeSelectSetQuery(ExecQuery &query, const Scheme &s, uint64_t oid, const Field &f) {
	if (f.isReference()) {
		auto fs = f.getForeignScheme();
		query << "SELECT t.* FROM " << s.getName() << "_f_" << f.getName() << " s, " << fs->getName() <<
				" t WHERE s." << s.getName() << "_id="<< oid << " AND t.__oid=s." << fs->getName() << "_id;";
	} else {
		auto fs = f.getForeignScheme();
		auto l = s.getForeignLink(f);
		query << "SELECT t.* FROM " << fs->getName() << " t WHERE t.\"" << l->getName() << "\"=" << oid << ";";
	}
}

data::Value Handle::getProperty(Scheme *s, uint64_t oid, const Field &f) {
	ExecQuery query;
	data::Value ret;
	switch (f.getType()) {
	case storage::Type::File:
	case storage::Type::Image:
		query << "SELECT __files.* FROM __files, " << s->getName()
				<< " t WHERE t.__oid=" << oid << " AND __files.__oid=t.\"" << f.getName() << "\";";
		ret = select(*File::getScheme(), query);
		if (ret.isArray()) {
			ret = std::move(ret.getValue(0));
		}
		break;
	case storage::Type::Array:
		query << "SELECT data FROM " << s->getName() << "_f_" << f.getName() << " WHERE " << s->getName() << "_id=" << oid << ";";
		ret = select(f, query);
		break;
	case storage::Type::Object:
		if (auto fs = f.getForeignScheme()) {
			query << "SELECT t.* FROM " << fs->getName() << " t, " << s->getName()
					<< " s WHERE t.__oid=s.\"" << f.getName() << "\" AND s.__oid=" << oid <<";";
			ret = select(*fs, query);
			if (ret.isArray()) {
				ret = std::move(ret.getValue(0));
			}
		}
		break;
	case storage::Type::Set:
		if (auto fs = f.getForeignScheme()) {
			Handle_writeSelectSetQuery(query, *s, oid, f);
			ret = select(*fs, query);
		}
		break;
	default:
		query << "SELECT " << f.getName() << " FROM " << s->getName() << " WHERE __oid=" << oid << ";";
		ret = select(*s, query);
		if (ret.isArray()) {
			ret = std::move(ret.getValue(0));
		}
		if (ret.isDictionary()) {
			ret = ret.getValue(f.getName());
		}
		break;
	}
	return ret;
}

data::Value Handle::getProperty(Scheme *s, const data::Value &d, const Field &f) {
	ExecQuery query;
	data::Value ret;
	auto oid = d.getInteger("__oid");
	switch (f.getType()) {
	case storage::Type::File:
	case storage::Type::Image:
		return storage::File::getData(this, d.getInteger(f.getName()));
		break;
	case storage::Type::Array:
		query << "SELECT data FROM " << s->getName() << "_f_" << f.getName() << " WHERE " << s->getName() << "_id=" << oid << ";";
		ret = select(f, query);
		break;
	case storage::Type::Object:
		if (auto fs = f.getForeignScheme()) {
			query << "SELECT t.* FROM " << fs->getName() << " t WHERE t.__oid=" << d.getInteger(f.getName()) << ";";
			ret = select(*fs, query);
			if (ret.isArray()) {
				ret = std::move(ret.getValue(0));
			}
		}
		break;
	case storage::Type::Set:
		if (auto fs = f.getForeignScheme()) {
			Handle_writeSelectSetQuery(query, *s, oid, f);
			ret = select(*fs, query);
		}
		break;
	default:
		return d.getValue(f.getName());
		break;
	}
	return ret;
}

data::Value Handle::setProperty(Scheme *s, uint64_t oid, const Field &f, data::Value && val) {
	ExecQuery query;
	data::Value ret;
	switch (f.getType()) {
	case storage::Type::File:
	case storage::Type::Image:
		return data::Value(); // file update should be done by scheme itself
		break;
	case storage::Type::Array:
		if (val.isArray()) {
			clearProperty(s, oid, f);
			insertIntoArray(query, *s, oid, f, val);
		}
		break;
	case storage::Type::Set:
		// not implemented
		break;
	default: {
		data::Value patch;
		patch.setValue(val, f.getName());
		patchObject(s, oid, patch);
		return val;
	}
		break;
	}
	return ret;
}
data::Value Handle::setProperty(Scheme *s, const data::Value &d, const Field &f, data::Value && val) {
	return setProperty(s, (uint64_t)d.getInteger("__oid"), f, std::move(val));
}

void Handle::clearProperty(Scheme *s, uint64_t oid, const Field &f) {
	ExecQuery query;
	data::Value ret;
	switch (f.getType()) {
	case storage::Type::Array:
		query << "DELETE FROM " << s->getName() << "_f_" << f.getName() << " WHERE " << s->getName() << "_id=" << oid << ";";
		perform(query);
		break;
	case storage::Type::Set:
		if (f.isReference()) {
			auto objField = static_cast<const storage::FieldObject *>(f.getSlot());
			if (objField->onRemove == storage::RemovePolicy::Reference) {
				query << "DELETE FROM " << s->getName() << "_f_" << f.getName() << " WHERE " << s->getName() << "_id=" << oid << ";";
				perform(query);
			} else {
				performInTransaction([&] {
					auto obj = static_cast<const storage::FieldObject *>(f.getSlot());

					auto & source = s->getName();
					auto & target = obj->scheme->getName();

					query << "DELETE FROM " << target << " WHERE __oid IN (SELECT " << target << "_id FROM "
							<< s->getName() << "_f_" << f.getName() << " WHERE "<< source << "_id=" << oid << ");";
					perform(query);

					query.clear();
					query << "DELETE FROM " << s->getName() << "_f_" << f.getName() << " WHERE " << s->getName() << "_id=" << oid << ";";
					perform(query);
					return true;
				});
			}
		}
		break;
	default: {
		data::Value patch;
		patch.setValue(data::Value(), f.getName());
		patchObject(s, oid, patch);
	}
		break;
	}
}
void Handle::clearProperty(Scheme *s, const data::Value &d, const Field &f) {
	return clearProperty(s, (uint64_t)d.getInteger("__oid"), f);
}

data::Value Handle::appendProperty(Scheme *s, uint64_t oid, const Field &f, data::Value && val) {
	ExecQuery query;
	data::Value ret;
	switch (f.getType()) {
	case storage::Type::Array:
		insertIntoArray(query, *s, oid, f, val);
		return val;
		break;
	case storage::Type::Set:
		// not implemented
		break;
	default:
		break;
	}
	return ret;
}
data::Value Handle::appendProperty(Scheme *s, const data::Value &d, const Field &f, data::Value && val) {
	return appendProperty(s, d.getInteger("__oid"), f, std::move(val));
}

NS_SA_EXT_END(pg)
