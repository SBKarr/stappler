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

#ifndef STELLATOR_DB_STSTORAGEFIELD_H_
#define STELLATOR_DB_STSTORAGEFIELD_H_

#include "STStorageInterface.h"
#include "STStorageQueryList.h"

NS_DB_BEGIN

template <typename F, typename V>
struct FieldOption;

enum class Type {
	None,
	Integer, // 64-bit signed integer
	Float, // 64-bit float
	Boolean, // true/false
	Text, // simple text
	Bytes, // binary data
	Data, // raw Value
	Extra, // raw binary object
	Object, // linked object (one-to-one connection or endpoint for many-to-one)
	Set, // linked set of objects (one-to-many)
	Array, // set of raw Value
	File,
	Image,
	View, // immutable predicate-based reference set of objects
	FullTextView, // full-text search resource

	Custom
};

inline bool checkIfComparationIsValid(Type t, Comparation c) {
	if (t == Type::Integer || t == Type::Object || t == Type::Float) {
		return true;
	} else if (t == Type::Bytes || t == Type::Text || t == Type::Boolean) {
		return c == Comparation::Equal || c == Comparation::NotEqual || c == Comparation::IsNull || c == Comparation::IsNotNull;
	} else if (t == Type::Custom) {
		return true;
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
	// deprecated: AutoNamed = 1 << 5, /** field will be automatically filled with new UUID */
	AutoCTime = 1 << 6, /** Property will be automatically set to object creation time */
	AutoMTime = 1 << 7, /** Property will be automatically update to modification time */
	AutoUser = 1 << 8, /** Property will be automatically set to current user (if available) */
	Indexed = 1 << 9, /** Create index, that allows select queries on that field */
	Admin = 1 << 10, /** Field can be accessed by administrative queries only */
	ForceInclude = 1 << 11, /** field will be internally included in all queries (useful for access control) */
	ForceExclude = 1 << 12, /** field will be excluded, if not requested directly */
	Composed = 1 << 13, /** propagate modification events from objects in that field (for object and set fields) */

	TsNormalize_DocLengthLog = 1 << 24, /** Text search normalization: divides the rank by 1 + the logarithm of the document length */
	TsNormalize_DocLength = 1 << 25, /** Text search normalization: divides the rank by the document length */
	TsNormalize_UniqueWordsCount = 1 << 26, /** Text search normalization: divides the rank by the number of unique words in document */
	TsNormalize_UniqueWordsCountLog = 1 << 27, /** Text search normalization: divides the rank by 1 + the logarithm of the number of unique words in document */
};

SP_DEFINE_ENUM_AS_MASK(Flags)

enum class Transform {
	None,

	// Text
	Text,
	Identifier,
	Alias,
	Url,
	Email,
	Number,
	Hexadecimial,
	Base64,

	// Bytes
	Uuid,
	PublicKey,

	// Extra
	Array, // handle extra field as array

	Password, // deprecated
};

enum class ValidationLevel {
	NamesAndTypes,
	Slots,
	Full,
};

enum class Linkage {
	Auto,
	Manual,
	None,
};

using MinLength = stappler::ValueWrapper<size_t, class MinLengthTag>; // min utf8 length for string
using MaxLength = stappler::ValueWrapper<size_t, class MaxLengthTag>; // max utf8 length for string
using PasswordSalt = stappler::ValueWrapper<mem::String, class PasswordSaltTag>; // hashing salt for password
using ForeignLink = stappler::ValueWrapper<mem::String, class ForeignLinkTag>; // hashing salt for password

// policy for images, that do not match bounds
enum class ImagePolicy {
	Resize, // resize to match bounds
	Reject, // reject input field
};

// max size for files
using MaxFileSize = stappler::ValueWrapper<size_t, class MaxFileSizeTag>;

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
	mem::String name;

	Thumbnail(mem::String && name, size_t w, size_t h)
	: width(w), height(h), name(std::move(name)) { }
};

