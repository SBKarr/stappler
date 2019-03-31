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

#include "SPugToken.h"
#include "SPugExpression.h"

NS_SP_EXT_BEGIN(pug)

Token::Token(Type t, const StringView &d) : type(t), data(d) { }

Token::Token(Type t, const StringView &d, Expression *e)
: type(t), data(d), expression(e) { }

void Token::addChild(Token *tok) {
	if (!child) {
		tail = child = tok;
	} else {
		tail->next = tok;
		tok->prev = tail;
		tail = tok;
	}
}

/// Forward declaration
static void stl_print_token(std::ostream &stream, const Token * t, uint16_t depth);
static void stl_print_token_tree(std::ostream &stream, const Token * t, uint16_t depth);

/// Print contents of the token based on specified string
static void stl_print_token(std::ostream &stream, const Token * t, uint16_t depth) {
	if (t) {
		for (int i = 0; i < depth; ++i) {
			stream << "  ";
		}

		stream << "* " << toInt(t->type) << " ";
		switch (t->type) {
		case Token::Root: stream << "<root>"; break;
		case Token::Line: stream << "<line>"; break;

		case Token::LineData: stream << "<line-data>"; break;
		case Token::LinePiped: stream << "<line-piped>"; break;
		case Token::LinePlainText: stream << "<line-plain-text>"; break;
		case Token::LineComment: stream << "<line-comment>"; break;
		case Token::LineDot: stream << "<line-dot>"; break;
		case Token::LineOut: stream << "<line-out>"; break;
		case Token::LineCode: stream << "<line-code>"; break;
		case Token::LineCodeBlock: stream << "<line-code-block>"; break;

		case Token::PlainText: stream << "<plain-text>"; break;
		case Token::Code: stream << "<code>"; break;
		case Token::OutputEscaped: stream << "<out-escaped>"; break;
		case Token::OutputUnescaped: stream << "<out-unescaped>"; break;

		case Token::CommentTemplate: stream << "<comment-template>"; break;
		case Token::CommentHtml: stream << "<comment-html>"; break;
		case Token::Tag: stream << "<tag>"; break;

		case Token::TagClassNote: stream << "<tag-class-note>"; break;
		case Token::TagIdNote: stream << "<tag-id-note>"; break;
		case Token::TagAttrList: stream << "<tag-attr-list>"; break;
		case Token::TagAttrExpr: stream << "<tag-attr-expr>"; break;
		case Token::TagTrailingSlash: stream << "<tag-trailing-slash>"; break;
		case Token::TagTrailingDot: stream << "<tag-trailing-dot>"; break;
		case Token::TagTrailingEq: stream << "<tag-trailing-eq>"; break;
		case Token::TagTrailingNEq: stream << "<tag-trailing-neq>"; break;

		case Token::AttrPairEscaped: stream << "<attr-pair-escaped>"; break;
		case Token::AttrPairUnescaped: stream << "<attr-pair-unescaped>"; break;
		case Token::AttrName: stream << "<attr-name>"; break;
		case Token::AttrValue: stream << "<attr-value>"; break;

		case Token::ControlCase: stream << "<control-case>"; break;
		case Token::ControlWhen: stream << "<control-when>"; break;
		case Token::ControlDefault: stream << "<control-default>"; break;

		case Token::ControlIf: stream << "<control-if>"; break;
		case Token::ControlUnless: stream << "<control-unless>"; break;
		case Token::ControlElseIf: stream << "<control-else-if>"; break;
		case Token::ControlElse: stream << "<control-else>"; break;
		case Token::ControlEach: stream << "<control-each>"; break;
		case Token::ControlEachPair: stream << "<control-each-pair>"; break;
		case Token::ControlWhile: stream << "<control-while>"; break;
		case Token::ControlMixin: stream << "<control-mixin>"; break;

		case Token::ControlEachVariable: stream << "<control-each-variable>"; break;

		case Token::PipeMark: stream << "<pipe-mark>"; break;
		case Token::Include: stream << "<include>"; break;
		case Token::Doctype: stream << "<doctype>"; break;
		case Token::MixinCall: stream << "<mixin-call>"; break;
		case Token::MixinArgs: stream << "<mixin-args>"; break;
		}

		if (t->expression) {
			stream << " expression: '" << t->data << "'";
		} else {
			stream << " '" << t->data << "'";
		}
		stream << "\n";
		if (t->expression) {
			t->expression->describe(stream, depth + 1);
		}
		if (t->child) {
			stl_print_token_tree(stream, t->child, depth + 1);
		}
	}
}

/// Print contents of the token tree based on specified string
static void stl_print_token_tree(std::ostream &stream, const Token * t, uint16_t depth) {
	while (t != NULL) {
		stl_print_token(stream, t, depth);
		t = t->next;
	}
}

void Token::describe(std::ostream &stream) const {
	stl_print_token(stream, this, 0);
}

NS_SP_EXT_END(pug)
