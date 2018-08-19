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

#include "SPugExpression.h"

NS_SP_EXT_BEGIN(pug)

inline void Lexer_parseBufferString(StringView &r, std::ostream &stream) {
#define Z16 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	static const char escape[256] = {
		Z16, Z16, 0, 0,'\"', 0, 0, 0, 0, '\'', 0, 0, 0, 0, 0, 0, 0,'/',
		Z16, Z16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,'\\', 0, 0, 0,
		0, 0,'\b', 0, 0, 0,'\f', 0, 0, 0, 0, 0, 0, 0,'\n', 0,
		0, 0,'\r', 0,'\t', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		Z16, Z16, Z16, Z16, Z16, Z16, Z16, Z16
	};
#undef Z16
	char quot = 0;
	if (r.is('"')) { r ++; quot = '"'; }
	if (r.is('\'')) { r ++; quot = '\''; }

	if (quot) {
		do {
			auto s = (quot == '"') ? r.readUntil<StringView::Chars<'"', '\\'>>() : r.readUntil<StringView::Chars<'\'', '\\'>>();
			stream << s;
			if (r.is('\\')) {
				++ r;
				if (r.is('u') && r.size() >= 5) {
					++ r;
					unicode::utf8Encode(stream, char16_t(base16::hexToChar(r[0], r[1]) << 8 | base16::hexToChar(r[2], r[3]) ));
					r += 4;
				} else {
					auto tmp = escape[(uint8_t)r[0]];
					if (tmp) {
						stream << tmp;
					} else {
						stream << r[0];
					}
					++ r;
				}
			}
		} while (!r.empty() && !r.is(quot));
		if (r.is(quot)) { ++ r; }
	} else {
		stream << r.readChars<StringView::CharGroup<CharGroupId::Alphanumeric>, StringView::Chars<'_'>>();
	}
}

static uint8_t Lexer_Expression_priority(Expression::Op o) {
	static const uint8_t p_arr[] = {
		0, 1, // NoOp, Var
		1, 1, // ++ --
		1, 1, 1, 1, 1, 1, // # . : () [] {}
		2, 2, // ++ --
		2, 2, 2, // - not ~
		3, 3, 3, // * / %
		4, 4, // + -
		5, 5, // << >>
		6, 6, 6, 6, // < <= > >=
		7, 7, // == !=
		8, 9, 10, // & ^ |
		11, 12, // and or
		13, 14,  // ?:
		15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, // = += -= *= /= %= <<= >>= &= ^= |=
		16, 17, 18 // ?: : , ;
	};
	return p_arr[toInt(o)];
}

static bool Lexer_Expression_associativity(Expression::Op o) { // false for Left-to-right, true for Right-to-left
	static const bool p_arr[] = {
		false, false, // NoOp, Var    Left-to-right
		false, false, // ++ --
		false, false, false, false, false, false, // # . :: () [] {}    Left-to-right
		true, true, // ++ --
		true, true, true, // - not ~   Right-to-left
		false, false, false, // * / %   Left-to-right
		false, false, // + -    Left-to-right
		false, false, // << >>  Left-to-right
		false, false, false, false, // < <= > >=    Left-to-right
		false, false, // == !=   Left-to-right
		false, false, false, // & ^ |    Left-to-right
		false, false, // and or
		true, true, // ?:
		true, true, true, true, true, true, true, true, true, true, true, // = Right-to-left
		false, false, false // , ; Left-to-right
	};
	return p_arr[toInt(o)];
}


Expression::Expression() : op(NoOp) { new (&value) data::Value(); }
Expression::Expression(Op op) : op (op) { new (&value) data::Value(); }
Expression::Expression(Op op, Expression *l, Expression *r) : op(op), left(l), right(r) { new (&value) data::Value(); }
Expression::Expression(Op op, Expression *l, Expression *r, const StringView & t) : op(op), left(l), right(r) {
	isToken = true;
	value.setString(t);
}
Expression::Expression(Op op, Expression *l, Expression *r, Value && d) : op(op), left(l), right(r), value(move(d)) { }

void Expression::set(Value && d) {
	isToken = false;
	value = std::move(d);
}
void Expression::set(const StringView & t) {
	isToken = true;
	value.setString(t);
}

