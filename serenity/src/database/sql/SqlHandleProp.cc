/**
Copyright (c) 2018 Roman Katuntsev <sbkarr@stappler.org>

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

#include "SqlHandle.h"
#include "StorageFile.h"
#include "StorageScheme.h"
#include "StorageWorker.h"

NS_SA_EXT_BEGIN(sql)

data::Value SqlHandle::getFileField(Worker &w, SqlQuery &query, uint64_t oid, uint64_t targetId, const Field &f) {
	if (auto fs = File::getScheme()) {
		auto sel = targetId ? query.select() : query.with("s", [&] (SqlQuery::GenericQuery &q) {
			q.select(f.getName()).from(w.scheme().getName()).where("__oid", Comparation::Equal, oid);
		}).select();
		String alias("t"); // do not touch;

		w.readFields(*fs, [&] (const StringView &name, const Field *) {
			sel = sel.field(SqlQuery::Field("t", name));
		});

		if (targetId) {
			sel.from(SqlQuery::Field("__files").as(alias))
				.where(SqlQuery::Field(alias, "__oid"), Comparation::Equal, targetId).finalize();
		} else {
			sel.from(SqlQuery::Field("__files").as(alias)).innerJoinOn("s", [&] (SqlQuery::WhereBegin &q) {
				q.where(SqlQuery::Field(alias,"__oid"), Comparation::Equal, SqlQuery::Field("s", f.getName()));
			}).finalize();
		}

		auto ret = selectValueQuery(*fs, query);
		if (ret.isArray()) {
			ret = std::move(ret.getValue(0));
		}
		return ret;
	}
	return data::Value();
}

data::Value SqlHandle::getArrayField(Worker &w, SqlQuery &query, uint64_t oid, const Field &f) {
	query.select("data").from(toString(w.scheme().getName(), "_f_", f.getName()))
		.where(toString(w.scheme().getName(), "_id"), Comparation::Equal, oid).finalize();
	return selectValueQuery(f, query);
}

data::Value SqlHandle::getObjectField(Worker &w, SqlQuery &query, uint64_t oid, uint64_t targetId, const Field &f) {
	if (auto fs = f.getForeignScheme()) {
		auto sel = targetId ? query.select() : query.with("s", [&] (SqlQuery::GenericQuery &q) {
			q.select(f.getName()).from(w.scheme().getName()).where("__oid", Comparation::Equal, oid);
		}).select();
		String alias("t"); // do not touch;

		w.readFields(*fs, [&] (const StringView &name, const Field *) {
			sel = sel.field(SqlQuery::Field("t", name));
		});

		if (targetId) {
			sel.from(SqlQuery::Field(fs->getName()).as(alias))
				.where(SqlQuery::Field(alias, "__oid"), Comparation::Equal, targetId).finalize();
		} else {
			sel.from(SqlQuery::Field(fs->getName()).as(alias)).innerJoinOn("s", [&] (SqlQuery::WhereBegin &q) {
				q.where(SqlQuery::Field("t", "__oid"), Comparation::Equal, SqlQuery::Field("s", f.getName()));
			}).finalize();
		}

		auto ret = selectValueQuery(*fs, query);
		if (ret.isArray()) {
			ret = std::move(ret.getValue(0));
		}
		return ret;
	}
	return data::Value();
}

data::Value SqlHandle::getSetField(Worker &w, SqlQuery &query, uint64_t oid, const Field &f) {
	if (auto fs = f.getForeignScheme()) {
		if (f.isReference()) {
			auto sel = query.with("s", [&] (SqlQuery::GenericQuery &q) {
				q.select(SqlQuery::Field(toString(fs->getName(), "_id")).as("id"))
						.from(toString(w.scheme().getName(), "_f_", f.getName()))
						.where(toString(w.scheme().getName(), "_id"), Comparation::Equal, oid);
			}).select();

			String alias("t"); // do not touch;
			w.readFields(*fs, [&] (const StringView &name, const Field *) {
				sel = sel.field(SqlQuery::Field("t", name));
			});

			sel.from(SqlQuery::Field(fs->getName()).as(alias))
					.innerJoinOn("s", [&] (SqlQuery::WhereBegin &q) {
				q.where(SqlQuery::Field(alias, "__oid"), Comparation::Equal, SqlQuery::Field("s", "id"));
			}).finalize();
			return selectValueQuery(*fs, query);
		} else {
			if (auto l = w.scheme().getForeignLink(f)) {
				auto sel = query.select();

				String alias("t"); // do not touch;
				w.readFields(*fs, [&] (const StringView &name, const Field *) {
					sel = sel.field(SqlQuery::Field("t", name));
				});

				sel.from(SqlQuery::Field(fs->getName()).as(alias))
					.where(l->getName(), Comparation::Equal, oid)
					.finalize();
				return selectValueQuery(*fs, query);
			}
		}
	}
	return data::Value();
}

data::Value SqlHandle::getViewField(Worker &w, SqlQuery &query, uint64_t oid, const Field &f) {
	if (auto fs = f.getForeignScheme()) {
		auto sel = query.with("s", [&] (SqlQuery::GenericQuery &q) {
			q.select(SqlQuery::Distinct::Distinct, SqlQuery::Field(toString(fs->getName(), "_id")).as("__id"))
					.from(toString(w.scheme().getName(), "_f_", f.getName(), "_view"))
					.where(toString(w.scheme().getName(), "_id"), Comparation::Equal, oid);
		}).select();

		String alias("t"); // do not touch;
		w.readFields(*fs, [&] (const StringView &name, const Field *) {
			sel = sel.field(SqlQuery::Field("t", name));
		});

		sel.from(SqlQuery::Field(fs->getName()).as(alias))
				.innerJoinOn("s", [&] (SqlQuery::WhereBegin &q) {
			q.where(SqlQuery::Field(alias, "__oid"), Comparation::Equal, SqlQuery::Field("s", "__id"));
		}).finalize();

		auto ret = selectValueQuery(*fs, query);
		if (ret.isArray() && ret.size() > 0) {
			query.clear();
			Handle_writeSelectViewDataQuery(query, w.scheme(), oid, f, ret);
			selectValueQuery(ret, f, query);
			return ret;
		}
	}
	return data::Value();
}

data::Value SqlHandle::getSimpleField(Worker &w, SqlQuery &query, uint64_t oid, const Field &f) {
	query.select(f.getName()).from(w.scheme().getName()).where("__oid", Comparation::Equal, oid).finalize();
	auto ret = selectValueQuery(w.scheme(), query);
	if (ret.isArray()) {
		ret = std::move(ret.getValue(0));
	}
	if (ret.isDictionary()) {
		ret = ret.getValue(f.getName());
	}
	return ret;
}

bool SqlHandle::insertIntoSet(SqlQuery &query, const Scheme &s, int64_t id, const FieldObject &field, const Field &ref, const data::Value &d) {
	if (field.type == Type::Object) {
		if (ref.getType() == Type::Object) {
			// object to object is not implemented
		} else {
			// object to set is maintained by trigger
		}
	} else if (field.type == Type::Set) {
		if (ref.getType() == Type::Object) {
			if (d.isArray() && d.getValue(0).isInteger()) {
				auto w = query.update(field.scheme->getName()).set(ref.getName(), id).where();
				for (auto & it : d.asArray()) {
					if (it.isInteger()) {
						w.where(Operator::Or, "__oid", Comparation::Equal, it.getInteger());
					}
				}
				w.finalize();
				return performQuery(query) != maxOf<size_t>();
			}
		} else {
			// set to set is not implemented
		}
	}
	return false;
}

bool SqlHandle::insertIntoArray(SqlQuery &query, const Scheme &scheme, int64_t id, const Field &field, const data::Value &d) {
	if (d.isNull()) {
		query.remove(toString(scheme.getName(), "_f_", field.getName()))
				.where(toString(scheme.getName(), "_id"), Comparation::Equal, id).finalize();
		return performQuery(query) != maxOf<size_t>();
	} else {
		if (field.transform(scheme, const_cast<data::Value &>(d))) {
			auto &arrf = static_cast<const storage::FieldArray *>(field.getSlot())->tfield;
			if (!d.empty()) {
				auto vals = query.insert(toString(scheme.getName(), "_f_", field.getName()))
						.fields(toString(scheme.getName(), "_id"), "data").values();
				for (auto &it : d.asArray()) {
					vals.values(id, Binder::DataField {it, arrf.isDataLayout()});
				}
				vals.onConflictDoNothing().finalize();
				return performQuery(query) != maxOf<size_t>();
			}
		}
	}
	return false;
}

bool SqlHandle::insertIntoRefSet(SqlQuery &query, const Scheme &scheme, int64_t id, const Field &field, const Vector<int64_t> &ids) {
	auto fScheme = field.getForeignScheme();
	if (!ids.empty() && fScheme) {
		auto vals = query.insert(toString(scheme.getName(), "_f_", field.getName()))
				.fields(toString(scheme.getName(), "_id"), toString(fScheme->getName(), "_id")).values();
		for (auto &it : ids) {
			vals.values(id, it);
		}
		vals.onConflictDoNothing().finalize();
		performQuery(query);
		return true;
	}
	return false;
}

bool SqlHandle::cleanupRefSet(SqlQuery &query, const Scheme &scheme, uint64_t oid, const Field &field, const Vector<int64_t> &ids) {
	auto objField = static_cast<const storage::FieldObject *>(field.getSlot());
	auto fScheme = objField->scheme;
	if (!ids.empty() && fScheme) {
		if (objField->onRemove == RemovePolicy::Reference) {
			auto w = query.remove(toString(scheme.getName(), "_f_", field.getName()))
					.where(toString(scheme.getName(), "_id"), Comparation::Equal, oid);
			w.parenthesis(Operator::And, [&] (SqlQuery::WhereBegin &wh) {
				auto whi = wh.where();
				for (auto &it : ids) {
					whi.where(Operator::Or, toString(fScheme->getName(), "_id"), Comparation::Equal, it);
				}
			}).finalize();
			performQuery(query);
			return true;
		} else if (objField->onRemove == RemovePolicy::StrongReference) {
			auto w = query.remove(fScheme->getName()).where();
			for (auto &it : ids) {
				w.where(Operator::Or, "__oid", Comparation::Equal, it);
			}
			w.finalize();
			performQuery(query);
			return true;
		}
	}
	return false;
}

data::Value SqlHandle::field(Action a, Worker &w, uint64_t oid, const Field &f, data::Value &&val) {
	data::Value ret;
	switch (a) {
	case Action::Get:
		makeQuery([&] (SqlQuery &query) {
			switch (f.getType()) {
			case storage::Type::File:
			case storage::Type::Image: ret = getFileField(w, query, oid, 0, f); break;
			case storage::Type::Array: ret = getArrayField(w, query, oid, f); break;
			case storage::Type::Object: ret = getObjectField(w, query, oid, 0, f); break;
			case storage::Type::Set: ret = getSetField(w, query, oid, f); break;
			case storage::Type::View: ret = getViewField(w, query, oid, f); break;
			default: ret = getSimpleField(w, query, oid, f); break;
			}
		});
		break;
	case Action::Set:
		switch (f.getType()) {
		case storage::Type::File:
		case storage::Type::Image:
		case storage::Type::View:
		case storage::Type::FullTextView: return data::Value(); break; // file update should be done by scheme itself
		case storage::Type::Array:
			if (val.isArray()) {
				field(Action::Remove, w, oid, f, data::Value());
				bool success = false;
				makeQuery([&] (SqlQuery &query) {
					success = insertIntoArray(query, w.scheme(), oid, f, val);
				});
				if (success) {
					ret = move(val);
				}
			}
			break;
		case storage::Type::Set:
			if (f.isReference()) {
				auto objField = static_cast<const storage::FieldObject *>(f.getSlot());
				if (objField->onRemove == storage::RemovePolicy::Reference) {
					field(Action::Remove, w, oid, f, data::Value());
				} else {
					makeQuery([&] (SqlQuery &query) {
						auto obj = static_cast<const storage::FieldObject *>(f.getSlot());

						auto source = w.scheme().getName();
						auto target = obj->scheme->getName();

						query << "DELETE FROM " << target << " WHERE __oid IN (SELECT " << target << "_id FROM "
								<< w.scheme().getName() << "_f_" << f.getName() << " WHERE "<< source << "_id=" << oid << ")";

						for (auto &it : val.asArray()) {
							if (it.isInteger()) {
								query << " AND __oid != " << it.asInteger();
							}
						}
						query << ";";
						performQuery(query);
					});
				}
				ret = field(Action::Append, w, oid, f, move(val));
			}
			// not implemented
			break;
		default: {
			data::Value patch;
			patch.setValue(val, f.getName().str());
			Worker(w.scheme(), w.transaction()).update(oid, patch);
			return val;
			break;
		}
		}
		break;
	case Action::Append:
		switch (f.getType()) {
		case storage::Type::Array:
			if (!val.isNull()) {
				Worker(w).touch(oid);
				bool success = false;
				makeQuery([&] (SqlQuery &query) {
					success = insertIntoArray(query, w.scheme(), oid, f, val);
				});
				if (success) {
					ret = move(val);
				}
			}
			break;
		case storage::Type::Set:
			if (f.isReference()) {
				Worker(w).touch(oid);
				Vector<int64_t> toAdd;
				for (auto &it : val.asArray()) {
					if (auto id = it.asInteger()) {
						toAdd.push_back(id);
					}
				}

				bool success = false;
				makeQuery([&] (SqlQuery &query) {
					success = insertIntoRefSet(query, w.scheme(), oid, f, toAdd);
				});
				if (success) {
					ret = move(val);
				}
			}
			break;
		default:
			break;
		}
		break;
	case Action::Remove:
		switch (f.getType()) {
		case storage::Type::File:
		case storage::Type::Image:
		case storage::Type::View:
		case storage::Type::FullTextView: return data::Value(); break; // file update should be done by scheme itself
		case storage::Type::Array:
			Worker(w).touch(oid);
			makeQuery([&] (SqlQuery &query) {
				query << "DELETE FROM " << w.scheme().getName() << "_f_" << f.getName() << " WHERE " << w.scheme().getName() << "_id=" << oid << ";";
				if (performQuery(query) != maxOf<size_t>()) {
					ret = data::Value(true);
				}
			});
			break;
		case storage::Type::Set:
			if (f.isReference()) {
				Worker(w).touch(oid);
				auto objField = static_cast<const storage::FieldObject *>(f.getSlot());
				if (!val.isArray()) {
					makeQuery([&] (SqlQuery &query) {
						if (objField->onRemove == storage::RemovePolicy::Reference) {
							query.remove(toString(w.scheme().getName(), "_f_", f.getName()))
									.where(toString(w.scheme().getName(), "_id"), Comparation::Equal, oid)
									.finalize();
							ret = data::Value(performQuery(query) != maxOf<size_t>());
						} else {
							// for strong refs
							auto obj = static_cast<const storage::FieldObject *>(f.getSlot());

							auto source = w.scheme().getName();
							auto target = obj->scheme->getName();

							query << "DELETE FROM " << target << " WHERE __oid IN (SELECT " << target << "_id FROM "
									<< w.scheme().getName() << "_f_" << f.getName() << " WHERE "<< source << "_id=" << oid << ");";
							ret = data::Value(performQuery(query) != maxOf<size_t>());
						}
					});
				} else {
					Vector<int64_t> toRemove;
					for (auto &it : val.asArray()) {
						if (auto id = it.asInteger()) {
							toRemove.push_back(id);
						}
					}

					makeQuery([&] (SqlQuery &query) {
						ret = data::Value(cleanupRefSet(query, w.scheme(), oid, f, toRemove));
					});
				}
			}
			break;
		default: {
			data::Value patch;
			patch.setValue(data::Value(), f.getName().str());
			Worker(w).update(oid, patch);
		}
			break;
		}
		break;
	}
	return ret;
}

data::Value SqlHandle::field(Action a, Worker &w, const data::Value &obj, const Field &f, data::Value &&val) {
	data::Value ret;
	auto oid = obj.isInteger() ? obj.asInteger() : obj.getInteger("__oid");
	auto targetId = obj.isInteger() ? obj.asInteger() : obj.getInteger(f.getName());
	switch (a) {
	case Action::Get:
		makeQuery([&] (SqlQuery &query) {
			switch (f.getType()) {
			case storage::Type::File:
			case storage::Type::Image: ret = getFileField(w, query, oid, targetId, f); break;
			case storage::Type::Array: ret = getArrayField(w, query, oid, f); break;
			case storage::Type::Object: ret = getObjectField(w, query, oid, targetId, f); break;
			case storage::Type::Set: ret = getSetField(w, query, oid, f); break;
			case storage::Type::View: ret = getViewField(w, query, oid, f); break;
			default:
				if (auto val = obj.getValue(f.getName())) {
					ret = move(val);
				} else {
					ret = getSimpleField(w, query, oid, f);
				}
				break;
			}
		});
		break;
	case Action::Set:
	case Action::Remove:
	case Action::Append:
		ret = field(a, w, oid, f, move(val));
		break;
	}
	return ret;
}

NS_SA_EXT_END(sql)
