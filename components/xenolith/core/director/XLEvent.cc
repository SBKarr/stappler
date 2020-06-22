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

#include "XLEvent.h"
#include "XLApplication.h"

namespace stappler::xenolith {

void Event::dispatch() const {
	Application::getInstance()->dispatchEvent(*this);
}

Ref *Event::getObject() const {
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
	return is(eventHeader);
}

Event::Event(const EventHeader &header, Ref *object)
: _header(header), _object(object) { _value.intValue = 0; }

Event::Event(const EventHeader &header, Ref *object, Value val, Type type)
: _header(header), _type(type), _object(object), _value(val) { }

void Event::send(const EventHeader &header, Ref *object, int64_t value) {
	Value val; val.intValue = value;
	send(header, object, val, Type::Int);
}
void Event::send(const EventHeader &header, Ref *object, double value) {
	Value val; val.floatValue = value;
	send(header, object, val, Type::Float);
}
void Event::send(const EventHeader &header, Ref *object, bool value) {
	Value val; val.boolValue = value;
	send(header, object, val, Type::Bool);
}
void Event::send(const EventHeader &header, Ref *object, Ref *value) {
	Value val; val.objValue = value;
	send(header, object, val, Type::Object);
}
void Event::send(const EventHeader &header, Ref *object, const char *value) {
	String str = value;
	send(header, object, str);
}
void Event::send(const EventHeader &header, Ref *object, const String &value) {
	Application::getInstance()->performOnMainThread([header, object, value] () {
		Value val; val.strValue = &value;
		Event event(header, object, val, Type::String);
		event.dispatch();
	});
}
void Event::send(const EventHeader &header, Ref *object, const StringView &value) {
	send(header, object, value.str());
}
void Event::send(const EventHeader &header, Ref *object, const data::Value &value) {
	Application::getInstance()->performOnMainThread([header, object, value] () {
		Value val; val.dataValue = &value;
		Event event(header, object, val, Type::Data);
		event.dispatch();
	});
}
void Event::send(const EventHeader &header, Ref *object, Value val, Type type) {
	Application::getInstance()->performOnMainThread([header, object, val, type] () {
		Event event(header, object, val, type);
		event.dispatch();
	});
}
void Event::send(const EventHeader &header, Ref *object) {
	Application::getInstance()->performOnMainThread([header, object] () {
		Event event(header, object);
		event.dispatch();
	});
}

}
