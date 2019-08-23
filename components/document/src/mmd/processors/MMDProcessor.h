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

#ifndef MMD_PROCESSORS_MMDPROCESSOR_H_
#define MMD_PROCESSORS_MMDPROCESSOR_H_

#include "MMDContent.h"

NS_MMD_BEGIN

constexpr int kMaxExportRecursiveDepth = 1000; //!< Maximum recursion depth when exporting token tree -- to prevent stack overflow with "pathologic" input
constexpr int kMaxTableColumns = 48; //!< Maximum number of table columns for specifying alignment

class Processor : public memory::AllocPool {
public:
	template <typename T>
	using Dict = Content::Dict<T>;

	template <typename T>
	using Vector = Content::Vector<T>;

	using String = Content::String;
	using StringStream = Content::StringStream;

	Processor() { }
	virtual ~Processor() { }

	virtual void process(const Content &, const StringView &, const Token &);

protected:
	StringView localize(const StringView &);

	virtual void processMetaDict(const Dict<Content::String> &);
	virtual void processMeta(const StringView &, const StringView &);

	void printTokenRaw(std::ostream & out, token * t);
	void printTokenTreeRaw(std::ostream & out, token * t);

	void readTableColumnAlignments(token * table);
	Content::Link * parseBrackets(token * bracket, int16_t * skip_token);
	Content::Footnote *parseAbbrBracket(token * t);
	Content::Footnote *parseCitationBracket(token * t);
	Content::Footnote *parseFootnoteBracket(token * t);
	Content::Footnote *parseGlossaryBracket(token * t);

	static StringView printToken(const StringView & source, token * t);
	static StringView getFenceLanguageSpecifier(const StringView & source, token * fence);

	/// Determine whether raw filter matches specified format
	static bool rawFilterMatches(token * t, const StringView & source, const char *filter);
	static bool rawFilterTextMatches(const StringView & pattern, const char *filter);

	static uint8_t rawLevelForHeader(token * header);
	static Content::String labelFromHeader(const StringView & source, token * t);

	static bool isImageFigure(token *t);

	const Content *content = nullptr;
	StringView source;
	QuotesLanguage quotes_lang = QuotesLanguage::English;

	int16_t padded = 2;
	int16_t skip_token = 0;
	bool list_is_tight = false;
	bool close_para = true;

	int16_t recurse_depth = 0;
	uint8_t base_header_level = 1;
	uint8_t odf_para_type = 0;
	int random_seed_base = 0;

	int16_t footnote_para_counter = -1;
	int16_t footnote_being_printed = 0;
	int16_t citation_being_printed = 0;
	int16_t glossary_being_printed = 0;

	bool in_table_header = false;
	int16_t table_column_count = 0;
	int16_t table_cell_count = 0;
	uint8_t table_alignment[kMaxTableColumns];

	Vector<Content::Footnote *> used_abbreviations;
	Vector<Content::Footnote *> used_footnotes;
	Vector<Content::Footnote *> used_citations;
	Vector<Content::Footnote *> used_glossaries;
};

NS_MMD_END

#endif /* MMD_PROCESSORS_MMDPROCESSOR_H_ */
