/**
Copyright (c) 2018-2019 Roman Katuntsev <sbkarr@stappler.org>

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

#include "STStorageScheme.h"
#include "STSqlHandle.h"

NS_DB_SQL_BEGIN

mem::Value SqlHandle::getFileField(Worker &w, SqlQuery &query, uint64_t oid, uint64_t targetId, const Field &f) {
	if (auto fs = internals::getFileScheme()) {
		auto sel = targetId ? query.select() : query.with("s", [&] (SqlQuery::GenericQuery &q) {
			q.select(f.getName()).from(w.scheme().getName()).where("__oid", Comparation::Equal, oid);
		}).select();
		mem::String alias("t"); // do not touch;

		w.readFields(*fs, [&] (const mem::StringView &name, const Field *) {
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
	return mem::Value();
}

size_t SqlHandle::getFileCount(Worker &w, SqlQuery &query, uint64_t oid, uint64_t targetId, const Field &f) {
	size_t ret = 0;
	auto sel = (targetId ? query.select() : query.with("s", [&] (SqlQuery::GenericQuery &q) {
		q.select(f.getName()).from(w.scheme().getName()).where("__oid", Comparation::Equal, oid);
	}).select()).aggregate("COUNT", "*");
	mem::String alias("t"); // do not touch;

	if (targetId) {
		sel.from(SqlQuery::Field("__files").as(alias))
			.where(SqlQuery::Field(alias, "__oid"), Comparation::Equal, targetId).finalize();
	} else {
		sel.from(SqlQuery::Field("__files").as(alias)).innerJoinOn("s", [&] (SqlQuery::WhereBegin &q) {
			q.where(SqlQuery::Field(alias,"__oid"), Comparation::Equal, SqlQuery::Field("s", f.getName()));
		}).finalize();
	}

	selectQuery(query, [&] (Result &result) {
		if (result && result.nrows() == 1) {
			ret = size_t(result.front().toInteger(0));
		}
	});
	return ret;
}

mem::Value SqlHandle::getArrayField(Worker &w, SqlQuery &query, uint64_t oid, const Field &f) {
	query.select("data").from(mem::toString(w.scheme().getName(), "_f_", f.getName()))
		.where(mem::toString(w.scheme().getName(), "_id"), Comparation::Equal, oid).finalize();
	return selectValueQuery(f, query);
}

size_t SqlHandle::getArrayCount(Worker &w, SqlQuery &query, uint64_t oid, const Field &f) {
	size_t ret = 0;
	query.select().aggregate("COUNT", "*").from(mem::toString(w.scheme().getName(), "_f_", f.getName()))
		.where(mem::toString(w.scheme().getName(), "_id"), Comparation::Equal, oid).finalize();

	selectQuery(query, [&] (Result &result) {
		if (result && result.nrows() == 1) {
			ret = size_t(result.front().toInteger(0));
		}
	});
	return ret;
}

mem::Value SqlHandle::getObjectField(Worker &w, SqlQuery &query, uint64_t oid, uint64_t targetId, const Field &f) {
	if (auto fs = f.getForeignScheme()) {
		auto sel = targetId ? query.select() : query.with("s", [&] (SqlQuery::GenericQuery &q) {
			q.select(f.getName()).from(w.scheme().getName()).where("__oid", Comparation::Equal, oid);
		}).select();
		mem::String alias("t"); // do not touch;

		w.readFields(*fs, [&] (const mem::StringView &name, const Field *) {
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
	return mem::Value();
}

size_t SqlHandle::getObjectCount(Worker &w, SqlQuery &query, uint64_t oid, uint64_t targetId, const Field &f) {
	size_t ret = 0;
	if (auto fs = f.getForeignScheme()) {
		auto sel = (targetId ? query.select() : query.with("s", [&] (SqlQuery::GenericQuery &q) {
			q.select(f.getName()).from(w.scheme().getName()).where("__oid", Comparation::Equal, oid);
		}).select()).aggregate("COUNT", "*");
		mem::String alias("t"); // do not touch;

		if (targetId) {
			sel.from(SqlQuery::Field(fs->getName()).as(alias))
				.where(SqlQuery::Field(alias, "__oid"), Comparation::Equal, targetId).finalize();
		} else {
			sel.from(SqlQuery::Field(fs->getName()).as(alias)).innerJoinOn("s", [&] (SqlQuery::WhereBegin &q) {
				q.where(SqlQuery::Field("t", "__oid"), Comparation::Equal, SqlQuery::Field("s", f.getName()));
			}).finalize();
		}

		selectQuery(query, [&] (Result &result) {
			if (result && result.nrows() == 1) {
				ret = size_t(result.front().toInteger(0));
			}
		});
	}
	return ret;
}

mem::Value SqlHandle::getSetField(Worker &w, SqlQuery &query, uint64_t oid, const Field &f) {
	if (auto fs = f.getForeignScheme()) {
		if (f.isReference()) {
			auto sel = query.with("s", [&] (SqlQuery::GenericQuery &q) {
				q.select(SqlQuery::Field(mem::toString(fs->getName(), "_id")).as("id"))
						.from(mem::toString(w.scheme().getName(), "_f_", f.getName()))
						.where(mem::toString(w.scheme().getName(), "_id"), Comparation::Equal, oid);
			}).select();

			mem::String alias("t"); // do not touch;
			w.readFields(*fs, [&] (const mem::StringView &name, const Field *) {
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

				mem::String alias("t"); // do not touch;
				w.readFields(*fs, [&] (const mem::StringView &name, const Field *) {
					sel = sel.field(SqlQuery::Field("t", name));
				});

				sel.from(SqlQuery::Field(fs->getName()).as(alias))
					.where(l->getName(), Comparation::Equal, oid)
					.finalize();
				return selectValueQuery(*fs, query);
			}
		}
	}
	return mem::Value();
}

size_t SqlHandle::getSetCount(Worker &w, SqlQuery &query, uint64_t oid, const Field &f) {
	size_t ret = 0;
	if (auto fs = f.getForeignScheme()) {
		if (f.isReference()) {
			auto sel = query.with("s", [&] (SqlQuery::GenericQuery &q) {
				q.select(SqlQuery::Field(mem::toString(fs->getName(), "_id")).as("id"))
						.from(mem::toString(w.scheme().getName(), "_f_", f.getName()))
						.where(mem::toString(w.scheme().getName(), "_id"), Comparation::Equal, oid);
			}).select().aggregate("COUNT", "*");

			mem::String alias("t"); // do not touch;
			sel.from(SqlQuery::Field(fs->getName()).as(alias))
					.innerJoinOn("s", [&] (SqlQuery::WhereBegin &q) {
				q.where(SqlQuery::Field(alias, "__oid"), Comparation::Equal, SqlQuery::Field("s", "id"));
			}).finalize();
		} else {
			if (auto l = w.scheme().getForeignLink(f)) {
				auto sel = query.select().aggregate("COUNT", "*");

				mem::String alias("t"); // do not touch;
				sel.from(SqlQuery::Field(fs->getName()).as(alias))
					.where(l->getName(), Comparation::Equal, oid)
					.finalize();
			} else {
				return 0;
			}
		}
		selectQuery(query, [&] (Result &result) {
			if (result && result.nrows() == 1) {
				ret = size_t(result.front().toInteger(0));
			}
		});
	}
	return ret;
}

mem::Value SqlHandle::getViewField(Worker &w, SqlQuery &query, uint64_t oid, const Field &f) {
	if (auto fs = f.getForeignScheme()) {
		auto sel = query.with("s", [&] (SqlQuery::GenericQuery &q) {
			q.select(SqlQuery::Distinct::Distinct, SqlQuery::Field(mem::toString(fs->getName(), "_id")).as("__id"))
					.from(mem::toString(w.scheme().getName(), "_f_", f.getName(), "_view"))
					.where(mem::toString(w.scheme().getName(), "_id"), Comparation::Equal, oid);
		}).select();

		mem::String alias("t"); // do not touch;
		w.readFields(*fs, [&] (const mem::StringView &name, const Field *) {
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
	return mem::Value();
}

size_t SqlHandle::getViewCount(Worker &w, SqlQuery &query, uint64_t oid, const Field &f) {
	size_t ret = 0;
	if (auto fs = f.getForeignScheme()) {
		auto sel = query.with("s", [&] (SqlQuery::GenericQuery &q) {
			q.select(SqlQuery::Distinct::Distinct, SqlQuery::Field(mem::toString(fs->getName(), "_id")).as("__id"))
					.from(mem::toString(w.scheme().getName(), "_f_", f.getName(), "_view"))
					.where(mem::toString(w.scheme().getName(), "_id"), Comparation::Equal, oid);
		}).select().aggregate("COUNT", "*");

		mem::String alias("t"); // do not touch;
		sel.from(SqlQuery::Field(fs->getName()).as(alias))
				.innerJoinOn("s", [&] (SqlQuery::WhereBegin &q) {
			q.where(SqlQuery::Field(alias, "__oid"), Comparation::Equal, SqlQuery::Field("s", "__id"));
		}).finalize();

		selectQuery(query, [&] (Result &result) {
			if (result && result.nrows() == 1) {
				ret = size_t(result.front().toInteger(0));
			}
		});
	}
	return ret;
}

mem::Value SqlHandle::getSimpleField(Worker &w, SqlQuery &query, uint64_t oid, const Field &f) {
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

size_t SqlHandle::getSimpleCount(Worker &w, SqlQuery &query, uint64_t oid, const Field &f) {
	size_t ret = 0;
	query.select().aggregate("COUNT", f.getName()).from(w.scheme().getName()).where("__oid", Comparation::Equal, oid).finalize();
	selectQuery(query, [&] (Result &result) {
		if (result && result.nrows() == 1) {
			ret = size_t(result.front().toInteger(0));
		}
	});
	return ret;
}

bool SqlHandle::insertIntoSet(SqlQuery &query, const Scheme &s, int64_t id, const db::FieldObject &field, const Field &ref, const mem::Value &d) {
	if (field.type == db::Type::Object) {
		if (ref.getType() == db::Type::Object) {
			// object to object is not implemented
		} else {
			// object to set is maintained by trigger
		}
	} else if (field.type == db::Type::Set) {
		if (ref.getType() == db::Type::Object) {
			if (d.isArray() && d.getValue(0).isInteger()) {
				auto w = query.update(field.scheme->getName()).set(ref.getName(), id).where();
				for (auto & it : d.asArray()) {
					if (it.isInteger()) {
						w.where(Operator::Or, "__oid", Comparation::Equal, it.getInteger());
					}
				}
				w.finalize();
				return performQuery(query) != stappler::maxOf<size_t>();
			}
		} else {
			// set to set is not implemented
		}
	}
	return false;
}

bool SqlHandle::insertIntoArray(SqlQuery &query, const Scheme &scheme, int64_t id, const Field &field, const mem::Value &d) {
	if (d.isNull()) {
		query.remove(mem::toString(scheme.getName(), "_f_", field.getName()))
				.where(mem::toString(scheme.getName(), "_id"), Comparation::Equal, id).finalize();
		return performQuery(query) != stappler::maxOf<size_t>();
	} else {
		if (field.transform(scheme, id, const_cast<mem::Value &>(d))) {
			auto &arrf = static_cast<const db::FieldArray *>(field.getSlot())->tfield;
			if (!d.empty()) {
				auto vals = query.insert(mem::toString(scheme.getName(), "_f_", field.getName()))
						.fields(mem::toString(scheme.getName(), "_id"), "data").values();
				for (auto &it : d.asArray()) {
					vals.values(id, db::Binder::DataField {&arrf, it, arrf.isDataLayout()});
				}
				vals.onConflictDoNothing().finalize();
				return performQuery(query) != stappler::maxOf<size_t>();
			}
		}
	}
	return false;
}

bool SqlHandle::insertIntoRefSet(SqlQuery &query, const Scheme &scheme, int64_t id, const Field &field, const mem::Vector<int64_t> &ids) {
	auto fScheme = field.getForeignScheme();
	if (!ids.empty() && fScheme) {
		auto vals = query.insert(mem::toString(scheme.getName(), "_f_", field.getName()))
				.fields(mem::toString(scheme.getName(), "_id"), mem::toString(fScheme->getName(), "_id")).values();
		for (auto &it : ids) {
			vals.values(id, it);
		}
		vals.onConflictDoNothing().finalize();
		performQuery(query);
		return true;
	}
	return false;
}

bool SqlHandle::cleanupRefSet(SqlQuery &query, const Scheme &scheme, uint64_t oid, const Field &field, const mem::Vector<int64_t> &ids) {
	auto objField = static_cast<const db::FieldObject *>(field.getSlot());
	auto fScheme = objField->scheme;
	if (!ids.empty() && fScheme) {
		if (objField->onRemove == db::RemovePolicy::Reference) {
			auto w = query.remove(mem::toString(scheme.getName(), "_f_", field.getName()))
					.where(mem::toString(scheme.getName(), "_id"), Comparation::Equal, oid);
			w.parenthesis(Operator::And, [&] (SqlQuery::WhereBegin &wh) {
				auto whi = wh.where();
				for (auto &it : ids) {
					whi.where(Operator::Or, mem::toString(fScheme->getName(), "_id"), Comparation::Equal, it);
				}
			}).finalize();
			performQuery(query);
			return true;
		} else if (objField->onRemove == db::RemovePolicy::StrongReference) {
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

mem::Value SqlHandle::field(db::Action a, Worker &w, uint64_t oid, const Field &f, mem::Value &&val) {
	mem::Value ret;
	switch (a) {
	case db::Action::Get:
		makeQuery([&] (SqlQuery &query) {
			switch (f.getType()) {
			case db::Type::File:
			case db::Type::Image: ret = getFileField(w, query, oid, 0, f); break;
			case db::Type::Array: ret = getArrayField(w, query, oid, f); break;
			case db::Type::Object: ret = getObjectField(w, query, oid, 0, f); break;
			case db::Type::Set: ret = getSetField(w, query, oid, f); break;
			case db::Type::View: ret = getViewField(w, query, oid, f); break;
			default: ret = getSimpleField(w, query, oid, f); break;
			}
		});
		break;
	case db::Action::Count:
		makeQuery([&] (SqlQuery &query) {
			switch (f.getType()) {
			case db::Type::File:
			case db::Type::Image: ret = mem::Value(getFileCount(w, query, oid, 0, f)); break;
			case db::Type::Array: ret = mem::Value(getArrayCount(w, query, oid, f)); break;
			case db::Type::Object: ret = mem::Value(getObjectCount(w, query, oid, 0, f)); break;
			case db::Type::Set: ret = mem::Value(getSetCount(w, query, oid, f)); break;
			case db::Type::View: ret = mem::Value(getViewCount(w, query, oid, f)); break;
			default: ret = mem::Value(getSimpleCount(w, query, oid, f)); break;
			}
		});
		break;
	case db::Action::Set:
		switch (f.getType()) {
		case db::Type::File:
		case db::Type::Image:
		case db::Type::View:
		case db::Type::FullTextView: return mem::Value(); break; // file update should be done by scheme itself
		case db::Type::Array:
			if (val.isArray()) {
				field(db::Action::Remove, w, oid, f, mem::Value());
				bool success = false;
				makeQuery([&] (SqlQuery &query) {
					success = insertIntoArray(query, w.scheme(), oid, f, val);
				});
				if (success) {
					ret = std::move(val);
				}
			}
			break;
		case db::Type::Set:
			if (f.isReference()) {
				auto objField = static_cast<const db::FieldObject *>(f.getSlot());
				if (objField->onRemove == db::RemovePolicy::Reference) {
					field(db::Action::Remove, w, oid, f, mem::Value());
				} else {
					makeQuery([&] (SqlQuery &query) {
						auto obj = static_cast<const db::FieldObject *>(f.getSlot());

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
				ret = field(db::Action::Append, w, oid, f, std::move(val));
			}
			// not implemented
			break;
		default: {
			mem::Value patch;
			patch.setValue(val, f.getName().str<mem::Interface>());
			Worker(w.scheme(), w.transaction()).update(oid, patch);
			return val;
			break;
		}
		}
		break;
	case db::Action::Append:
		switch (f.getType()) {
		case db::Type::Array:
			if (!val.isNull()) {
				Worker(w).touch(oid);
				bool success = false;
				makeQuery([&] (SqlQuery &query) {
					success = insertIntoArray(query, w.scheme(), oid, f, val);
				});
				if (success) {
					ret = std::move(val);
				}
			}
			break;
		case db::Type::Set:
			if (f.isReference()) {
				Worker(w).touch(oid);
				mem::Vector<int64_t> toAdd;
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
					ret = std::move(val);
				}
			}
			break;
		default:
			break;
		}
		break;
	case db::Action::Remove:
		switch (f.getType()) {
		case db::Type::File:
		case db::Type::Image:
		case db::Type::View:
		case db::Type::FullTextView: return mem::Value(); break; // file update should be done by scheme itself
		case db::Type::Array:
			Worker(w).touch(oid);
			makeQuery([&] (SqlQuery &query) {
				query << "DELETE FROM " << w.scheme().getName() << "_f_" << f.getName() << " WHERE " << w.scheme().getName() << "_id=" << oid << ";";
				if (performQuery(query) != stappler::maxOf<size_t>()) {
					ret = mem::Value(true);
				}
			});
			break;
		case db::Type::Set:
			if (f.isReference()) {
				Worker(w).touch(oid);
				auto objField = static_cast<const db::FieldObject *>(f.getSlot());
				if (!val.isArray()) {
					makeQuery([&] (SqlQuery &query) {
						if (objField->onRemove == db::RemovePolicy::Reference) {
							query.remove(mem::toString(w.scheme().getName(), "_f_", f.getName()))
									.where(mem::toString(w.scheme().getName(), "_id"), Comparation::Equal, oid)
									.finalize();
							ret = mem::Value(performQuery(query) != stappler::maxOf<size_t>());
						} else {
							// for strong refs
							auto obj = static_cast<const db::FieldObject *>(f.getSlot());

							auto source = w.scheme().getName();
							auto target = obj->scheme->getName();

							query << "DELETE FROM " << target << " WHERE __oid IN (SELECT " << target << "_id FROM "
									<< w.scheme().getName() << "_f_" << f.getName() << " WHERE "<< source << "_id=" << oid << ");";
							ret = mem::Value(performQuery(query) != stappler::maxOf<size_t>());
						}
					});
				} else {
					mem::Vector<int64_t> toRemove;
					for (auto &it : val.asArray()) {
						if (auto id = it.asInteger()) {
							toRemove.push_back(id);
						}
					}

					makeQuery([&] (SqlQuery &query) {
						ret = mem::Value(cleanupRefSet(query, w.scheme(), oid, f, toRemove));
					});
				}
			}
			break;
		default: {
			mem::Value patch;
			patch.setValue(mem::Value(), f.getName().str<mem::Interface>());
			Worker(w).update(oid, patch);
		}
			break;
		}
		break;
	}
	return ret;
}

mem::Value SqlHandle::field(db::Action a, Worker &w, const mem::Value &obj, const Field &f, mem::Value &&val) {
	mem::Value ret;
	auto oid = obj.isInteger() ? obj.asInteger() : obj.getInteger("__oid");
	auto targetId = obj.isInteger() ? obj.asInteger() : obj.getInteger(f.getName());
	switch (a) {
	case db::Action::Get:
		makeQuery([&] (SqlQuery &query) {
			switch (f.getType()) {
			case db::Type::File:
			case db::Type::Image: ret = getFileField(w, query, oid, targetId, f); break;
			case db::Type::Array: ret = getArrayField(w, query, oid, f); break;
			case db::Type::Object: ret = getObjectField(w, query, oid, targetId, f); break;
			case db::Type::Set: ret = getSetField(w, query, oid, f); break;
			case db::Type::View: ret = getViewField(w, query, oid, f); break;
			default:
				if (auto val = obj.getValue(f.getName())) {
					ret = std::move(val);
				} else {
					ret = getSimpleField(w, query, oid, f);
				}
				break;
			}
		});
		break;
	case db::Action::Count:
		makeQuery([&] (SqlQuery &query) {
			switch (f.getType()) {
			case db::Type::File:
			case db::Type::Image: ret = mem::Value(getFileCount(w, query, oid, 0, f)); break;
			case db::Type::Array: ret = mem::Value(getArrayCount(w, query, oid, f)); break;
			case db::Type::Object: ret = mem::Value(getObjectCount(w, query, oid, 0, f)); break;
			case db::Type::Set: ret = mem::Value(getSetCount(w, query, oid, f)); break;
			case db::Type::View: ret = mem::Value(getViewCount(w, query, oid, f)); break;
			default:
				if (auto val = obj.getValue(f.getName())) {
					ret = mem::Value(1);
				} else {
					ret = mem::Value(getSimpleCount(w, query, oid, f));
				}
				break;
			}
		});
		break;
	case db::Action::Set:
	case db::Action::Remove:
	case db::Action::Append:
		ret = field(a, w, oid, f, std::move(val));
		break;
	}
	return ret;
}

NS_DB_SQL_END
