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

#ifndef __stappler__SPEventHeader__
#define __stappler__SPEventHeader__

#include "SPDefine.h"

#define SP_DECLARE_EVENT_CLASS(class, event) \
	stappler::EventHeader class::event(#class, #class "." #event);

#define SP_DECLARE_EVENT(class, catName, event) \
	stappler::EventHeader class::event(catName, catName "." #event);

NS_SP_BEGIN

/* Event headers contains category and id of event.
 Headers should be used to statically declare event to be recognized by dispatcher and listeners
 Header declares unique event name, to be used, when we need to recognize this event

 EventHeaders should be declared statically with simple constructor:
	EventHeader eventHeader(Category);
 */

class EventHeader {
public:
	using Category = int;
	using EventID = int;

	static Category getCategoryForName(const String &catName);
public:

	EventHeader() = delete;
	EventHeader(const String &catName, const String &eventName);
	EventHeader(Category cat, const String &eventName);
	~EventHeader();

	EventHeader(const EventHeader &other);
	EventHeader(EventHeader &&other);

	EventHeader &operator=(const EventHeader &other);
	EventHeader &operator=(EventHeader &&other);

	Category getCategory() const;
	EventID getEventID() const;
	const String &getName() const;

	bool isInCategory(Category cat) const;

	operator int ();

	bool operator == (const Event &event) const;

#if SP_EVENT_RTTI
	template<class T> void operator() (T *object, int64_t value) const {
        static_assert(std::is_convertible<T *, Ref *>::value,
					  "Invalid Type for stappler::Event target!");
		send(object, typeid(*object).name(), value);
	}

	template<class T> void operator() (T *object, double value) const {
        static_assert(std::is_convertible<T *, Ref *>::value,
					  "Invalid Type for stappler::Event target!");
		send(object, typeid(*object).name(), value);
	}

	template<class T> void operator() (T *object, bool value) const {
        static_assert(std::is_convertible<T *, Ref *>::value,
					  "Invalid Type for stappler::Event target!");
		send(object, typeid(*object).name(), value);
	}

	template<class T, class V> void operator() (T *object, V *value) const {
		static_assert(std::is_convertible<T *, Ref *>::value,
					  "Invalid Type for stappler::Event target!");

		static_assert(std::is_convertible<V *, Ref *>::value,
					  "Invalid Type for stappler::Event target!");

		send(object, typeid(*object).name(), value, typeid(*value).name());
	}

	template<class T> void operator() (T *object, const char *value) const  {
        static_assert(std::is_convertible<T *, Ref *>::value,
					  "Invalid Type for stappler::Event target!");
		send(object, typeid(*object).name(), value);
	}

	template<class T> void operator() (T *object, const String &value) const  {
        static_assert(std::is_convertible<T *, Ref *>::value,
					  "Invalid Type for stappler::Event target!");
		send(object, typeid(*object).name(), value);
	}

	template<class T> void operator() (T *object = nullptr) const {
        static_assert(std::is_convertible<T *, Ref *>::value,
					  "Invalid Type for stappler::Event target!");
		send(object, typeid(*object).name());
	}
#else
	inline void operator() (Ref *object, int64_t value) const { send(object, value); }
	inline void operator() (Ref *object, double value) const { send(object, value); }
	inline void operator() (Ref *object, bool value) const { send(object, value); }
	inline void operator() (Ref *object, Ref *value) const { send(object, value); }
	inline void operator() (Ref *object, const char *value) const { send(object, value); }
	inline void operator() (Ref *object, const String &value) const { send(object, value); }
	inline void operator() (Ref *object, const data::Value &value) const { send(object, value); }
	inline void operator() (Ref *object = nullptr) const { send(object); }
#endif
protected:

#if SP_EVENT_RTTI
	void send(Ref *object, const String &tname, int64_t value) const;
	void send(Ref *object, const String &tname, double value) const;
	void send(Ref *object, const String &tname, bool value) const;
	void send(Ref *object, const String &tname, Ref *value, const String &oname) const;
	void send(Ref *object, const String &tname, const char *value) const;
	void send(Ref *object, const String &tname, const String &value) const;
	void send(Ref *object, const String &tname) const;
#else
	void send(Ref *object, int64_t value) const;
	void send(Ref *object, double value) const;
	void send(Ref *object, bool value) const;
	void send(Ref *object, Ref *value) const;
	void send(Ref *object, const char *value) const;
	void send(Ref *object, const String &value) const;
	void send(Ref *object, const data::Value &value) const;
	void send(Ref *object = nullptr) const;
#endif

	Category _category = 0;
	EventID _id = 0;
	String _name;
};

NS_SP_END

#endif /* defined(__stappler__SPEventHeader__) */
