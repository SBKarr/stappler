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
	View, // immutable predicate-based reference set of objects
	FullTextView, // full-text search resource
};

inline bool checkIfComparationIsValid(Type t, Comparation c) {
	if (t == Type::Integer || t == Type::Object || t == Type::Float) {
		return true;
	} else if (t == Type::Bytes || t == Type::Text || t == Type::Boolean) {
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
	// deprecated: AutoNamed = 1 << 5, /** field will be automatically filled with new UUID */
	AutoCTime = 1 << 6, /** Property will be automatically set to object creation time */
	AutoMTime = 1 << 7, /** Property will be automatically update to modification time */
	AutoUser = 1 << 8, /** Property will be automatically set to current user (if available) */
	Indexed = 1 << 9, /** Create index, that allows select queries on that field */
	Admin = 1 << 10, /** Field can be accessed by administrative queries only */
	ForceInclude = 1 << 11, /** field will be internally included in all queries (useful for access control) */
	Composed = 1 << 12, /** propagate modification events from objects in that field (for object and set fields) */

	TsNormalize_DocLengthLog = 1 << 24, /** Text search normalization: divides the rank by 1 + the logarithm of the document length */
	TsNormalize_DocLength = 1 << 25, /** Text search normalization: divides the rank by the document length */
	TsNormalize_UniqueWordsCount = 1 << 26, /** Text search normalization: divides the rank by the number of unique words in document */
	TsNormalize_UniqueWordsCountLog = 1 << 27, /** Text search normalization: divides the rank by 1 + the logarithm of the number of unique words in document */
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

	Uuid,
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

using MinLength = ValueWrapper<size_t, class MinLengthTag>; // min utf8 length for string
using MaxLength = ValueWrapper<size_t, class MaxLengthTag>; // max utf8 length for string
using PasswordSalt = ValueWrapper<String, class PasswordSaltTag>; // hashing salt for password
using ForeignLink = ValueWrapper<String, class ForeignLinkTag>; // hashing salt for password

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
	StrongReference, // only for Set: no linkage action, objects will be owned
	Null, // set object to null
};

using FilterFn = Function<bool(const Scheme &, data::Value &)>;
using DefaultFn = Function<data::Value(const data::Value &)>;

using ViewLinkageFn = Function<Vector<uint64_t>(const Scheme &targetScheme, const Scheme &objScheme, const data::Value &obj)>;
using ViewFn = Function<Vector<data::Value>(const Scheme &objScheme, const data::Value &obj)>;

using FullTextViewFn = Function<Vector<FullTextData>(const Scheme &objScheme, const data::Value &obj)>;
using FullTextQueryFn = Function<Vector<FullTextData>(const data::Value &searchData)>;

enum class FieldAccessAction {
	Read,
	Write,
	Create
};

using FieldAccessFn = Function<bool(FieldAccessAction, const Scheme &, const data::Value &obj, data::Value &value)>;

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
	template <typename ... Args> static Field View(String && name, Args && ... args);
	template <typename ... Args> static Field FullTextView(String && name, Args && ... args);

	template <typename ... Args> static Field Extra(String &&name, InitializerList<Field> &&, Args && ... args);

	struct Slot : public AllocPool {
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

		Slot(String && n, Type t) : name(n), type(t) { }

		StringView getName() const { return name; }
		bool hasFlag(Flags f) const { return ((flags & f) != Flags::None); }
		Type getType() const { return type; }
		bool isProtected() const;
		Transform getTransform() const { return transform; }

		bool isSimpleLayout() const { return type == Type::Integer || type == Type::Float ||
				type == Type::Boolean || type == Type::Text || type == Type::Bytes ||
				type == Type::Data || type == Type::Extra; }

		bool isDataLayout() const { return type == Type::Data || type == Type::Extra; }

		bool isIndexed() const { return hasFlag(Flags::Indexed) || transform == Transform::Alias || type == Type::Object; }
		bool isFile() const { return type == Type::File || type == Type::Image; }

		virtual bool hasDefault() const;
		virtual data::Value getDefault(const data::Value &patch) const;

		virtual bool transformValue(FieldAccessAction, const Scheme &, const data::Value &, data::Value &) const;
		virtual void hash(StringStream &stream, ValidationLevel l) const;

		data::Value def;
		String name;
		Flags flags = Flags::None;
		Type type = Type::None;
		Transform transform = Transform::None;
		FilterFn filter;
		DefaultFn defaultFn;
		FieldAccessFn fieldAccessFn;
	};

	StringView getName() const { return slot->getName(); }
	Type getType() const { return slot->getType(); }
	Transform getTransform() const { return slot->getTransform(); }
	data::Value getDefault(const data::Value &patch) const { return slot->getDefault(patch); }

	const FieldAccessFn &getAccessFn() const { return slot->fieldAccessFn; }

	bool hasFlag(Flags f) const { return slot->hasFlag(f); }
	bool hasDefault() const { return slot->hasDefault(); }

	bool isProtected() const { return slot->isProtected(); }
	bool isSimpleLayout() const { return slot->isSimpleLayout(); }
	bool isDataLayout() const { return slot->isDataLayout(); }
	bool isIndexed() const { return slot->isIndexed(); }
	bool isFile() const { return slot->isFile(); }
	bool isReference() const;

	bool hasAccessChecker() const { return slot->fieldAccessFn != nullptr; }

	const Scheme * getForeignScheme() const;

	void hash(StringStream &stream, ValidationLevel l) const { slot->hash(stream, l); }

	bool transform(FieldAccessAction, const Scheme &, int64_t, data::Value &) const;
	bool transform(FieldAccessAction, const Scheme &, const data::Value &, data::Value &) const;

	operator bool () const { return slot != nullptr; }

	template <typename SlotType = Slot>
	auto getSlot() const -> const SlotType * { return static_cast<const SlotType *>(slot); }

	data::Value getTypeDesc() const;

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
	FieldText(String && n, Type t, Args && ... args) : Field::Slot(move(n), t) {
		init<FieldText, Args...>(*this, std::forward<Args>(args)...);
	}

	virtual bool transformValue(FieldAccessAction, const Scheme &, const data::Value &, data::Value &) const override;
	virtual void hash(StringStream &stream, ValidationLevel l) const override;

	size_t minLength = config::getDefaultTextMin(), maxLength = config::getDefaultTextMax();
};

