/*

	Copyright © 2016 - 2017 Fletcher T. Penney.
	Copyright © 2017 Roman Katuntsev <sbkarr@stappler.org>


	The `MultiMarkdown 6` project is released under the MIT License..

	GLibFacade.c and GLibFacade.h are from the MultiMarkdown v4 project:

		https://github.com/fletcher/MultiMarkdown-4/

	MMD 4 is released under both the MIT License and GPL.


	CuTest is released under the zlib/libpng license. See CuTest.c for the text
	of the license.


	## The MIT License ##

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

*/

#ifndef MMD_COMMON_MMDCHARS_H_
#define MMD_COMMON_MMDCHARS_H_

#include "SPCommon.h"
#include "MMDCommon.h"
#include "SPStringView.h"

NS_MMD_BEGIN

namespace chars {

/// Define character types
enum class Type : uint8_t {
	Whitespace	= 1 << 0,	//!< ' ','\t'
	Punctuation	= 1 << 1,	//!< .!?,;:"'`~(){}[]#$%+-=<>&@\/^*_|
	Alpha		= 1 << 2,	//!< a-zA-Z
	Digit		= 1 << 3,	//!< 0-9
	LineEnding	= 1 << 4,	//!< \n,\r,\0
};

SP_DEFINE_ENUM_AS_MASK(Type);

bool isType(char, Type);

// Is character whitespace?
inline bool isWhitespace(char c) { return isType(c, Type::Whitespace); }
inline bool isLineEnding(char c) { return isType(c, Type::LineEnding); }
inline bool isPunctuation(char c) { return isType(c, Type::Punctuation); }
inline bool isAlpha(char c) { return isType(c, Type::Alpha); }
inline bool isDigit(char c) { return isType(c, Type::Digit); }
inline bool isAlphanumeric(char c) { return isType(c, Type::Digit | Type::Alpha); }
inline bool isWhitespaceOrLineEnding(char c) { return isType(c, Type::Whitespace | Type::LineEnding); }
inline bool isWhitespaceOrLineEndingOrPunctuation(char c) { return isType(c, Type::Whitespace | Type::LineEnding | Type::Punctuation); }

// Is byte a UTF-8 continuation byte
inline bool isContinuationByte(char x) { return (x & 0xC0) == 0x80; }
inline bool isLeadMultibyte(char x) { return (x & 0xC0) == 0xC0; }

}

NS_MMD_END

#endif /* MMD_COMMON_MMDCHARS_H_ */
