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

#ifndef SERENITY_SRC_STORAGE_STORAGEWORKER_H_
#define SERENITY_SRC_STORAGE_STORAGEWORKER_H_

#include "StorageTransaction.h"
#include "StorageField.h"
#include "StorageQuery.h"
#include "InputFilter.h"

NS_SA_EXT_BEGIN(storage)

enum class UpdateFlags : uint32_t {
	None = 0,
	Protected = 1 << 0,
	NoReturn = 1 << 1,
	GetAll = 1 << 2,
	GetForUpdate = 1 << 3,
};

SP_DEFINE_ENUM_AS_MASK(UpdateFlags)

class Worker : public AllocPool {
public:
	using FieldCallback = Callback<void(const StringView &name, const Field *f)>;

	struct RequiredFields {
		const Scheme *scheme = nullptr;
		Vector<const Field *> includeFields;
		Vector<const Field *> excludeFields;
		bool includeNone = false;
		bool includeAll = false;

		void clear();
		void reset(const Scheme &);

		void include(std::initializer_list<StringView>);
		void include(const Set<const Field *> &);
		void include(const StringView &);
		void include(const Field *);

		void exclude(std::initializer_list<StringView>);
		void exclude(const Set<const Field *> &);
		void exclude(const StringView &);
		void exclude(const Field *);
	};

	Worker(const Scheme &);
	Worker(const Scheme &, const Adapter &);
	Worker(const Scheme &, const Transaction &);
	explicit Worker(const Worker &);

	template <typename Callback>
	bool perform(const Callback &) const;

	const Transaction &transaction() const;
	const Scheme &scheme() const;

	void includeNone();

	template <typename T>
	void include(T && t) { _required.include(std::forward<T>(t)); }

	template <typename T>
	void exclude(T && t) { _required.exclude(std::forward<T>(t)); }

	void clearRequiredFields();

	bool readFields(const Scheme &, const FieldCallback &, const data::Value &patchFields = data::Value());
	void readFields(const Scheme &, const Query &, const FieldCallback &);

	bool shouldIncludeNone() const;
	bool shouldIncludeAll() const;

	Worker &asSystem();
	bool isSystem() const;

public:
	data::Value get(uint64_t oid, bool forUpdate = false);
	data::Value get(const String &alias, bool forUpdate = false);
	data::Value get(const data::Value &id, bool forUpdate = false);

	data::Value get(uint64_t oid, UpdateFlags);
	data::Value get(const String &alias, UpdateFlags);
	data::Value get(const data::Value &id, UpdateFlags);

	data::Value get(uint64_t oid, std::initializer_list<StringView> &&fields, bool forUpdate = false);
	data::Value get(const String &alias, std::initializer_list<StringView> &&fields, bool forUpdate = false);
	data::Value get(const data::Value &id, std::initializer_list<StringView> &&fields, bool forUpdate = false);

	data::Value get(uint64_t oid, std::initializer_list<const char *> &&fields, bool forUpdate = false);
	data::Value get(const String &alias, std::initializer_list<const char *> &&fields, bool forUpdate = false);
	data::Value get(const data::Value &id, std::initializer_list<const char *> &&fields, bool forUpdate = false);

	data::Value get(uint64_t oid, std::initializer_list<const Field *> &&fields, bool forUpdate = false);
	data::Value get(const String &alias, std::initializer_list<const Field *> &&fields, bool forUpdate = false);
	data::Value get(const data::Value &id, std::initializer_list<const Field *> &&fields, bool forUpdate = false);

	// returns Array with zero or more Dictionaries with object data or Null value
	data::Value select(const Query &);

	// returns Dictionary with single object data or Null value
	data::Value create(const data::Value &data, bool isProtected = false);
	data::Value create(const data::Value &data, UpdateFlags);

	data::Value update(uint64_t oid, const data::Value &data, bool isProtected = false);
	data::Value update(const data::Value & obj, const data::Value &data, bool isProtected = false);

	data::Value update(uint64_t oid, const data::Value &data, UpdateFlags);
	data::Value update(const data::Value & obj, const data::Value &data, UpdateFlags);

	bool remove(uint64_t oid);

	size_t count();
	size_t count(const Query &);

	void touch(uint64_t id);
	void touch(const data::Value & obj);

public:
	data::Value getField(uint64_t oid, const StringView &, std::initializer_list<StringView> fields);
	data::Value getField(const data::Value &, const StringView &, std::initializer_list<StringView> fields);
	data::Value getField(uint64_t oid, const StringView &, const Set<const Field *> & = Set<const Field *>());
	data::Value getField(const data::Value &, const StringView &, const Set<const Field *> & = Set<const Field *>());

	data::Value setField(uint64_t oid, const StringView &, data::Value &&);
	data::Value setField(const data::Value &, const StringView &, data::Value &&);
	data::Value setField(uint64_t oid, const StringView &, InputFile &);
	data::Value setField(const data::Value &, const StringView &, InputFile &);

	bool clearField(uint64_t oid, const StringView &, data::Value && = data::Value());
	bool clearField(const data::Value &, const StringView &, data::Value && = data::Value());

	data::Value appendField(uint64_t oid, const StringView &, data::Value &&);
	data::Value appendField(const data::Value &, const StringView &, data::Value &&);

	size_t countField(uint64_t oid, const StringView &);
	size_t countField(const data::Value &, const StringView &);

public:
	data::Value getField(uint64_t oid, const Field &, std::initializer_list<StringView> fields);
	data::Value getField(const data::Value &, const Field &, std::initializer_list<StringView> fields);
	data::Value getField(uint64_t oid, const Field &, const Set<const Field *> & = Set<const Field *>());
	data::Value getField(const data::Value &, const Field &, const Set<const Field *> & = Set<const Field *>());

	data::Value setField(uint64_t oid, const Field &, data::Value &&);
	data::Value setField(const data::Value &, const Field &, data::Value &&);
	data::Value setField(uint64_t oid, const Field &, InputFile &);
	data::Value setField(const data::Value &, const Field &, InputFile &);

	bool clearField(uint64_t oid, const Field &, data::Value && = data::Value());
	bool clearField(const data::Value &, const Field &, data::Value && = data::Value());

	data::Value appendField(uint64_t oid, const Field &, data::Value &&);
	data::Value appendField(const data::Value &, const Field &, data::Value &&);

	size_t countField(uint64_t oid, const Field &);
	size_t countField(const data::Value &, const Field &);

protected:
	Set<const Field *> getFieldSet(const Field &f, std::initializer_list<StringView> il) const;

	RequiredFields _required;
	const Scheme *_scheme = nullptr;
	Transaction _transaction;
	bool _isSystem = false;
};

template <typename Callback>
inline bool Worker::perform(const Callback &cb) const {
	return _transaction.perform([&] () -> bool {
		return cb(_transaction);
	});
}

NS_SA_EXT_END(storage)

#endif /* SERENITY_SRC_STORAGE_STORAGEWORKER_H_ */