struct FieldPassword : Field::Slot {
	virtual ~FieldPassword() { }

	template <typename ... Args>
	FieldPassword(String && n, Args && ... args) : Field::Slot(move(n), Type::Bytes) {
		init<FieldPassword, Args...>(*this, std::forward<Args>(args)...);
		transform = Transform::Password;
	}

	virtual bool transformValue(FieldAccessAction, const Scheme &, const data::Value &, data::Value &) const override;
	virtual void hash(StringStream &stream, ValidationLevel l) const override;

	size_t minLength = config::getDefaultTextMin(), maxLength = config::getDefaultTextMax();
	String salt = config::getDefaultPasswordSalt();
};

struct FieldExtra : Field::Slot {
	virtual ~FieldExtra() { }

	template <typename ... Args>
	FieldExtra(String && n, Args && ... args) : Field::Slot(move(n), Type::Extra) {
		init<FieldExtra, Args...>(*this, std::forward<Args>(args)...);
	}

	virtual bool hasDefault() const override;
	virtual data::Value getDefault(const data::Value &) const override;

	virtual bool transformValue(FieldAccessAction, const Scheme &, const data::Value &, data::Value &) const override;
	virtual void hash(StringStream &stream, ValidationLevel l) const override;

	Map<String, Field> fields;
};

struct FieldFile : Field::Slot {
	virtual ~FieldFile() { }

	template <typename ... Args>
	FieldFile(String && n, Args && ... args) : Field::Slot(move(n), Type::File) {
		init<FieldFile, Args...>(*this, std::forward<Args>(args)...);
	}

	virtual void hash(StringStream &stream, ValidationLevel l) const override;

	size_t maxSize = config::getMaxInputFileSize();
	Vector<String> allowedTypes;
};

struct FieldImage : Field::Slot {
	virtual ~FieldImage() { }

	template <typename ... Args>
	FieldImage(String && n, Args && ... args) : Field::Slot(move(n), Type::Image) {
		init<FieldImage, Args...>(*this, std::forward<Args>(args)...);
	}

	virtual void hash(StringStream &stream, ValidationLevel l) const override;

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
	FieldObject(String && n, Type t, Args && ... args) : Field::Slot(move(n), t) {
		init<FieldObject, Args...>(*this, std::forward<Args>(args)...);
		if (t == Type::Set && (toInt(flags) & toInt(Flags::Reference))) {
			if (onRemove != RemovePolicy::Reference && onRemove != RemovePolicy::StrongReference) {
				onRemove = RemovePolicy::Reference;
			}
		}
		if (t == Type::Set && (onRemove == RemovePolicy::Reference || onRemove == RemovePolicy::StrongReference)) {
			flags |= Flags::Reference;
		}
	}

	virtual bool transformValue(FieldAccessAction, const Scheme &, const data::Value &, data::Value &) const override;
	virtual void hash(StringStream &stream, ValidationLevel l) const override;

	const Scheme *scheme = nullptr;
	RemovePolicy onRemove = RemovePolicy::Null;
	Linkage linkage = Linkage::Auto;
	String link;
};

struct FieldArray : Field::Slot {
	virtual ~FieldArray() { }

	template <typename ... Args>
	FieldArray(String && n, Args && ... args) : Field::Slot(move(n), Type::Array), tfield(new FieldText("", Type::Text)) {
		init<FieldArray, Args...>(*this, std::forward<Args>(args)...);
	}

	virtual bool transformValue(FieldAccessAction, const Scheme &, const data::Value &, data::Value &) const override;
	virtual void hash(StringStream &stream, ValidationLevel l) const override;

	Field tfield;
};

struct FieldView : Field::Slot {
	enum DeltaOptions {
		Delta
	};