bool Expression::empty() const {
	return value.empty();
}

static bool Expression_isConst(const Expression &expr, Expression::Op op) {
	if (expr.op == Expression::Call) {
		return false;
	}
	if (expr.left && expr.right) {
		return Expression_isConst(*expr.left, expr.op) && Expression_isConst(*expr.right, Expression::NoOp);
	} else if (expr.left) {
		return Expression_isConst(*expr.left, Expression::NoOp);
	} else {
		return !expr.isToken || op == Expression::Colon;
	}
}

bool Expression::isConst() const {
	return Expression_isConst(*this, NoOp);
}

static bool Lexer_readExpression(Expression **node, StringView &r, const Expression::Options &opts, bool isRoot);
static bool Lexer_Expression_readOperand(Expression **node, StringView &r, Expression::Op op, const Expression::Options &opts);
static bool Lexer_Expression_readOperandValue(Expression *current, StringView &r, Expression::Op op, const Expression::Options &opts);

static Expression::Op Lexer_Expression_readOperator(StringView &r, char brace, const Expression::Options &opts) {
	auto checkOperator = [&] (Expression::Op op) {
		return opts.hasOperator(op) ? op : Expression::NoOp;
	};

	if (r.is("+=")) { r += 2; return checkOperator(Expression::SumAssignment); }
	else if (r.is("-=")) { r += 2; return checkOperator(Expression::DiffAssignment); }
	else if (r.is("*=")) { r += 2; return checkOperator(Expression::MultAssignment); }
	else if (r.is("/=")) { r += 2; return checkOperator(Expression::DivAssignment); }
	else if (r.is("%=")) { r += 2; return checkOperator(Expression::RemAssignment); }
	else if (r.is("<<=")) { r += 3; return checkOperator(Expression::LShiftAssignment); }
	else if (r.is(">>=")) { r += 3; return checkOperator(Expression::RShiftAssignment); }
	else if (r.is("&=")) { r += 2; return checkOperator(Expression::AndAssignment); }
	else if (r.is("^=")) { r += 2; return checkOperator(Expression::XorAssignment); }
	else if (r.is("|=")) { r += 2; return checkOperator(Expression::OrAssignment); }
	else if (r.is("&&")) { r += 2; return checkOperator(Expression::And); }
	else if (r.is("||")) { r += 2; return checkOperator(Expression::Or); }
	else if (r.is("++")) { r += 2; return checkOperator(Expression::SuffixIncr); }
	else if (r.is("--")) { r += 2; return checkOperator(Expression::SuffixDecr); }
	else if (r.is("<<")) { r += 2; return checkOperator(Expression::ShiftLeft); }
	else if (r.is(">>")) { r += 2; return checkOperator(Expression::ShiftRight); }
	else if (r.is("<=")) { r += 2; return checkOperator(Expression::LtEq); }
	else if (r.is(">=")) { r += 2; return checkOperator(Expression::GtEq); }
	else if (r.is("==")) { r += 2; return checkOperator(Expression::Eq); }
	else if (r.is("!=")) { r += 2; return checkOperator(Expression::NotEq); }
	else if (r.is("::")) { r += 2; return checkOperator(Expression::Scope); }
	else if (r.is('=')) { ++ r; return checkOperator(Expression::Assignment); }
	else if (r.is('#')) { ++ r; return checkOperator(Expression::Sharp); }
	else if (r.is('.')) { ++ r; return checkOperator(Expression::Dot); }
	else if (r.is('?')) { return checkOperator(Expression::Conditional); }
	else if (r.is('(')) { return checkOperator(Expression::Call); }
	else if (r.is('[')) { return checkOperator(Expression::Subscript); }
	else if (r.is('{')) { return checkOperator(Expression::Construct); }
	else if (r.is('*')) { ++ r; return checkOperator(Expression::Mult); }
	else if (r.is('/')) { ++ r; return checkOperator(Expression::Div); }
	else if (r.is('/')) { ++ r; return checkOperator(Expression::Rem); }
	else if (r.is('+')) { ++ r; return checkOperator(Expression::Sum); }
	else if (r.is('-')) { ++ r; return checkOperator(Expression::Sub); }
	else if (r.is('<')) { ++ r; return checkOperator(Expression::Lt); }
	else if (r.is('>')) { ++ r; return checkOperator(Expression::Gt); }
	else if (r.is('&')) { ++ r; return checkOperator(Expression::BitAnd); }
	else if (r.is('^')) { ++ r; return checkOperator(Expression::BitXor); }
	else if (r.is('|')) { ++ r; return checkOperator(Expression::BitOr); }
	else if (r.is(':') && (brace || !opts.hasFlag(Expression::StopOnRootComma))) { ++ r; return checkOperator(Expression::Colon); }
	else if (r.is(',') && (brace || !opts.hasFlag(Expression::StopOnRootColon))) { ++ r; return checkOperator(Expression::Comma); }
	else if (r.is(';') && (brace || !opts.hasFlag(Expression::StopOnRootSequence))) { ++ r; return checkOperator(Expression::Sequence); }

	return Expression::NoOp;
}

