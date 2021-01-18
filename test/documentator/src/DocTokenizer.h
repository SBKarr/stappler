/**
Copyright (c) 2020 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef TEST_DOCUMENTATOR_SRC_DOCTOKENIZER_H_
#define TEST_DOCUMENTATOR_SRC_DOCTOKENIZER_H_

#include "SPCommon.h"
#include "SPMemory.h"

namespace doc {

using namespace stappler::mem_pool;

using WhiteSpace = StringView::CharGroup<CharGroupId::WhiteSpace>;
using NewLine = StringView::Chars<'\r', '\n'>;
using Identifier = StringView::Compose<StringView::CharGroup<CharGroupId::Alphanumeric>, StringView::Chars<'_'>>;
using Number = StringView::CharGroup<CharGroupId::Numbers>;
using Operator = StringView::Chars<'=', '+', '-', '*', '/', '%', '&', '|', '^', '<', '>', '~', '!', '.', ',', '?', ':', ';', '#'>;

struct Token : AllocBase {
	enum Class {
		None,
		File,
		MultiLineComment,
		LineComment,
		Preprocessor,
		PreprocessorCondition,
		CharLiteral,
		StringLiteral,
		RawStringLiteral,
		Keyword,
		Word,
		NumWord,
		Operator,
		ParentesisBlock, // ( )
		OperatorBlock, //  { }
		CompositionBlock, // [ ]
	};

	Token(Class, StringView t, Token *root, size_t idx);

	Class tokClass = Class::None;
	StringView token;
	Token *root = nullptr;
	Token *next = nullptr;
	Token *prev = nullptr;
	Vector<Token *> tokens;

	size_t idx = 0;

	void makeFlow();
	void describe(std::ostream &, size_t depth = 0) const;
};

Token * tokenize(StringView data, const Callback<void(StringView, StringView)> &errCb);

}

#endif /* TEST_DOCUMENTATOR_SRC_DOCTOKENIZER_H_ */
