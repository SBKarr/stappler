/**
Copyright (c) 2020 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef COMPONENTS_XENOLITH_CORE_DIRECTOR_XLEVENT_H_
#define COMPONENTS_XENOLITH_CORE_DIRECTOR_XLEVENT_H_

#include "XLEventHeader.h"

namespace stappler::xenolith {

/* common usage:
 Event::send(eventHeader, sourceObject)
 */

class Event {
protected:
	enum class Type {
		Int,
		Float,
		Bool,
		Object,
		String,
		Data,
		None
	};

public:
	Ref *getObject() const;

	const EventHeader &getHeader() const;
	EventHeader::Category getCategory() const;
	EventHeader::EventID getEventID() const;

	bool is(const EventHeader &eventHeader) const;
	bool operator == (const EventHeader &eventHeader) const;

	template <class T = Ref> inline T * getTarget() const {
        static_assert(std::is_convertible<T *, Ref *>::value,
					  "Invalid Type for stappler::Event target!");
		return static_cast<T *>(_object);
	}

	inline bool valueIsVoid() const { return _type == Type::None; }
	inline bool valueIsBool() const { return _type == Type::Bool; }
	inline bool valueIsInt() const { return _type == Type::Int; }
	inline bool valueIsFloat() const { return _type == Type::Float; }
	inline bool valueIsObject() const { return _type == Type::Object; }
	inline bool valueIsString() const { return _type == Type::String; }
	inline bool valueIsData() const { return _type == Type::Data; }

	inline int64_t getIntValue() const { return (_type == Type::Int)?_value.intValue:0; }
	inline double getFloatValue() const { return (_type == Type::Float)?_value.floatValue:0; }
	inline bool getBoolValue() const { return (_type == Type::Bool)?_value.boolValue:false; }

	template <class T> inline T getObjValueImpl(const std::true_type &type) const {
		return (_type == Type::Object)?static_cast<T>(_value.objValue):nullptr;
	}
	template <class T> inline T getObjValueImpl(const std::false_type &type) const {
        return static_cast<T>(0);
	}
	template <class T = Ref *> inline T getObjectValue() const {
        static_assert(std::is_convertible<T, Ref *>::value,
					  "Invalid Type for stappler::Event target!");
		return getObjValueImpl<T>(std::is_convertible<T, Ref *>());
	}
	inline StringView getStringValue() const {
		return (_type == Type::String)?(*_value.strValue):ZERO_STRING;
	}
	inline const data::Value &getDataValue() const {
		return (_type == Type::Data)?(*_value.dataValue):data::Value::Null;
	}

protected:
	union Value {
		int64_t intValue;
		double floatValue;
		bool boolValue;
		Ref *objValue;
		const String *strValue;
		const data::Value * dataValue;
	};

	Event(const EventHeader &header, Ref *object, Value val, Type type);
	Event(const EventHeader &header, Ref *object);

	static void send(const EventHeader &header, Ref *object, Value val, Type type);
	static void send(const EventHeader &header, Ref *object, int64_t value);
	static void send(const EventHeader &header, Ref *object, double value);
	static void send(const EventHeader &header, Ref *object, bool value);
	static void send(const EventHeader &header, Ref *object, Ref *value);
	static void send(const EventHeader &header, Ref *object, const char *value);
	static void send(const EventHeader &header, Ref *object, const String &value);
	static void send(const EventHeader &header, Ref *object, const StringView &value);
	static void send(const EventHeader &header, Ref *object, const data::Value &value);
	static void send(const EventHeader &header, Ref *object);

protected:
	void dispatch() const;

	const EventHeader &_header;
	Type _type = Type::None;
	Ref *_object = nullptr;
	Value _value;

	friend class EventHeader;

private:
	static String ZERO_STRING;
};

}

#endif /* COMPONENTS_XENOLITH_CORE_EVENT_XLEVENT_H_ */
