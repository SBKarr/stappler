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

#ifndef STELLATOR_DB_STSTORAGEWORKER_H_
#define STELLATOR_DB_STSTORAGEWORKER_H_

#include "STStorageField.h"
#include "STStorageQueryList.h"
#include "STStorageTransaction.h"

NS_DB_BEGIN

enum class UpdateFlags : uint32_t {
	None = 0,
	Protected = 1 << 0,
	NoReturn = 1 << 1,
	GetAll = 1 << 2,
	GetForUpdate = 1 << 3,
};

SP_DEFINE_ENUM_AS_MASK(UpdateFlags)

class Worker : public mem::AllocBase {
public:
	using FieldCallback = stappler::Callback<void(const mem::StringView &name, const Field *f)>;

	struct RequiredFields {
		const Scheme *scheme = nullptr;
		mem::Vector<const Field *> includeFields;
		mem::Vector<const Field *> excludeFields;
		bool includeNone = false;
		bool includeAll = false;

		void clear();
		void reset(const Scheme &);

		void include(std::initializer_list<mem::StringView>);
		void include(const mem::Set<const Field *> &);
		void include(const mem::StringView &);
		void include(const Field *);

		void exclude(std::initializer_list<mem::StringView>);
		void exclude(const mem::Set<const Field *> &);
		void exclude(const mem::StringView &);
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

	bool readFields(const Scheme &, const FieldCallback &, const mem::Value &patchFields = mem::Value());
	void readFields(const Scheme &, const Query &, const FieldCallback &);

	bool shouldIncludeNone() const;
	bool shouldIncludeAll() const;

	Worker &asSystem();
	bool isSystem() const;

	const RequiredFields &getRequiredFields() const;

public:
	mem::Value get(uint64_t oid, bool forUpdate = false);
	mem::Value get(const mem::StringView &alias, bool forUpdate = false);
	mem::Value get(const mem::Value &id, bool forUpdate = false);

	mem::Value get(uint64_t oid, UpdateFlags);
	mem::Value get(const mem::StringView &alias, UpdateFlags);
	mem::Value get(const mem::Value &id, UpdateFlags);

	mem::Value get(uint64_t oid, std::initializer_list<mem::StringView> &&fields, bool forUpdate = false);
	mem::Value get(const mem::StringView &alias, std::initializer_list<mem::StringView> &&fields, bool forUpdate = false);
	mem::Value get(const mem::Value &id, std::initializer_list<mem::StringView> &&fields, bool forUpdate = false);

	mem::Value get(uint64_t oid, std::initializer_list<const char *> &&fields, bool forUpdate = false);
	mem::Value get(const mem::StringView &alias, std::initializer_list<const char *> &&fields, bool forUpdate = false);
	mem::Value get(const mem::Value &id, std::initializer_list<const char *> &&fields, bool forUpdate = false);

	mem::Value get(uint64_t oid, std::initializer_list<const Field *> &&fields, bool forUpdate = false);
	mem::Value get(const mem::StringView &alias, std::initializer_list<const Field *> &&fields, bool forUpdate = false);
	mem::Value get(const mem::Value &id, std::initializer_list<const Field *> &&fields, bool forUpdate = false);

	// returns Array with zero or more Dictionaries with object data or Null value
	mem::Value select(const Query &, UpdateFlags = UpdateFlags::None);

	// returns Dictionary with single object data or Null value
	mem::Value create(const mem::Value &data, bool isProtected = false);
	mem::Value create(const mem::Value &data, UpdateFlags);

	mem::Value update(uint64_t oid, const mem::Value &data, bool isProtected = false);
	mem::Value update(const mem::Value & obj, const mem::Value &data, bool isProtected = false);

	mem::Value update(uint64_t oid, const mem::Value &data, UpdateFlags);
	mem::Value update(const mem::Value & obj, const mem::Value &data, UpdateFlags);

	bool remove(uint64_t oid);

	size_t count();
	size_t count(const Query &);

	void touch(uint64_t id);
	void touch(const mem::Value & obj);

public:
	mem::Value getField(uint64_t oid, const mem::StringView &, std::initializer_list<mem::StringView> fields);
	mem::Value getField(const mem::Value &, const mem::StringView &, std::initializer_list<mem::StringView> fields);
	mem::Value getField(uint64_t oid, const mem::StringView &, const mem::Set<const Field *> & = mem::Set<const Field *>());
	mem::Value getField(const mem::Value &, const mem::StringView &, const mem::Set<const Field *> & = mem::Set<const Field *>());

	mem::Value setField(uint64_t oid, const mem::StringView &, mem::Value &&);
	mem::Value setField(const mem::Value &, const mem::StringView &, mem::Value &&);
	mem::Value setField(uint64_t oid, const mem::StringView &, InputFile &);
	mem::Value setField(const mem::Value &, const mem::StringView &, InputFile &);

	bool clearField(uint64_t oid, const mem::StringView &, mem::Value && = mem::Value());
	bool clearField(const mem::Value &, const mem::StringView &, mem::Value && = mem::Value());

	mem::Value appendField(uint64_t oid, const mem::StringView &, mem::Value &&);
	mem::Value appendField(const mem::Value &, const mem::StringView &, mem::Value &&);

	size_t countField(uint64_t oid, const mem::StringView &);
	size_t countField(const mem::Value &, const mem::StringView &);

public:
	mem::Value getField(uint64_t oid, const Field &, std::initializer_list<mem::StringView> fields);
	mem::Value getField(const mem::Value &, const Field &, std::initializer_list<mem::StringView> fields);
	mem::Value getField(uint64_t oid, const Field &, const mem::Set<const Field *> & = mem::Set<const Field *>());
	mem::Value getField(const mem::Value &, const Field &, const mem::Set<const Field *> & = mem::Set<const Field *>());

	mem::Value setField(uint64_t oid, const Field &, mem::Value &&);
	mem::Value setField(const mem::Value &, const Field &, mem::Value &&);
	mem::Value setField(uint64_t oid, const Field &, InputFile &);
	mem::Value setField(const mem::Value &, const Field &, InputFile &);

	bool clearField(uint64_t oid, const Field &, mem::Value && = mem::Value());
	bool clearField(const mem::Value &, const Field &, mem::Value && = mem::Value());

	mem::Value appendField(uint64_t oid, const Field &, mem::Value &&);
	mem::Value appendField(const mem::Value &, const Field &, mem::Value &&);

	size_t countField(uint64_t oid, const Field &);
	size_t countField(const mem::Value &, const Field &);

protected:
	mem::Set<const Field *> getFieldSet(const Field &f, std::initializer_list<mem::StringView> il) const;

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

NS_DB_END

#endif /* STELLATOR_DB_STSTORAGEWORKER_H_ */
