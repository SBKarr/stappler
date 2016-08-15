/*
 * DatabaseField.h
 *
 *  Created on: 27 янв. 2016 г.
 *      Author: sbkarr
 */

#ifndef SERENITY_SRC_DATABASE_DATABASEFIELD_H_
#define SERENITY_SRC_DATABASE_DATABASEFIELD_H_

#include "StorageQuery.h"

NS_SA_EXT_BEGIN(storage)

template <typename F, typename V>
struct FieldOption;

enum class Type {
	None,
	Integer, // 64-bit signed integer
	Float, // 64-bit float
	Boolean, // true/false
	Text, // simple text
	Bytes, // binary data
	Data, // raw data::Value
	Extra, // raw binary object
	Object, // linked object (one-to-one connection or endpoint for many-to-one)
	Set, // linked set of objects (one-to-many)
	Array, // set of raw data::Value
	File,
	Image,
};

inline bool checkIfComparationIsValid(Type t, Comparation c) {
	if (t == Type::Integer || t == Type::Object || t == Type::Float) {
		return true;
	} else if (t == Type::Text || t == Type::Boolean) {
		return c == Comparation::Equal || c == Comparation::NotEqual;
	}
	return false;
}

enum class Flags : uint32_t {
	/** empty flag */
	None = 0,

	Required = 1 << 0, /** field is required to create a new object, this field can not be removed from object */
	Protected = 1 << 1, /** field does not appear in client's direct output */
	ReadOnly = 1 << 2, /** field can not be modified by client's edit request */

	Reference = 1 << 3, /** object or set stored by reference */
	Unique = 1 << 4, /** field or array should contain only unique values */
	AutoNamed = 1 << 5, /** field will be automatically filled with new UUID */
	AutoCTime = 1 << 6, /** Property will be automatically set to object creation time */
	AutoMTime = 1 << 7, /** Property will be automatically update to modification time */
	AutoUser = 1 << 8, /** Property will be automatically set to current user (if available) */
	Indexed = 1 << 9, /** Create index, that allows select queries on that field */
	Admin = 1 << 10, /** Field can be accessed by administrative queries only */
};

SP_DEFINE_ENUM_AS_MASK(Flags)

enum class Transform {
	None,

	Text,
	Identifier,
	Alias,
	Url,
	Email,
	Number,
	Hexadecimial,
	Base64,

	Password,
};

enum class ValidationLevel {
	NamesAndTypes,
	Slots,
	Full,
};

enum class Linkage {
	Auto,
	Manual,
};

using MinLength = ValueWrapper<size_t, class MinLengthTag>; // min utf8 length for string
using MaxLength = ValueWrapper<size_t, class MaxLengthTag>; // max utf8 length for string
using PasswordSalt = ValueWrapper<apr::string, class PasswordSaltTag>; // hashing salt for password
using ForeignLink = ValueWrapper<apr::string, class ForeignLinkTag>; // hashing salt for password

// policy for images, that do not match bounds
enum class ImagePolicy {
	Resize, // resize to match bounds
	Reject, // reject input field
};

// max size for files
using MaxFileSize = ValueWrapper<size_t, class MaxFileSizeTag>;

struct MaxImageSize {
	size_t width = 128;
	size_t height = 128;
	ImagePolicy policy = ImagePolicy::Resize;

	MaxImageSize(): width(128), height(128), policy(ImagePolicy::Resize) { }
	MaxImageSize(size_t w, size_t h, ImagePolicy p = ImagePolicy::Resize)
	: width(w), height(h), policy(p) { }
};

struct MinImageSize {
	size_t width = 0;
	size_t height = 0;
	ImagePolicy policy = ImagePolicy::Reject;

	MinImageSize(): width(0), height(0), policy(ImagePolicy::Reject) { }
	MinImageSize(size_t w, size_t h, ImagePolicy p = ImagePolicy::Reject)
	: width(w), height(h), policy(p) { }
};

struct Thumbnail {
	size_t width;
	size_t height;
	String name;

	Thumbnail(String && name, size_t w, size_t h)
	: width(w), height(h), name(std::move(name)) { }
};

// what to do if object is removed
enum class RemovePolicy {
	Cascade, // remove object in set or field
	Restrict, // reject request, if object or set is not empty
	Reference, // no linkage action, object is reference
	Null, // set object to null
};

using FilterFn = Function<bool(Scheme &, data::Value &)>;
using DefaultFn = Function<data::Value()>;

