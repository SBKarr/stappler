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

#ifndef STELLATOR_DB_STSTORAGESCHEME_H_
#define STELLATOR_DB_STSTORAGESCHEME_H_

#include "STStorageWorker.h"

NS_DB_BEGIN

class Scheme : public mem::AllocBase {
public:
	enum Options {
		None = 0,
		WithDelta = 1 << 0,
		Detouched = 1 << 1,
		Compressed = 1 << 2,
	};

	struct ViewScheme : mem::AllocBase {
		const Scheme *scheme = nullptr;
		const Field *viewField = nullptr;
		mem::Set<const Field *> fields;
		const Field *autoLink = nullptr;
		const AutoFieldScheme *autoField = nullptr;

		ViewScheme(const Scheme *s, const Field *v, const FieldView &) : scheme(s), viewField(v) { }
		ViewScheme(const Scheme *s, const Field *v, const AutoFieldScheme &af) : scheme(s), viewField(v), autoField(&af) { }
	};

	struct ParentScheme : AllocPool {
		const Scheme *scheme = nullptr;
		const Field *pointerField = nullptr;
		const Field *backReference = nullptr;

		ParentScheme(const Scheme *s, const Field *v) : scheme(s), pointerField(v) { }
	};

	struct UniqueConstraint {
		mem::StringView name;
		mem::Vector<const Field *> fields;

		UniqueConstraint(mem::StringView n, mem::Vector<const Field *> &&f) : name(n), fields(std::move(f)) { }
	};

	enum class TransformAction {
		Create,
		Update,
		Compare,
		ProtectedCreate,
		ProtectedUpdate,
		Touch
	};

	using FieldVec = mem::Vector<const Field *>;
	using AccessTable = std::array<AccessRole *, stappler::toInt(AccessRoleId::Max)>;

	// field list to send, when no field is required to return
	static FieldVec EmptyFieldList() { return FieldVec{nullptr}; }

	static bool initSchemes(const mem::Map<mem::StringView, const Scheme *> &);

public:
	Scheme(const mem::StringView &name, bool delta = false);
	Scheme(const mem::StringView &name, Options);
	Scheme(const mem::StringView &name, std::initializer_list<Field> il, bool delta = false);
	Scheme(const mem::StringView &name, std::initializer_list<Field> il, Options);

	Scheme(const Scheme &) = delete;
	Scheme& operator=(const Scheme &) = delete;

	Scheme(Scheme &&) = default;
	Scheme& operator=(Scheme &&) = default;

	bool hasDelta() const;
	bool isDetouched() const;
	bool isCompressed() const;

	void define(std::initializer_list<Field> il);
	void define(mem::Vector<Field> &&il);
	void define(AccessRole &&role);
	void define(UniqueConstraintDef &&);
	void define(mem::Bytes &&);

	template <typename T, typename ... Args>
	void define(T &&il, Args && ... args);

	void addFlags(Options);

	void cloneFrom(Scheme *);

	mem::StringView getName() const;
	bool hasAliases() const;

	bool isProtected(const mem::StringView &) const;
	bool save(const Transaction &, Object *) const;

	bool hasFiles() const;
	bool hasForceExclude() const;
	bool hasAccessControl() const;
	bool hasVirtuals() const;

	const mem::Set<const Field *> & getForceInclude() const;
	const mem::Map<mem::String, Field> & getFields() const;
	const Field *getField(const mem::StringView &str) const;
	const mem::Vector<UniqueConstraint> &getUnique() const;
	mem::BytesView getCompressDict() const;

	const Field *getForeignLink(const FieldObject *f) const;
	const Field *getForeignLink(const Field &f) const;
	const Field *getForeignLink(const mem::StringView &f) const;

	void setConfig(InputConfig cfg) { config = cfg; }
	const InputConfig &getConfig() const { return config; }

	size_t getMaxRequestSize() const { return config.maxRequestSize; }
	size_t getMaxVarSize() const { return config.maxVarSize; }
	size_t getMaxFileSize() const { return std::max(config.maxFileSize, config.maxVarSize); }

	bool isAtomicPatch(const mem::Value &) const;

	uint64_t hash(ValidationLevel l = ValidationLevel::NamesAndTypes) const;

	const mem::Vector<ViewScheme *> &getViews() const;
	mem::Vector<const Field *> getPatchFields(const mem::Value &patch) const;

