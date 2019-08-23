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

#ifndef MMD_COMMON_MMDCOMMON_H_
#define MMD_COMMON_MMDCOMMON_H_

#include "SPCommon.h"
#include "SPStringView.h"

extern "C" {
typedef struct _sp_mmd_token _sp_mmd_token;
}

#define NS_MMD_BEGIN NS_SP_EXT_BEGIN(mmd)
#define NS_MMD_END NS_SP_EXT_END(mmd)

NS_MMD_BEGIN

using token = _sp_mmd_token;

class Engine;
class Token;
class Content;

class Processor;
class HtmlProcessor;
class HtmlOutputProcessor;

class LayoutDocument;
class LayoutProcessor;

struct TokenPair;
struct TokenPairEngine;

enum class MetaType {
	PlainString,
	HtmlString,
	HtmlEntity,
};

struct Extensions {
	using MetaCallback = memory::function<Pair<memory::string, MetaType>(const StringView &)>;

	enum Value : uint32_t {
		None			= 0,
		Compatibility	= 1 << 0,    //!< Markdown compatibility mode
		Complete		= 1 << 1,    //!< Create complete document
		Snippet			= 1 << 2,    //!< Create snippet only
		Smart			= 1 << 3,    //!< Enable Smart quotes
		Notes			= 1 << 4,    //!< Enable Footnotes
		NoLabels		= 1 << 5,    //!< Don't add anchors to headers, etc.
		ProcessHtml		= 1 << 6,    //!< Process Markdown inside HTML
		NoMetadata		= 1 << 7,    //!< Don't parse Metadata
		Obfuscate		= 1 << 8,    //!< Mask email addresses
		Critic			= 1 << 9,    //!< Critic Markup Support
		CriticAccept	= 1 << 10,   //!< Accept all proposed changes
		CriticReject	= 1 << 11,   //!< Reject all proposed changes
		RandomFoot		= 1 << 12,   //!< Use random numbers for footnote links
		Transclude		= 1 << 13,   //!< Perform transclusion(s)
		StapplerLayout	= 1 << 14,   //!< Use stappler layout engine
		Fake			= uint32_t(1 << 31),   //!< 31 is highest number allowed
	};

	Value flags;
	MetaCallback metaCallback = nullptr;

	Extensions(Value f) : flags(f) { }

	Extensions(const Extensions &v) : flags(v.flags), metaCallback(v.metaCallback) { }
	Extensions(Extensions &&v) : flags(v.flags), metaCallback(move(v.metaCallback)) { }

	Extensions& operator=(const Extensions &v) {
		flags = v.flags;
		metaCallback = v.metaCallback;
		return *this;
	}
	Extensions& operator=(Extensions &&v) {
		flags = v.flags;
		metaCallback = move(v.metaCallback);
		return *this;
	}

	bool hasFlag(Value v) const {
		return (flags & v) != None;
	}
};

SP_DEFINE_ENUM_AS_MASK(Extensions::Value)

extern Extensions DefaultExtensions;
extern Extensions StapplerExtensions;

/// Define smart typography languages -- first in list is default
enum class QuotesLanguage {
	English = 0,
	Dutch,
	French,
	German,
	GermanGuill,
	Swedish,
	Russian
};

NS_MMD_END

#endif /* MMD_COMMON_MMDCOMMON_H_ */