// what to do if object is removed
enum class RemovePolicy {
	Cascade, // remove object in set or field
	Restrict, // reject request, if object or set is not empty
	Reference, // no linkage action, object is reference
	StrongReference, // only for Set: no linkage action, objects will be owned
	Null, // set object to null
};

// old-fashion filter fn (use WriteFilterFn instead)
using FilterFn = mem::Function<bool(const Scheme &, mem::Value &)>;

// function to deduce default value from object data
using DefaultFn = mem::Function<mem::Value(const mem::Value &)>;

// function to modify out value of object's field to return it to users
using ReadFilterFn = mem::Function<bool(const Scheme &, const mem::Value &obj, mem::Value &value)>;

// function to modify input value of object's field to write it into storage
using WriteFilterFn = mem::Function<bool(const Scheme &, const mem::Value &patch, mem::Value &value, bool isCreate)>;

// function to replace previous value of field with another
using ReplaceFilterFn = mem::Function<bool(const Scheme &, const mem::Value &obj, const mem::Value &oldValue, mem::Value &newValue)>;

// function to deduce root object ids list from object of external scheme
// Used by:
// - View field: to deduce id of root object id from external objects
// - AutoField: to deduce id of object with auto field from external objects
using ViewLinkageFn = mem::Function<mem::Vector<uint64_t>(const Scheme &targetScheme, const Scheme &objScheme, const mem::Value &obj)>;

// function to deduce view data from object of external scheme
using ViewFn = mem::Function<bool(const Scheme &objScheme, const mem::Value &obj)>;

// function to extract fulltext search data from object
using FullTextViewFn = mem::Function<mem::Vector<FullTextData>(const Scheme &objScheme, const mem::Value &obj)>;

// function to prepare fulltext query from input string
using FullTextQueryFn = mem::Function<mem::Vector<FullTextData>(const mem::Value &searchData)>;

struct AutoFieldScheme : mem::AllocBase {
	using ReqVec = mem::Vector<mem::String>;

	const Scheme &scheme;
	ReqVec requiresForAuto;

	ViewLinkageFn linkage;
	ReqVec requiresForLinking;

	AutoFieldScheme(const Scheme &, ReqVec && = ReqVec(), ViewLinkageFn && = nullptr, ReqVec && = ReqVec());
	AutoFieldScheme(const Scheme &, ReqVec &&, ReqVec &&);
};

struct AutoFieldDef {
	mem::Vector<AutoFieldScheme> schemes;
	DefaultFn defaultFn;
	mem::Vector<mem::String> requires;
};

struct FieldCustom;

class Field : public mem::AllocBase {
public:
	template <typename ... Args> static Field Data(mem::String && name, Args && ... args);
	template <typename ... Args> static Field Integer(mem::String && name, Args && ... args);
	template <typename ... Args> static Field Float(mem::String && name, Args && ... args);
	template <typename ... Args> static Field Boolean(mem::String && name, Args && ... args);
	template <typename ... Args> static Field Text(mem::String && name, Args && ... args);
	template <typename ... Args> static Field Bytes(mem::String &&name, Args && ... args);
	template <typename ... Args> static Field Password(mem::String && name, Args && ... args);
	template <typename ... Args> static Field Extra(mem::String &&name, Args && ... args);
	template <typename ... Args> static Field Extra(mem::String &&name, stappler::InitializerList<Field> &&, Args && ... args);
	template <typename ... Args> static Field File(mem::String &&name, Args && ... args);
	template <typename ... Args> static Field Image(mem::String &&name, Args && ... args);
	template <typename ... Args> static Field Object(mem::String &&name, Args && ... args);
	template <typename ... Args> static Field Set(mem::String && name, Args && ... args);
	template <typename ... Args> static Field Array(mem::String && name, Args && ... args);
	template <typename ... Args> static Field View(mem::String && name, Args && ... args);
	template <typename ... Args> static Field FullTextView(mem::String && name, Args && ... args);
	template <typename ... Args> static Field Custom(FieldCustom *);