	const AccessTable &getAccessTable() const;
	const AccessRole *getAccessRole(AccessRoleId) const;
	void setAccessRole(AccessRoleId, AccessRole &&);

	mem::Value &transform(mem::Value &, TransformAction = TransformAction::Create) const;

public: // worker interface
	template <typename Storage, typename _Value> auto get(Storage &&, _Value &&, UpdateFlags = UpdateFlags::None) const -> mem::Value;
	template <typename Storage, typename _Value> auto get(Storage &&, _Value &&, mem::StringView, UpdateFlags = UpdateFlags::None) const -> mem::Value;
	template <typename Storage, typename _Value> auto get(Storage &&, _Value &&, std::initializer_list<mem::StringView> &&fields, UpdateFlags = UpdateFlags::None) const -> mem::Value;
	template <typename Storage, typename _Value> auto get(Storage &&, _Value &&, std::initializer_list<const char *> &&fields, UpdateFlags = UpdateFlags::None) const -> mem::Value;
	template <typename Storage, typename _Value> auto get(Storage &&, _Value &&, std::initializer_list<const Field *> &&fields, UpdateFlags = UpdateFlags::None) const -> mem::Value;
	template <typename Storage, typename _Value> auto get(Storage &&, _Value &&, mem::SpanView<const Field *> fields, UpdateFlags = UpdateFlags::None) const -> mem::Value;

	// Select objects by query
	// - db::Transaction, db::Query
	template <typename T, typename ... Args> auto select(T &&t, Args && ... args) const -> mem::Value;

	// Create new object (single for dict or multi for array)
	// - db::Transaction, mem::Value[, UpdateFlags][, Conflict]
	template <typename T, typename ... Args> auto create(T &&t, Args && ... args) const -> mem::Value;

	// Update object
	// - db::Transaction, mem::Value obj, mem::Value patch[, UpdateFlags]
	// - db::Transaction, int64_t id, mem::Value patch[, UpdateFlags]
	template <typename T, typename ... Args> auto update(T &&t, Args && ... args) const  -> mem::Value;

	// Remove object
	// - db::Transaction, mem::Value obj
	// - db::Transaction, int64_t id
	template <typename T, typename ... Args> auto remove(T &&t, Args && ... args) const -> bool;

	// Count resulting objects by query
	// - db::Transaction, db::Query
	template <typename T, typename ... Args> auto count(T &&t, Args && ... args) const -> size_t;

	// Touch object (update autofields)
	// - db::Transaction, mem::Value obj
	// - db::Transaction, int64_t id
	template <typename T, typename ... Args> auto touch(T &&t, Args && ... args) const -> void;

	template <typename _Storage, typename _Value, typename _Field>
	auto getProperty(_Storage &&, _Value &&, _Field &&, std::initializer_list<mem::StringView> fields) const -> mem::Value;

	template <typename _Storage, typename _Value, typename _Field>
	auto getProperty(_Storage &&, _Value &&, _Field &&, const mem::Set<const Field *> & = mem::Set<const Field *>()) const -> mem::Value;

	template <typename T, typename ... Args> auto setProperty(T &&t, Args && ... args) const -> mem::Value;
	template <typename T, typename ... Args> auto appendProperty(T &&t, Args && ... args) const -> mem::Value;
	template <typename T, typename ... Args> auto clearProperty(T &&t, Args && ... args) const -> bool;
	template <typename T, typename ... Args> auto countProperty(T &&t, Args && ... args) const -> size_t;

protected:// CRUD functions
	friend class Worker;

	// returns Array with zero or more Dictionaries with object data or Null value
	mem::Value selectWithWorker(Worker &, const Query &) const;

	// returns Dictionary with single object data or Null value
	mem::Value createWithWorker(Worker &, const mem::Value &data, bool isProtected = false) const;

	mem::Value updateWithWorker(Worker &, uint64_t oid, const mem::Value &data, bool isProtected = false) const;
	mem::Value updateWithWorker(Worker &, const mem::Value & obj, const mem::Value &data, bool isProtected = false) const;

	bool removeWithWorker(Worker &, uint64_t oid) const;

	size_t countWithWorker(Worker &, const Query &) const;

	void touchWithWorker(Worker &, uint64_t id) const;
	void touchWithWorker(Worker &, const mem::Value & obj) const;