static Expression **Lexer_Expression_pushUnaryOp(Expression *node, Expression::Op op) {
	node->op = op;
	node->left = new Expression{Expression::Op::NoOp};
	return &node->left;
}

static Expression *Lexer_Expression_insertOp(Expression **node, Expression::Op op) {
	Expression **insert = node;
	Expression *prev = nullptr;
	Expression *current = *node;
	while (Lexer_Expression_priority(current->op) > Lexer_Expression_priority(op) && (current->right || current->left) && !current->block) {
		prev = current;
		if (current->right) {
			insert = &prev->right;
			current = current->right;
		} else {
			insert = &prev->left;
			current = current->left;
		}
	}

	if (Lexer_Expression_associativity(op) && Lexer_Expression_priority(current->op) >= Lexer_Expression_priority(op)) {
		while (current->right->left && Lexer_Expression_priority(current->right->op) == Lexer_Expression_priority(op)) {
			current = current->right;
		}
		current->right = new Expression(op, current->right, new Expression{Expression::NoOp});
		return current->right;
	} else {
		*insert = new Expression(op, current,
				(op != Expression::SuffixIncr && op != Expression::SuffixDecr)
					? new Expression{Expression::NoOp}
					: nullptr);
		return *insert;
	}
}

static bool Lexer_Expression_readOperandValue(Expression *current, StringView &r, Expression::Op op, const Expression::Options &opts) {
	bool ret = false;
	if (r.is('\'') || r.is('"')) {
		StringStream stream;
		Lexer_parseBufferString(r, stream);
		current->set(data::Value(stream.str()));
		ret = true;
	} else if (r.is<StringView::CharGroup<CharGroupId::Numbers>>()) { // numeric literal
		bool isFloat = false;
		auto tmp = r;
		r.skipChars<StringView::CharGroup<CharGroupId::Numbers>>();
		if (r.is('.')) {
			isFloat = true;
			++ r;
			r.skipChars<StringView::CharGroup<CharGroupId::Numbers>>();
		}
		if (r.is('E') || r.is('e')) {
			isFloat = true;
			++ r;
			if (r.is('+') || r.is('-')) {
				++ r;
			}
			r.skipChars<StringView::CharGroup<CharGroupId::Numbers>>();
		}

		auto value = StringView(tmp.data(), tmp.size() - r.size());
		if (isFloat) {
			current->set(data::Value(value.readDouble().get()));
		} else {
			current->set(data::Value(value.readInteger().get()));
		}
		ret = true;
	} else { // token literal
		if (op == Expression::Dot || op == Expression::Colon || op == Expression::Sharp) {
			auto tmp = r;
			if (op == Expression::Colon && (r.is('+') || r.is('-'))) {
				++ r;
			}
			r.skipChars<StringView::CharGroup<CharGroupId::Alphanumeric>, StringView::Chars<'_'>>();
			auto value = StringView(tmp.data(), tmp.size() - r.size());
			current->set(String(value.data(), value.size()));
			ret = true;
		} else {
			auto value = r.readChars<StringView::CharGroup<CharGroupId::Alphanumeric>, StringView::Chars<'_'>>();

			if (value.empty() && (op == Expression::Call || op == Expression::Subscript || op == Expression::Construct)) {
				current->isToken = true;
			} else if (value.is("true")) {
				current->set(data::Value(true));
			} else if (value.is("false")) {
				current->set(data::Value(false));
			} else if (value.is("null")) {
				current->set(data::Value());
			} else if (value.is("inf")) {
				current->set(data::Value(std::numeric_limits<double>::infinity()));
			} else if (value.is("nan")) {
				current->set(data::Value(nan()));
			} else if (!value.empty()) {
				current->set(String(value.data(), value.size()));
			} else {
				return false;
			}
			ret = true;
		}
	}
	return ret;
}

