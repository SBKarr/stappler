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

#ifndef SPUG_SPUGTOKEN_H_
#define SPUG_SPUGTOKEN_H_

#include "SPug.h"

NS_SP_EXT_BEGIN(pug)

struct Token : memory::AllocPool {
	enum Type : uint16_t {
		Root,
		Line,

		LineData,
		LinePiped,
		LinePlainText,
		LineComment,
		LineDot,
		LineOut,
		LineCode,
		LineCodeBlock,

		PlainText,
		Code,
		OutputEscaped,
		OutputUnescaped,

		CommentTemplate,
		CommentHtml,
		Tag,

		TagClassNote,
		TagIdNote,
		TagAttrList,
		TagAttrExpr,
		TagTrailingSlash,
		TagTrailingDot,
		TagTrailingEq,
		TagTrailingNEq,

		AttrPairEscaped,
		AttrPairUnescaped,
		AttrName,
		AttrValue,

		ControlCase,
		ControlWhen,
		ControlDefault,

		ControlIf,
		ControlUnless,
		ControlElseIf,
		ControlElse,
		ControlEach,
		ControlEachPair,
		ControlWhile,
		ControlMixin,

		ControlEachVariable,

		PipeMark,
		Include,
		Doctype,

		MixinCall,
		MixinArgs,
	};

	Token(Type t, const StringView &d);
	Token(Type t, const StringView &d, Expression *);

	void addChild(Token *);
	void describe(std::ostream &stream) const;

	Type type = Root;
	StringView data;
	Expression *expression = nullptr;

	Token *prev = nullptr;
	Token *next = nullptr;
	Token *child = nullptr;
	Token *tail = nullptr;
};

NS_SP_EXT_END(pug)

#endif /* SRC_SPUGTOKEN_H_ */
