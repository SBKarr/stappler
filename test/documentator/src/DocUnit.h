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

#ifndef TEST_DOCUMENTATOR_SRC_DOCUNIT_H_
#define TEST_DOCUMENTATOR_SRC_DOCUNIT_H_

#include "DocTokenizer.h"

namespace doc {

class Unit : public AllocBase {
public:
	struct File {
		String key;
		StringView content;
		Token *token;

		File(StringView, StringView, Token *);
	};

	struct Define {
		StringView name;
		Vector<Pair<File *, Token *>> refs;
		Vector<Pair<File *, Token *>> usage;

		bool isFunctional = false;
		bool isGuard = false;
		bool isExternal = false;
		Vector<StringView> args;
		StringView body;
	};

	void exclude(StringView);

	File * addFile(StringView path, StringView key);

	Pair<size_t, StringView> getLineNumber(StringView f, StringView pos);

	void onError(StringView key, StringView file, StringView pos, StringView err);

	void parseDefines();

	const Map<StringView, Define> getDefines() const;

protected:
	void parseDefinesTokens(File *, Token *);
	void parseConditionTokens(File *, Token *);

	Set<String> _excludes;
	Map<String, File> _files;
	Map<StringView, Define> _defines;
};

}

#endif /* TEST_DOCUMENTATOR_SRC_DOCUNIT_H_ */
