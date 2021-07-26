/**
 Copyright (c) 2021 Roman Katuntsev <sbkarr@stappler.org>

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

#include "SPCommon.h"
#include "SPTime.h"
#include "SPString.h"
#include "SPData.h"
#include "SPRef.h"
#include "Test.h"

NS_SP_BEGIN

class NamedRef : public Ref {
public:
	virtual ~NamedRef() { }
	virtual StringView getName() const = 0;
};

using HashFunc = uint32_t (*)(const char *key, ssize_t *klen);

template <typename Value>
class HashTable;

template <typename Value>
struct HashTraits;

template <>
struct HashTraits<NamedRef *> {
	static uint32_t hash(uint32_t salt, const NamedRef *value) {
		auto name = value->getName();
		return hash::hash32(name.data(), name.size(), salt);
	}

	static bool equal(const NamedRef *l, const NamedRef *r) {
		return l == r;
	}
};

template <>
struct HashTraits<Rc<NamedRef>> {
	static uint32_t hash(uint32_t salt, const NamedRef *value) {
		auto name = value->getName();
		return hash::hash32(name.data(), name.size(), salt);
	}

	static uint32_t hash(uint32_t salt, StringView value) {
		return hash::hash32(value.data(), value.size(), salt);
	}

	static bool equal(const NamedRef *l, const NamedRef *r) {
		return l == r;
	}

	static bool equal(const NamedRef *l, StringView value) {
		return l->getName() == value;
	}
};

template <typename Value>
struct HashTraitDiscovery;

template <typename Value>
struct HashTraitDiscovery<Rc<Value>> {
	using type = typename std::conditional<
			std::is_base_of<NamedRef, Value>::value,
			HashTraits<Rc<NamedRef>>,
			HashTraitDiscovery<Value *>>::type;
};

template <typename Value>
struct HashEntry {
	using Type = typename std::remove_cv<typename std::remove_reference<Value>::type>::type;
	using Traits = typename HashTraitDiscovery<Value>::type;

	template <typename ... Args>
	static uint32_t getHash(uint32_t salt, Args && ... args) {
		return Traits::hash(salt, std::forward<Args>(args)...);
	}

	template <typename ... Args>
	static bool isEqual(const Value &l, Args && ... args) {
		return Traits::equal(l, std::forward<Args>(args)...);
	}

	HashEntry *next;
	uint32_t hash;
	std::array<uint8_t, sizeof(Value)> data;

	Value *get() { return (Value *)data.data(); }
	const Value *get() const { return (const Value *)data.data(); }
};

template <typename Value>
struct HashIndex {
	using Type = typename std::remove_cv<typename std::remove_reference<Value>::type>::type;

	HashTable<Value> *ht;
	HashEntry<Type> *_self;
	HashEntry<Type> *_next;
	uint32_t index;

	HashIndex * next() {
		_self = _next;
		while (!_self) {
			if (index > ht->max) {
				_self = _next = nullptr;
				return nullptr;
			}
			_self = ht->array[index++];
		}
		_next = _self->next;
		return this;
	}

	bool operator==(const HashIndex &l) const { return l.ht == ht && l._self == _self && l._next == _next && l.index == index; }
	bool operator!=(const HashIndex &l) const { return l.ht != ht || l._self != _self || l._next != _next || l.index != index; }

	HashIndex& operator++() { this->next(); return *this; }
	HashIndex operator++(int) { auto tmp = *this; this->next(); return tmp; }

	Type & operator*() const { return *_self->get(); }
	Type * operator->() const { return _self->get(); }
};

template <typename Value>
struct ConstHashIndex {
	using Type = typename std::remove_cv<typename std::remove_reference<Value>::type>::type;

	const HashTable<Value> *ht;
	const HashEntry<Type> *_self;
	const HashEntry<Type> *_next;
	uint32_t index;

	ConstHashIndex * next() {
		_self = _next;
		while (!_self) {
			if (index > ht->max) {
				_self = _next = nullptr;
				return nullptr;
			}
			_self = ht->array[index++];
		}
		_next = _self->next;
		return this;
	}

	bool operator==(const ConstHashIndex &l) const { return l.ht == ht && l._self == _self && l._next == _next && l.index == index; }
	bool operator!=(const ConstHashIndex &l) const { return l.ht != ht || l._self != _self || l._next != _next || l.index != index; }

	ConstHashIndex& operator++() { this->next(); return *this; }
	ConstHashIndex operator++(int) { auto tmp = *this; this->next(); return tmp; }

	const Type & operator*() const { return *_self->get(); }
	const Type * operator->() const { return _self->get(); }
};

template <typename Value>
class HashTable {
public:
	using Pool = memory::pool_t;
	using ValueType = HashEntry<Value>;

	using merge_fn = void *(*)(Pool *p, const void *key, ssize_t klen, const void *h1_val, const void *h2_val, const void *data);
	using foreach_fn = bool (*)(void *rec, const void *key, ssize_t klen, const void *value);

	static constexpr auto INITIAL_MAX = 15; /* tunable == 2^n - 1 */

	using iterator = HashIndex<Value>;
	using const_iterator = ConstHashIndex<Value>;

	HashTable(Pool *p = nullptr) {
		if (!p) {
			p = memory::pool::acquire();
		}

		auto now = Time::now().toMicros();
		this->pool = p;
		this->free = nullptr;
		this->count = 0;
		this->max = INITIAL_MAX;
		this->seed = (unsigned int)((now >> 32) ^ now ^ (uintptr_t)pool ^ (uintptr_t)this ^ (uintptr_t)&now) - 1;
		this->array = alloc_array(this, max);
	}

	HashTable(const HashTable &copy, Pool *p = nullptr) {
		if (!p) {
			p = memory::pool::acquire();
		}

		this->pool = p;
		this->free = nullptr;
		this->count = copy.count;
		this->max = copy.max;
		this->seed = copy.seed;
		this->array = alloc_array(this, copy.max);

		auto new_vals = (HashEntry<Value> *)memory::pool::palloc(this->pool, sizeof(ValueType) * this->count);

		size_t j = 0;
		for (size_t i = 0; i <= copy.max; i++) {
			auto target = &this->array[i];
			auto orig_entry = copy.array[i];
			while (orig_entry) {
				auto new_entry = &new_vals[j++];
				new_entry->next = nullptr;
				new_entry->hash = orig_entry->hash;
				new (new_entry->data.data()) Value(*orig_entry->get());
				*target = new_entry;
				target = &new_entry->next;
				orig_entry = orig_entry->next;
			}
		}
	}

	template <typename ... Args>
	Pair<iterator, bool> assign(Args && ... args) {
		iterator iter;
		auto ret = set_value(true, std::forward<Args>(args)...);

		iter._self = ret.first;
		iter._next = ret.first->next;
		iter.index = (ret.first->hash & this->max);
		iter.ht = this;
		return pair(iter, ret.second);
	}

	template <typename ... Args>
	Pair<iterator, bool> emplace(Args && ... args) {
		iterator iter;
		auto ret = set_value(false, std::forward<Args>(args)...);
		iter._self = ret.first;
		iter._next = ret.first->next;
		iter.index = (ret.first->hash & this->max);
		iter.ht = this;
		return pair(iter, ret.second);
	}

	template <typename ... Args>
	bool contains(Args && ... args) const {
		if (auto ret = get_value(std::forward<Args>(args)...)) {
			return true;
		}
		return false;
	}

	template <typename ... Args>
	iterator find(Args && ... args) {
		if (auto ret = get_value(std::forward<Args>(args)...)) {
			iterator iter;
			iter._self = ret;
			iter._next = ret->next;
			iter.index = (ret->hash & this->max);
			iter.ht = this;
			return iter;
		}
		return end();
	}

	template <typename ... Args>
	const_iterator find(Args && ... args) const {
		if (auto ret = get_value(std::forward<Args>(args)...)) {
			const_iterator iter;
			iter._self = ret;
			iter._next = ret->next;
			iter.index = (ret->hash & this->max);
			iter.ht = this;
			return iter;
		}
		return end();
	}

	template <typename ... Args>
	iterator erase(Args && ... args) {
		ValueType **hep, *he;
		const auto hash = ValueType::getHash(seed, std::forward<Args>(args)...);
		const auto idx = hash & this->max;

		/* scan linked list */
		for (hep = &this->array[idx], he = *hep; he; hep = &he->next, he = *hep) {
			if (he->hash == hash && ValueType::isEqual(*he->get(), std::forward<Args>(args)...)) {
				break;
			}
		}

		if (he) {
			iterator iter;
			iter._self = he;
			iter._next = he->next;
			iter.index = (he->hash & this->max);
			iter.ht = this;
			iter.next();

			ValueType *old = *hep;
			*hep = (*hep)->next;
			old->next = this->free;
			this->free = old;
			--this->count;

			return iter;
		} else {
			return end();
		}
	}

	size_t size() const { return count; }
	bool empty() const { return count == 0; }

	void reserve(size_t c) {
		if (c <= this->count) {
			return;
		}

		if (c > this->max) {
			expand_array(this, c);
		}

		auto mem = (ValueType *)memory::pool::palloc(this->pool, sizeof(ValueType) * (c - this->count));

		for (size_t i = this->count; i < c; ++ i) {
			mem->next = free;
			free = mem;
			++ mem;
		}
	}

	void clear() {
		for (size_t i = 0; i <= max; ++ i) {
			auto v = array[i];
			while (v) {
				auto tmp = v;
				v = v->next;

				tmp->next = free;
				free = tmp;
				tmp->get()->~ValueType();
			}
			array[i] = nullptr;
		}
		count = 0;
	}

	iterator begin() { return begin_index(); }
	iterator end() { return end_index(); }

	const_iterator begin() const {
		ConstHashIndex<Value> hi;
		hi.ht = this;
		hi.index = 0;
		hi._self = nullptr;
		hi._next = nullptr;
		hi.next();
		return hi;
	}

	const_iterator end() const {
		ConstHashIndex<Value> hi;
		hi.ht = this;
		hi.index = this->max + 1;
		hi._self = nullptr;
		hi._next = nullptr;
		return hi;
	}

	size_t get_cell_count() const {
		size_t count = 0;
		for (size_t i = 0; i <= max; ++ i) {
			if (array[i]) {
				++ count;
			}
		}
		return count;
	}

	size_t get_free_count() const {
		size_t count = 0;
		auto f = free;
		while (f) {
			f = f->next;
			++ count;
		}
		return count;
	}

