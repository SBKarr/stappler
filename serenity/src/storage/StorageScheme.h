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

#ifndef SERENITY_SRC_DATABASE_DATABASESCHEME_H_
#define SERENITY_SRC_DATABASE_DATABASESCHEME_H_

#include "StorageField.h"
#include "StorageQuery.h"
#include "InputFilter.h"

NS_SA_EXT_BEGIN(storage)

class Scheme : public AllocPool {
public:
	Scheme(const String &name);
	Scheme(const String &name, std::initializer_list<Field> il);

	void define(std::initializer_list<Field> il);
	void cloneFrom(Scheme *);

	const String &getName() const;
	bool hasAliases() const;

	bool isProtected(const String &) const;
	bool saveObject(Adapter *, Object *);

	bool hasFiles() const;

	const apr::map<apr::string, Field> & getFields() const;
	const Field *getField(const String &str) const;

	const Field *getForeignLink(const FieldObject *f) const;
	const Field *getForeignLink(const Field &f) const;
	const Field *getForeignLink(const String &f) const;

	size_t getMaxRequestSize() const { return maxRequestSize; }
	size_t getMaxVarSize() const { return maxRequestSize; }
	size_t getMaxFileSize() const { return maxRequestSize; }

	bool isAtomicPatch(const data::Value &) const;

	uint64_t hash(ValidationLevel l = ValidationLevel::NamesAndTypes) const;

public:// CRUD functions
	// returns Dictionary with single object data or Null value
	data::Value create(Adapter *, const data::Value &data, bool isProtected = false);
	data::Value create(Adapter *, const data::Value &data, apr::array<InputFile> &files, bool isProtected = false);

	data::Value get(Adapter *, uint64_t oid);
	data::Value get(Adapter *, const String &alias);
	data::Value get(Adapter *, const data::Value &id);

	data::Value update(Adapter *, uint64_t oid, const data::Value &data, bool isProtected = false);
	data::Value update(Adapter *, uint64_t oid, const data::Value &data, apr::array<InputFile> &files, bool isProtected = false);
	data::Value update(Adapter *, const data::Value & obj, const data::Value &data, bool isProtected = false);
	data::Value update(Adapter *, const data::Value & obj, const data::Value &data, apr::array<InputFile> &files, bool isProtected = false);

	bool remove(Adapter *, uint64_t oid);

	// returns Array with zero or more Dictionaries with object data or Null value
	data::Value select(Adapter *, const Query &);

	size_t count(Adapter *);
	size_t count(Adapter *, const Query &);

public:
	data::Value getProperty(Adapter *, uint64_t oid, const String &);
	data::Value getProperty(Adapter *, const data::Value &, const String &);

	data::Value setProperty(Adapter *, uint64_t oid, const String &, data::Value &&);
	data::Value setProperty(Adapter *, const data::Value &, const String &, data::Value &&);
	data::Value setProperty(Adapter *, uint64_t oid, const String &, InputFile &);
	data::Value setProperty(Adapter *, const data::Value &, const String &, InputFile &);

	void clearProperty(Adapter *, uint64_t oid, const String &);
	void clearProperty(Adapter *, const data::Value &, const String &);

	data::Value appendProperty(Adapter *, uint64_t oid, const String &, data::Value &&);
	data::Value appendProperty(Adapter *, const data::Value &, const String &, data::Value &&);

public:
	data::Value getProperty(Adapter *, uint64_t oid, const Field &);
	data::Value getProperty(Adapter *, const data::Value &, const Field &);

	data::Value setProperty(Adapter *, uint64_t oid, const Field &, data::Value &&);
	data::Value setProperty(Adapter *, const data::Value &, const Field &, data::Value &&);
	data::Value setProperty(Adapter *, uint64_t oid, const Field &, InputFile &);
	data::Value setProperty(Adapter *, const data::Value &, const Field &, InputFile &);

	void clearProperty(Adapter *, uint64_t oid, const Field &);
	void clearProperty(Adapter *, const data::Value &, const Field &);

	data::Value appendProperty(Adapter *, uint64_t oid, const Field &, data::Value &&);
	data::Value appendProperty(Adapter *, const data::Value &, const Field &, data::Value &&);

protected:
	data::Value createFilePatch(Adapter *, apr::array<InputFile> &);
	void purgeFilePatch(Adapter *, const data::Value &);
	void mergeValues(const Field &f, data::Value &original, data::Value &newVal);

	data::Value updateObject(Adapter *, data::Value && obj, data::Value &data);

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

	data::Value &transform(data::Value &, TransformAction = TransformAction::Create);

	// call before object is created, used for additional checking or default values
	data::Value createFile(Adapter *, const Field &, InputFile &);

	// call after object is created, used for custom field initialization
	data::Value initField(Adapter *, Object *, const Field &, const data::Value &);

	void prepareUpdate(Adapter *, const Field &, data::Value &changeset, const data::Value &value);

	void updateLimits();

	bool validateHint(uint64_t, const data::Value &);
	bool validateHint(const String &alias, const data::Value &);
	bool validateHint(const data::Value &);

	apr::map<apr::string, Field> fields;
	String name;

	size_t maxRequestSize = 0;
	size_t maxVarSize = 256;
	size_t maxFileSize = 0;
};

NS_SA_EXT_END(storage)

#endif /* SERENITY_SRC_DATABASE_DATABASESCHEME_H_ */