	struct Slot : public mem::AllocBase {
	public:
		template <typename F, typename T>
		static void setOptions(F &f, T && t) {
			FieldOption<F, typename std::remove_reference<T>::type>::assign(f, std::forward<T>(t));
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

		Slot(mem::String && n, Type t) : name(n), type(t) { }

		mem::StringView getName() const { return name; }
		bool hasFlag(Flags f) const { return ((flags & f) != Flags::None); }
		Type getType() const { return type; }
		bool isProtected() const;
		Transform getTransform() const { return transform; }

		virtual bool isSimpleLayout() const { return type == Type::Integer || type == Type::Float ||
				type == Type::Boolean || type == Type::Text || type == Type::Bytes ||
				type == Type::Data || type == Type::Extra; }

		virtual bool isDataLayout() const { return type == Type::Data || type == Type::Extra; }

		bool isIndexed() const { return hasFlag(Flags::Indexed) || transform == Transform::Alias || type == Type::Object; }
		bool isFile() const { return type == Type::File || type == Type::Image; }

		virtual bool hasDefault() const;
		virtual mem::Value getDefault(const mem::Value &patch) const;

		virtual bool transformValue(const Scheme &, const mem::Value &, mem::Value &, bool isCreate) const;
		virtual void hash(mem::StringStream &stream, ValidationLevel l) const;

		mem::Value def;
		mem::String name;
		Flags flags = Flags::None;
		Type type = Type::None;
		Transform transform = Transform::None;
		DefaultFn defaultFn;

		ReadFilterFn readFilterFn;
		WriteFilterFn writeFilterFn;
		ReplaceFilterFn replaceFilterFn;

		AutoFieldDef autoField;
	};

	mem::StringView getName() const { return slot->getName(); }
	Type getType() const { return slot->getType(); }
	Transform getTransform() const { return slot->getTransform(); }
	mem::Value getDefault(const mem::Value &patch) const { return slot->getDefault(patch); }

	bool hasFlag(Flags f) const { return slot->hasFlag(f); }
	bool hasDefault() const { return slot->hasDefault(); }

	bool isProtected() const { return slot->isProtected(); }
	bool isSimpleLayout() const { return slot->isSimpleLayout(); }
	bool isDataLayout() const { return slot->isDataLayout(); }
	bool isIndexed() const { return slot->isIndexed(); }
	bool isFile() const { return slot->isFile(); }
	bool isReference() const;

	const Scheme * getForeignScheme() const;

	void hash(mem::StringStream &stream, ValidationLevel l) const { slot->hash(stream, l); }

	bool transform(const Scheme &, int64_t, mem::Value &, bool isCreate = false) const;
	bool transform(const Scheme &, const mem::Value &, mem::Value &, bool isCreate) const;

	operator bool () const { return slot != nullptr; }

	template <typename SlotType = Slot>
	auto getSlot() const -> const SlotType * { return static_cast<const SlotType *>(slot); }

	mem::Value getTypeDesc() const;

	Field(const Slot *s) : slot(s) { }

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
	FieldText(mem::String && n, Type t, Args && ... args) : Field::Slot(std::move(n), t) {
		init<FieldText, Args...>(*this, std::forward<Args>(args)...);
	}

	virtual bool transformValue(const Scheme &, const mem::Value &, mem::Value &, bool isCreate) const override;
	virtual void hash(mem::StringStream &stream, ValidationLevel l) const override;

	size_t minLength = config::getDefaultTextMin(), maxLength = config::getDefaultTextMax();
};

struct FieldPassword : Field::Slot {
	virtual ~FieldPassword() { }

	template <typename ... Args>
	FieldPassword(mem::String && n, Args && ... args) : Field::Slot(std::move(n), Type::Bytes) {
		init<FieldPassword, Args...>(*this, std::forward<Args>(args)...);
		transform = Transform::Password;
	}

