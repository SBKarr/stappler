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

#ifndef LIBS_STAPPLER_FEATURES_STORAGE_SPSCHEME_H
#define LIBS_STAPPLER_FEATURES_STORAGE_SPSCHEME_H

#include "SPData.h"
#include "SPStorage.h"

/**
 * storage module
 *
 * Storage is asynchronous database concept, back-ended by sqlite
 * all operations on storage (by key-value interface of Schemes) are
 * conditionally-asynchronous
 *
 * "Conditionally" means, that on base storage thread (named "SqlStorageThread")
 * all operations will be performed immediately. Callbacks will be called directly
 * from operation function call.
 *
 * On all other threads all actions will be scheduled on storage thread, callbacks
 * will be called on main thread after operations are completed.
 *
 * This concept allow you to extend Storage functionality, write your own logic,
 * backended by this system, that will work on storage thread without environment
 * overhead.
 */

NS_SP_EXT_BEGIN(storage)

/**
 *  Scheme - interface to sqlite table
 *  After creation, scheme lives forever in private memory
 *  Do NOT delete scheme manually, it will remove only pointer to it
 *  There is no reason to delete Scheme pointer, since it has minimal memory footprint
 *
 *  All memory operations in Scheme and Commands is automatic,
 *   you should not manually call new or delete
 */
class Scheme {
public:
	class Field;
	class Command;

	/** Available internal storage types */
	enum class Type : uint8_t {
		Real,
		Integer,
		Text,
		Blob
	};

	/** Modifiers for fields */
	enum Flags : uint8_t {
		IndexAsc = 1,
		IndexDesc = 2,
		PrimaryKey = 4,
	};

	using DataCallback = data::DataCallback;
	using CountCallback = Function<void(size_t)>;
	using SuccessCallback = Function<void(bool)>;
	using InsertCallback = Function<data::Value(size_t)>;

	using FieldsList = std::initializer_list<Field>;
	using AliasesList = std::initializer_list<std::pair<String, String>>;

public:
	Scheme();

	Scheme(const String &name, FieldsList &&, AliasesList && = {}, Handle * = nullptr);
	~Scheme();

	Scheme(const Scheme &);
	Scheme &operator= (const Scheme &);

	Scheme(Scheme &&);
	Scheme &operator= (Scheme &&);

	inline operator bool() const { return _internal != nullptr; }

	String getName() const;

	/** Returns new Get (SELECT *) command pointer
	 * Without options this command will return all rows in table */
	Command *get(const DataCallback & cb) const;

	/** Returns new Get (SELECT *) command pointer
	 * Without options this command will return all rows in table,
	 * callback will be called for every row separately */
	Command *getByRow(const DataCallback & cb) const;

	/** Returns new Count (SELECT COUNT(*)) command pointer
	 * Without options this command will return count of all rows in table */
	Command *count(const CountCallback &cb) const;

	/** Returns new Insert (UPDATE) command pointer
	 * Without options this command will insert given object as table row */
	Command *insert(const data::Value &val, const SuccessCallback &cb = nullptr) const;
	Command *insert(data::Value &&val, const SuccessCallback &cb = nullptr) const;

	/** Returns new Insert (UPDATE) command pointer
	 * Without options this command will insert given object as table row */
	Command *insertByRow(const InsertCallback & cb, size_t, const SuccessCallback & = nullptr) const;


	/** Returns new Remove (DELETE) command pointer
	 * Without options this command will remove all rows from table */
	Command *remove(const SuccessCallback &cb = nullptr) const;

	bool perform(const String &sqlString, const DataCallback &cb = nullptr) const;

protected:
	class Filter;

public:
	/**
	 * Field (column) specification for table requires name and field type,
	 * Optionally you can specify field size (like in INT(8))
	 * Available flags:
	 *  - PrimaryKey: make column primary for table (only one column can be primary)
	 *  - IndexAsc | IndexDesc: create index with specified direction on this column
	 */
	class Field {
	public:
		Field(const String &name, Type type, uint8_t flags = 0, uint8_t size = 0);

		Field(Field &&other);
		Field &operator= (Field &&other);

		Field(const Field &other) = delete;
		Field &operator= (const Field &other) = delete;
	protected:
		friend class Scheme;
		friend class Internal;

