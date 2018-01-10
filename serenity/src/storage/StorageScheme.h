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

#ifndef SERENITY_SRC_DATABASE_DATABASESCHEME_H_
#define SERENITY_SRC_DATABASE_DATABASESCHEME_H_

#include "StorageField.h"
#include "StorageQuery.h"
#include "InputFilter.h"

NS_SA_EXT_BEGIN(storage)

class Scheme : public AllocPool {
public:
	struct ViewScheme : AllocPool {
		const Scheme *scheme = nullptr;
		const Field *viewField = nullptr;
		Set<const Field *> fields;
		const Field *autoLink = nullptr;

		ViewScheme(const Scheme *s, const Field *v) : scheme(s), viewField(v) { }
	};

	static bool initSchemes(Server &serv, const Map<String, const Scheme *> &);

	Scheme(const String &name);
	Scheme(const String &name, std::initializer_list<Field> il);

	void setDelta(bool value);
	bool hasDelta() const;

	void define(std::initializer_list<Field> il);
	void cloneFrom(Scheme *);

	const String &getName() const;
	bool hasAliases() const;

	bool isProtected(const StringView &) const;
	bool saveObject(Adapter *, Object *) const;

	bool hasFiles() const;

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

public:// CRUD functions
	// returns Dictionary with single object data or Null value
	data::Value create(Adapter *, const data::Value &data, bool isProtected = false) const;

	data::Value get(Adapter *, uint64_t oid, bool forUpdate = false) const;
	data::Value get(Adapter *, const String &alias, bool forUpdate = false) const;
	data::Value get(Adapter *, const data::Value &id, bool forUpdate = false) const;

	data::Value get(Adapter *, uint64_t oid, std::initializer_list<StringView> &&fields, bool forUpdate = false) const;
	data::Value get(Adapter *, const String &alias, std::initializer_list<StringView> &&fields, bool forUpdate = false) const;
	data::Value get(Adapter *, const data::Value &id, std::initializer_list<StringView> &&fields, bool forUpdate = false) const;

	data::Value get(Adapter *, uint64_t oid, std::initializer_list<const Field *> &&fields, bool forUpdate = false) const;
	data::Value get(Adapter *, const String &alias, std::initializer_list<const Field *> &&fields, bool forUpdate = false) const;
	data::Value get(Adapter *, const data::Value &id, std::initializer_list<const Field *> &&fields, bool forUpdate = false) const;

	data::Value update(Adapter *, uint64_t oid, const data::Value &data, bool isProtected = false) const;
	data::Value update(Adapter *, const data::Value & obj, const data::Value &data, bool isProtected = false) const;

	bool remove(Adapter *, uint64_t oid) const;

	// returns Array with zero or more Dictionaries with object data or Null value
	data::Value select(Adapter *, const Query &) const;

	size_t count(Adapter *) const;
	size_t count(Adapter *, const Query &) const;

	void touch(Adapter *adapter, uint64_t id) const;
	void touch(Adapter *adapter, const data::Value & obj) const;

public:
	data::Value getProperty(Adapter *, uint64_t oid, const String &, const Set<const Field *> & = Set<const Field *>()) const;
	data::Value getProperty(Adapter *, const data::Value &, const String &, const Set<const Field *> & = Set<const Field *>()) const;

	data::Value setProperty(Adapter *, uint64_t oid, const String &, data::Value &&) const;
	data::Value setProperty(Adapter *, const data::Value &, const String &, data::Value &&) const;
	data::Value setProperty(Adapter *, uint64_t oid, const String &, InputFile &) const;
	data::Value setProperty(Adapter *, const data::Value &, const String &, InputFile &) const;

	bool clearProperty(Adapter *, uint64_t oid, const String &, data::Value && = data::Value()) const;
	bool clearProperty(Adapter *, const data::Value &, const String &, data::Value && = data::Value()) const;

	data::Value appendProperty(Adapter *, uint64_t oid, const String &, data::Value &&) const;
	data::Value appendProperty(Adapter *, const data::Value &, const String &, data::Value &&) const;

public:
	data::Value getProperty(Adapter *, uint64_t oid, const Field &, const Set<const Field *> & = Set<const Field *>()) const;
	data::Value getProperty(Adapter *, const data::Value &, const Field &, const Set<const Field *> & = Set<const Field *>()) const;

	data::Value setProperty(Adapter *, uint64_t oid, const Field &, data::Value &&) const;
	data::Value setProperty(Adapter *, const data::Value &, const Field &, data::Value &&) const;
	data::Value setProperty(Adapter *, uint64_t oid, const Field &, InputFile &) const;
	data::Value setProperty(Adapter *, const data::Value &, const Field &, InputFile &) const;

	bool clearProperty(Adapter *, uint64_t oid, const Field &, data::Value && = data::Value()) const;
	bool clearProperty(Adapter *, const data::Value &, const Field &, data::Value && = data::Value()) const;

	data::Value appendProperty(Adapter *, uint64_t oid, const Field &, data::Value &&) const;
	data::Value appendProperty(Adapter *, const data::Value &, const Field &, data::Value &&) const;

protected:
	void addView(const Scheme *, const Field *);

	data::Value createFilePatch(Adapter *, const data::Value &val) const;
	void purgeFilePatch(Adapter *, const data::Value &) const;
	void mergeValues(const Field &f, data::Value &original, data::Value &newVal) const;

	Pair<bool, data::Value> prepareUpdate(const data::Value &data, bool isProtected) const;
	data::Value updateObject(Adapter *, data::Value && obj, data::Value &data) const;

	data::Value patchOrUpdate(Adapter *adapter, uint64_t id, data::Value & patch) const;
	data::Value patchOrUpdate(Adapter *adapter, const data::Value & obj, data::Value & patch) const;

	// returns:
	// - true if field was successfully removed
	// - null of false if field was not removed, we should abort transaction
	// - value, that will be sent to finalizeField if all fields will be removed
	data::Value removeField(Adapter *, data::Value &, const Field &, const data::Value &old);
	void finalizeField(Adapter *, const Field &, const data::Value &old);

	enum class TransformAction {
		Create,
		Update,
		Compare,
		ProtectedCreate,
		ProtectedUpdate,
	};

	data::Value &transform(data::Value &, TransformAction = TransformAction::Create) const;

	// call before object is created, used for additional checking or default values
	data::Value createFile(Adapter *, const Field &, InputFile &) const;

	// call after object is created, used for custom field initialization
	data::Value initField(Adapter *, Object *, const Field &, const data::Value &);

	void updateLimits();

	bool validateHint(uint64_t, const data::Value &);
	bool validateHint(const String &alias, const data::Value &);
	bool validateHint(const data::Value &);

	void updateView(Adapter *, const data::Value &, const ViewScheme *) const;

	Map<String, Field> fields;
	String name;

	bool delta = false;

	size_t maxRequestSize = 0;
	size_t maxVarSize = 256;
	size_t maxFileSize = 0;

	Vector<ViewScheme *> views;
	Set<const Field *> forceInclude;
};

NS_SA_EXT_END(storage)

#endif /* SERENITY_SRC_DATABASE_DATABASESCHEME_H_ */
