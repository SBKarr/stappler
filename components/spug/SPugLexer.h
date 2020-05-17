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

#ifndef SPUG_SPUGLEXER_H_
#define SPUG_SPUGLEXER_H_

#include "SPugToken.h"

NS_SP_EXT_BEGIN(pug)

struct Lexer {
	using ErrCb = Callback<void(const StringView &)>;

	Lexer(const StringView &, const ErrCb & = nullptr);

	bool perform(const ErrCb &);
	bool parseToken(const ErrCb &, Token &);

	bool readAttributes(const ErrCb &, Token *, StringView &) const;
	bool readOutputExpression(Token *, StringView &) const;
	bool readTagInfo(const ErrCb &, Token *, StringView &, bool interpolated = false) const;
	bool readCode(Token *, StringView &) const;
	bool readCodeBlock(Token *, StringView &) const;

	bool readPlainTextInterpolation(const ErrCb &errCb, Token *, StringView &, bool interpolated = false) const;

	Token *readLine(const ErrCb &errCb, const StringView &line, StringView &, Token *rootLine);
	Token *readPlainLine(const ErrCb &errCb, const StringView &line, StringView &);
	Token *readCommonLine(const ErrCb &errCb, const StringView &line, StringView &);
	Token *readKeywordLine(const ErrCb &errCb, const StringView &line, StringView &);

	bool onError(const ErrCb &, const StringView &r, const StringView &) const;

	operator bool () const { return success; }

	uint32_t offset = 0;
	uint32_t lineOffset = 0;

	uint32_t indentStep = maxOf<uint32_t>();
	uint32_t indentLevel = 0;

	bool success = false;

	StringView content;
	Token root;
};

NS_SP_EXT_END(pug)

#endif /* SRC_SPUGLEXER_H_ */
