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

#ifndef SERENITY_SRC_DATABASE_DATABASESCHEME_H_
#define SERENITY_SRC_DATABASE_DATABASESCHEME_H_

#include "StorageWorker.h"

NS_SA_EXT_BEGIN(storage)

class Scheme : public AllocPool {
public:
	struct ViewScheme : AllocPool {
		const Scheme *scheme = nullptr;
		const Field *viewField = nullptr;
		Set<const Field *> fields;
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

	using FieldVec = Vector<const Field *>;
	using AccessTable = std::array<AccessRole *, toInt(AccessRoleId::Max)>;

	// field list to send, when no field is required to return
	static FieldVec EmptyFieldList() { return FieldVec{nullptr}; }

	static bool initSchemes(Server &serv, const Map<String, const Scheme *> &);

public:
	Scheme(const String &name, bool delta = false);
	Scheme(const String &name, std::initializer_list<Field> ile, bool delta = false);

	Scheme(const Scheme &) = delete;
	Scheme& operator=(const Scheme &) = delete;

	Scheme(Scheme &&) = default;
	Scheme& operator=(Scheme &&) = default;

	void setDelta(bool value);
	bool hasDelta() const;

	void define(std::initializer_list<Field> il);
	void define(Vector<Field> &&il);
	void define(AccessRole &&role);

	template <typename T, typename ... Args>
	void define(T &&il, Args && ... args);

	void cloneFrom(Scheme *);

	StringView getName() const;
	bool hasAliases() const;

	bool isProtected(const StringView &) const;
	bool save(const Transaction &, Object *) const;

	bool hasFiles() const;
	bool hasForceExclude() const;
	bool hasAccessControl() const;

	const Set<const Field *> & getForceInclude() const;
	const Map<String, Field> & getFields() const;
	const Field *getField(const StringView &str) const;

	const Field *getForeignLink(const FieldObject *f) const;
	const Field *getForeignLink(const Field &f) const;
	const Field *getForeignLink(const StringView &f) const;

	size_t getMaxRequestSize() const { return maxRequestSize; }
	size_t getMaxVarSize() const { return maxRequestSize; }
	size_t getMaxFileSize() const { return maxRequestSize; }

	bool isAtomicPatch(const data::Value &) const;

	uint64_t hash(ValidationLevel l = ValidationLevel::NamesAndTypes) const;

	const Vector<ViewScheme *> &getViews() const;
	Vector<const Field *> getPatchFields(const data::Value &patch) const;

	const AccessTable &getAccessTable() const;
	const AccessRole *getAccessRole(AccessRoleId) const;
	void setAccessRole(AccessRoleId, AccessRole &&);

public: // worker interface
	template <typename Storage, typename Value> auto get(Storage &&, Value &&, bool forUpdate = false) const -> data::Value;
	template <typename Storage, typename Value> auto get(Storage &&, Value &&, std::initializer_list<StringView> &&fields, bool forUpdate = false) const -> data::Value;
	template <typename Storage, typename Value> auto get(Storage &&, Value &&, std::initializer_list<const char *> &&fields, bool forUpdate = false) const -> data::Value;
	template <typename Storage, typename Value> auto get(Storage &&, Value &&, std::initializer_list<const Field *> &&fields, bool forUpdate = false) const -> data::Value;

	template <typename T, typename ... Args> auto select(T &&t, Args && ... args) const -> data::Value;
	template <typename T, typename ... Args> auto create(T &&t, Args && ... args) const -> data::Value;
	template <typename T, typename ... Args> auto update(T &&t, Args && ... args) const  -> data::Value;
	template <typename T, typename ... Args> auto remove(T &&t, Args && ... args) const -> bool;
	template <typename T, typename ... Args> auto count(T &&t, Args && ... args) const -> size_t;
	template <typename T, typename ... Args> auto touch(T &&t, Args && ... args) const -> void;

	template <typename _Storage, typename _Value, typename _Field>
	auto getProperty(_Storage &&, _Value &&, _Field &&, std::initializer_list<StringView> fields) const -> data::Value;