class Field : public AllocPool {
public:
	template <typename ... Args> static Field Data(String && name, Args && ... args);
	template <typename ... Args> static Field Integer(String && name, Args && ... args);
	template <typename ... Args> static Field Float(String && name, Args && ... args);
	template <typename ... Args> static Field Boolean(String && name, Args && ... args);
	template <typename ... Args> static Field Text(String && name, Args && ... args);
	template <typename ... Args> static Field Bytes(String &&name, Args && ... args);
	template <typename ... Args> static Field Password(String && name, Args && ... args);
	template <typename ... Args> static Field Extra(String &&name, Args && ... args);
	template <typename ... Args> static Field File(String &&name, Args && ... args);
	template <typename ... Args> static Field Image(String &&name, Args && ... args);
	template <typename ... Args> static Field Object(String &&name, Args && ... args);
	template <typename ... Args> static Field Set(String && name, Args && ... args);
	template <typename ... Args> static Field Array(String && name, Args && ... args);

	struct Slot : public AllocPool {
	public:
		template <typename F, typename T>
		static void setOptions(F &f, T && t) {
			FieldOption<F, T>::assign(f, std::forward<T>(t));
		}

		template <typename F, typename T, typename ... Args>
		static void setOptions(F &f, T && t, Args && ... args) {
			setOptions(f, std::forward<T>(t));
			setOptions(f, std::forward<Args>(args)...);
		}

		template <typename F>
		static void init(F &f) { };

		template <typename F, typename ... Args>
		static void init(F &f, Args && ... args) {
			setOptions(f, std::forward<Args>(args)...);
		};

		Slot(apr::string && n, Type t) : name(n), type(t) { }

		const apr::string &getName() const { return name; }
		bool hasFlag(Flags f) const { return ((flags & f) != Flags::None); }
		Type getType() const { return type; }
		bool isProtected() const;
		Transform getTransform() const { return transform; }

		virtual bool hasDefault() const { return defaultFn || !def.isNull(); }
		virtual data::Value getDefault() const { if (defaultFn) { return defaultFn(); } else { return def; } }

		bool isSimpleLayout() const { return type == Type::Integer || type == Type::Float ||
				type == Type::Boolean || type == Type::Text || type == Type::Bytes ||
				type == Type::Data || type == Type::Extra; }

		bool isDataLayout() const { return type == Type::Data || type == Type::Extra; }

		bool isIndexed() const { return hasFlag(Flags::Indexed) || transform == Transform::Alias || type == Type::Object; }
		bool isFile() const { return type == Type::File || type == Type::Image; }

		virtual bool transformValue(Scheme &, data::Value &) const;
		virtual void hash(apr::ostringstream &stream, ValidationLevel l) const;

		data::Value def;
		apr::string name;
		Flags flags = Flags::None;
		Type type = Type::None;
		Transform transform = Transform::None;
		FilterFn filter;
		DefaultFn defaultFn;
	};

	const apr::string &getName() const { return slot->getName(); }
	Type getType() const { return slot->getType(); }
	Transform getTransform() const { return slot->getTransform(); }
	data::Value getDefault() const { return slot->getDefault(); }

	bool hasFlag(Flags f) const { return slot->hasFlag(f); }
	bool hasDefault() const { return slot->hasDefault(); }

	bool isProtected() const { return slot->isProtected(); }
	bool isSimpleLayout() const { return slot->isSimpleLayout(); }
	bool isDataLayout() const { return slot->isDataLayout(); }
	bool isIndexed() const { return slot->isIndexed(); }
	bool isFile() const { return slot->isFile(); }
	bool isReference() const;
	Scheme * getForeignScheme() const;

	void hash(apr::ostringstream &stream, ValidationLevel l) const { slot->hash(stream, l); }

	bool transform(Scheme &, data::Value &) const;

	operator bool () const { return slot != nullptr; }

	const Slot *getSlot() const { return slot; }

	data::Value getTypeDesc() const;

	Field(Slot *s) : slot(s) { }

	Field(const Field & s) = default;
	Field &operator=(const Field & s) = default;

	Field(Field && s) = default;
	Field &operator=(Field && s) = default;

protected:
	const Slot *slot;
};


struct FieldText : Field::Slot {
	virtual ~FieldText() { }

	template <typename ... Args>
	FieldText(apr::string && n, Type t, Args && ... args) : Field::Slot(std::move(n), t) {
		init<FieldText, Args...>(*this, std::forward<Args>(args)...);
	}

	virtual bool transformValue(Scheme &, data::Value &) const override;
	virtual void hash(apr::ostringstream &stream, ValidationLevel l) const override;

	size_t minLength = config::getDefaultTextMin(), maxLength = config::getDefaultTextMax();
};

