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

#ifndef APPS_TEMPLATEEXEC_H_
#define APPS_TEMPLATEEXEC_H_

#include "Resource.h"
#include "TemplateExpression.h"

NS_SA_EXT_BEGIN(tpl)

class Exec : public ReaderClassBase<char> {
public:
	using Callback = Function<void(const String &, Exec &, Request &)>;

	struct Variable {
		const data::Value *value = nullptr;
		const storage::Scheme *scheme = nullptr;
	};

	Exec();

	void set(const String &, const storage::Scheme *);
	void set(const String &, data::Value &&);
	void set(const String &, data::Value &&, const storage::Scheme *);
	void set(const String &, const data::Value *);
	void set(const String &, const data::Value *, const storage::Scheme *);
	void clear(const String &);

	Variable getVariable(const StringView &);

	Variable exec(const Expression &);
	void print(const Expression &, Request &);

	void setIncludeCallback(const Callback &);
	const Callback &getIncludeCallback() const;

	void setTemplateCallback(const Callback &);
	const Callback &getTemplateCallback() const;

protected:
	using ReaderVec = Vector<StringView>;
	using ReaderVecIt = Vector<StringView>::iterator;

	Variable selectSchemeSetVariable(ReaderVec &path, ReaderVecIt &pathIt, const Variable &val, const storage::Field *obj);
	Variable selectSchemeByPath(ReaderVec &path, ReaderVecIt &pathIt, const storage::Scheme *scheme, int64_t oid = 0, const StringView &field = StringView());

	const data::Value * selectDataValue(const data::Value &val, StringView &r);

	Variable execNode(const Expression::Node *, Expression::Op);
	void printNode(const Expression::Node *, Expression::Op, Request &);

	Variable execPath(ReaderVec &);
	void execPathNode(ReaderVec &, const Expression::Node *);

	Variable execPath(ReaderVec &, ReaderVecIt, Variable &);
	Variable execPathVariable(ReaderVec &, ReaderVecIt, const data::Value *);
	bool execPathObject(ReaderVec &, ReaderVecIt &, Variable &);

	Variable eval(const String &);
	Variable perform(Variable &, Expression::Op);
	Variable perform(Variable &, Variable &, Expression::Op);

	storage::Adapter _storage;
	Map<String, Variable> _variables;

	Callback _templateCallback;
	Callback _includeCallback;
};

NS_SA_EXT_END(tpl)

#endif /* APPS_TEMPLATEEXEC_H_ */
