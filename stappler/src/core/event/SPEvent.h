//
//  SPEvent.h
//  stappler
//
//  Created by SBKarr on 10.01.14.
//  Copyright (c) 2014 SBKarr. All rights reserved.
//

#ifndef __stappler__SPEvent__
#define __stappler__SPEvent__

#include "SPDefine.h"
#include "SPEventHeader.h"
#include "SPData.h"

NS_SP_BEGIN

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
	cocos2d::Ref *getObject() const;

	const EventHeader &getHeader() const;
	EventHeader::Category getCategory() const;
	EventHeader::EventID getEventID() const;

#if SP_EVENT_RTTI
	const std::string &getTargetTypename() const { return _targetTypename; }
	const std::string &getObjectTypename() const { return _objectTypename; }
#endif

	bool is(const EventHeader &eventHeader) const;
	bool operator == (const EventHeader &eventHeader) const;

	template <class T = cocos2d::Ref> inline T * getTarget() const {
        static_assert(std::is_convertible<T *, cocos2d::Ref *>::value,
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
	template <class T = cocos2d::Ref *> inline T getObjectValue() const {
        static_assert(std::is_convertible<T, cocos2d::Ref *>::value,
					  "Invalid Type for stappler::Event target!");
		return getObjValueImpl<T>(std::is_convertible<T, cocos2d::Ref *>());
	}
	inline const std::string &getStringValue() const {
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
		cocos2d::Ref *objValue;
		const std::string *strValue;
		const data::Value * dataValue;
	};

#if SP_EVENT_RTTI
	static void send(const EventHeader &header, cocos2d::Ref *object, const std::string &tname, Value val, Type type, const std::string oname);
	static void send(const EventHeader &header, cocos2d::Ref *object, const std::string &tname, int64_t value);
	static void send(const EventHeader &header, cocos2d::Ref *object, const std::string &tname, double value);
	static void send(const EventHeader &header, cocos2d::Ref *object, const std::string &tname, bool value);
	static void send(const EventHeader &header, cocos2d::Ref *object, const std::string &tname, cocos2d::Ref *value, const std::string oname);
	static void send(const EventHeader &header, cocos2d::Ref *object, const std::string &tname, const char *value);
	static void send(const EventHeader &header, cocos2d::Ref *object, const std::string &tname, const std::string &value);
	static void send(const EventHeader &header, cocos2d::Ref *object, const std::string &tname);

	Event(const EventHeader &header, cocos2d::Ref *object, const std::string &tname, Value val, Type type, const std::string &oname);
	Event(const EventHeader &header, cocos2d::Ref *object, const std::string &tname);

	std::string _targetTypename;
	std::string _objectTypename;
#else

	Event(const EventHeader &header, cocos2d::Ref *object, Value val, Type type);
	Event(const EventHeader &header, cocos2d::Ref *object);

	static void send(const EventHeader &header, cocos2d::Ref *object, Value val, Type type);
	static void send(const EventHeader &header, cocos2d::Ref *object, int64_t value);
	static void send(const EventHeader &header, cocos2d::Ref *object, double value);
	static void send(const EventHeader &header, cocos2d::Ref *object, bool value);
	static void send(const EventHeader &header, cocos2d::Ref *object, cocos2d::Ref *value);
	static void send(const EventHeader &header, cocos2d::Ref *object, const char *value);
	static void send(const EventHeader &header, cocos2d::Ref *object, const std::string &value);
	static void send(const EventHeader &header, cocos2d::Ref *object, const data::Value &value);
	static void send(const EventHeader &header, cocos2d::Ref *object);
#endif

protected:
	void dispatch() const;

	const EventHeader &_header;
	Type _type = Type::None;
	cocos2d::Ref *_object = nullptr;
	Value _value;

	friend class EventHeader;

private:
	static std::string ZERO_STRING;
};

NS_SP_END

#endif /* defined(__stappler__SPEvent__) */
