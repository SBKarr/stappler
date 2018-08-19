/**
 Copyright (c) 2018 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef SRC_SPUGCONTEXT_H_
#define SRC_SPUGCONTEXT_H_

#include "SPugExpression.h"
#include "SPugVariable.h"

NS_SP_EXT_BEGIN(pug)

class Context : public memory::AllocPool {
public:
	static constexpr size_t StackSize = 256;

	using Callback = VarClass::Callback;
	using IncludeCallback = Function<bool(const StringView &, Context &, std::ostream &, const Template *)>;

	struct VarScope : memory::AllocPool {
		memory::dict<String, VarStorage> namedVars;
		VarScope *prev = nullptr;
	};

	struct VarList : memory::AllocPool {
		static constexpr size_t MinStaticVars = 8;

		size_t staticCount = 0;
		std::array<Var, MinStaticVars> staticList;
		Vector<Var> dynamicList;

		void emplace(Var &&);

		size_t size() const;
		Var * data();
	};

	static bool isConstExpression(const Expression &);
	static bool printConstExpr(const Expression &, std::ostream &, bool escapeOutput = false);
	static bool printAttrVar(const StringView &, const Expression &, std::ostream &, bool escapeOutput = false);
	static bool printAttrExpr(const Expression &, std::ostream &);

	Context();

	void setErrorStream(std::ostream &);

	bool print(const Expression &, std::ostream &, bool escapeOutput = false);
	bool printAttr(const StringView &name, const Expression &, std::ostream &, bool escapeOutput = false);
	bool printAttrExprList(const Expression &, std::ostream &);
	Var exec(const Expression &, std::ostream &);

	void set(const StringView &name, const Value &, VarClass * = nullptr);
	void set(const StringView &name, Value &&, VarClass * = nullptr);
	void set(const StringView &name, bool isConst, const Value *, VarClass * = nullptr);

	void set(const StringView &name, Callback &&);

	VarClass * set(const StringView &name, VarClass &&);

	bool runInclude(const StringView &, std::ostream &, const Template *);

	void setIncludeCallback(IncludeCallback &&);

	void pushVarScope(VarScope &);
	void popVarScope();

protected:
	IncludeCallback _includeCallback;
	VarScope *currentScope = nullptr;
	VarScope globalScope;
	Map<String, VarClass> classes;
};

NS_SP_EXT_END(pug)

#endif /* SRC_SPUGCONTEXT_H_ */