static bool Lexer_Expression_skipWhitespace(StringView &r, const Expression::Options &opts, bool isFinalizable) {
	auto tryEmptyString = [] (StringView &r, const StringView &token) -> bool {
		auto tmp = r;
		if (tmp.is("\r\n")) { tmp += 2; }
		else if (tmp.is('\n') || tmp.is('\r')) { ++ tmp; }
		tmp.skipChars<StringView::Chars<' ', '\t'>>();
		if (tmp.is('\n') || tmp.is('\r')) {
			r = tmp;
			return true;
		}
		return false;
	};

	if (opts.hasFlag(Expression::UseNewlineToken)) {
		while (r.is<StringView::Chars<' ', '\t', '\n', '\r'>>()) {
			r.skipChars<StringView::Chars<' ', '\t'>>();
			if (r.is('\n') || r.is('\r')) {
				if (!tryEmptyString(r, opts.newlineToken)) {
					if (isFinalizable || !r.is(opts.newlineToken)) {
						return false;
					} else {
						r += opts.newlineToken.size();
						r.skipChars<StringView::Chars<' ', '\t'>>();
					}
				}
			}
			r.skipChars<StringView::Chars<' ', '\t'>>();
		}
		return !r.empty();
	} else if (!opts.hasFlag(Expression::StopOnNewLine)) {
		if (isFinalizable) {
			r.skipChars<StringView::Chars<' ', '\t'>>();
			if (r.is('\n') || r.is('\r')) {
				return false;
			}
		}
		r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		return !r.empty();
	} else {
		r.skipChars<StringView::Chars<' ', '\t'>>();
		if (r.is('\n' || r.is('\r'))) {
			return false;
		}
		r.skipChars<StringView::Chars<' ', '\t'>>();
		return !r.empty();
	}
}

static bool Lexer_Expression_readOperand(Expression **node, StringView &r, Expression::Op op, const Expression::Options &opts) {
	if (!Lexer_Expression_skipWhitespace(r, opts, false)) {
		return false;
	}

	if (op != Expression::Dot && op != Expression::Sharp) {
		bool finalized = false;
		do {
			r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
			if (r.is('!')) {
				++ r;
				node = (Lexer_Expression_pushUnaryOp(*node, Expression::Neg));
			} else if (r.is('-')) {
				++ r;
				node = (Lexer_Expression_pushUnaryOp(*node, Expression::Minus));
			} else if (r.is('~')) {
				++ r;
				node = (Lexer_Expression_pushUnaryOp(*node, Expression::BitNot));
			} else if (r.is("++")) {
				r += 2;
				node = (Lexer_Expression_pushUnaryOp(*node, Expression::PrefixIncr));
			} else if (r.is("--")) {
				r += 2;
				node = (Lexer_Expression_pushUnaryOp(*node, Expression::PrefixDecr));
			} else if (r.is('+')) {
				++ r;
			} else if (r.is("var")) {
				auto tmp = r;
				tmp += 3;
				if (tmp.is<StringView::Chars<' ', '\t'>>()) {
					tmp.skipChars<StringView::Chars<' ', '\t'>>();
					r = tmp;
					node = (Lexer_Expression_pushUnaryOp(*node, Expression::Var));
				} else {
					finalized = true;
				}
			} else {
				finalized = true;
			}
		} while (!finalized);
	}

	// read as expression
	if (r.is('(') || r.is('[') || r.is('{')) {
		if (Lexer_readExpression(node, r, opts, false)) {
			return true;
		}
		return false;
	}

	// read as literal
	return Lexer_Expression_readOperandValue(*node, r, op, opts);
}

