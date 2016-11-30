// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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

#include "SPDefine.h"
#include "SPEvent.h"
#include "SPThread.h"
#include "SPEventDispatcher.h"

USING_NS_SP;

std::string Event::ZERO_STRING = "";

void Event::dispatch() const {
	EventDispatcher::getInstance()->dispatchEvent(this);
}

cocos2d::Ref *Event::getObject() const {
	return _object;
}

const EventHeader &Event::getHeader() const {
	return _header;
}

EventHeader::Category Event::getCategory() const {
	return _header.getCategory();
}
EventHeader::EventID Event::getEventID() const {
	return _header.getEventID();
}

bool Event::is(const EventHeader &eventHeader) const {
	return _header.getEventID() == eventHeader.getEventID();
}

bool Event::operator == (const EventHeader &eventHeader) const {
	return _header.getEventID() == eventHeader.getEventID();
}

#if SP_EVENT_RTTI
Event::Event(const EventHeader &header, cocos2d::Ref *object, const std::string &tname)
: _header(header), _object(object), _targetTypename(tname) { _value.intValue = 0; }

Event::Event(const EventHeader &header, cocos2d::Ref *object, const std::string &tname, Value val, Type type, const std::string &oname)
: _header(header), _object(object), _value(val), _type(type), _targetTypename(tname), _objectTypename(oname) { }

void Event::send(const EventHeader &header, cocos2d::Ref *object, const std::string &tname, int64_t value) {
	Value val; val.intValue = value;
	send(header, object, tname, val, Type::Int, "");
}
void Event::send(const EventHeader &header, cocos2d::Ref *object, const std::string &tname, double value) {
	Value val; val.floatValue = value;
	send(header, object, tname, val, Type::Float, "");
}
void Event::send(const EventHeader &header, cocos2d::Ref *object, const std::string &tname, bool value) {
	Value val; val.boolValue = value;
	send(header, object, tname, val, Type::Bool, "");
}
void Event::send(const EventHeader &header, cocos2d::Ref *object, const std::string &tname, cocos2d::Ref *value, const std::string oname) {
	Value val; val.objValue = value;
	send(header, object, tname, val, Type::Object, oname);
}
void Event::send(const EventHeader &header, cocos2d::Ref *object, const std::string &tname, const char *value) {
	std::string str = value;
	send(header, object, str);
}
void Event::send(const EventHeader &header, cocos2d::Ref *object, const std::string &tname, const std::string &value) {
	Thread::onMainThread([header, object, value, tname] () {
		Value val; val.strValue = &value;
		Event event(header, object, tname, val, Type::String, "");
		event.dispatch();
	});
}
void Event::send(const EventHeader &header, cocos2d::Ref *object, const std::string &tname, Value val, Type type, const std::string oname) {
	Thread::onMainThread([header, object, val, type, tname, oname] () {
		Event event(header, object, tname, val, type, oname);
		event.dispatch();
	});
}
void Event::send(const EventHeader &header, cocos2d::Ref *object, const std::string &tname) {
	Thread::onMainThread([header, object, tname] () {
		Event event(header, object, tname);
		event.dispatch();
	});
}
#else
Event::Event(const EventHeader &header, cocos2d::Ref *object)
: _header(header), _object(object) { _value.intValue = 0; }

Event::Event(const EventHeader &header, cocos2d::Ref *object, Value val, Type type)
: _header(header), _type(type), _object(object), _value(val) { }

void Event::send(const EventHeader &header, cocos2d::Ref *object, int64_t value) {
	Value val; val.intValue = value;
	send(header, object, val, Type::Int);
}
void Event::send(const EventHeader &header, cocos2d::Ref *object, double value) {
	Value val; val.floatValue = value;
	send(header, object, val, Type::Float);
}
void Event::send(const EventHeader &header, cocos2d::Ref *object, bool value) {
	Value val; val.boolValue = value;
	send(header, object, val, Type::Bool);
}
void Event::send(const EventHeader &header, cocos2d::Ref *object, cocos2d::Ref *value) {
	Value val; val.objValue = value;
	send(header, object, val, Type::Object);
}
void Event::send(const EventHeader &header, cocos2d::Ref *object, const char *value) {
	std::string str = value;
	send(header, object, str);
}
void Event::send(const EventHeader &header, cocos2d::Ref *object, const std::string &value) {
	Thread::onMainThread([header, object, value] () {
		Value val; val.strValue = &value;
		Event event(header, object, val, Type::String);
		event.dispatch();
	});
}
void Event::send(const EventHeader &header, cocos2d::Ref *object, const data::Value &value) {
	Thread::onMainThread([header, object, value] () {
		Value val; val.dataValue = &value;
		Event event(header, object, val, Type::Data);
		event.dispatch();
	});
}
void Event::send(const EventHeader &header, cocos2d::Ref *object, Value val, Type type) {
	Thread::onMainThread([header, object, val, type] () {
		Event event(header, object, val, type);
		event.dispatch();
	});
}
void Event::send(const EventHeader &header, cocos2d::Ref *object) {
	Thread::onMainThread([header, object] () {
		Event event(header, object);
		event.dispatch();
	});
}

#endif