	virtual bool transformValue(const Scheme &, const mem::Value &, mem::Value &, bool isCreate) const override;
	virtual void hash(mem::StringStream &stream, ValidationLevel l) const override;

	size_t minLength = config::getDefaultTextMin(), maxLength = config::getDefaultTextMax();
	mem::String salt = config::getDefaultPasswordSalt();
};

struct FieldExtra : Field::Slot {
	virtual ~FieldExtra() { }

	template <typename ... Args>
	FieldExtra(mem::String && n, Args && ... args) : Field::Slot(std::move(n), Type::Extra) {
		init<FieldExtra, Args...>(*this, std::forward<Args>(args)...);
	}

	virtual bool hasDefault() const override;
	virtual mem::Value getDefault(const mem::Value &) const override;

	virtual bool transformValue(const Scheme &, const mem::Value &, mem::Value &, bool isCreate) const override;
	virtual void hash(mem::StringStream &stream, ValidationLevel l) const override;

	mem::Map<mem::String, Field> fields;
};

struct FieldFile : Field::Slot {
	virtual ~FieldFile() { }

	template <typename ... Args>
	FieldFile(mem::String && n, Args && ... args) : Field::Slot(std::move(n), Type::File) {
		init<FieldFile, Args...>(*this, std::forward<Args>(args)...);
	}

	virtual void hash(mem::StringStream &stream, ValidationLevel l) const override;

	size_t maxSize = config::getMaxInputFileSize();
	mem::Vector<mem::String> allowedTypes;
};

struct FieldImage : Field::Slot {
	virtual ~FieldImage() { }

	template <typename ... Args>
	FieldImage(mem::String && n, Args && ... args) : Field::Slot(std::move(n), Type::Image) {
		init<FieldImage, Args...>(*this, std::forward<Args>(args)...);
	}

	virtual void hash(mem::StringStream &stream, ValidationLevel l) const override;

	size_t maxSize = config::getMaxInputFileSize();
	mem::Vector<mem::String> allowedTypes;
	MaxImageSize maxImageSize;
	MinImageSize minImageSize;
	mem::Vector<Thumbnail> thumbnails;
	bool primary = true;
};

struct FieldObject : Field::Slot {
	virtual ~FieldObject() { }

	template <typename ... Args>
	FieldObject(mem::String && n, Type t, Args && ... args) : Field::Slot(std::move(n), t) {
		init<FieldObject, Args...>(*this, std::forward<Args>(args)...);
		if (t == Type::Set && (stappler::toInt(flags) & stappler::toInt(Flags::Reference))) {
			if (onRemove != RemovePolicy::Reference && onRemove != RemovePolicy::StrongReference) {
				onRemove = RemovePolicy::Reference;
			}
		}
		if (t == Type::Set && (onRemove == RemovePolicy::Reference || onRemove == RemovePolicy::StrongReference)) {
			flags |= Flags::Reference;
		}
	}

	virtual bool transformValue(const Scheme &, const mem::Value &, mem::Value &, bool isCreate) const override;
	virtual void hash(mem::StringStream &stream, ValidationLevel l) const override;

	const Scheme *scheme = nullptr;
	RemovePolicy onRemove = RemovePolicy::Null;
	Linkage linkage = Linkage::Auto;
	mem::String link;
};

struct FieldArray : Field::Slot {
	virtual ~FieldArray() { }

	template <typename ... Args>
	FieldArray(mem::String && n, Args && ... args) : Field::Slot(std::move(n), Type::Array), tfield(new FieldText("", Type::Text)) {
		init<FieldArray, Args...>(*this, std::forward<Args>(args)...);
	}

	virtual bool transformValue(const Scheme &, const mem::Value &, mem::Value &, bool isCreate) const override;
	virtual void hash(mem::StringStream &stream, ValidationLevel l) const override;

