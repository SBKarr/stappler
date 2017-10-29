// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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

#include "Define.h"
#include "SPFilesystem.h"
#include "TemplateFile.h"
#include "Tools.h"

NS_SA_EXT_BEGIN(tpl)

struct TemplateFileParser : public AllocBase, public ReaderClassBase<char> {
	enum State {
		None,
		OpenTemplate,
		RunTemplate,
		CloseTemplate,
	} state = None;

	StackBuffer<1_KiB + 16> buffer;
	char quote = 0;
	bool backslash = false;

	void parse(File &f, const char *str, size_t len);
	void read(File &f, StringView &r);

	void readEmpty(File &f, StringView &r);
	void readTemplate(File &f, StringView &r);

	void pushString(const StringView &r);
	void pushChar(char c);
	void flush(File &f);
};

File::File(const String &ipath) {
	StringView view(ipath);
	if (view.is("virtual://")) {
		view += "virtual:/"_len;
		_root.type = Block;
		auto data = tools::VirtualFile::get(view.data());
		if (!data.empty()) {
			_mtime = 0;
			_stack.reserve(10);
			_stack.push_back(&_root);
			TemplateFileParser p;
			p.parse(*this, data.data(), data.size());
		}
	} else {
		_root.type = Block;
		auto path = filesystem::writablePath(ipath);
		_mtime = filesystem::mtime(path);
		if (_mtime) {
			_stack.reserve(10);
			_stack.push_back(&_root);
			TemplateFileParser p;
			auto file = filesystem::openForReading(path);
			io::read(file, [&] (const io::Buffer &buf) {
				p.parse(*this, (const char *)buf.data(), buf.size());
			});
		}
	}
}

void File::push(const StringView &ir, ChunkType type) {
	if (type == Text) {
		_stack.back()->chunks.emplace_back(Chunk{Text, ir.str()});
	} else {
		StringView r(ir);
		r.skipChars<Group<CharGroupId::WhiteSpace>>();
		if (r.is('=')) {
			++ r;
			_stack.back()->chunks.emplace_back(Chunk{Output, r.str()});
			parsePrint(_stack.back()->chunks.back());
		} else if (r.is("print ")) {
			r.skipString("print ");
			_stack.back()->chunks.emplace_back(Chunk{Output, r.str()});
			parsePrint(_stack.back()->chunks.back());
		} else if (r.is("for ")) {
			r.skipString("for ");
			_stack.back()->chunks.emplace_back(Chunk{Foreach, r.str()});
			_stack.emplace_back(&_stack.back()->chunks.back());
			parseForeach(*_stack.back());
		} else if (r.is("endfor")) {
			while (!_stack.empty() && _stack.back()->type != Foreach) {
				_stack.pop_back();
			}
			if (!_stack.empty() && _stack.back()->type == Foreach) {
				_stack.pop_back();
			}
		} else if (r.is("if ")) {
			r.skipString("if ");
			_stack.back()->chunks.emplace_back(Chunk{Condition, String()});
			_stack.emplace_back(&_stack.back()->chunks.back());

			_stack.back()->chunks.emplace_back(Chunk{Variant, r.str()});
			_stack.emplace_back(&_stack.back()->chunks.back());
			parseConditional(*_stack.back());
		} else if (r.is("elseif ") || r.is("else if ")) {
			if (r.is("elseif ")) { r.skipString("elseif "); }
			else if (r.is("else if ")) { r.skipString("else if "); }

			if (_stack.size() >= 2 && _stack.back()->type == Variant) {
				_stack.pop_back();
				_stack.back()->chunks.emplace_back(Chunk{Variant, r.str()});
				_stack.emplace_back(&_stack.back()->chunks.back());
				parseConditional(*_stack.back());
			}
		} else if (r.is("else")) {
			if (_stack.size() >= 2 && _stack.back()->type == Variant) {
				_stack.pop_back();
				_stack.back()->chunks.emplace_back(Chunk{EndVariant, String("true")});
				_stack.emplace_back(&_stack.back()->chunks.back());
			}
		} else if (r.is("endif")) {
			if (_stack.size() >= 2 && _stack.back()->type == Variant) {
				_stack.pop_back();
				_stack.pop_back();
			}
		} else if (r.is("end")) {
			if (_stack.size() >= 2 && (_stack.back()->type == Variant || _stack.back()->type == EndVariant)) {
				_stack.pop_back();
				_stack.pop_back();
			} else if (!_stack.empty() && _stack.back()->type == Foreach) {
				_stack.pop_back();
			}
		} else if (r.is("set ")) {
			r.skipString("set ");
			_stack.back()->chunks.emplace_back(Chunk{Set, r.str()});
			parseSet(_stack.back()->chunks.back());
		} else if (r.is("template ")) {
			r.skipString("template ");
			_stack.back()->chunks.emplace_back(Chunk{RunTemplate, r.str()});
			parsePrint(_stack.back()->chunks.back());
		} else if (r.is("include ")) {
			r.skipString("include ");
			_stack.back()->chunks.emplace_back(Chunk{Include, r.str()});
			parsePrint(_stack.back()->chunks.back());
		}
	}
}

