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

#ifndef SERENITY_SRC_STORAGE_STORAGEOBJECT_H_
#define SERENITY_SRC_STORAGE_STORAGEOBJECT_H_

#include "Define.h"
#include "SPDataWrapper.h"

NS_SA_EXT_BEGIN(storage)

class Object : public data::Wrapper {
public:
	Object(data::Value &&, Scheme *);

	Scheme *getScheme() const;
	uint64_t getObjectId() const;

	void lockProperty(const String &prop);
	void unlockProperty(const String &prop);
	bool isPropertyLocked(const String &prop) const;

	bool isFieldProtected(const String &) const;

	auto begin() { return Wrapper::begin<Object>(this); }
	auto end() { return Wrapper::end<Object>(this); }

	auto begin() const { return Wrapper::begin<Object>(this); }
	auto end() const { return Wrapper::end<Object>(this); }

	bool save(Adapter *, bool force = false);

protected:
	friend class Scheme;

	uint64_t _oid;
	Set<String> _locked;
	Scheme *_scheme;
};

NS_SA_EXT_END(storage)

#endif /* SERENITY_SRC_STORAGE_STORAGEOBJECT_H_ */
