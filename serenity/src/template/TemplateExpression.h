/*
 * TemplateExpression.h
 *
 *  Created on: 24 нояб. 2016 г.
 *      Author: sbkarr
 */

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
	Expression(CharReaderBase &);
	Expression(const CharReaderBase &);
	Expression(const String &);

	bool parse(CharReaderBase &);
	bool parse(const CharReaderBase &);
	bool parse(const String &);

	Op readOperator(CharReaderBase &r);

	bool readOperand(Node **, CharReaderBase &r, Op op);
	bool readOperandValue(Node *, CharReaderBase &r, Op op);
	bool readExpression(Node **, CharReaderBase &r, bool root = false);

	Node *insertOp(Node **, Op);

	Node *root = nullptr;
};

NS_SA_EXT_END(tpl)

#endif /* APPS_TEMPLATEEXPRESSION_H_ */
