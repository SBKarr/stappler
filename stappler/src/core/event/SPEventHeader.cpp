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
#include "SPEventHeader.h"
#include "SPEvent.h"
#include "SPEventDispatcher.h"
#include "SPString.h"

USING_NS_SP;

EventHeader::Category EventHeader::getCategoryForName(const std::string &catName) {
	return string::hash32(catName);
}

EventHeader::EventHeader(Category cat, const std::string &name)
: _category(cat), _name(name) {
	assert(!name.empty());
	_id = EventDispatcher::getInstance()->addEventHeader(this);
}

EventHeader::EventHeader(const std::string &catName, const std::string &eventName)
: EventHeader(string::hash32(catName), eventName) { }

EventHeader::EventHeader(const EventHeader &other) : _category(other._category), _id(other._id), _name(other._name) { }
EventHeader::EventHeader(EventHeader &&other) : _category(other._category), _id(other._id), _name(other._name) { }

EventHeader::~EventHeader() {
	EventDispatcher::getInstance()->removeEventHeader(this);
}

EventHeader::Category EventHeader::getCategory() const {
	return _category;
}

EventHeader::EventID EventHeader::getEventID() const {
	return _id;
}

const std::string &EventHeader::getName() const {
	return _name;
}

bool EventHeader::isInCategory(EventHeader::Category cat) const {
	return cat == _category;
}

EventHeader::operator int() {
	return _id;
}

bool EventHeader::operator == (const Event &event) const {
	return event.getEventID() == _id;
}

#if SP_EVENT_RTTI
void EventHeader::send(cocos2d::Ref *object, const std::string &tname, int64_t value) const {
	Event::send(*this, object, tname, value);
}
void EventHeader::send(cocos2d::Ref *object, const std::string &tname, double value) const {
	Event::send(*this, object, tname, value);
}
void EventHeader::send(cocos2d::Ref *object, const std::string &tname, bool value) const {
	Event::send(*this, object, tname, value);
}
void EventHeader::send(cocos2d::Ref *object, const std::string &tname, cocos2d::Ref *value, const std::string &oname) const {
	Event::send(*this, object, tname, value, oname);
}
void EventHeader::send(cocos2d::Ref *object, const std::string &tname, const char *value) const {
	Event::send(*this, object, tname, value);
}
void EventHeader::send(cocos2d::Ref *object, const std::string &tname, const std::string &value) const {
	Event::send(*this, object, tname, value);
}
void EventHeader::send(cocos2d::Ref *object, const std::string &tname) const {
	Event::send(*this, object, tname);
}
#else
void EventHeader::send(cocos2d::Ref *object, int64_t value) const {
	Event::send(*this, object, value);
}
void EventHeader::send(cocos2d::Ref *object, double value) const {
	Event::send(*this, object, value);
}
void EventHeader::send(cocos2d::Ref *object, bool value) const {
	Event::send(*this, object, value);
}
void EventHeader::send(cocos2d::Ref *object, cocos2d::Ref *value) const {
	Event::send(*this, object, value);
}
void EventHeader::send(cocos2d::Ref *object, const char *value) const {
	Event::send(*this, object, value);
}
void EventHeader::send(cocos2d::Ref *object, const std::string &value) const {
	Event::send(*this, object, value);
}
void EventHeader::send(cocos2d::Ref *object, const data::Value &value) const {
	Event::send(*this, object, value);
}
void EventHeader::send(cocos2d::Ref *object) const {
	Event::send(*this, object);
}
#endif
