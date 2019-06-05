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

#ifndef STELLATOR_DB_STSTORAGEOBJECT_H_
#define STELLATOR_DB_STSTORAGEOBJECT_H_

#include "SPDataWrapper.h"
#include "STStorage.h"

NS_DB_BEGIN

class Object : public stappler::data::WrapperTemplate<mem::Interface> {
public:
	Object(mem::Value &&, const Scheme &);

	const Scheme &getScheme() const;
	uint64_t getObjectId() const;

	void lockProperty(const mem::StringView &prop);
	void unlockProperty(const mem::StringView &prop);
	bool isPropertyLocked(const mem::StringView &prop) const;

	bool isFieldProtected(const mem::StringView &) const;

	auto begin() { return WrapperTemplate::begin<Object>(this); }
	auto end() { return WrapperTemplate::end<Object>(this); }

	auto begin() const { return WrapperTemplate::begin<Object>(this); }
	auto end() const { return WrapperTemplate::end<Object>(this); }

	bool save(const Adapter &, bool force = false);

protected:
	friend class Scheme;

	uint64_t _oid;
	mem::Set<mem::String> _locked;
	const Scheme &_scheme;
};

NS_DB_END

#endif /* STELLATOR_DB_STSTORAGEOBJECT_H_ */