private:
	friend class HashIndex<Value>;

	static HashEntry<Value> **alloc_array(HashTable *ht, uint32_t max) {
		return (ValueType **)memory::pool::calloc(ht->pool, max + 1, sizeof(ValueType));
	}

	static void expand_array(HashTable *ht, uint32_t new_max = 0) {
		HashEntry<Value> **new_array;

		if (!new_max) {
			new_max = ht->max * 2 + 1;
		} else {
			new_max = math::npot(new_max) - 1;
			if (new_max <= ht->max) {
				return;
			}
		}

		new_array = alloc_array(ht, new_max);

		auto end = ht->end_index();
		auto hi = ht->begin_index();
		while (hi != end) {
			uint32_t i = hi._self->hash & new_max;
			hi._self->next = new_array[i];
			new_array[i] = hi._self;
			++ hi;
		}
		ht->array = new_array;
		ht->max = new_max;
	}

	template <typename ... Args>
	ValueType * get_value(Args &&  ... args) const {
		ValueType **hep, *he;
		const auto hash = ValueType::getHash(seed, std::forward<Args>(args)...);
		const auto idx = hash & this->max;

		/* scan linked list */
		for (hep = &this->array[idx], he = *hep; he; hep = &he->next, he = *hep) {
			if (he->hash == hash && ValueType::isEqual(*he->get(), std::forward<Args>(args)...)) {
				break;
			}
		}

		return he;
	}

	template <typename ... Args>
	Pair<ValueType *, bool> set_value(bool replace, Args &&  ... args) {
		ValueType **hep, *he;
		const auto hash = ValueType::getHash(seed, std::forward<Args>(args)...);
		const auto idx = hash & this->max;

		/* scan linked list */
		for (hep = &this->array[idx], he = *hep; he; hep = &he->next, he = *hep) {
			if (he->hash == hash && ValueType::isEqual(*he->get(), std::forward<Args>(args)...)) {
				break;
			}
		}

		if (he) {
			if (replace) {
				he->get()->~Value();
				new (he->data.data()) Value(std::forward<Args>(args)...);
			}
			return pair(he, false);
		} else {
			/* add a new entry for non-NULL values */
			if ((he = this->free) != NULL) {
				this->free = he->next;
			} else {
				he = (ValueType *)memory::pool::palloc(this->pool, sizeof(*he));
			}

			this->count++;
			he->next = NULL;
			he->hash = hash;
			new (he->data.data()) Value(std::forward<Args>(args)...);

			*hep = he;

			/* check that the collision rate isn't too high */
			if (this->count > this->max) {
				expand_array(this);
			}
			return pair(he, true);
		}
	}

	HashIndex<Value> begin_index() {
		HashIndex<Value> hi;
		hi.ht = this;
		hi.index = 0;
		hi._self = nullptr;
		hi._next = nullptr;
		hi.next();
		return hi;
	}

	HashIndex<Value> end_index() {
		HashIndex<Value> hi;
		hi.ht = this;
		hi.index = this->max + 1;
		hi._self = nullptr;
		hi._next = nullptr;
		return hi;
	}

	Pool *pool = nullptr;
	HashEntry<Value> **array;
	uint32_t count, max, seed;
	HashEntry<Value> *free = nullptr; /* List of recycled entries */
};


