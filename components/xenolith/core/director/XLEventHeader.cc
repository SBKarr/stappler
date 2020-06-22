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

#include "XLEventHeader.h"

namespace stappler::xenolith {

struct EventList {
	static EventList *getInstance() {
		static EventList *instance = nullptr;
		if (!instance) {
			instance = new EventList();
		}
		return instance;
	}

	uint32_t addEventHeader(const EventHeader *header) {
		_knownEvents.insert(header);
		return (uint32_t)_knownEvents.size();
	}
	void removeEventHeader(const EventHeader *header) {
		_knownEvents.erase(header);
	}

	Set<const EventHeader *> _knownEvents;
};

EventHeader::Category EventHeader::getCategoryForName(const String &catName) {
	return string::hash32(catName);
}

EventHeader::EventHeader(Category cat, const String &name)
: _category(cat), _name(name) {
	assert(!name.empty());
	_id = EventList::getInstance()->addEventHeader(this);
}

EventHeader::EventHeader(const String &catName, const String &eventName)
: EventHeader(string::hash32(catName), eventName) { }

EventHeader::EventHeader(const EventHeader &other) : _category(other._category), _id(other._id), _name(other._name) { }
EventHeader::EventHeader(EventHeader &&other) : _category(other._category), _id(other._id), _name(std::move(other._name)) { }

EventHeader &EventHeader::operator=(const EventHeader &other) {
	_category = other._category;
	_id = other._id;
	_name = other._name;
	return *this;
}
EventHeader &EventHeader::operator=(EventHeader &&other) {
	_category = other._category;
	_id = other._id;
	_name = std::move(other._name);
	return *this;
}

EventHeader::~EventHeader() {
	EventList::getInstance()->removeEventHeader(this);
}

EventHeader::Category EventHeader::getCategory() const {
	return _category;
}

EventHeader::EventID EventHeader::getEventID() const {
	return _id;
}

const String &EventHeader::getName() const {
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

void EventHeader::send(Ref *object, int64_t value) const {
	Event::send(*this, object, value);
}
void EventHeader::send(Ref *object, double value) const {
	Event::send(*this, object, value);
}
void EventHeader::send(Ref *object, bool value) const {
	Event::send(*this, object, value);
}
void EventHeader::send(Ref *object, Ref *value) const {
	Event::send(*this, object, value);
}
void EventHeader::send(Ref *object, const char *value) const {
	Event::send(*this, object, value);
}
void EventHeader::send(Ref *object, const String &value) const {
	Event::send(*this, object, value);
}
void EventHeader::send(Ref *object, const StringView &value) const {
	Event::send(*this, object, value);
}
void EventHeader::send(Ref *object, const data::Value &value) const {
	Event::send(*this, object, value);
}
void EventHeader::send(Ref *object) const {
	Event::send(*this, object);
}

}