	mem::Value fieldWithWorker(Action, Worker &, uint64_t oid, const Field &, mem::Value && = mem::Value()) const;
	mem::Value fieldWithWorker(Action, Worker &, const mem::Value &, const Field &, mem::Value && = mem::Value()) const;

	mem::Value setFileWithWorker(Worker &w, uint64_t oid, const Field &, InputFile &) const;

protected:
	void initScheme();

	mem::Set<const Field *> getFieldSet(const Field &, std::initializer_list<mem::StringView>) const;

	void addView(const Scheme *, const Field *);
	void addAutoField(const Scheme *, const Field *f, const AutoFieldScheme &);
	void addParent(const Scheme *, const Field *);

	mem::Value createFilePatch(const Transaction &, const mem::Value &val, mem::Value &changeSet) const;
	void purgeFilePatch(const Transaction &t, const mem::Value &) const;
	void mergeValues(const Field &f, const mem::Value &obj, mem::Value &original, mem::Value &newVal) const;

	stappler::Pair<bool, mem::Value> prepareUpdate(const mem::Value &data, bool isProtected) const;
	mem::Value updateObject(Worker &, mem::Value && obj, mem::Value &data) const;

	mem::Value doPatch(Worker &w, const Transaction &t, uint64_t obj, mem::Value & patch) const;

	mem::Value patchOrUpdate(Worker &, uint64_t id, mem::Value & patch) const;
	mem::Value patchOrUpdate(Worker &, const mem::Value & obj, mem::Value & patch) const;

	void touchParents(const Transaction &, const mem::Value &obj) const;
	void extractParents(mem::Map<int64_t, const Scheme *> &, const Transaction &, const mem::Value &obj, bool isChangeSet = false) const;

	// returns:
	// - true if field was successfully removed
	// - null of false if field was not removed, we should abort transaction
	// - value, that will be sent to finalizeField if all fields will be removed
	mem::Value removeField(const Transaction &, mem::Value &, const Field &, const mem::Value &old);
	void finalizeField(const Transaction &, const Field &, const mem::Value &old);

	// call before object is created, used for additional checking or default values
	mem::Value createFile(const Transaction &, const Field &, InputFile &) const;

	// call before object is created, when file is embedded into patch
	mem::Value createFile(const Transaction &, const Field &, const mem::BytesView &, const mem::StringView &type, int64_t = 0) const;

	void processFullTextFields(mem::Value &patch, mem::Vector<mem::String> *updateFields = nullptr) const;

	mem::Value makeObjectForPatch(const Transaction &, uint64_t id, const mem::Value &, const mem::Value &patch) const;

	void updateLimits();

	bool validateHint(uint64_t, const mem::Value &);
	bool validateHint(const mem::String &alias, const mem::Value &);
	bool validateHint(const mem::Value &);

	mem::Vector<uint64_t> getLinkageForView(const mem::Value &, const ViewScheme &) const;

	void updateView(const Transaction &, const mem::Value &, const ViewScheme *, const mem::Vector<uint64_t> &) const;

protected:
	mem::Map<mem::String, Field> fields;
	mem::String name;

	Options flags = Options::None;

	InputConfig config;

	mem::Vector<ViewScheme *> views;
	mem::Vector<ParentScheme *> parents;
	mem::Set<const Field *> forceInclude;
	mem::Set<const Field *> fullTextFields;
	mem::Set<const Field *> autoFieldReq;

	bool _hasFiles = false;
	bool _hasForceExclude = false;
	bool _hasAccessControl = false;
	bool _hasVirtuals = false;

	AccessTable roles;
	Field oidField;
	mem::Vector<UniqueConstraint> unique;
	mem::Bytes _compressDict;
};

SP_DEFINE_ENUM_AS_MASK(Scheme::Options)

template <typename Storage, typename _Value>
inline auto Scheme::get(Storage &&s, _Value &&v, UpdateFlags flags) const -> mem::Value {
	return Worker(*this, std::forward<Storage>(s)).get(std::forward<_Value>(v), flags);
}

template <typename Storage, typename _Value>
inline auto Scheme::get(Storage &&s, _Value &&v, mem::StringView it, UpdateFlags flags) const -> mem::Value {
	return Worker(*this, std::forward<Storage>(s)).get(std::forward<_Value>(v), it, flags);
}