	virtual ~FieldView() { }

	template <typename ... Args>
	FieldView(String && n, Args && ... args) : Field::Slot(move(n), Type::View) {
		init<FieldView, Args...>(*this, std::forward<Args>(args)...);
	}

	virtual bool transformValue(FieldAccessAction, const Scheme &, const data::Value &, data::Value &) const override { return false; }

	const Scheme *scheme = nullptr;
	Map<String, Field> fields;
	Vector<String> requires;
	ViewLinkageFn linkage;
	ViewFn viewFn;

	bool delta = false;
};

struct FieldFullTextView : Field::Slot {
	virtual ~FieldFullTextView() { }

	template <typename ... Args>
	FieldFullTextView(String && n, Args && ... args) : Field::Slot(move(n), Type::FullTextView) {
		init<FieldFullTextView, Args...>(*this, std::forward<Args>(args)...);
	}

	virtual bool transformValue(FieldAccessAction, const Scheme &, const data::Value &, data::Value &) const override { return false; }

	Vector<String> requires;
	FullTextViewFn viewFn;
	FullTextQueryFn queryFn;
};

template <typename ... Args> Field Field::Data(String && name, Args && ... args) {
	auto newSlot = new Field::Slot(std::move(name), Type::Data);
	Slot::init<Field::Slot>(*newSlot, std::forward<Args>(args)...);
	return Field(newSlot);
}

template <typename ... Args> Field Field::Integer(String && name, Args && ... args) {
	auto newSlot = new Field::Slot(std::move(name), Type::Integer);
	Slot::init<Field::Slot>(*newSlot, std::forward<Args>(args)...);
	return Field(newSlot);
}

template <typename ... Args> Field Field::Float(String && name, Args && ... args) {
	auto newSlot = new Field::Slot(std::move(name), Type::Float);
	Slot::init<Field::Slot>(*newSlot, std::forward<Args>(args)...);
	return Field(newSlot);
}

template <typename ... Args> Field Field::Boolean(String && name, Args && ... args) {
	auto newSlot = new Field::Slot(std::move(name), Type::Boolean);
	Slot::init<Field::Slot>(*newSlot, std::forward<Args>(args)...);
	return Field(newSlot);
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

template <typename ... Args> Field Field::Extra(String &&name, InitializerList<Field> &&f, Args && ... args) {
	return Field(new FieldExtra(std::move(name), move(f), std::forward<Args>(args)...));
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
	return Field(new FieldArray(move(name), forward<Args>(args)...));
}

template <typename ... Args> Field Field::View(String && name, Args && ... args) {
	return Field(new FieldView(move(name), forward<Args>(args)...));
}

template <typename ... Args> Field Field::FullTextView(String && name, Args && ... args) {
	return Field(new FieldFullTextView(move(name), forward<Args>(args)...));
}


template <typename F> struct FieldOption<F, Flags> {
	static inline void assign(F & f, Flags flags) { f.flags |= flags; }
};

template <typename F> struct FieldOption<F, FilterFn> {
	static inline void assign(F & f, const FilterFn &fn) { f.filter = fn; }
};

template <typename F> struct FieldOption<F, DefaultFn> {
	static inline void assign(F & f, const DefaultFn &fn) { f.defaultFn = fn; }
};

template <typename F> struct FieldOption<F, FieldAccessFn> {
	static inline void assign(F & f, const FieldAccessFn &fn) { f.fieldAccessFn = fn; }
};

template <typename F> struct FieldOption<F, Function<data::Value()>> {
	static inline void assign(F & f, const Function<data::Value()> &fn) {
		f.defaultFn = DefaultFn([fn] (const data::Value &) -> data::Value { return fn(); });
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
			f.fields.emplace(it.getName().str(), it);
		}
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

template <> struct FieldOption<FieldView, Vector<String>> {
	static inline void assign(FieldView & f, Vector<String> && s) {
		f.requires = move(s);
	}
};

template <> struct FieldOption<FieldFullTextView, Vector<String>> {
	static inline void assign(FieldFullTextView & f, Vector<String> && s) {
		f.requires = move(s);
	}
};

template <typename F> struct FieldOption<F, ViewLinkageFn> {
	static inline void assign(F & f, ViewLinkageFn && s) {
		f.linkage = move(s);
	}
};

template <typename F> struct FieldOption<F, ViewFn> {
	static inline void assign(F & f, ViewFn && s) {
		f.viewFn = move(s);
	}
};

template <typename F> struct FieldOption<F, FullTextViewFn> {
	static inline void assign(F & f, FullTextViewFn && s) {
		f.viewFn = move(s);
	}
};

template <typename F> struct FieldOption<F, FieldView::DeltaOptions> {
	static inline void assign(F & f, FieldView::DeltaOptions d) {
		if (d == FieldView::Delta) { f.delta = true; } else { f.delta = false; }
	}
};

NS_SA_EXT_END(storage)

#endif /* SERENITY_SRC_DATABASE_DATABASEFIELD_H_ */