static bool Lexer_readExpression(Expression **node, StringView &r, const Expression::Options &opts, bool isRoot) {
	Expression::Op targetOp = Expression::NoOp;
	Expression::Block block = Expression::Block::Parentesis;
	char brace = 0;
	if ((r.is('(') || r.is('[') || r.is('{') || r.is('?')) && !isRoot) {
		auto tmp = r[0];
		switch (tmp) {
		case '(': brace = ')'; block = Expression::Parentesis; targetOp = Expression::Call; break;
		case '?': brace = ':'; block = Expression::Parentesis; targetOp = Expression::Conditional; break;
		case '[': brace = ']'; block = Expression::Composition; targetOp = Expression::Subscript; break;
		case '{': brace = '}'; block = Expression::Operator; targetOp = Expression::Construct; break;
		default: break;
		}
		++ r;
		if (!Lexer_Expression_skipWhitespace(r, opts, false)) {
			return false;
		}

		if (brace == '}' && r.is('}')) {
			++ r;
			(*node)->op = Expression::Construct;
			(*node)->block = Expression::Operator;
			return true;
		}
		if (brace == ']' && r.is(']')) {
			++ r;
			(*node)->op = Expression::Subscript;
			(*node)->block = Expression::Composition;
			return true;
		}
	}

	// read first operand
	if (!Lexer_Expression_readOperand(node, r, targetOp, opts)) {
		return false;
	}
	if (!Lexer_Expression_skipWhitespace(r, opts, isRoot)) {
		return isRoot;
	}

	Expression *current = *node;
	// if there is only first operand - no other ops will be executed
	while (!r.empty() && (brace == 0 || !r.is(brace))) {
		// read operator
		if (!Lexer_Expression_skipWhitespace(r, opts, false)) {
			return isRoot;
		}
		auto op = Lexer_Expression_readOperator(r, brace, opts);
		if (op == Expression::NoOp) {
			return isRoot;
		}

		current = Lexer_Expression_insertOp(node, op);
		if (op == Expression::Conditional) {
			if (!Lexer_readExpression(&current->right, r, opts, false)) {
				return false;
			}
			current = Lexer_Expression_insertOp(node, Expression::ConditionalSwitch);
			if (!Lexer_Expression_readOperand(&current->right, r, current->op, opts)) {
				return false;
			}
			if (!Lexer_Expression_skipWhitespace(r, opts, isRoot)) {
				return isRoot;
			}
		} else if (op == Expression::Call || op == Expression::Subscript || op == Expression::Construct) {
			if (!Lexer_readExpression(&current->right, r, opts, false)) {
				return false;
			}
			if (!Lexer_Expression_skipWhitespace(r, opts, false)) {
				return isRoot;
			}
		} else if (op == Expression::SuffixIncr || op == Expression::SuffixDecr) {
			// pass thru
		} else {
			if (!Lexer_Expression_readOperand(&current->right, r, current->op, opts)) {
				return false;
			}
			if (!Lexer_Expression_skipWhitespace(r, opts, isRoot)) {
				return isRoot;
			}
		}
	}

	(*node)->block = block;
	if (brace != 0 && r.is(brace)) {
		++ r;
	}
	return true;
}

