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

#ifndef APPS_TEMPLATE_H_
#define APPS_TEMPLATE_H_

#include "TemplateExec.h"
#include "TemplateExpression.h"

NS_SA_EXT_BEGIN(tpl)

class File : public ReaderClassBase<char> {
public:
	enum ChunkType {
		Text,
		Output,
		Set,
		Condition,
		Variant,
		EndVariant,
		Foreach,
		Block,
		RunTemplate,
		Include,
	};

	struct Chunk {
		ChunkType type;
		String value;
		Expression expr;
		Vector<Chunk> chunks;
	};

	File(const String &file);

	void run(Exec &, Request &) const;

	time_t mtime() const { return _mtime; }
	const Chunk &root() const { return _root; }
	operator bool() const { return _mtime != 0 && !_root.chunks.empty(); }

protected:
	friend struct TemplateFileParser;

	void push(const CharReaderBase &, ChunkType);
	void parseConditional(Chunk &);
	void parsePrint(Chunk &);
	void parseForeach(Chunk &);
	void parseSet(Chunk &);

	void runChunk(const Chunk &, Exec &, Request &) const;
	void runOutputChunk(const Expression &, Exec &, Request &) const;
	void runConditionChunk(const Vector<Chunk> &, Exec &, Request &) const;
	void runLoop(const String &, const Expression &, const Vector<Chunk> &, Exec &, Request &) const;
	void runSet(const String &, const Expression &, Exec &) const;

	void runInclude(const Expression &, Exec &, Request &) const;
	void runTemplate(const Expression &, Exec &, Request &) const;

	time_t _mtime = 0;
	String _file;
	Chunk _root;
	Vector<Chunk *> _stack;
};

NS_SA_EXT_END(tpl)

#endif /* APPS_TEMPLATE_H_ */
