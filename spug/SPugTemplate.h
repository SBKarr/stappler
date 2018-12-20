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

#ifndef SPUG_SPUGTEMPLATE_H_
#define SPUG_SPUGTEMPLATE_H_

#include "SPugLexer.h"

NS_SP_EXT_BEGIN(pug)

class Template : public memory::AllocPool {
public:
	enum ChunkType {
		Block,
		Text,
		OutputEscaped,
		OutputUnescaped,
		AttributeEscaped,
		AttributeUnescaped,
		AttributeList,
		Code,

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

		Include,

		ControlMixin,

		MixinCall,
	};

	struct Chunk {
		ChunkType type = Block;
		String value;
		Expression *expr = nullptr;
		size_t indent = 0;
		Vector<Chunk *> chunks;
	};

	struct Options {
		enum Flags {
			Pretty,
			StopOnError,
		};

		static Options getDefault();
		static Options getPretty();

		Options &setFlags(std::initializer_list<Flags> &&);
		Options &clearFlags(std::initializer_list<Flags> &&);

		bool hasFlag(Flags) const;

		std::bitset<toInt(Flags::StopOnError) + 1> flags;
	};

	static Template *read(const StringView &, const Options & = Options::getDefault(),
			const Function<void(const StringView &)> &err = nullptr);

	static Template *read(memory::pool_t *, const StringView &, const Options & = Options::getDefault(),
			const Function<void(const StringView &)> &err = nullptr);

	bool run(Context &, std::ostream &) const;

	void describe(std::ostream &stream, bool tokens = false) const;

protected:
	Template(memory::pool_t *, const StringView &, const Options &opts, const Function<void(const StringView &)> &err);

	bool runChunk(const Chunk &chunk, Context &, std::ostream &) const;
	bool runCase(const Chunk &chunk, Context &, std::ostream &) const;

	void pushWithPrettyFilter(memory::ostringstream &, size_t indent, std::ostream &) const;

	memory::pool_t *_pool;
	Lexer _lexer;
	Time _mtime;
	Chunk _root;
	Options _opts;

	Vector<StringView> _includes;
};

NS_SP_EXT_END(pug)

#endif /* SRC_SPUGTEMPLATE_H_ */