struct FieldPassword : Field::Slot {
	virtual ~FieldPassword() { }

	template <typename ... Args>
	FieldPassword(apr::string && n, Args && ... args) : Field::Slot(std::move(n), Type::Bytes) {
		init<FieldPassword, Args...>(*this, std::forward<Args>(args)...);
		transform = Transform::Password;
	}

	virtual bool transformValue(Scheme &, data::Value &) const override;
	virtual void hash(apr::ostringstream &stream, ValidationLevel l) const override;

	size_t minLength = config::getDefaultTextMin(), maxLength = config::getDefaultTextMax();
	apr::string salt = config::getDefaultPasswordSalt();
};

struct FieldExtra : Field::Slot {
	virtual ~FieldExtra() { }

	template <typename ... Args>
	FieldExtra(apr::string && n, Args && ... args) : Field::Slot(std::move(n), Type::Extra) {
		init<FieldExtra, Args...>(*this, std::forward<Args>(args)...);
	}

	virtual bool hasDefault() const override;
	virtual data::Value getDefault() const override;

	virtual bool transformValue(Scheme &, data::Value &) const override;
	virtual void hash(apr::ostringstream &stream, ValidationLevel l) const override;

	apr::map<String, Field> fields;
};

struct FieldFile : Field::Slot {
	virtual ~FieldFile() { }

	template <typename ... Args>
	FieldFile(apr::string && n, Args && ... args) : Field::Slot(std::move(n), Type::File) {
		init<FieldFile, Args...>(*this, std::forward<Args>(args)...);
	}

	virtual void hash(apr::ostringstream &stream, ValidationLevel l) const override;

	size_t maxSize = config::getMaxInputFileSize();
	Vector<String> allowedTypes;
};

struct FieldImage : Field::Slot {
	virtual ~FieldImage() { }

	template <typename ... Args>
	FieldImage(apr::string && n, Args && ... args) : Field::Slot(std::move(n), Type::Image) {
		init<FieldImage, Args...>(*this, std::forward<Args>(args)...);
	}

	virtual void hash(apr::ostringstream &stream, ValidationLevel l) const override;

	size_t maxSize = config::getMaxInputFileSize();
	Vector<String> allowedTypes;
	MaxImageSize maxImageSize;
	MinImageSize minImageSize;
	Vector<Thumbnail> thumbnails;
	bool primary = true;
};

struct FieldObject : Field::Slot {
	virtual ~FieldObject() { }

	template <typename ... Args>
	FieldObject(apr::string && n, Type t, Args && ... args) : Field::Slot(std::move(n), t) {
		init<FieldObject, Args...>(*this, std::forward<Args>(args)...);
	}

	virtual void hash(apr::ostringstream &stream, ValidationLevel l) const override;

	Scheme *scheme = nullptr;
	RemovePolicy onRemove = RemovePolicy::Null;
	Linkage linkage;
	String link;
};

struct FieldArray : Field::Slot {
	virtual ~FieldArray() { }

	template <typename ... Args>
	FieldArray(apr::string && n, Args && ... args) : Field::Slot(std::move(n), Type::Array), tfield(new FieldText("", Type::Text)) {
		init<FieldArray, Args...>(*this, std::forward<Args>(args)...);
	}

	virtual bool transformValue(Scheme &, data::Value &) const override;
	virtual void hash(apr::ostringstream &stream, ValidationLevel l) const override;

	Field tfield;
};

template <typename ... Args> Field Field::Data(String && name, Args && ... args) {
	auto slot = new Field::Slot(std::move(name), Type::Data);
	Slot::init<Field::Slot>(*slot, std::forward<Args>(args)...);
	return Field(slot);
}

template <typename ... Args> Field Field::Integer(String && name, Args && ... args) {
	auto slot = new Field::Slot(std::move(name), Type::Integer);
	Slot::init<Field::Slot>(*slot, std::forward<Args>(args)...);
	return Field(slot);
}

template <typename ... Args> Field Field::Float(String && name, Args && ... args) {
	auto slot = new Field::Slot(std::move(name), Type::Float);
	Slot::init<Field::Slot>(*slot, std::forward<Args>(args)...);
	return Field(slot);
}

template <typename ... Args> Field Field::Boolean(String && name, Args && ... args) {
	auto slot = new Field::Slot(std::move(name), Type::Boolean);
	Slot::init<Field::Slot>(*slot, std::forward<Args>(args)...);
	return Field(slot);
}

template <typename ... Args> Field Field::Text(String && name, Args && ... args) {
	return Field(new FieldText(std::move(name), Type::Text, std::forward<Args>(args)...));
}