void File::parseConditional(Chunk &variant) {
	if (!variant.expr.parse(variant.value)) {
		variant.expr.root = nullptr;
	}
}

void File::parsePrint(Chunk &variant) {
	if (!variant.expr.parse(variant.value)) {
		variant.expr.root = nullptr;
	}
}

void File::parseForeach(Chunk &variant) {
	StringView r1(variant.value); r1.skipUntil<Chars<':'>>();
	if (r1.is(':')) {
		++ r1;
	}

	StringView r2(variant.value); r2.skipUntilString(" in ");
	if (r2.is(" in ")) {
		r2 += " in "_len;
	}

	auto r = (r1.size() > r2.size())?r1:r2;
	if (!r.empty()) {
		if (!variant.expr.parse(r)) {
			variant.expr.root = nullptr;
		}
	}
}

void File::parseSet(Chunk &variant) {
	StringView r(variant.value);
	r.skipChars<Group<GroupId::WhiteSpace>>();
	if (r.is('$')) {
		++ r;
		r.skipChars<Group<GroupId::Alphanumeric>, Chars<'_'>>();
		r.skipChars<Group<GroupId::WhiteSpace>>();
		if (!variant.expr.parse(r)) {
			variant.expr.root = nullptr;
		}
	}
}

void File::run(Exec &exec, Request &rctx) const {
	runChunk(_root, exec, rctx);
}

void File::runChunk(const Chunk &chunk, Exec &exec, Request &req) const {
	switch (chunk.type) {
	case Text: req << chunk.value; break;
	case Output: runOutputChunk(chunk.expr, exec, req); break;
	case Set: runSet(chunk.value, chunk.expr, exec); break;
	case Condition: runConditionChunk(chunk.chunks, exec, req); break;
	case Foreach:
		runLoop(chunk.value, chunk.expr, chunk.chunks, exec, req);
		break;
	case Block:
		for (auto &it : chunk.chunks) {
			runChunk(it, exec, req);
		}
		break;
	case RunTemplate: runTemplate(chunk.expr, exec, req); break;
	case Include: runInclude(chunk.expr, exec, req); break;
	default: break;
	}
}

template <char C>
static void pushQuotedString(StringView &r, Request &req) {
	++ r;
	while (!r.empty()) {
		auto str = r.readUntil<StringView::Chars<C, '\\'>>();
		if (r.is(C)) {
			req << str;
			++ r;
			break;
		} else if (r.is('\\')) {
			++ r;
			if (!r.empty()) {
				if (r.is('n')) {
					req << ('\n');
				} else if (r.is('t')) {
					req << ('\t');
				} else if (r.is('r')) {
					req << ('\r');
				} else {
					req << (r.front());
				}
				++ r;
			}
		}

	}
}

void File::runOutputChunk(const Expression &expr, Exec &exec, Request &req) const {
	exec.print(expr, req);
}

void File::runConditionChunk(const Vector<Chunk> &chunks, Exec &exec, Request &req) const {
	for (auto &it : chunks) {
		if (it.type == Variant) {
			auto var = exec.exec(it.expr);
			if (var.value && (((var.value->isDictionary() || var.value->isArray()) && var.value->size() > 0) || var.value->asBool())) {
				for (auto &c_it : it.chunks) {
					runChunk(c_it, exec, req);
				}
				return;
			}
		} else if (it.type == EndVariant) {
			for (auto &c_it : it.chunks) {
				runChunk(c_it, exec, req);
			}
			return;
		}
	}
}

void File::runLoop(const String &value, const Expression &expr, const Vector<Chunk> &chunks, Exec &exec, Request &req) const {
	if (!expr.root) {
		return;
	}

	StringView r(value);

	String first;
	String second;
	Exec::Variable source;

	r.skipChars<Group<GroupId::WhiteSpace>>();
	if (r.is('$')) {
		++ r;
		auto tmp = r.readChars<Group<GroupId::Alphanumeric>, Chars<'_'>>();
		first.assign_weak(tmp.data(), tmp.size());
		r.skipChars<Group<GroupId::WhiteSpace>>();
		if (r.is(',')) {
			++ r;
			r.skipChars<Group<GroupId::WhiteSpace>>();
			if (r.is('$')) {
				++ r;
				tmp = r.readChars<Group<GroupId::Alphanumeric>, Chars<'_'>>();
				second.assign_weak(tmp.data(), tmp.size());
			}
		}
	}

	source = exec.exec(expr);
	if (!source.value || !(source.value->isArray() || source.value->isDictionary()) || first.empty()) {
		return;
	}

	if (source.value->isArray()) {
		auto &arr = source.value->getArray();
		data::Value iter(0);
		if (!second.empty()) {
			exec.set(first, &iter);
		}
		for (auto it = arr.begin(); it != arr.end(); ++ it) {
			auto idx = it - arr.begin();
			if (second.empty()) {
				exec.set(first, &(*it), source.scheme);
			} else {
				iter.setInteger(idx);
				exec.set(second, &(*it), source.scheme);
			}

			for (auto &c_it : chunks) {
				runChunk(c_it, exec, req);
			}
		}
	} else {
		auto &dict = source.value->getDict();
		data::Value iter;
		if (!second.empty()) {
			exec.set(first, &iter);
		}
		for (auto it = dict.begin(); it != dict.end(); ++ it) {
			if (second.empty()) {
				exec.set(first, &it->second);
			} else {
				iter.setString(it->first);
				exec.set(second, &it->second);
			}

			for (auto &c_it : chunks) {
				runChunk(c_it, exec, req);
			}
		}
	}
}