template <typename Storage, typename _Value>
inline auto Scheme::get(Storage &&s, _Value &&v, std::initializer_list<mem::StringView> &&fields, UpdateFlags flags) const -> mem::Value {
	return Worker(*this, std::forward<Storage>(s)).get(std::forward<_Value>(v), std::move(fields), flags);
}

template <typename Storage, typename _Value>
inline auto Scheme::get(Storage &&s, _Value &&v, std::initializer_list<const char *> &&fields, UpdateFlags flags) const -> mem::Value {
	return Worker(*this, std::forward<Storage>(s)).get(std::forward<_Value>(v), std::move(fields), flags);
}

template <typename Storage, typename _Value>
inline auto Scheme::get(Storage &&s, _Value &&v, std::initializer_list<const Field *> &&fields, UpdateFlags flags) const -> mem::Value {
	return Worker(*this, std::forward<Storage>(s)).get(std::forward<_Value>(v), std::move(fields), flags);
}

template <typename Storage, typename _Value>
inline auto Scheme::get(Storage &&s, _Value &&v, mem::SpanView<const Field *> fields, UpdateFlags flags) const -> mem::Value {
	return Worker(*this, std::forward<Storage>(s)).get(std::forward<_Value>(v), fields, flags);
}

template <typename T, typename ... Args>
inline auto Scheme::select(T &&t, Args && ... args) const -> mem::Value {
	return Worker(*this, std::forward<T>(t)).select(std::forward<Args>(args)...);
}

template <typename T, typename ... Args>
inline auto Scheme::create(T &&t, Args && ... args) const -> mem::Value {
	return Worker(*this, std::forward<T>(t)).create(std::forward<Args>(args)...);
}

template <typename T, typename ... Args>
inline auto Scheme::update(T &&t, Args && ... args) const  -> mem::Value {
	return Worker(*this, std::forward<T>(t)).update(std::forward<Args>(args)...);
}

template <typename T, typename ... Args>
inline auto Scheme::remove(T &&t, Args && ... args) const -> bool {
	return Worker(*this, std::forward<T>(t)).remove(std::forward<Args>(args)...);
}

template <typename T, typename ... Args>
inline auto Scheme::count(T &&t, Args && ... args) const -> size_t {
	return Worker(*this, std::forward<T>(t)).count(std::forward<Args>(args)...);
}

template <typename T, typename ... Args>
inline auto Scheme::touch(T &&t, Args && ... args) const -> void {
	Worker(*this, std::forward<T>(t)).touch(std::forward<Args>(args)...);
}


template <typename _Storage, typename _Value, typename _Field>
inline auto Scheme::getProperty(_Storage &&s, _Value &&v, _Field &&f, std::initializer_list<mem::StringView> fields) const -> mem::Value {
	return Worker(*this, std::forward<_Storage>(s)).getField(std::forward<_Value>(v), std::forward<_Field>(f), fields);
}

template <typename _Storage, typename _Value, typename _Field>
auto Scheme::getProperty(_Storage &&s, _Value &&v, _Field &&f, const mem::Set<const Field *> &fields) const -> mem::Value {
	return Worker(*this, std::forward<_Storage>(s)).getField(std::forward<_Value>(v), std::forward<_Field>(f), fields);
}

template <typename T, typename ... Args>
inline auto Scheme::setProperty(T &&t, Args && ... args) const -> mem::Value {
	return Worker(*this, std::forward<T>(t)).setField(std::forward<Args>(args)...);
}

template <typename T, typename ... Args>
inline auto Scheme::appendProperty(T &&t, Args && ... args) const -> mem::Value {
	return Worker(*this, std::forward<T>(t)).appendField(std::forward<Args>(args)...);
}

template <typename T, typename ... Args>
inline auto Scheme::clearProperty(T &&t, Args && ... args) const -> bool {
	return Worker(*this, std::forward<T>(t)).clearField(std::forward<Args>(args)...);
}

template <typename T, typename ... Args>
inline auto Scheme::countProperty(T &&t, Args && ... args) const -> size_t {
	return Worker(*this, std::forward<T>(t)).countField(std::forward<Args>(args)...);
}

template <typename T, typename ... Args>
void Scheme::define(T &&il, Args && ... args) {
	define(std::forward<T>(il));
	define(std::forward<Args>(args)...);
}

NS_DB_END

#endif /* STELLATOR_DB_STSTORAGESCHEME_H_ */
