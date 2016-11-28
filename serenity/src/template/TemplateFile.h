/*
 * Template.h
 *
 *  Created on: 24 нояб. 2016 г.
 *      Author: sbkarr
 */

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