	template <typename _Storage, typename _Value, typename _Field>
	auto getProperty(_Storage &&, _Value &&, _Field &&, const Set<const Field *> & = Set<const Field *>()) const -> data::Value;

	template <typename T, typename ... Args> auto setProperty(T &&t, Args && ... args) const -> data::Value;
	template <typename T, typename ... Args> auto appendProperty(T &&t, Args && ... args) const -> data::Value;
	template <typename T, typename ... Args> auto clearProperty(T &&t, Args && ... args) const -> bool;
	template <typename T, typename ... Args> auto countProperty(T &&t, Args && ... args) const -> size_t;

protected:// CRUD functions
	friend class Worker;

	// returns Array with zero or more Dictionaries with object data or Null value
	data::Value selectWithWorker(Worker &, const Query &) const;

	// returns Dictionary with single object data or Null value
	data::Value createWithWorker(Worker &, const data::Value &data, bool isProtected = false) const;

	data::Value updateWithWorker(Worker &, uint64_t oid, const data::Value &data, bool isProtected = false) const;
	data::Value updateWithWorker(Worker &, const data::Value & obj, const data::Value &data, bool isProtected = false) const;

	bool removeWithWorker(Worker &, uint64_t oid) const;

	size_t countWithWorker(Worker &, const Query &) const;

	void touchWithWorker(Worker &, uint64_t id) const;
	void touchWithWorker(Worker &, const data::Value & obj) const;

	data::Value fieldWithWorker(Action, Worker &, uint64_t oid, const Field &, data::Value && = data::Value()) const;
	data::Value fieldWithWorker(Action, Worker &, const data::Value &, const Field &, data::Value && = data::Value()) const;

	data::Value setFileWithWorker(Worker &w, uint64_t oid, const Field &, InputFile &) const;

protected:
	void initScheme();

	Set<const Field *> getFieldSet(const Field &, std::initializer_list<StringView>) const;

	void addView(const Scheme *, const Field *);
	void addAutoField(const Scheme *, const Field *f, const AutoFieldScheme &);
	void addParent(const Scheme *, const Field *);

	data::Value createFilePatch(const Transaction &, const data::Value &val) const;
	void purgeFilePatch(const Transaction &t, const data::Value &) const;
	void mergeValues(const Field &f, const data::Value &obj, data::Value &original, data::Value &newVal) const;

	Pair<bool, data::Value> prepareUpdate(const data::Value &data, bool isProtected) const;
	data::Value updateObject(Worker &, data::Value && obj, data::Value &data) const;

	data::Value patchOrUpdate(Worker &, uint64_t id, data::Value & patch) const;
	data::Value patchOrUpdate(Worker &, const data::Value & obj, data::Value & patch) const;

	void touchParents(const Transaction &, const data::Value &obj) const;
	void extractParents(Map<int64_t, const Scheme *> &, const Transaction &, const data::Value &obj, bool isChangeSet = false) const;

	// returns:
	// - true if field was successfully removed
	// - null of false if field was not removed, we should abort transaction
	// - value, that will be sent to finalizeField if all fields will be removed
	data::Value removeField(const Transaction &, data::Value &, const Field &, const data::Value &old);
	void finalizeField(const Transaction &, const Field &, const data::Value &old);

	enum class TransformAction {
		Create,
		Update,
		Compare,
		ProtectedCreate,
		ProtectedUpdate,
		Touch
	};

	data::Value &transform(data::Value &, TransformAction = TransformAction::Create) const;

	// call before object is created, used for additional checking or default values
	data::Value createFile(const Transaction &, const Field &, InputFile &) const;

	// call before object is created, when file is embedded into patch
	data::Value createFile(const Transaction &, const Field &, const Bytes &, const StringView &type, int64_t = 0) const;

	void processFullTextFields(data::Value &patch, Vector<String> *updateFields = nullptr) const;