class TestNamedRef : public NamedRef {
public:
	virtual ~TestNamedRef() { }
	virtual StringView getName() const { return _name; }

	TestNamedRef(StringView name) : _name(name) { }

protected:
	StringView _name;
};


struct HashMapTest : MemPoolTest {
	HashMapTest() : MemPoolTest("HashMapTest") { }

	virtual bool run(pool_t *pool) {
		StringStream stream;
		size_t count = 0;
		size_t passed = 0;
		stream << "\n";

		auto emplace = [&] (HashTable<Rc<TestNamedRef>> &t, std::set<StringView> &control, StringView str) {
			control.emplace(str);
			t.emplace(Rc<TestNamedRef>::alloc(str));
		};

		auto fill = [&] (HashTable<Rc<TestNamedRef>> &t, std::set<StringView> &control) {
			emplace(t, control, "One");
			emplace(t, control, "Two");
			emplace(t, control, "3");
			emplace(t, control, "Four");
			emplace(t, control, "Five");
			emplace(t, control, "Six");
			emplace(t, control, "Seven");
			emplace(t, control, "8");
			emplace(t, control, "Nine");
			emplace(t, control, "10");
			emplace(t, control, "11");
			emplace(t, control, "12");
			emplace(t, control, "13");
			emplace(t, control, "14");
			emplace(t, control, "15");
			emplace(t, control, "16");
			emplace(t, control, "17");
			emplace(t, control, "18");
			emplace(t, control, "19");
			emplace(t, control, "20");
		};

		runTest(stream, "emplace", count, passed, [&] {
			HashTable<Rc<TestNamedRef>> t;
			std::set<StringView> control;
			t.reserve(30);
			fill(t, control);

			for (auto &it : control) {
				auto iit = t.find(it);
				if (iit == t.end() || (*iit)->getName() != it) {
					std::cout << "Not found: " << it << "\n";
					return false;
				}
			}

			stream << t.get_cell_count() << " / " << t.size();

			auto it = t.begin();
			while (it != t.end()) {
				control.erase((*it)->getName());
				++ it;
			}

			return control.empty();
		});

		runTest(stream, "erase", count, passed, [&] {
			HashTable<Rc<TestNamedRef>> t;
			std::set<StringView> control;
			fill(t, control);

			auto it = control.begin();
			while (it != control.end()) {
				auto iit = t.find(*it);
				if (iit == t.end() || (*iit)->getName() != *it) {
					std::cout << "Not found: " << *it << "\n";
					return false;
				}

				iit = t.erase(*it);
				it = control.erase(it);
			}

			stream << t.get_free_count();
			return control.empty() && t.empty();
		});

		runTest(stream, "copy", count, passed, [&] {
			HashTable<Rc<TestNamedRef>> tmp;
			std::set<StringView> control;
			fill(tmp, control);

			HashTable<Rc<TestNamedRef>> copy(tmp);

			for (auto &it : control) {
				auto iit = copy.find(it);
				if (iit == copy.end() || (*iit)->getName() != it) {
					std::cout << "Not found: " << it << "\n";
					return false;
				}
			}

			stream << copy.get_cell_count() << " / " << copy.size();

			auto it = copy.begin();
			while (it != copy.end()) {
				control.erase((*it)->getName());
				++ it;
			}

			return control.empty();
		});

		_desc = stream.str();

		return count == passed;
	}
} _HashMapTest;

NS_SP_END