static void stl_print_expression_op(std::ostream &stream, Expression::Op op) {
	switch (op) {

	case Expression::NoOp: stream << "<noop>"; break;
	case Expression::Var: stream << "<var>"; break;
	case Expression::SuffixIncr: stream << "() ++"; break;
	case Expression::SuffixDecr: stream << "() --"; break;

	case Expression::Sharp: stream << "# <sharp>"; break;
	case Expression::Dot: stream << ". <dot>"; break;
	case Expression::Scope: stream << ":: <scope>"; break;
	case Expression::Call: stream << "() <call>"; break;
	case Expression::Subscript: stream << "[] <subscript>"; break;
	case Expression::Construct: stream << "{} <construct>"; break;

	case Expression::PrefixIncr: stream << "++ ()"; break;
	case Expression::PrefixDecr: stream << "-- ()"; break;

	case Expression::Minus: stream << "- <minus>"; break;
	case Expression::Neg: stream << "! <neg>"; break;
	case Expression::BitNot: stream << "~ <bit-not>"; break;

	case Expression::Mult: stream << "* <mult>"; break;
	case Expression::Div: stream << "/ <div>"; break;
	case Expression::Rem: stream << "% <rem>"; break;

	case Expression::Sum: stream << "+ <sum>"; break;
	case Expression::Sub: stream << "- <sub>"; break;

	case Expression::ShiftLeft: stream << "<< <shift-left>"; break;
	case Expression::ShiftRight: stream << ">> <shift-right>"; break;

	case Expression::Lt: stream << "< <lt>"; break;
	case Expression::LtEq: stream << "<= <lt-eq>"; break;
	case Expression::Gt: stream << "> <gt>"; break;
	case Expression::GtEq: stream << ">= <gt-eq>"; break;

	case Expression::Eq: stream << "== <eq>"; break;
	case Expression::NotEq: stream << "!= <eq>"; break;

	case Expression::BitAnd: stream << "& <bit-and>"; break;
	case Expression::BitXor: stream << "^ <bit-xor>"; break;
	case Expression::BitOr: stream << "| <bit-or>"; break;

	case Expression::And: stream << "&& <and>"; break;
	case Expression::Or: stream << "|| <or>"; break;

	case Expression::Conditional: stream << "? <conditional>"; break;
	case Expression::ConditionalSwitch: stream << "?: <conditional-switch>"; break;

	case Expression::Assignment: stream << "= <assignment>"; break;
	case Expression::SumAssignment: stream << "+= <assignment>"; break;
	case Expression::DiffAssignment: stream << "-= <assignment>"; break;
	case Expression::MultAssignment: stream << "*= <assignment>"; break;
	case Expression::DivAssignment: stream << "/= <assignment>"; break;
	case Expression::RemAssignment: stream << "%= <assignment>"; break;
	case Expression::LShiftAssignment: stream << "<<= <assignment>"; break;
	case Expression::RShiftAssignment: stream << ">>= <assignment>"; break;
	case Expression::AndAssignment: stream << "&= <assignment>"; break;
	case Expression::XorAssignment: stream << "^= <assignment>"; break;
	case Expression::OrAssignment: stream << "|= <assignment>"; break;

	case Expression::Colon: stream << ": <colon>"; break;
	case Expression::Comma: stream << ", <comma>"; break;
	case Expression::Sequence: stream << "; <sequence>"; break;

	}
}

static void stl_print_expression_open_block(std::ostream &stream, Expression::Block b, uint16_t depth) {
	switch (b) {
	case Expression::None:
		break;
	case Expression::Parentesis:
		for (size_t i = 0; i < depth; ++ i) { stream << "  "; }
		stream << "(\n";
		break;
	case Expression::Composition:
		for (size_t i = 0; i < depth; ++ i) { stream << "  "; }
		stream << "[\n";
		break;
	case Expression::Operator:
		for (size_t i = 0; i < depth; ++ i) { stream << "  "; }
		stream << "{\n";
		break;
	}
}

static void stl_print_expression_close_block(std::ostream &stream, Expression::Block b, uint16_t depth) {
	switch (b) {
	case Expression::None:
		break;
	case Expression::Parentesis:
		for (size_t i = 0; i < depth; ++ i) { stream << "  "; }
		stream << ")\n";
		break;
	case Expression::Composition:
		for (size_t i = 0; i < depth; ++ i) { stream << "  "; }
		stream << "]\n";
		break;
	case Expression::Operator:
		for (size_t i = 0; i < depth; ++ i) { stream << "  "; }
		stream << "}\n";
		break;
	}
}

