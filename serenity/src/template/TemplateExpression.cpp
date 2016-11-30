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

#include "Define.h"
#include "TemplateExpression.h"

NS_SA_EXT_BEGIN(tpl)

bool Expression::readOperand(Node **node, CharReaderBase &r, Op op) {
	r.skipChars<Group<GroupId::WhiteSpace>>();

	bool isRoot = true;
	// read unary operators
	if (op != Dot && op != Colon && op != Sharp) {
		while (r.is("not ") || r.is('!') || r.is('-') || r.is('+')) {
			if (!isRoot && !r.is('+')) {
				(*node)->left = new Node{Op::NoOp};
				node = &(*node)->left;
			} else {
				isRoot = r.is('+');
			}

			if (r.is('!')) {
				++ r;
				(*node)->op = Op::Neg;
			} else if (r.is('-')) {
				++ r;
				(*node)->op = Op::Minus;
			} else if (r.is("not ")) {
				r.skipString("not ");
				(*node)->op = Op::Neg;
			} else if (r.is('+')) {
				++ r;
			}

			r.skipChars<Group<GroupId::WhiteSpace>>();
		}
	}

	// read as expression
	if (r.is('(')) {
		return readExpression(node, r);
	}

	// read as literal
	return readOperandValue(*node, r, op);
}

bool Expression::readOperandValue(Node *current, CharReaderBase &r, Op op) {
	bool ret = false;
	if (r.is('\'')) { // string literal variant 1
		auto tmp = r;
		++ r;
		while (!r.empty() && !r.is('\'')) {
			r.skipUntil<Chars<'\'', '\\'>>();
			if (r.is('\\')) {
				r += 2;
			}
		}
		if (r.is('\'')) {
			++ r;
			auto value = CharReaderBase(tmp.data(), tmp.size() - r.size());
			current->value.assign_weak(value.data(), value.size());
			ret = true;
		}
	} else if (r.is('"')) { // string literal variant 2
		auto tmp = r;
		++ r;
		while (!r.empty() && !r.is('"')) {
			r.skipUntil<Chars<'"', '\\'>>();
			if (r.is('\\')) {
				r += 2;
			}
		}
		if (r.is('"')) {
			++ r;
			auto value = CharReaderBase(tmp.data(), tmp.size() - r.size());
			current->value.assign_weak(value.data(), value.size());
			ret = true;
		}
	} else if (r.is('$')) { // variable literal
		auto tmp = r;
		++ r;
		r.skipChars<Group<GroupId::Alphanumeric>, Chars<'_'>>();
		auto value = CharReaderBase(tmp.data(), tmp.size() - r.size());
		current->value.assign_weak(value.data(), value.size());
		ret = true;
	} else if (r.is<Group<GroupId::Numbers>>()) { // numeric literal
		auto tmp = r;
		r.skipChars<Group<GroupId::Numbers>>();
		if (r.is('.')) {
			++ r;
			r.skipChars<Group<GroupId::Numbers>>();
		}
		if (r.is('E') || r.is('e')) {
			++ r;
			if (r.is('+') || r.is('-')) {
				++ r;
			}
			r.skipChars<Group<GroupId::Numbers>>();
		}

		auto value = CharReaderBase(tmp.data(), tmp.size() - r.size());
		current->value.assign_weak(value.data(), value.size());
		ret = true;
	} else { // keyword literal
		if (op == Dot || op == Colon || op == Sharp) {
			auto tmp = r;
			if (r.is('+') || r.is('-')) {
				++ r;
			}
			r.skipChars<Group<GroupId::Alphanumeric>, Chars<'_'>>();
			auto value = CharReaderBase(tmp.data(), tmp.size() - r.size());
			current->value.assign_weak(value.data(), value.size());
			ret = true;
		} else {
			auto value = r.readChars<Group<GroupId::Alphanumeric>>();
			if (value.compare("true") || value.compare("false") || value.compare("null") || value.compare("NaN") || value.compare("inf")) {
				current->value.assign_weak(value.data(), value.size());
				ret = true;
			}
		}
	}
	r.skipChars<Group<GroupId::WhiteSpace>>();
	return ret;
}

bool Expression::readExpression(Node **node, CharReaderBase &r, bool root) {
	bool braced = false;
	if (!root && r.is('(')) {
		++ r;
		r.skipChars<Group<GroupId::WhiteSpace>>();
		braced = true;
	}

	// read first operand
	if (!readOperand(node, r, Op::NoOp)) {
		return false;
	}

	Node *current = *node;

	// if there is only first operand - no other ops will be executed
	while (!r.empty() && (!braced || !r.is(')'))) {
		// read operator
		auto op = readOperator(r);
		if (op == NoOp) {
			return false;
		}

		current = insertOp(node, op);
		if (!readOperand(&current->right, r, current->op)) {
			return false;
		}
	}

	(*node)->block = true;

	if (braced && r.is(')')) {
		++ r;
		r.skipChars<Group<GroupId::WhiteSpace>>();
	}
	return true;
}

Expression::Node *Expression::insertOp(Node **node, Op op) {
	Node **insert = node;
	Node *prev = nullptr;
	Node *current = *node;
	while (priority(current->op) > priority(op) && current->right && !current->block) {
		prev = current;
		insert = &prev->right;
		current = current->right;
	}

	*insert = new Node{op, false, String(), current, new Node{NoOp}};
	return *insert;
}

Expression::Op Expression::readOperator(CharReaderBase &r) {
	r.skipChars<Group<GroupId::WhiteSpace>>();
	if (r.is('#')) { ++ r; return Sharp; }
	else if (r.is('.')) { ++ r; return Dot; }
	else if (r.is(':')) { ++ r; return Colon; }
	else if (r.is('*')) { ++ r; return Mult; }
	else if (r.is('/')) { ++ r; return Div; }
	else if (r.is('+')) { ++ r; return Sum; }
	else if (r.is('-')) { ++ r; return Sub; }
	else if (r.is("<=")) { r += 2; return ltEq; }
	else if (r.is('<')) { ++ r; return Lt; }
	else if (r.is(">=")) { r += 2; return GtEq; }
	else if (r.is('>')) { ++ r; return Gt; }
	else if (r.is("==")) { r += 2; return Eq; }
	else if (r.is('=')) { ++ r; return Eq; }
	else if (r.is("!=")) { r += 2; return NotEq; }
	else if (r.is("&&")) { r += 2; return And; }
	else if (r.is("||")) { r += 2; return Or; }

	else if (r.is("and")) {
		r += 3;
		if (r.is<Group<GroupId::Alphanumeric>>() || r.is('$') || r.is('_')) {
			return NoOp;
		}
		return And;
	} else if (r.is("or")) {
		r += 2; return Or;
		if (r.is<Group<GroupId::Alphanumeric>>() || r.is('$') || r.is('_')) {
			return NoOp;
		}
	} else if (r.is(',')) {
		++ r; return Comma;
	}

	return NoOp;
}

Expression::Expression(const String &str) { parse(str); }
Expression::Expression(CharReaderBase &r) { parse(r); }
Expression::Expression(const CharReaderBase &r) { parse(r); }

bool Expression::parse(CharReaderBase &r) {
	root = new Node{Op::NoOp};
	return readExpression(&root, r, true);
}

bool Expression::parse(const CharReaderBase &ir) {
	CharReaderBase r(ir);
	return parse(r);
}

bool Expression::parse(const String &str) {
	CharReaderBase r(str);
	return parse(r);
}

NS_SA_EXT_END(tpl)
