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

#ifndef SRC_SPUGEXPRESSION_H_
#define SRC_SPUGEXPRESSION_H_

#include "SPug.h"

NS_SP_EXT_BEGIN(pug)

struct Expression : memory::AllocPool {
	enum Op : uint8_t {
		NoOp,

		Var,

		SuffixIncr, // 1 ++
		SuffixDecr, // 1 --

		Sharp, // #  1
		Dot, // .    1
		Scope, // :  1
		Call, // ()  1
		Subscript, // [] 1
		Construct, // {} 1

		PrefixIncr, // 2 ++
		PrefixDecr, // 2 --

		Minus, // -  2
		Neg, // not  2
		BitNot, // ~  2

		Mult, // *   3
		Div, // /    3
		Rem, // %	 3

		Sum, // +    4
		Sub, // -    4

		ShiftLeft, //  <<   5
		ShiftRight, // >>   5

		Lt, // <     6
		LtEq, // <=  6
		Gt, // >     6
		GtEq, // >=  6

		Eq, // ==    7
		NotEq, // != 7

		BitAnd, // & 8
		BitXor, // ^ 9
		BitOr, // | 10

		And, // &&  11
		Or, // ||    12

		Conditional, // ? 13
		ConditionalSwitch, // ?: 14

		Assignment, // = 15
		SumAssignment, // += 15
		DiffAssignment, // -= 15
		MultAssignment, // *= 15
		DivAssignment, // /= 15
		RemAssignment, // %= 15
		LShiftAssignment, // <<= 15
		RShiftAssignment, // >>= 15
		AndAssignment, // &= 15
		XorAssignment, // ^= 15
		OrAssignment, // |= 15

		Colon, // :  16
		Comma, // ,  17
		Sequence, // ;  18
	};

	enum Block {
		None,
		Parentesis,
		Composition,
		Operator
	};

	enum Flags {
		StopOnNewLine,
		StopOnRootColon,
		StopOnRootComma,
		StopOnRootSequence,
		UseNewlineToken,
	};

	struct Options {
		static Options getDefaultInline();
		static Options getDefaultScript();
		static Options getWithhNewlineToken(const StringView &);

		Options &enableAllOperators();
		Options &disableAllOperators();

		Options &enableOperators(std::initializer_list<Op> &&);
		Options &disableOperators(std::initializer_list<Op> &&);

		Options &setFlags(std::initializer_list<Flags> &&);
		Options &clearFlags(std::initializer_list<Flags> &&);

		Options &useNewlineToken(const StringView &);

		bool hasFlag(Flags) const;
		bool hasOperator(Op) const;

		std::bitset<toInt(UseNewlineToken) + 1> flags;
		std::bitset<toInt(Op::Sequence) + 1> operators;
		StringView newlineToken;
	};

	static Expression *parse(StringView &, const Options &);

	Expression();
	Expression(Op);
	Expression(Op, Expression *, Expression *);
	Expression(Op, Expression *, Expression *, const StringView &);
	Expression(Op, Expression *, Expression *, Value &&);
	~Expression();

	void set(Value &&);
	void set(const StringView &);

	bool empty() const;
	bool isConst() const;

	void describe(std::ostream &stream, size_t depth = 0);

	Op op;
	Expression *left = nullptr;
	Expression *right = nullptr;
	Block block = None;
	bool isToken = false;
	Value value;
};

NS_SP_EXT_END(pug)

#endif /* SRC_SPUGEXPRESSION_H_ */