static void stl_print_expression(std::ostream &stream, Expression * t, uint16_t depth) {
	if (!t->left && !t->right) {
		for (size_t i = 0; i < depth; ++ i) { stream << "  "; }
		if (t->isToken) {
			stream << "(token) " << data::toString<stappler::memory::PoolInterface>(t->value, false) << "\n";
		} else if (t->op == Expression::NoOp) {
			switch (t->value.getType()) {
			case Value::Type::EMPTY: break;
			case Value::Type::NONE: break;
			case Value::Type::BOOLEAN: stream << "(bool) "; break;
			case Value::Type::INTEGER: stream << "(int) "; break;
			case Value::Type::DOUBLE: stream << "(float) "; break;
			case Value::Type::CHARSTRING: stream << "(string) "; break;
			case Value::Type::BYTESTRING: stream << "(bytes) "; break;
			case Value::Type::ARRAY: stream << "(array) "; break;
			case Value::Type::DICTIONARY: stream << "(dict) "; break;
			}
			stream << data::toString<stappler::memory::PoolInterface>(t->value, false) << "\n";
		} else {
			switch (t->op) {
			case Expression::Subscript: stream << "[] <subscript>\n"; break;
			case Expression::Construct: stream << "{} <construct>\n"; break;
			default: break;
			}
		}
	} else if (t->left && t->right) {
		stl_print_expression_open_block(stream, t->block, depth);
		stl_print_expression(stream, t->left, depth + 1);

		for (size_t i = 0; i < depth; ++ i) { stream << "  "; }
		stream << "-> ";
		stl_print_expression_op(stream, t->op);
		stream << "\n";

		stl_print_expression(stream, t->right, depth + 1);
		stl_print_expression_close_block(stream, t->block, depth);
	} else if (t->left) {
		for (size_t i = 0; i < depth; ++ i) { stream << "  "; }

		switch (t->block) {
		case Expression::None: break;
		case Expression::Parentesis: stream << "( "; break;
		case Expression::Composition: stream << "{ "; break;
		case Expression::Operator: stream << "[ "; break;
		}

		stl_print_expression_op(stream, t->op);

		switch (t->block) {
		case Expression::None: break;
		case Expression::Parentesis: stream << " )"; break;
		case Expression::Composition: stream << " }"; break;
		case Expression::Operator: stream << " ]"; break;
		}

		stream << "\n";

		stl_print_expression(stream, t->left, depth + 1);
	}
}

void Expression::describe(std::ostream &stream, size_t depth) {
	stl_print_expression(stream, this, depth);
}


Expression::Options Expression::Options::getDefaultInline() {
	return Options().enableAllOperators().setFlags({StopOnNewLine, StopOnRootColon, StopOnRootComma, StopOnRootSequence});
}
Expression::Options Expression::Options::getDefaultScript() {
	return Options().enableAllOperators().setFlags({StopOnRootColon, StopOnRootComma, StopOnRootSequence});
}
Expression::Options Expression::Options::getWithhNewlineToken(const StringView &token) {
	return Options().enableAllOperators().setFlags({StopOnRootColon, StopOnRootComma, StopOnRootSequence}).useNewlineToken(token);
}

Expression::Options &Expression::Options::enableAllOperators() {
	operators.set();
	return *this;
}

Expression::Options &Expression::Options::disableAllOperators() {
	operators.reset();
	return *this;
}

Expression::Options &Expression::Options::enableOperators(std::initializer_list<Op> && il) {
	for (auto &it : il) { operators.set(toInt(it)); }
	return *this;
}
Expression::Options &Expression::Options::disableOperators(std::initializer_list<Op> && il) {
	for (auto &it : il) { operators.reset(toInt(it)); }
	return *this;
}

Expression::Options &Expression::Options::useNewlineToken(const StringView &str) {
	newlineToken = str;
	setFlags({UseNewlineToken});
	return *this;
}

Expression::Options &Expression::Options::setFlags(std::initializer_list<Flags> && il) {
	for (auto &it : il) { flags.set(toInt(it)); }
	return *this;
}
Expression::Options &Expression::Options::clearFlags(std::initializer_list<Flags> && il) {
	for (auto &it : il) { flags.reset(toInt(it)); }
	return *this;
}

bool Expression::Options::hasFlag(Flags f) const {
	return flags.test(toInt(f));
}
bool Expression::Options::hasOperator(Op op) const {
	return operators.test(toInt(op));
}

Expression * Expression::parse(StringView &r, const Options &opts) {
	auto root = new Expression{Op::NoOp};
	if (!Lexer_readExpression(&root, r, opts, true)) {
		return nullptr;
	}
	return root;
}

NS_SP_EXT_END(pug)