	Field tfield;
};

struct FieldView : Field::Slot {
	enum DeltaOptions {
		Delta
	};

	virtual ~FieldView() { }

	template <typename ... Args>
	FieldView(mem::String && n, Args && ... args) : Field::Slot(std::move(n), Type::View) {
		init<FieldView, Args...>(*this, std::forward<Args>(args)...);
	}

	virtual bool transformValue(const Scheme &, const mem::Value &, mem::Value &, bool isCreate) const override { return false; }

	const Scheme *scheme = nullptr;
	mem::Vector<mem::String> requires;
	ViewLinkageFn linkage;
	ViewFn viewFn;
	bool delta = false;
};

struct FieldFullTextView : Field::Slot {
	virtual ~FieldFullTextView() { }

	template <typename ... Args>
	FieldFullTextView(mem::String && n, Args && ... args) : Field::Slot(std::move(n), Type::FullTextView) {
		init<FieldFullTextView, Args...>(*this, std::forward<Args>(args)...);
	}

	virtual bool transformValue(const Scheme &, const mem::Value &, mem::Value &, bool isCreate) const override { return false; }

	mem::Vector<FullTextData> parseQuery(const mem::Value &) const;

	mem::Vector<mem::String> requires;
	FullTextViewFn viewFn;
	FullTextQueryFn queryFn;
};

struct FieldCustom : Field::Slot {
	virtual ~FieldCustom() { }

	template <typename ... Args>
	FieldCustom(mem::String && n, Args && ... args) : Field::Slot(std::move(n), Type::Custom) {
		init<FieldCustom, Args...>(*this, std::forward<Args>(args)...);
	}

	virtual mem::Value readFromStorage(ResultInterface &, size_t row, size_t field) const = 0;
	virtual bool writeToStorage(QueryInterface &, mem::StringStream &, const mem::Value &) const = 0;

	virtual mem::StringView getTypeName() const = 0;

	virtual mem::String getIndexName() const { return name; }
	virtual mem::String getIndexField() const { return mem::toString("( \"", name, "\" )"); }

	virtual bool isComparationAllowed(Comparation) const { return false; }

