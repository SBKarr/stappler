/**
 Copyright (c) 2018-2019 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef SPUG_SPUGVARIABLE_H_
#define SPUG_SPUGVARIABLE_H_

#include "SPug.h"

NS_SP_EXT_BEGIN(pug)

struct VarStorage;

struct VarClass : memory::AllocPool {
	using Callback = Function<Var (VarStorage &, Var *, size_t argc)>;

	Map<String, Callback> staticFunctions;
	Map<String, Callback> functions;
};

struct VarData {
	enum Type {
		Null,
		Inline,
		Reference,
	};

	Type type = Null;
	union {
		Value value;
		struct {
			bool isConstant;
			const Value *value;
		} pointer;
	};

	~VarData();

	VarData();
	VarData(bool isConst, const Value *);
	VarData(const Value &);
	VarData(Value &&);

	VarData(const VarData &);
	VarData(VarData &&);

	VarData& operator=(const VarData &);
	VarData& operator=(VarData &&);

	void clear();

	const Value &readValue() const;
	Value *getMutable() const;

	void assign(const VarData &);
};

struct VarStorage {
	using Callback = VarClass::Callback;

	enum Type {
		Undefined,
		ValueReference, // value
		ObjectReference, // value reference + class pointer
		ClassPointer,
		StandaloneFunction,
		ClassFunction,
		ValueFunction,
		MemberFunction,
	};

	VarData value;
	VarClass *classPointer = nullptr;
	Callback *functionPointer = nullptr;

	Type type = Undefined;

	void set(const Value &, VarClass *);
	void set(Value &&, VarClass *);
	void set(bool isConst, const Value *, VarClass *);
	void set(Callback *);
	void set(VarClass *);

	bool assign(const Var &);
	void clear();

	const Value &readValue() const;
	Value *getMutable() const;

	Callback * getCallable() const;
};

struct VarWritable {
	Value *value;
	StringView key;
};

struct Var {
	using Callback = VarClass::Callback;

	enum Type {
		Undefined,
		Static, // variables in expressions
		Temporary, // temporary
		Variable, // variable in context storage
		Writable, // writing will create storage for variable
	};

	struct StoragePointer {
		VarStorage *storage = nullptr;
	};

	Type type = Undefined;
	union {
		VarData staticStorage;
		VarStorage *variableStorage;
		VarStorage temporaryStorage;
		VarWritable writableStorage;
	};

	~Var();

	Var();

	Var(const Var &);
	Var(Var &&);

	Var(Value &&);
	Var(const Value &);
	Var(bool isConst, const Value *);
	Var(Value *);
	Var(Value *, const StringView &);

	Var(VarStorage *);
	Var(VarClass *);
	Var(const Var &, Callback &, VarStorage::Type);

	Var& operator=(const Var &);
	Var& operator=(Var &&);

	operator bool () const;

	const Value &readValue() const;
	Value *getMutable() const;

	void clear();

	bool assign(const Var &);

	Var subscript(const StringView &, bool mut = false);
	Var subscript(int64_t, bool mut = false);

	Callback * getCallable() const;
	VarStorage * getStorage() const;
};

NS_SP_EXT_END(pug)

#endif /* SRC_SPUGVARIABLE_H_ */