void File::runSet(const String &value, const Expression &expr, Exec &exec) const {
	StringView r(value);
	r.skipChars<Group<GroupId::WhiteSpace>>();
	if (r.is('$')) {
		++ r;
		auto tmp = r.readChars<Group<GroupId::Alphanumeric>, Chars<'_'>>();
		if (!tmp.empty()) {
			auto var = exec.exec(expr);
			if (var.value){
				exec.set(String::make_weak(tmp.data(), tmp.size()), var.value, var.scheme);
			}
		}
	}
}

void File::runInclude(const Expression &expr, Exec &exec, Request &req) const {
	auto &func = exec.getIncludeCallback();
	auto var = exec.exec(expr);
	if (func && var.value && var.value->isString()) {
		func(var.value->getString(), exec, req);
	}
}

void File::runTemplate(const Expression &expr, Exec &exec, Request &req) const {
	auto &func = exec.getTemplateCallback();
	auto var = exec.exec(expr);
	if (func && var.value && var.value->isString()) {
		func(var.value->getString(), exec, req);
	}
}

void TemplateFileParser::parse(File &chunk, const char *str, size_t len) {
	StringView r(str, len);
	while (!r.empty()) {
		read(chunk, r);
	}
	if (state == None || state == OpenTemplate) {
		flush(chunk);
	}
}

void TemplateFileParser::read(File &chunk, StringView &r) {
	switch (state) {
	case None:
		readEmpty(chunk, r);
		break;
	case OpenTemplate:
		if (r.is('?')) {
			state = RunTemplate;
			++ r;
			flush(chunk);
		} else {
			pushChar('{');
			state = None;
			readEmpty(chunk, r);
		}
		break;
	case RunTemplate:
		readTemplate(chunk, r);
		break;
	case CloseTemplate:
		if (r.is('}')) {
			++ r;
			flush(chunk);
			state = None;
		} else {
			pushChar('?');
			state = RunTemplate;
		}
		break;
	}
}

void TemplateFileParser::readEmpty(File &chunk, StringView &r) {
	auto tmp = r.readUntil<Chars<'{'>>();
	pushString(tmp);
	if (r.is('{')) {
		++ r;
		state = OpenTemplate;
	}
}

void TemplateFileParser::readTemplate(File &chunk, StringView &r) {
	if (backslash) {
		pushChar('\\');
		pushChar(r.front());
		++ r;
		backslash = false;
	} else if (quote == 0) {
		auto tmp = r.readUntil<Chars<'?', '\'', '"'>>();
		pushString(tmp);
		if (r.is('\'')) {
			pushChar('\'');
			quote = '\'';
			++ r;
		} else if (r.is('"')) {
			pushChar('"');
			quote = '"';
			++ r;
		} else if (r.is('?')) {
			state = CloseTemplate;
			++ r;
		}
	} else if (quote == '\'') {
		auto tmp = r.readUntil<Chars<'\'', '\\'>>();
		pushString(tmp);
		if (r.is('\'')) {
			pushChar('\'');
			quote = 0;
			++ r;
		} else if (r.is('\\')) {
			backslash = true;
			++ r;
		}
	} else if (quote == '"') {
		auto tmp = r.readUntil<Chars<'"', '\\'>>();
		pushString(tmp);
		if (r.is('"')) {
			pushChar('"');
			quote = 0;
			++ r;
		} else if (r.is('\\')) {
			backslash = true;
			++ r;
		}
	}
}

void TemplateFileParser::pushString(const StringView &r) {
	buffer.put((const uint8_t *)r.data(), r.size());
}

void TemplateFileParser::pushChar(char c) {
	buffer.putc((uint8_t)c);
}

void TemplateFileParser::flush(File &chunk) {
	if (state == CloseTemplate) {
		chunk.push(StringView((const char *)buffer.data(), buffer.size()), File::ChunkType::Output);
	} else {
		chunk.push(StringView((const char *)buffer.data(), buffer.size()), File::ChunkType::Text);
	}
	buffer.clear();
}

NS_SA_EXT_END(tpl)