	virtual void writeQuery(const Scheme &, stappler::sql::Query<db::Binder, mem::Interface>::WhereContinue &,
			stappler::sql::Operator, const mem::StringView &, stappler::sql::Comparation, const mem::Value &, const mem::Value &) const { };
};

template <typename ... Args> Field Field::Data(mem::String && name, Args && ... args) {
	auto newSlot = new Field::Slot(std::move(name), Type::Data);
	Slot::init<Field::Slot>(*newSlot, std::forward<Args>(args)...);
	return Field(newSlot);
}

template <typename ... Args> Field Field::Integer(mem::String && name, Args && ... args) {
	auto newSlot = new Field::Slot(std::move(name), Type::Integer);
	Slot::init<Field::Slot>(*newSlot, std::forward<Args>(args)...);
	return Field(newSlot);
}

template <typename ... Args> Field Field::Float(mem::String && name, Args && ... args) {
	auto newSlot = new Field::Slot(std::move(name), Type::Float);
	Slot::init<Field::Slot>(*newSlot, std::forward<Args>(args)...);
	return Field(newSlot);
}

template <typename ... Args> Field Field::Boolean(mem::String && name, Args && ... args) {
	auto newSlot = new Field::Slot(std::move(name), Type::Boolean);
	Slot::init<Field::Slot>(*newSlot, std::forward<Args>(args)...);
	return Field(newSlot);
}

template <typename ... Args> Field Field::Text(mem::String && name, Args && ... args) {
	return Field(new FieldText(std::move(name), Type::Text, std::forward<Args>(args)...));
}

template <typename ... Args> Field Field::Bytes(mem::String &&name, Args && ... args) {
	return Field(new FieldText(std::move(name), Type::Bytes, std::forward<Args>(args)...));
}

template <typename ... Args> Field Field::Password(mem::String && name, Args && ... args) {
	return Field(new FieldPassword(std::move(name), std::forward<Args>(args)...));
}

template <typename ... Args> Field Field::Extra(mem::String &&name, Args && ... args) {
	return Field(new FieldExtra(std::move(name), std::forward<Args>(args)...));
}

template <typename ... Args> Field Field::Extra(mem::String &&name, stappler::InitializerList<Field> &&f, Args && ... args) {
	return Field(new FieldExtra(std::move(name), move(f), std::forward<Args>(args)...));
}

template <typename ... Args> Field Field::File(mem::String &&name, Args && ... args) {
	return Field(new FieldFile(std::move(name), std::forward<Args>(args)...));
}

template <typename ... Args> Field Field::Image(mem::String &&name, Args && ... args) {
	return Field(new FieldImage(std::move(name), std::forward<Args>(args)...));
}

template <typename ... Args> Field Field::Object(mem::String &&name, Args && ... args) {
	return Field(new FieldObject(std::move(name), Type::Object, std::forward<Args>(args)...));
}

template <typename ... Args> Field Field::Set(mem::String && name, Args && ... args) {
	return Field(new FieldObject(std::move(name), Type::Set, std::forward<Args>(args)...));
}

template <typename ... Args> Field Field::Array(mem::String && name, Args && ... args) {
	return Field(new FieldArray(std::move(name), std::forward<Args>(args)...));
}

template <typename ... Args> Field Field::View(mem::String && name, Args && ... args) {
	return Field(new FieldView(std::move(name), std::forward<Args>(args)...));
}

template <typename ... Args> Field Field::FullTextView(mem::String && name, Args && ... args) {
	return Field(new FieldFullTextView(std::move(name), std::forward<Args>(args)...));
}

template <typename ... Args> Field Field::Custom(FieldCustom *custom) {
	return Field(custom);
}

template <typename F> struct FieldOption<F, Flags> {
	static inline void assign(F & f, Flags flags) { f.flags |= flags; }
};

template <typename F> struct FieldOption<F, FilterFn> {
	static inline void assign(F & f, const FilterFn &fn) {
		f.writeFilterFn = WriteFilterFn([fn] (const Scheme &scheme, const mem::Value &patch, mem::Value &value, bool isCreate) -> bool {
			return fn(scheme, value);
		});
	}
};

template <typename F> struct FieldOption<F, WriteFilterFn> {
	static inline void assign(F & f, const WriteFilterFn &fn) { f.writeFilterFn = fn; }
};

template <typename F> struct FieldOption<F, ReadFilterFn> {
	static inline void assign(F & f, const ReadFilterFn &fn) { f.readFilterFn = fn; }
};

template <typename F> struct FieldOption<F, ReplaceFilterFn> {
	static inline void assign(F & f, const ReplaceFilterFn &fn) { f.replaceFilterFn = fn; }
};

template <typename F> struct FieldOption<F, DefaultFn> {
	static inline void assign(F & f, const DefaultFn &fn) { f.defaultFn = fn; }
};

template <typename F> struct FieldOption<F, mem::Function<mem::Value()>> {
	static inline void assign(F & f, const mem::Function<mem::Value()> &fn) {
		f.defaultFn = DefaultFn([fn] (const mem::Value &) -> mem::Value { return fn(); });
	}
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

template <typename F> struct FieldOption<F, mem::Value> {
	static inline void assign(F & f, mem::Value && v) { f.def = std::move(v); }
};

template <typename F> struct FieldOption<F, PasswordSalt> {
	static inline void assign(F & f, PasswordSalt && s) { f.salt = std::move(s.get()); }
};

template <typename F> struct FieldOption<F, ForeignLink> {
	static inline void assign(F & f, ForeignLink && s) { f.link = std::move(s.get()); f.linkage = Linkage::Manual; }
};

template <typename F> struct FieldOption<F, mem::Vector<Field>> {
	static inline void assign(F & f, mem::Vector<Field> && s) {
		for (auto &it : s) {
			f.fields.emplace(it.getName().str<mem::Interface>(), it);
		}
	}
};

template <typename F> struct FieldOption<F, AutoFieldDef> {
	static inline void assign(F & f, AutoFieldDef &&def) {
		f.autoField = std::move(def);
	}
};

template <typename F> struct FieldOption<F, std::initializer_list<Field>> {
	static inline void assign(F & f, std::initializer_list<Field> && s) {
		for (auto &it : s) {
			f.fields.emplace(it.getName().str(), it);
		}
	}
};

template <typename F> struct FieldOption<F, MaxFileSize> {
	static inline void assign(F & f, MaxFileSize l) { f.maxSize = l.get(); }
};

template <typename F> struct FieldOption<F, mem::Vector<mem::String>> {
	static inline void assign(F & f, mem::Vector<mem::String> && l) { f.allowedTypes = std::move(l); }
};

template <typename F> struct FieldOption<F, MaxImageSize> {
	static inline void assign(F & f, MaxImageSize && s) { f.maxImageSize = std::move(s); }
};

template <typename F> struct FieldOption<F, MinImageSize> {
	static inline void assign(F & f, MinImageSize && s) { f.minImageSize = std::move(s); }
};

template <typename F> struct FieldOption<F, mem::Vector<Thumbnail>> {
	static inline void assign(F & f, mem::Vector<Thumbnail> && s) { f.thumbnails = std::move(s); }
};

template <typename F> struct FieldOption<F, RemovePolicy> {
	static inline void assign(F & f, RemovePolicy p) {
		f.onRemove = p;
		if (p == RemovePolicy::Reference || p == RemovePolicy::StrongReference) {
			f.flags |= Flags::Reference;
		}
	}
};

template <typename F> struct FieldOption<F, Linkage> {
	static inline void assign(F & f, Linkage p) { f.linkage = p; }
};

template <typename F> struct FieldOption<F, const Scheme *> {
	static inline void assign(F & f, const Scheme *s) { f.scheme = s; }
};
template <typename F> struct FieldOption<F, Scheme> {
	static inline void assign(F & f, const Scheme &s) { f.scheme = &s; }
};
template <typename F> struct FieldOption<F, Field> {
	static inline void assign(F & f, Field && s) { f.tfield = s; }
};

// view options

template <> struct FieldOption<FieldView, mem::Vector<mem::String>> {
	static inline void assign(FieldView & f, mem::Vector<mem::String> && s) {
		f.requires = std::move(s);
	}
};

template <> struct FieldOption<FieldFullTextView, mem::Vector<mem::String>> {
	static inline void assign(FieldFullTextView & f, mem::Vector<mem::String> && s) {
		f.requires = std::move(s);
	}
};

template <typename F> struct FieldOption<F, ViewLinkageFn> {
	static inline void assign(F & f, ViewLinkageFn && s) {
		f.linkage = std::move(s);
	}
};

template <typename F> struct FieldOption<F, ViewFn> {
	static inline void assign(F & f, ViewFn && s) {
		f.viewFn = std::move(s);
	}
};

template <typename F> struct FieldOption<F, FullTextViewFn> {
	static inline void assign(F & f, FullTextViewFn && s) {
		f.viewFn = std::move(s);
	}
};

template <typename F> struct FieldOption<F, FullTextQueryFn> {
	static inline void assign(F & f, FullTextQueryFn && s) {
		f.queryFn = std::move(s);
	}
};

template <typename F> struct FieldOption<F, FieldView::DeltaOptions> {
	static inline void assign(F & f, FieldView::DeltaOptions d) {
		if (d == FieldView::Delta) { f.delta = true; } else { f.delta = false; }
	}
};

NS_DB_END

#endif /* STELLATOR_DB_STSTORAGEFIELD_H_ */