	data::Value makeObjectForPatch(const Transaction &, uint64_t id, const data::Value &, const data::Value &patch) const;

	void updateLimits();

	bool validateHint(uint64_t, const data::Value &);
	bool validateHint(const String &alias, const data::Value &);
	bool validateHint(const data::Value &);

	Vector<uint64_t> getLinkageForView(const data::Value &, const ViewScheme &) const;

	void updateView(const Transaction &, const data::Value &, const ViewScheme *, const Vector<uint64_t> &) const;

private:
	template <typename Source>
	void addViewScheme(const Scheme *s, const Field *f, const Source &source);

protected:
	Map<String, Field> fields;
	String name;

	bool delta = false;

	size_t maxRequestSize = 0;
	size_t maxVarSize = 256;
	size_t maxFileSize = 0;

	Vector<ViewScheme *> views;
	Vector<ParentScheme *> parents;
	Set<const Field *> forceInclude;
	Set<const Field *> fullTextFields;

	bool _hasFiles = false;
	bool _hasForceExclude = false;
	bool _hasAccessControl = false;

	AccessTable roles;
};


template <typename Storage, typename Value>
inline auto Scheme::get(Storage &&s, Value &&v, bool forUpdate) const -> data::Value {
	return Worker(*this, std::forward<Storage>(s)).get(std::forward<Value>(v), forUpdate);
}

template <typename Storage, typename Value>
inline auto Scheme::get(Storage &&s, Value &&v, std::initializer_list<StringView> &&fields, bool forUpdate) const -> data::Value {
	return Worker(*this, std::forward<Storage>(s)).get(std::forward<Value>(v), move(fields), forUpdate);
}

template <typename Storage, typename Value>
inline auto Scheme::get(Storage &&s, Value &&v, std::initializer_list<const char *> &&fields, bool forUpdate) const -> data::Value {
	return Worker(*this, std::forward<Storage>(s)).get(std::forward<Value>(v), move(fields), forUpdate);
}

template <typename Storage, typename Value>
inline auto Scheme::get(Storage &&s, Value &&v, std::initializer_list<const Field *> &&fields, bool forUpdate) const -> data::Value {
	return Worker(*this, std::forward<Storage>(s)).get(std::forward<Value>(v), move(fields), forUpdate);
}

template <typename T, typename ... Args>
inline auto Scheme::select(T &&t, Args && ... args) const -> data::Value {
	return Worker(*this, std::forward<T>(t)).select(std::forward<Args>(args)...);
}

template <typename T, typename ... Args>
inline auto Scheme::create(T &&t, Args && ... args) const -> data::Value {
	return Worker(*this, std::forward<T>(t)).create(std::forward<Args>(args)...);
}

template <typename T, typename ... Args>
inline auto Scheme::update(T &&t, Args && ... args) const  -> data::Value {
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
inline auto Scheme::getProperty(_Storage &&s, _Value &&v, _Field &&f, std::initializer_list<StringView> fields) const -> data::Value {
	return Worker(*this, std::forward<_Storage>(s)).getField(std::forward<_Value>(v), std::forward<_Field>(f), fields);
}

template <typename _Storage, typename _Value, typename _Field>
auto Scheme::getProperty(_Storage &&s, _Value &&v, _Field &&f, const Set<const Field *> &fields) const -> data::Value {
	return Worker(*this, std::forward<_Storage>(s)).getField(std::forward<_Value>(v), std::forward<_Field>(f), fields);
}

template <typename T, typename ... Args>
inline auto Scheme::setProperty(T &&t, Args && ... args) const -> data::Value {
	return Worker(*this, std::forward<T>(t)).setField(std::forward<Args>(args)...);
}

template <typename T, typename ... Args>
inline auto Scheme::appendProperty(T &&t, Args && ... args) const -> data::Value {
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
	define(forward<T>(il));
	define(forward<Args>(args)...);
}

NS_SA_EXT_END(storage)

#endif /* SERENITY_SRC_DATABASE_DATABASESCHEME_H_ */