		String name;
		Type type;
		uint8_t size;
		uint8_t flags;
	};

	/**
	 * Command is a common way to perform (async) database operation
	 * Commands are created from template with one of Scheme's functions
	 * After command is executed (perform function), it will be destroyed intarnally
	 * You should not construct command manually in most use-cases
	 */
	class Command {
	public:
		enum Action {
			Get,
			Count,
			Insert,
			Remove
		};

		enum InsertMode {
			Replace,
			Abort,
			Ignore
		};

		Command(const Scheme *scheme, Action a);
		~Command();

		/** Adds filter, based on integer primary key.
		 * If primary key is not integer, command will not be performed.
		 * Allowed for Get, Count, Remove */
		Command *select(int64_t value);
		Command *select() { return this; }

		template<class T, typename std::enable_if<std::is_integral<T>{}>::type* = nullptr>
		Command *_select(T t) {
			return select((int64_t)t);
		}

		Command *_select(const String &str) {
			return select(str);
		}

		template <class T, class... Args>
		inline Command *select(T value, Args&&... args) { _select(value); return select(args...); }

		/** Adds filter, based on string primary key.
		 * If primary key is not string, command will not be performed.
		 * Allowed for Get, Count, Remove */
		Command *select(const String &value);

		/** Adds filter, based on integer column (field=value)
		 * If field is not integer or there is no index on
		 * field, command will not be performed.
		 * Allowed for Get, Count, Remove */
		Command *filterBy(const String &field, int64_t value);

		/** Adds filter, based on string column (field=value)
		 * If field is not string or there is no index on
		 * field, command will not be performed.
		 * Allowed for Get, Count, Remove */
		Command *filterBy(const String &field, const String &value);

		/** Adds filter, based on string column ( instr(field,value) )
		 * If field is not string, command will not be performed.
		 * Allowed for Get, Count, Remove */
		Command *filterLike(const String &field, const String &value);

		/** Sets ordering rules to field, command will not be performed,
		 * if there is no index on ordering field in specified direction.
		 * Allowed for Get */
		Command *orderBy(const String &field, Flags orderMode);

		/** Sets ordering rules to field, command will not be performed,
		 * if there is no index on ordering field in specified direction.
		 * Allowed for Get */
		Command *orderByPrimary(Flags orderMode);

		/** Limits number of returned rows.
		 * Allowed for Get, Remove */
		Command *limit(uint32_t count);

		/** Offsets resulting row number, if limit is specified.
		 * Allowed for Get, Remove */
		Command *offset(uint32_t offset);

		Command *setInsertMode(InsertMode);

		/** Performs command on database asynchronously. If command contains
		 * logical errors, returns false, command will not be performed. */
		bool perform();

	protected:
		friend class Scheme;

		const Scheme *_scheme = nullptr;
		Action _action = Get;
		bool _valid = true;
		bool _separateRows = false;
		bool _threaded = false;

		union Callback {
			DataCallback data;
			CountCallback count;
			SuccessCallback success;

			Callback();
			~Callback();
		} _callback;

		String _order;
		Flags _orderMode = IndexDesc;

		uint32_t _count = maxOf<uint32_t>();
		uint32_t _offset = maxOf<uint32_t>();
		InsertMode _insertMode = Replace;

		union Data {
			std::vector<Filter> filters;
			data::Value value;
			struct {
				InsertCallback cb;
				size_t count;
			} insert;

			Data();
			~Data();
		} _data;
	};

protected:
	/** Private struct to store filters in command */
	class Filter {
	public:
		enum Type : uint8_t {
			None,
			Integer,
			Text,
			Like,
		};

		Filter(const String &field, int64_t value);
		Filter(const String &field, const String &value, bool like = false);

		Filter(Filter &&other);
		Filter &operator= (Filter &&other);

		Filter(const Filter &other) = delete;
		Filter &operator= (const Filter &other) = delete;

		~Filter();

		Type type = None;
		String field;

		int64_t valueInteger = 0;
		String valueString;
	};

	class Internal;
	Internal *_internal = nullptr;

	Handle *_storage = nullptr;
};

NS_SP_EXT_END(storage)

#endif