template <typename ... Args> Field Field::Bytes(String &&name, Args && ... args) {
	return Field(new FieldText(std::move(name), Type::Bytes, std::forward<Args>(args)...));
}

template <typename ... Args> Field Field::Password(String && name, Args && ... args) {
	return Field(new FieldPassword(std::move(name), std::forward<Args>(args)...));
}

template <typename ... Args> Field Field::Extra(String &&name, Args && ... args) {
	return Field(new FieldExtra(std::move(name), std::forward<Args>(args)...));
}

template <typename ... Args> Field Field::File(String &&name, Args && ... args) {
	return Field(new FieldFile(std::move(name), std::forward<Args>(args)...));
}

template <typename ... Args> Field Field::Image(String &&name, Args && ... args) {
	return Field(new FieldImage(std::move(name), std::forward<Args>(args)...));
}

template <typename ... Args> Field Field::Object(String &&name, Args && ... args) {
	return Field(new FieldObject(std::move(name), Type::Object, std::forward<Args>(args)...));
}

template <typename ... Args> Field Field::Set(String && name, Args && ... args) {
	return Field(new FieldObject(std::move(name), Type::Set, std::forward<Args>(args)...));
}

template <typename ... Args> Field Field::Array(String && name, Args && ... args) {
	return Field(new FieldArray(std::move(name), std::forward<Args>(args)...));
}


template <typename F> struct FieldOption<F, Flags> {
	static inline void assign(F & f, Flags flags) { f.flags = flags; }
};

template <typename F> struct FieldOption<F, FilterFn> {
	static inline void assign(F & f, const FilterFn &fn) { f.filter = fn; }
};

template <typename F> struct FieldOption<F, DefaultFn> {
	static inline void assign(F & f, const DefaultFn &fn) { f.defaultFn = fn; }
};

template <typename F> struct FieldOption<F, Transform> {
	static inline void assign(F & f, Transform t) { f.transform = t; }
};

template <typename F> struct FieldOption<F, MinLength> {
	static inline void assign(F & f, MinLength l) { f.minLength = l.get(); }
};

template <typename F> struct FieldOption<F, MaxLength> {
	static inline void assign(F & f, MaxLength l) { f.maxLength = l.get(); }
};

template <typename F> struct FieldOption<F, data::Value> {
	static inline void assign(F & f, data::Value && v) { f.def = std::move(v); }
};

template <typename F> struct FieldOption<F, PasswordSalt> {
	static inline void assign(F & f, PasswordSalt && s) { f.salt = std::move(s.get()); }
};

template <typename F> struct FieldOption<F, ForeignLink> {
	static inline void assign(F & f, ForeignLink && s) { f.link = std::move(s.get()); f.linkage = Linkage::Manual; }
};

template <typename F> struct FieldOption<F, Vector<Field>> {
	static inline void assign(F & f, Vector<Field> && s) {
		for (auto &it : s) {
			f.fields.emplace(it.getName(), it);
		}
	}
};

template <typename F> struct FieldOption<F, MaxFileSize> {
	static inline void assign(F & f, MaxFileSize l) { f.maxSize = l.get(); }
};

template <typename F> struct FieldOption<F, Vector<String>> {
	static inline void assign(F & f, Vector<String> && l) { f.allowedTypes = std::move(l); }
};

template <typename F> struct FieldOption<F, MaxImageSize> {
	static inline void assign(F & f, MaxImageSize && s) { f.maxImageSize = std::move(s); }
};

template <typename F> struct FieldOption<F, MinImageSize> {
	static inline void assign(F & f, MinImageSize && s) { f.minImageSize = std::move(s); }
};

template <typename F> struct FieldOption<F, Vector<Thumbnail>> {
	static inline void assign(F & f, Vector<Thumbnail> && s) { f.thumbnails = std::move(s); }
};

template <typename F> struct FieldOption<F, RemovePolicy> {
	static inline void assign(F & f, RemovePolicy p) { f.onRemove = p; }
};

template <typename F> struct FieldOption<F, Linkage> {
	static inline void assign(F & f, Linkage p) { f.linkage = p; }
};

template <typename F> struct FieldOption<F, Scheme * &> {
	static inline void assign(F & f, Scheme * & s) { f.scheme = s; }
};
template <typename F> struct FieldOption<F, Field> {
	static inline void assign(F & f, Field && s) { f.tfield = s; }
};

NS_SA_EXT_END(storage)

#endif /* SERENITY_SRC_DATABASE_DATABASEFIELD_H_ */
