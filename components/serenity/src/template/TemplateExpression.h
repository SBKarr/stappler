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

#ifndef APPS_TEMPLATEEXPRESSION_H_
#define APPS_TEMPLATEEXPRESSION_H_

#include "Define.h"

NS_SA_EXT_BEGIN(tpl)

struct Expression : public ReaderClassBase<char> {
	enum Op : uint8_t {
		NoOp,

		// accessors
		Sharp, // #  1
		Dot, // .    1
		Colon, // :  1

		Minus, // -  2
		Neg, // not  2

		Mult, // *   3
		Div, // /    3

		Sum, // +    4
		Sub, // -    4

		Lt, // <     5
		ltEq, // <=  5
		Gt, // >     5
		GtEq, // >=  5

		Eq, // =     6
		NotEq, // != 6


		And, // and  7
		Or, // or    8
		Comma, // ,  9
	};

	static uint32_t priority(Op o) {
		static const uint32_t p_arr[] = {
			0, // NoOp
			1, 1, 1, // # . :
			2, 2, // - not
			3, 3, // * /
			4, 4, // + -
			5, 5, 5, 5, // < <= > >=
			6, 6, // = !=
			7, 8, 9, // and or ,
		};
		return p_arr[toInt(o)];
	}

	struct Node {
		Op op;
		bool block = false;
		String value;
		Node *left = nullptr;
		Node *right = nullptr;
	};

	Expression() { }
	Expression(StringView &);
	Expression(const StringView &);
	Expression(const String &);

	bool parse(StringView &);
	bool parse(const StringView &);
	bool parse(const String &);

	Op readOperator(StringView &r);

	bool readOperand(Node **, StringView &r, Op op);
	bool readOperandValue(Node *, StringView &r, Op op);
	bool readExpression(Node **, StringView &r, bool root = false);

	Node *insertOp(Node **, Op);

	Node *root = nullptr;
};

NS_SA_EXT_END(tpl)

#endif /* APPS_TEMPLATEEXPRESSION_H_ */
