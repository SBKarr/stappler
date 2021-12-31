/**
Copyright (c) 2016-2019 Roman Katuntsev <sbkarr@stappler.org>

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

#include "STStorageAdapter.h"
#include "STStorageScheme.h"

NS_DB_BEGIN

Adapter Adapter::FromContext() {
	return internals::getAdapterFromContext();
}

Adapter::Adapter(Interface *iface) : _interface(iface) { }
Adapter::Adapter(const Adapter &other) { _interface = other._interface; }
Adapter& Adapter::operator=(const Adapter &other) { _interface = other._interface; return *this; }

Interface *Adapter::interface() const {
	return _interface;
}

mem::String Adapter::getTransactionKey() const {
	if (!_interface) {
		return mem::String();
	}

	char buf[32] = { 0 };
	auto prefix = mem::StringView(config::getTransactionPrefixKey());
	memcpy(buf, prefix.data(), prefix.size());
	stappler::base16::encode(buf + prefix.size(), 32 - prefix.size(),
			stappler::CoderSource((const uint8_t *)(&_interface), sizeof(void *)));
	return mem::String(buf, prefix.size() + sizeof(void *) * 2);
}

bool Adapter::set(const stappler::CoderSource &key, const mem::Value &val, stappler::TimeInterval maxAge) const {
	return _interface->set(key, val, maxAge);
}

mem::Value Adapter::get(const stappler::CoderSource &key) const {
	return _interface->get(key);
}

bool Adapter::clear(const stappler::CoderSource &key) const {
	return _interface->clear(key);
}

mem::Vector<int64_t> Adapter::performQueryListForIds(const QueryList &ql, size_t count) const {
	return _interface->performQueryListForIds(ql, count);
}
mem::Value Adapter::performQueryList(const QueryList &ql, size_t count, bool forUpdate) const {
	auto targetScheme = ql.getScheme();
	if (targetScheme) {
		// virtual fields should be resolved within interface
		return _interface->performQueryList(ql, count, forUpdate);
	}
	return mem::Value();
}

bool Adapter::init(const Interface::Config &cfg, const mem::Map<mem::StringView, const Scheme *> &schemes) const {
	return _interface->init(cfg, schemes);
}

void Adapter::makeSessionsCleanup() const {
	_interface->makeSessionsCleanup();
}

User * Adapter::authorizeUser(const Auth &auth, const mem::StringView &name, const mem::StringView &password) const {
	if (_interface) {
		return _interface->authorizeUser(auth, name, password);
	}
	return nullptr;
}

void Adapter::broadcast(const mem::Bytes &data) const {
	_interface->broadcast(data);
}

void Adapter::broadcast(const mem::Value &val) const {
	broadcast(mem::writeData(val, mem::EncodeFormat::Cbor));
}

void Adapter::broadcast(mem::StringView url, mem::Value &&val, bool exclusive) const {
	broadcast(mem::Value({
		stappler::pair("url", mem::Value(url)),
		stappler::pair("exclusive", mem::Value(exclusive)),
		stappler::pair("data", std::move(val)),
	}));
}

int64_t Adapter::getDeltaValue(const Scheme &s) {
	return _interface->getDeltaValue(s);
}

int64_t Adapter::getDeltaValue(const Scheme &s, const FieldView &v, uint64_t id) {
	return _interface->getDeltaValue(s, v, id);
}

bool Adapter::foreach(Worker &w, const Query &q, const mem::Callback<bool(mem::Value &)> &cb) const {
	return _interface->foreach(w, q, cb);
}

mem::Value Adapter::select(Worker &w, const Query &q) const {
	auto targetScheme = &w.scheme();
	auto ordField = q.getQueryField();
	if (!ordField.empty()) {
		if (auto f = targetScheme->getField(ordField)) {
			targetScheme = f->getForeignScheme();
		} else {
			return mem::Value();
		}
	}

	// virtual fields should be resolved within interface
	return _interface->select(w, q);
}

mem::Value Adapter::create(Worker &w, mem::Value &d) const {
	auto ret = _interface->create(w, d);
	if (ret) {
		auto updateData = [&] (mem::Value &value) -> bool {
			for (auto &it : value.asDict()) {
				auto f = w.scheme().getField(it.first);
				if (f && f->getType() == Type::Virtual) {
					auto slot = f->getSlot<FieldVirtual>();
					if (slot->writeFn) {
						if (!slot->writeFn(w.scheme(), ret, it.second)) {
							return false;
						}
					} else {
						return false;
					}
				}
			}
			return true;
		};

		if (ret.isArray()) {
			for (auto &it : ret.asArray()) {
				if (!updateData(it)) {
					_interface->cancelTransaction();
					return mem::Value();
				}
			}
		} else if (ret.isDictionary()) {
			if (!updateData(ret)) {
				_interface->cancelTransaction();
				return mem::Value();
			}
		}
	}
	return ret;
}

mem::Value Adapter::save(Worker &w, uint64_t oid, const mem::Value &obj, const mem::Vector<mem::String> &fields) const {
	bool hasNonVirtualUpdates = false;
	mem::Map<const FieldVirtual *, mem::Value> virtualWrites;
	auto &dict = obj.asDict();

	for (auto &it : fields) {
		if (dict.find(it) == dict.end()) {
			if (auto f = w.scheme().getField(it)) {
				if (f->getType() != Type::Virtual) {
					hasNonVirtualUpdates = true;
				}
			}
		}
	}

	if (!hasNonVirtualUpdates) {
		auto it = dict.begin();
		while (it != dict.end()) {
			if (auto f = w.scheme().getField(it->first)) {
				if (std::find(fields.begin(), fields.end(), f->getName()) != fields.end()) {
					if (f->getType() == Type::Virtual) {
						virtualWrites.emplace(f->getSlot<FieldVirtual>(), it->second);
					} else {
						hasNonVirtualUpdates = true;
					}
				}
			}
			++ it;
		}
	}

	mem::Value ret;
	if (hasNonVirtualUpdates) {
		ret = _interface->save(w, oid, obj, fields);
	} else {
		ret = obj;
	}
	if (ret) {
		for (auto &it : virtualWrites) {
			if (it.first->writeFn) {
				if (it.first->writeFn(w.scheme(), obj, it.second)) {
					ret.setValue(std::move(it.second), it.first->getName());
				} else {
					_interface->cancelTransaction();
					return mem::Value();
				}
			} else {
				_interface->cancelTransaction();
				return mem::Value();
			}
		}
	}
	return ret;
}

mem::Value Adapter::patch(Worker &w, uint64_t oid, const mem::Value &patch) const {
	return _interface->patch(w, oid, patch);
}

bool Adapter::remove(Worker &w, uint64_t oid) const {
	return _interface->remove(w, oid);
}

size_t Adapter::count(Worker &w, const Query &q) const {
	return _interface->count(w, q);
}

void Adapter::scheduleAutoField(const Scheme &scheme, const Field &field, uint64_t id) {
	struct Adapter_TaskData : AllocPool {
		const Scheme *scheme = nullptr;
		const Field *field = nullptr;
		mem::Set<uint64_t> objects;
	};

	if (!id) {
		return;
	}

	stappler::memory::pool_t * p = stappler::memory::pool::acquire();
	if (Adapter_TaskData *obj = stappler::memory::pool::get<Adapter_TaskData>(p, mem::toString(scheme.getName(), "_f_", field.getName()))) {
		obj->objects.emplace(id);
	} else {
		auto d = new (p) Adapter_TaskData;
		d->scheme = &scheme;
		d->field = &field;
		d->objects.emplace(id);
		stappler::memory::pool::store(p, d, mem::toString(scheme.getName(), "_f_", field.getName()), [d, this] {
			internals::scheduleAyncDbTask([this, d] (stappler::memory::pool_t *p) -> mem::Function<void(const Transaction &t)> {
				auto vec = new (p) mem::Vector<uint64_t>(p);
				for (auto &it : d->objects) {
					vec->push_back(it);
				}

				return [this, vec, scheme = d->scheme, field = d->field] (const Transaction &t) {
					runAutoFields(t, *vec, *scheme, *field);
				};
			});
		});
	}
}

mem::Value Adapter::field(Action a, Worker &w, uint64_t oid, const Field &f, mem::Value &&data) const {
	return _interface->field(a, w, oid, f, std::move(data));
}

mem::Value Adapter::field(Action a, Worker &w, const mem::Value &obj, const Field &f, mem::Value &&data) const {
	return _interface->field(a, w, obj, f, std::move(data));
}

bool Adapter::addToView(const FieldView &v, const Scheme *s, uint64_t oid, const mem::Value &data) const {
	return _interface->addToView(v, s, oid, data);
}
bool Adapter::removeFromView(const FieldView &v, const Scheme *s, uint64_t oid) const {
	return _interface->removeFromView(v, s, oid);
}

mem::Vector<int64_t> Adapter::getReferenceParents(const Scheme &s, uint64_t oid, const Scheme *fs, const Field *f) const {
	return _interface->getReferenceParents(s, oid, fs, f);
}

bool Adapter::beginTransaction() const {
	return _interface->beginTransaction();
}

bool Adapter::endTransaction() const {
	return _interface->endTransaction();
}

void Adapter::cancelTransaction() const {
	_interface->cancelTransaction();
}

bool Adapter::isInTransaction() const {
	return _interface->isInTransaction();
}

TransactionStatus Adapter::getTransactionStatus() const {
	return _interface->getTransactionStatus();
}

void Adapter::runAutoFields(const Transaction &t, const mem::Vector<uint64_t> &vec, const Scheme &scheme, const Field &field) {
	auto &defs = field.getSlot()->autoField;
	if (defs.defaultFn) {
		auto includeSelf = (std::find(defs.requires.begin(), defs.requires.end(), field.getName()) == defs.requires.end());
		for (auto &id : vec) {
			Query q; q.select(id);
			for (auto &req : defs.requires) {
				q.include(req);
			}
			if (includeSelf) {
				q.include(field.getName());
			}

			auto objs = scheme.select(t, q);
			if (auto obj = objs.getValue(0)) {
				auto newValue = defs.defaultFn(obj);
				if (newValue != obj.getValue(field.getName())) {
					mem::Value patch;
					patch.setValue(std::move(newValue), field.getName().str<mem::Interface>());
					scheme.update(t, obj, patch, UpdateFlags::Protected | UpdateFlags::NoReturn);
				}
			}
		}
	}
}


void Binder::setInterface(QueryInterface *iface) {
	_iface = iface;
}
QueryInterface * Binder::getInterface() const {
	return _iface;
}

void Binder::writeBind(mem::StringStream &query, int64_t val) {
	_iface->bindInt(*this, query, val);
}
void Binder::writeBind(mem::StringStream &query, uint64_t val) {
	_iface->bindUInt(*this, query, val);
}
void Binder::writeBind(mem::StringStream &query, double val) {
	_iface->bindDouble(*this, query, val);
}
void Binder::writeBind(mem::StringStream &query, stappler::Time val) {
	_iface->bindUInt(*this, query, val.toMicros());
}
void Binder::writeBind(mem::StringStream &query, stappler::TimeInterval val) {
	_iface->bindUInt(*this, query, val.toMicros());
}
void Binder::writeBind(mem::StringStream &query, const mem::String &val) {
	_iface->bindString(*this, query, val);
}
void Binder::writeBind(mem::StringStream &query, mem::String &&val) {
	_iface->bindMoveString(*this, query, std::move(val));
}
void Binder::writeBind(mem::StringStream &query, const mem::StringView &val) {
	_iface->bindStringView(*this, query, val);
}
void Binder::writeBind(mem::StringStream &query, const mem::Bytes &val) {
	_iface->bindBytes(*this, query, val);
}
void Binder::writeBind(mem::StringStream &query, mem::Bytes &&val) {
	_iface->bindMoveBytes(*this, query, std::move(val));
}
void Binder::writeBind(mem::StringStream &query, const stappler::CoderSource &val) {
	_iface->bindCoderSource(*this, query, val);
}
void Binder::writeBind(mem::StringStream &query, const mem::Value &val) {
	_iface->bindValue(*this, query, val);
}
void Binder::writeBind(mem::StringStream &query, const DataField &f) {
	_iface->bindDataField(*this, query, f);
}
void Binder::writeBind(mem::StringStream &query, const TypeString &type) {
	_iface->bindTypeString(*this, query, type);
}
void Binder::writeBind(mem::StringStream &query, const FullTextField &d) {
	_iface->bindFullText(*this, query, d);
}
void Binder::writeBind(mem::StringStream &query, const FullTextRank &rank) {
	_iface->bindFullTextRank(*this, query, rank);
}
void Binder::writeBind(mem::StringStream &query, const FullTextData &data) {
	_iface->bindFullTextData(*this, query, data);
}
void Binder::writeBind(mem::StringStream &query, const stappler::sql::PatternComparator<const mem::Value &> &cmp) {
	if (cmp.value->isString()) {
		switch (cmp.cmp) {
		case Comparation::Prefix: {
			mem::String str; str.reserve(cmp.value->getString().size() + 1);
			str.append(cmp.value->getString());
			str.append("%");
			_iface->bindMoveString(*this, query, std::move(str));
			break;
		}
		case Comparation::Suffix: {
			mem::String str; str.reserve(cmp.value->getString().size() + 1);
			str.append("%");
			str.append(cmp.value->getString());
			_iface->bindMoveString(*this, query, std::move(str));
			break;
		}
		case Comparation::WordPart: {
			mem::String str; str.reserve(cmp.value->getString().size() + 2);
			str.append("%");
			str.append(cmp.value->getString());
			str.append("%");
			_iface->bindMoveString(*this, query, std::move(str));
			break;
		}
		default:
			_iface->bindValue(*this, query, mem::Value());
			break;
		}
	} else {
		_iface->bindValue(*this, query, mem::Value());
	}
}
void Binder::writeBind(mem::StringStream &query, const stappler::sql::PatternComparator<const mem::StringView &> &cmp) {
	switch (cmp.cmp) {
	case Comparation::Prefix: {
		mem::String str; str.reserve(cmp.value->size() + 1);
		str.append(cmp.value->data(), cmp.value->size());
		str.append("%");
		_iface->bindMoveString(*this, query, std::move(str));
		break;
	}
	case Comparation::Suffix: {
		mem::String str; str.reserve(cmp.value->size() + 1);
		str.append("%");
		str.append(cmp.value->data(), cmp.value->size());
		_iface->bindMoveString(*this, query, std::move(str));
		break;
	}
	case Comparation::WordPart: {
		mem::String str; str.reserve(cmp.value->size() + 2);
		str.append("%");
		str.append(cmp.value->data(), cmp.value->size());
		str.append("%");
		_iface->bindMoveString(*this, query, std::move(str));
		break;
	}
	default:
		break;
	}
	_iface->bindMoveString(*this, query, "NULL");
}
void Binder::writeBind(mem::StringStream &query, const mem::Vector<int64_t> &vec) {
	_iface->bindIntVector(*this, query, vec);
}

void Binder::writeBind(mem::StringStream &query, const mem::Vector<double> &vec) {
	_iface->bindDoubleVector(*this, query, vec);
}

void Binder::writeBind(mem::StringStream &query, const mem::Vector<mem::StringView> &vec) {
	_iface->bindStringVector(*this, query, vec);
}

void Binder::clear() {
	_iface->clear();
}

ResultRow::ResultRow(const ResultCursor *res, size_t r) : result(res), row(r) { }

ResultRow::ResultRow(const ResultRow & other) noexcept : result(other.result), row(other.row) { }
ResultRow & ResultRow::operator=(const ResultRow &other) noexcept {
	result = other.result;
	row = other.row;
	return *this;
}

size_t ResultRow::size() const {
	return result->getFieldsCount();
}
mem::Value ResultRow::toData(const db::Scheme &scheme, const mem::Map<mem::String, db::Field> &viewFields,
		const mem::Vector<const Field *> &virtuals) {
	mem::Value row(mem::Value::Type::DICTIONARY);
	row.asDict().reserve(result->getFieldsCount() + virtuals.size());
	mem::Value *deltaPtr = nullptr;
	for (size_t i = 0; i < result->getFieldsCount(); i++) {
		auto n = result->getFieldName(i);
		if (n == "__oid") {
			if (!isNull(i)) {
				row.setInteger(toInteger(i), n.str<mem::Interface>());
			}
		} else if (n == "__vid") {
			auto val = isNull(i)?int64_t(0):toInteger(i);
			row.setInteger(val, n.str<mem::Interface>());
			if (deltaPtr && val == 0) {
				deltaPtr->setString("delete", "action");
			}
		} else if (n == "__d_action") {
			if (!deltaPtr) {
				deltaPtr = &row.emplace("__delta");
			}
			switch (DeltaAction(toInteger(i))) {
			case DeltaAction::Create: deltaPtr->setString("create", "action"); break;
			case DeltaAction::Update: deltaPtr->setString("update", "action"); break;
			case DeltaAction::Delete: deltaPtr->setString("delete", "action"); break;
			case DeltaAction::Append: deltaPtr->setString("append", "action"); break;
			case DeltaAction::Erase: deltaPtr->setString("erase", "action");  break;
			default: break;
			}
		} else if (n == "__d_object") {
			row.setInteger(toInteger(i), "__oid");
		} else if (n == "__d_time") {
			if (!deltaPtr) {
				deltaPtr = &row.emplace("__delta");
			}
			deltaPtr->setInteger(toInteger(i), "time");
		} else if (n.starts_with("__ts_rank_")) {
			auto d = toDouble(i);
			row.setDouble(d, n.sub("__ts_rank_"_len).str<mem::Interface>());
			row.setDouble(d, n.str<mem::Interface>());
		} else if (!isNull(i)) {
			if (auto f_it = scheme.getField(n)) {
				row.setValue(toData(i, *f_it), n.str<mem::Interface>());
			} else {
				auto ef_it = viewFields.find(n);
				if (ef_it != viewFields.end()) {
					row.setValue(toData(i, ef_it->second), n.str<mem::Interface>());
				}
			}
		}
	}

	if (!virtuals.empty()) {
		for (auto &it : virtuals) {
			auto slot = it->getSlot<FieldVirtual>();
			if (slot->readFn) {
				if (auto v = slot->readFn(scheme, row)) {
					row.setValue(std::move(v), it->getName());
				}
			}
		}
	}

	return row;
}

mem::StringView ResultRow::front() const {
	return at(0);
}
mem::StringView ResultRow::back() const {
	return at(result->getFieldsCount() - 1);
}

bool ResultRow::isNull(size_t n) const {
	return result->isNull(n);
}

mem::StringView ResultRow::at(size_t n) const {
	return result->toString(n);
}

mem::StringView ResultRow::toString(size_t n) const {
	return result->toString(n);
}
mem::BytesView ResultRow::toBytes(size_t n) const {
	return result->toBytes(n);
}

int64_t ResultRow::toInteger(size_t n) const {
	return result->toInteger(n);
}

double ResultRow::toDouble(size_t n) const {
	return result->toDouble(n);
}

bool ResultRow::toBool(size_t n) const {
	return result->toBool(n);
}

mem::Value ResultRow::toTypedData(size_t n) const {
	return result->toTypedData(n);
}

mem::Value ResultRow::toData(size_t n, const db::Field &f) {
	switch(f.getType()) {
	case db::Type::Integer:
	case db::Type::Object:
	case db::Type::Set:
	case db::Type::Array:
	case db::Type::File:
	case db::Type::Image:
		return mem::Value(toInteger(n));
		break;
	case db::Type::Float:
		return mem::Value(toDouble(n));
		break;
	case db::Type::Boolean:
		return mem::Value(toBool(n));
		break;
	case db::Type::Text:
		return mem::Value(toString(n));
		break;
	case db::Type::Bytes:
		return mem::Value(toBytes(n));
		break;
	case db::Type::Data:
	case db::Type::Extra:
		return stappler::data::read<mem::BytesView, mem::Interface>(toBytes(n));
		break;
	case db::Type::Custom:
		return f.getSlot<db::FieldCustom>()->readFromStorage(*result, n);
		break;
	default:
		break;
	}

	return mem::Value();
}

Result::Result(db::ResultCursor *iface) : _cursor(iface) {
	_success = _cursor->isSuccess();
	if (_success) {
		_nfields = _cursor->getFieldsCount();
	}
}
Result::~Result() {
	clear();
}

Result::Result(Result &&res) : _cursor(res._cursor), _success(res._success), _nfields(res._nfields) {
	res._cursor = nullptr;
}
Result & Result::operator=(Result &&res) {
	clear();
	_cursor = res._cursor;
	_success = res._success;
	_nfields = res._nfields;
	res._cursor = nullptr;
	return *this;
}

Result::operator bool () const {
	return _success;
}
bool Result::success() const {
	return _success;
}

mem::Value Result::info() const {
	return _cursor->getInfo();
}

bool Result::empty() const {
	return _cursor->isEmpty();
}

int64_t Result::readId() {
	return _cursor->toId();
}

size_t Result::getAffectedRows() const {
	return _cursor->getAffectedRows();
}

size_t Result::getRowsHint() const {
	return _cursor->getRowsHint();
}

void Result::clear() {
	if (_cursor) {
		_cursor->clear();
	}
}

Result::Iter Result::begin() {
	if (_row != 0) {
		_cursor->reset();
		_row = 0;
	}
	if (_cursor->isEmpty()) {
		return Result::Iter(this, stappler::maxOf<size_t>());
	} else {
		return Result::Iter(this, _row);
	}
}

Result::Iter Result::end() {
	return Result::Iter(this, stappler::maxOf<size_t>());
}

ResultRow Result::current() const {
	return ResultRow(_cursor, _row);
}

bool Result::next() {
	if (_cursor->next()) {
		++ _row;
		return true;
	}
	_row = stappler::maxOf<size_t>();
	return false;
}

mem::StringView Result::name(size_t n) const {
	return _cursor->getFieldName(n);
}

mem::Value Result::decode(const db::Scheme &scheme, const mem::Vector<const Field *> &virtuals) {
	mem::Value ret(mem::Value::Type::ARRAY);
	ret.asArray().reserve(getRowsHint());
	for (auto it : *this) {
		ret.addValue(it.toData(scheme, mem::Map<mem::String, db::Field>(), virtuals));
	}
	return ret;
}
mem::Value Result::decode(const db::Field &field, const mem::Vector<const Field *> &virtuals) {
	mem::Value ret;
	if (!empty()) {
		if (field.getType() == db::Type::Array) {
			auto &arrF = static_cast<const db::FieldArray *>(field.getSlot())->tfield;
			for (auto it : *this) {
				ret.addValue(it.toData(0, arrF));
			}
		} else if (field.getType() == db::Type::View) {
			auto v = static_cast<const db::FieldView *>(field.getSlot());
			for (auto it : *this) {
				ret.addValue(it.toData(*v->scheme, mem::Map<mem::String, db::Field>(), virtuals));
			}
		} else {
			for (auto it : *this) {
				ret.addValue(it.toData(0, field));
			}
		}
	}
	return ret;
}

mem::Value Result::decode(const db::FieldView &field) {
	mem::Value ret;
	for (auto it : *this) {
		ret.addValue(it.toData(*field.scheme, mem::Map<mem::String, db::Field>()));
	}
	return ret;
}

NS_DB_END
