/**
 Copyright (c) 2018 Roman Katuntsev <sbkarr@stappler.org>

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

#include "SPugTemplate.h"

#include "SPugContext.h"
#include "SPugToken.h"

NS_SP_EXT_BEGIN(pug)

struct TemplateRender {
	template <typename T>
	using Vector = memory::PoolInterface::VectorType<T>;
	using String = memory::PoolInterface::StringType;
	using StringStream = memory::PoolInterface::StringStreamType;

	TemplateRender(Template::Chunk *root, bool pretty);

	bool renderControlToken(Token *, Template::ChunkType, bool allowEmpty);

	bool renderToken(Token *);
	bool renderTokenTree(Token *);

	bool renderComment(Token *);
	bool renderPlainText(Token *);
	bool renderLine(Token *, bool interpolated = false);
	bool renderTag(Token *, Token *nextTok, bool interpolated = false);
	Token * renderTagAttributes(Token *);

	bool makeStartIndent(bool validate);

	bool isCommand(const StringView &) const;
	bool isSelfClosing(const StringView &) const;
	bool isInlineTag(const StringView &) const;

	bool pushOutput(Expression *, Template::ChunkType);

	Template::Chunk *runCode(Token *);
	bool runCode(Expression *);

	void flushBuffer();

	bool pushChunk(Template::Chunk *);
	bool popChunk();

	Vector<StringView> &extractIncludes();

	StringStream _buffer;
	Template::Chunk *_root = nullptr;
	bool _pretty = false;
	bool _started = false;
	size_t _indentation = 0;

	Template::Chunk *_current = nullptr;

	size_t _stackSize = 0;
	std::array<Template::Chunk *, 16> _chunkStack;
	Vector<StringView> _includes;
};

TemplateRender::TemplateRender(Template::Chunk *root, bool pretty) : _root(root), _pretty(pretty), _current(root) { }

bool TemplateRender::renderControlToken(Token *tok, Template::ChunkType type, bool allowEmpty) {
	if (allowEmpty || tok->child) {
		flushBuffer();
		_current->chunks.emplace_back(new Template::Chunk{type, String(), tok->expression});
		pushChunk(_current->chunks.back());
		auto ret = renderTokenTree(tok->child);
		popChunk();
		return ret;
	}
	return false;
}

bool TemplateRender::renderToken(Token *tok) {
	switch (tok->type) {
	case Token::Root:
		renderTokenTree(tok->child);
		break;
	case Token::LineData:
		return renderLine(tok, true);
		break;
	case Token::Line:
		switch (tok->child->type) {
		case Token::LineData:
			return renderLine(tok->child);
			break;
		case Token::LinePiped:
			if (tok->prev && tok->prev->child && tok->prev->child->type == Token::LinePiped) {
				_buffer << '\n';
				if (_pretty) {
					for (size_t i = 0; i < _indentation; ++ i) {
						_buffer << '\t';
					}
				}
			}
			return renderTokenTree(tok->child->child);
			break;
		case Token::LinePlainText:
			if (_pretty || (tok->prev && (tok->prev->type == Token::LinePlainText || (
					tok->prev->child && tok->prev->child->type == Token::LinePlainText)))) {
				_buffer << '\n';
			}
			for (size_t i = 0; i < _indentation; ++ i) {
				_buffer << '\t';
			}
			if (tok->child->child) {
				renderTokenTree(tok->child->child);
			} else {
				_buffer << tok->child->data;
			}
			if (tok->child->next) {
				++ _indentation;
				auto ret = renderTokenTree(tok->child->next);
				-- _indentation;
				return ret;
			}
			return true;
			break;
		case Token::LineComment:
			return renderComment(tok->child);
			break;
		case Token::LineDot:
			return renderPlainText(tok->child);
			break;
		case Token::LineOut:
		case Token::LineCode:
			if (tok->child->child) {
				if (auto chunk = runCode(tok->child->child)) {
					pushChunk(chunk);
					renderTokenTree(tok->child->next);
					popChunk();
				}
			}
			break;
		case Token::LineCodeBlock:
			return renderTokenTree(tok->child->child);
			break;
		default:
			break;
		}
		break;
	case Token::PlainText:
		_buffer << tok->data;
		break;

	case Token::OutputEscaped: pushOutput(tok->expression, Template::OutputEscaped); break;
	case Token::OutputUnescaped: pushOutput(tok->expression, Template::OutputUnescaped); break;
	case Token::Code: runCode(tok->expression); break;

	case Token::ControlCase: return renderControlToken(tok, Template::ControlCase, false); break;
	case Token::ControlWhen: return renderControlToken(tok, Template::ControlWhen, true); break;
	case Token::ControlDefault: return renderControlToken(tok, Template::ControlDefault, false); break;

	case Token::ControlIf: return renderControlToken(tok, Template::ControlIf, false); break;
	case Token::ControlUnless: return renderControlToken(tok, Template::ControlUnless, false); break;
	case Token::ControlElseIf: return renderControlToken(tok, Template::ControlElseIf, false); break;
	case Token::ControlElse: return renderControlToken(tok, Template::ControlElse, false); break;
	case Token::ControlWhile: return renderControlToken(tok, Template::ControlWhile, false); break;

	case Token::ControlEach:
		if (tok->child && tok->child->next) {
			flushBuffer();
			auto var = tok->child->data;
			_current->chunks.emplace_back(new Template::Chunk{Template::ControlEach, String(var.data(), var.size()), tok->expression});
			pushChunk(_current->chunks.back());
			auto ret = renderTokenTree(tok->child);
			popChunk();
			return ret;
		}
		break;
	case Token::ControlEachPair:
		if (tok->child && tok->child->next && tok->child->next->next) {
			flushBuffer();
			StringStream str;
			str << tok->child->data << " " << tok->child->next->data;
			_current->chunks.emplace_back(new Template::Chunk{Template::ControlEachPair, str.str(), tok->expression});
			pushChunk(_current->chunks.back());
			auto ret = renderTokenTree(tok->child);
			popChunk();
			return ret;
		}
		break;
	case Token::Include:
		flushBuffer();
		_current->chunks.emplace_back(new Template::Chunk{Template::Include, String(tok->data.data(), tok->data.size()), nullptr, _indentation});
		_includes.emplace_back(StringView(_current->chunks.back()->value));
		return true;
		break;
	case Token::Doctype:
		if (tok->data == "html") {
			_buffer << "<!DOCTYPE html>\n";
		} else if (tok->data == "xml") {
			_buffer << "<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n";
		} else if (tok->data == "transitional") {
			_buffer << "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n";
		} else if (tok->data == "strict") {
			_buffer << "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n";
		} else if (tok->data == "frameset") {
			_buffer << "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Frameset//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-frameset.dtd\">\n";
		} else if (tok->data == "1.1") {
			_buffer << "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.1//EN" "http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\">\n";
		} else if (tok->data == "basic") {
			_buffer << "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML Basic 1.1//EN" "http://www.w3.org/TR/xhtml-basic/xhtml-basic11.dtd\">\n";
		} else if (tok->data == "mobile") {
			_buffer << "<!DOCTYPE html PUBLIC \"-//WAPFORUM//DTD XHTML Mobile 1.2//EN" "http://www.openmobilealliance.org/tech/DTD/xhtml-mobile12.dtd\">\n";
		} else if (tok->data == "plist") {
			_buffer << "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n";
		} else {
			_buffer << "<!DOCTYPE " << tok->data << ">\n";
		}
		return true;
		break;
	default: break;
	}
	return false;
}

bool TemplateRender::renderTokenTree(Token *tok) {
	bool ret = false;
	while (tok != NULL) {
		if (renderToken(tok)) {
			ret = true;
		}
		tok = tok->next;
	}
	return ret;
}

bool TemplateRender::renderComment(Token *tok) {
	switch (tok->child->type) {
	case Token::CommentHtml:
		makeStartIndent(true);
		_buffer << "<!--";
		if (tok->child->next) {
			renderTokenTree(tok->child->next);
		}
		if (tok->next) {
			if (_pretty) { ++ _indentation; }
			renderTokenTree(tok->next);
			if (_pretty) { -- _indentation; }
			if (_pretty) {
				_buffer << "\n";
				for (size_t i = 0; i < _indentation; ++ i) {
					_buffer << '\t';
				}
			}
		}
		_buffer << "-->";
		return true;
		break;
	case Token::CommentTemplate:
		break;
	default:
		break;
	}
	return false;
}

bool TemplateRender::renderPlainText(Token *tok) {
	return renderTokenTree(tok->next);
}

bool TemplateRender::renderLine(Token *tok, bool interpolated) {
	switch (tok->child->type) {
	case Token::Tag:
		if (isCommand(tok->child->data)) {
			return false;
		} else {
			if (renderTag(tok->child, interpolated ? nullptr : tok->next, interpolated)) {
				return true;
			}
		}
		break;
	default:
		break;
	}
	return false;
}

bool TemplateRender::renderTag(Token *tok, Token *nextTok, bool interpolated) {
	auto shouldIndent = interpolated ? false : makeStartIndent(!isInlineTag(tok->data));

	bool isOutput = tok->data.empty() && tok->next && tok->next->type == Token::TagTrailingEq;

	Token *tagEval = nullptr;

	if (!isOutput) {
		_buffer << "<" << (tok->data.empty() ? StringView("div") : tok->data); // it's a tag

		// read attributes
		tagEval = renderTagAttributes(tok->next);
		if ((tagEval && tagEval->type == Token::TagTrailingSlash) || isSelfClosing(tok->data)) {
			_buffer << "/>";
			return true;
		}

		_buffer << ">";
	} else {
		tagEval = tok->next->next;
	}

	if (_pretty) { ++ _indentation; }

	bool finalizeIndent = false;
	if (tagEval) {
		renderTokenTree(tagEval);
	}

	if (nextTok) {
		if (renderTokenTree(nextTok)) {
			finalizeIndent = true;
		}
	}

	if (_pretty) { -- _indentation; }

	if (shouldIndent && finalizeIndent) {
		_buffer << "\n";
		for (size_t i = 0; i < _indentation; ++ i) {
			_buffer << '\t';
		}
	}

	if (!isOutput) {
		_buffer << "</" << (tok->data.empty() ? StringView("div") : tok->data) << ">";
	}
	return shouldIndent;
}

Token * TemplateRender::renderTagAttributes(Token *tok) {
	bool hasClasses = false;
	StringStream classes;
	StringView id;

	auto writeAttrName = [] (Token *tok) -> String {
		StringStream out;
		while (tok && tok->type == Token::PlainText) {
			out << tok->data;
			tok = tok->next;
		}
		return out.str();
	};

	auto pushAttribute = [&] (const StringView &name, Expression *expression, bool esc) {
		if (!expression) {
			_buffer << " " << name;
			return;
		}

		if (expression->isConst()) {
			Context::printAttrVar(name, *expression, _buffer, esc);
		} else {
			flushBuffer();
			_current->chunks.emplace_back(new Template::Chunk{esc ? Template::AttributeEscaped : Template::AttributeUnescaped,
					String(name.data(), name.size()), expression});
		}
	};

	auto processAttrList = [&] (Token *tok) {
		while (tok && (tok->type == Token::AttrPairEscaped || tok->type == Token::AttrPairUnescaped)) {
			if (auto nameTok = tok->child) {
				auto name = writeAttrName(nameTok->child);
				if (!name.empty()) {
					if (auto valueTok = nameTok->next) {
						if (valueTok->expression) {
							pushAttribute(name, valueTok->expression, tok->type == Token::AttrPairEscaped);
						}
					} else {
						pushAttribute(name, nullptr, tok->type == Token::AttrPairEscaped);
					}
				}
			}

			tok = tok->next;
		}
	};

	auto processAttrExpr = [&] (Expression *expr) {
		if (expr) {
			if (expr->isConst()) {
				Context::printAttrExpr(*expr, _buffer);
			} else {
				flushBuffer();
				_current->chunks.emplace_back(new Template::Chunk{Template::AttributeList,
						String(), expr});
			}
		}
	};

	bool stop = false;
	while (!stop && tok) {
		switch (tok->type) {
		case Token::TagClassNote:
			if (!tok->data.empty()) {
				if (hasClasses) { classes << " "; } else { hasClasses = true; }
				classes << tok->data;
			}
			break;
		case Token::TagIdNote:
			id = tok->data;
			break;
		case Token::TagAttrList:
			processAttrList(tok->child);
			break;
		case Token::TagAttrExpr:
			processAttrExpr(tok->expression);
			break;
		default:
			stop = true;
			break;
		}

		if (!stop) {
			tok = tok->next;
		}
	}

	if (!id.empty()) {
		_buffer << " id=\"" << id << "\"";
	}
	if (hasClasses) {
		_buffer << " class=\"" << classes.weak() << "\"";
	}

	return tok;
}

bool TemplateRender::makeStartIndent(bool validate) {
	bool shouldIndent = _pretty && validate;
	if (shouldIndent && _started) {
		_buffer << "\n";
		for (size_t i = 0; i < _indentation; ++ i) {
			_buffer << '\t';
		}
	} else {
		_started = true;
	}
	return shouldIndent;
}

bool TemplateRender::isCommand(const StringView &r) const {
	return false;
}

bool TemplateRender::isSelfClosing(const StringView &r) const {
	if (r == "area" || r == "base" || r == "br" || r == "col"
		|| r == "command" || r == "embed" || r == "hr" || r == "img"
		|| r == "input" || r == "keygen" || r == "link" || r == "meta"
		|| r == "param" || r == "source" || r == "track" || r == "wbr") {
		return true;
	}
	return false;
}

bool TemplateRender::isInlineTag(const StringView &r) const {
	if (r == "a" || r == "abbr" || r == "acronym" || r == "b"
		|| r == "br" || r == "code" || r == "em" || r == "font"
		|| r == "i" || r == "img" || r == "ins" || r == "kbd"
		|| r == "map" || r == "samp" || r == "small" || r == "span"
		|| r == "strong" || r == "sub" || r == "sup") {
		return true;
	}
	return false;
}

bool TemplateRender::pushOutput(Expression *expr, Template::ChunkType type) {
	if (!expr) {
		return false;
	}

	if (expr->isConst()) {
		return Context::printConstExpr(*expr, _buffer, type == Template::OutputEscaped);
	} else {
		flushBuffer();
		_current->chunks.emplace_back(new Template::Chunk{type, String(), expr});
	}
	return false;
}

Template::Chunk *TemplateRender::runCode(Token *tok) {
	Template::Chunk *ret = nullptr;
	while (tok) {
		if (runCode(tok->expression)) {
			ret = _current->chunks.back();
		}
		tok = tok->next;
	}
	return ret;
}

bool TemplateRender::runCode(Expression *expr) {
	if (!expr) {
		return false;
	}

	flushBuffer();
	_current->chunks.emplace_back(new Template::Chunk{Template::Code, String(), expr});
	return true;
}

void TemplateRender::flushBuffer() {
	if (!_buffer.empty() && _current) {
		_current->chunks.emplace_back(new Template::Chunk{Template::Text, _buffer.str(), nullptr});
		_buffer.clear();
	}
}

bool TemplateRender::pushChunk(Template::Chunk *c) {
	if (_stackSize == _chunkStack.size()) {
		return false;
	}

	flushBuffer();
	_chunkStack[_stackSize] = _current;
	++ _stackSize;
	_current = c;
	return true;
}

bool TemplateRender::popChunk() {
	if (_stackSize > 0) {
		flushBuffer();
		-- _stackSize;
		_current = _chunkStack[_stackSize];
		return true;
	}
	return false;
}

TemplateRender::Vector<StringView> &TemplateRender::extractIncludes() {
	return _includes;
}

Template::Options Template::Options::getDefault() {
	return Options();
}

Template::Options Template::Options::getPretty() {
	return Options().setFlags({ Pretty });
}

Template::Options &Template::Options::setFlags(std::initializer_list<Flags> &&il) {
	for (auto &it : il) { flags.set(toInt(it)); }
	return *this;
}

Template::Options &Template::Options::clearFlags(std::initializer_list<Flags> &&il) {
	for (auto &it : il) { flags.reset(toInt(it)); }
	return *this;
}

bool Template::Options::hasFlag(Flags f) const {
	return flags.test(toInt(f));
}

Template *Template::read(const StringView &str, const Options &opts, const Function<void(const StringView &)> &err) {
	auto p = memory::pool::create(memory::pool::acquire());
	return read(p, str, opts, err);
}

Template *Template::read(memory::pool_t *p, const StringView &str, const Options &opts, const Function<void(const StringView &)> &err) {
	memory::pool::push(p);
	auto ret = new (p) Template(p, str, opts, err);
	memory::pool::pop();
	return ret;
}

Template::Template(memory::pool_t *p, const StringView &str, const Options &opts, const Function<void(const StringView &)> &err)
: _pool(p), _lexer(str, err), _opts(opts) {
	if (_lexer) {
		TemplateRender renderer(&_root, opts.hasFlag(Options::Pretty));
		renderer.renderToken(&_lexer.root);
		renderer.flushBuffer();
		_includes = move(renderer.extractIncludes());
	}
}

bool Template::run(Context &ctx, std::ostream &out) const {
	return runChunk(_root, ctx, out);
}

static void Template_describeChunk(std::ostream &stream, const Template::Chunk &chunk, size_t depth) {
	for (size_t i = 0; i < depth; ++ i) { stream << "  "; }
	switch (chunk.type) {
	case Template::Block:
		stream << "<block> of " << chunk.chunks.size() << "\n";
		break;
	case Template::Text: stream << "<text>\n"; break;
	case Template::OutputEscaped: stream << "<escaped output expression>\n"; break;
	case Template::OutputUnescaped: stream << "<unescaped output expression>\n"; break;
	case Template::AttributeEscaped: stream << "<escaped attribute expression>\n"; break;
	case Template::AttributeUnescaped: stream << "<unescaped attribute expression>\n"; break;
	case Template::AttributeList: stream << "<attribute list>\n"; break;
	case Template::Code:
		stream << "<code>";
		if (auto n = chunk.chunks.size()) {
			stream << " of " << n;
		}
		stream << "\n";
		break;
	case Template::ControlCase: stream << "<case>\n"; break;
	case Template::ControlWhen: stream << "<when>\n"; break;
	case Template::ControlDefault: stream << "<default>\n"; break;
	case Template::ControlIf: stream << "<if>\n"; break;
	case Template::ControlUnless: stream << "<unless>\n"; break;
	case Template::ControlElse: stream << "<else>\n"; break;
	case Template::ControlElseIf: stream << "<elseif>\n"; break;

	case Template::ControlEach: stream << "<each> " << chunk.value << "\n"; break;
	case Template::ControlEachPair: stream << "<each> " << chunk.value << "\n"; break;
	case Template::ControlWhile: stream << "<while>\n"; break;
	case Template::Include: stream << "<include>" << chunk.value << "\n"; break;
	}
	for (auto &it : chunk.chunks) {
		Template_describeChunk(stream, *it, depth + 1);
	}
}

void Template::describe(std::ostream &stream, bool tokens) const {
	stream << "\n";
	if (tokens) {
		stream << "Tokens:\n";
		_lexer.root.describe(stream);
		stream << "\n";
	}
	Template_describeChunk(stream, _root, 0);
	stream << "\n";
}

bool Template::runChunk(const Chunk &chunk, Context &exec, std::ostream &out) const {
	auto runIf = [&] (auto &it) -> bool {
		Context::VarScope scope;
		exec.pushVarScope(scope);

		bool success = false;
		bool r = true;
		bool allowElseIf = (*it)->type == ControlIf;

		auto tryExec = [&] () -> bool {
			if (auto var = exec.exec(*(*it)->expr, out)) {
				auto val = var.readValue().asBool();
				if ((!allowElseIf && !val) || val) {
					if (!runChunk(**it, exec, out)) {
						r = false;
					}
					return true;
				} else {
					++ it;
				}
			} else {
				++ it;
				r = false;
			}
			return false;
		};


		if ((*it)->type == ControlIf || (*it)->type == ControlUnless) {
			if (tryExec()) {
				success = true;
				++ it;
			}
		}

		if (!success && allowElseIf) {
			while (it != chunk.chunks.end() && (*it)->type == ControlElseIf) {
				if (tryExec()) {
					success = true;
					++ it;
					break;
				}
			}
		}

		if (!success && it != chunk.chunks.end() && (*it)->type == ControlElse) {
			if (!runChunk(**it, exec, out)) {
				success = true;
				r = false;
			}
			++ it;
		}

		while (it != chunk.chunks.end() && ((*it)->type == ControlElse || (allowElseIf && (*it)->type == ControlElseIf))) {
			++ it;
		}

		exec.popVarScope();

		return r;
	};

	auto runEachBody = [&] (auto &it, const auto &cb) -> bool {
		bool r = true;
		Context::VarScope scope;
		exec.pushVarScope(scope);

		auto next = it + 1;
		bool hasElse = (next != chunk.chunks.end() && (*next)->type == ControlElse);
		bool runElse = false;

		if (auto var = exec.exec(*(*it)->expr, out)) {
			auto runWithVar = [&] (const Value &val, bool isConst) -> bool {
				if (val.isArray()) {
					size_t i = 0;
					if (val.size() > 0) {
						for (auto &v_it : val.asArray()) {
							cb(Value(i), &v_it, isConst);
							if (!runChunk(**it, exec, out) && _opts.hasFlag(Options::StopOnError)) {
								return false;
							}
							++ i;
						}
					} else {
						runElse = true;
					}
				} else if (val.isDictionary()) {
					if (val.size() > 0) {
						for (auto &v_it : val.asDict()) {
							cb(Value(v_it.first), &v_it.second, isConst);
							if (!runChunk(**it, exec, out) && _opts.hasFlag(Options::StopOnError)) {
								return false;
							}
						}
					} else {
						runElse = true;
					}
				} else {
					if (!hasElse) {
						cb(Value(0), &val, isConst);
						if (!runChunk(**it, exec, out) && _opts.hasFlag(Options::StopOnError)) {
							return false;
						}
					} else {
						runElse = true;
					}
				}
				return true;
			};

			if (Value * mut = var.getMutable()) {
				r = runWithVar(*mut, false);
			} else if (const Value &rv = var.readValue()) {
				r = runWithVar(rv, true);
			}
		} else {
			r = false;
			runElse = true;
		}

		if (hasElse) {
			++ it;
			if (runElse) {
				if (!runChunk(**it, exec, out) && _opts.hasFlag(Options::StopOnError)) {
					return false;
				}
			}
		}

		exec.popVarScope();
		++ it;
		return r;
	};

	auto runEach = [&] (auto &it) -> bool {
		StringView varName((*it)->value);
		if (varName.empty()) {
			return false;
		}

		return runEachBody(it, [&] (Value &&it, const Value *val, bool isConst) {
			exec.set(varName, isConst, val);
		});
	};

	auto runEachPair = [&] (auto &it) -> bool {
		StringView varFirst;
		StringView varSecond;

		string::split((*it)->value, " ", [&] (const StringView &val) {
			if (varFirst.empty()) {
				varFirst = val;
			} else {
				varSecond = val;
			}
		});

		if (varFirst.empty() || varSecond.empty()) {
			return false;
		}

		return runEachBody(it, [&] (Value &&it, const Value *val, bool isConst) {
			exec.set(varFirst, isConst, val);
			exec.set(varSecond, move(it));
		});
	};

	auto runWhile = [&] (const Chunk &ch) {
		while (true) {
			if (auto var = exec.exec(*ch.expr, out)) {
				if (var.readValue().asBool()) {
					if (!runChunk(ch, exec, out)) {
						return false;
					}
				} else {
					return true;
				}
			} else {
				break;
			}
		}
		return false;
	};

	auto it = chunk.chunks.begin();
	while (it != chunk.chunks.end()) {
		auto &c = **it;
		switch (c.type) {
		case Text:
			out << c.value;
			++ it;
			break;
		case OutputEscaped:
		case OutputUnescaped:
			if (!exec.print(*c.expr, out, c.type == OutputEscaped) && _opts.hasFlag(Options::StopOnError)) {
				return false;
			}
			++ it;
			break;
		case AttributeEscaped:
		case AttributeUnescaped:
			if (!exec.printAttr(c.value, *c.expr, out, c.type == AttributeEscaped) && _opts.hasFlag(Options::StopOnError)) {
				return false;
			}
			++ it;
			break;
		case AttributeList:
			if (!exec.printAttrExprList(*c.expr, out) && _opts.hasFlag(Options::StopOnError)) {
				return false;
			}
			++ it;
			break;
		case Block:
		case ControlWhen:
		case ControlDefault:
			runChunk(c, exec, out);
			++ it;
			break;
		case Code:
			if (!exec.exec(*c.expr, out) && _opts.hasFlag(Options::StopOnError)) {
				return false;
			}
			++ it;
			break;
		case ControlCase:
			if (!runCase(c, exec, out) && _opts.hasFlag(Options::StopOnError)) {
				return false;
			}
			++ it;
			break;
		case ControlIf:
			if (!runIf(it) && _opts.hasFlag(Options::StopOnError)) {
				return false;
			}
			break;
		case ControlUnless:
			if (!runIf(it) && _opts.hasFlag(Options::StopOnError)) {
				return false;
			}
			break;
		case ControlEach:
			if (!runEach(it) && _opts.hasFlag(Options::StopOnError)) {
				return false;
			}
			break;
		case ControlEachPair:
			if (!runEachPair(it) && _opts.hasFlag(Options::StopOnError)) {
				return false;
			}
			break;
		case ControlWhile:
			if (!runWhile(**it) && _opts.hasFlag(Options::StopOnError)) {
				return false;
			}
			++ it;
			break;
		case Include:
			if (_opts.hasFlag(Options::Pretty)) {
				memory::ostringstream stream;
				if (!exec.runInclude((*it)->value, stream, this) && _opts.hasFlag(Options::StopOnError)) {
					return false;
				}
				pushWithPrettyFilter(stream, (*it)->indent, out);
			} else {
				if (!exec.runInclude((*it)->value, out, this) && _opts.hasFlag(Options::StopOnError)) {
					return false;
				}
			}
			++ it;
			break;
		default:
			return false;
			break;
		}
	}

	return true;
}

bool Template::runCase(const Chunk &chunk, Context &exec, std::ostream &out) const {
	auto runWhenChunk = [&] (auto it) -> bool {
		if ((*it)->chunks.size() > 0) {
			return runChunk(**it, exec, out);
		} else {
			++ it;
			while (it != chunk.chunks.end() && (*it)->type == Template::ControlWhen) {
				if ((*it)->chunks.size() > 0) {
					return runChunk(**it, exec, out);
				}
			}
		}
		return false;
	};

	auto perform = [&] () -> bool {
		if (auto var = exec.exec(*chunk.expr, out)) {
			if (auto val = var.readValue()) {
				const Template::Chunk *def = nullptr;
				auto it = chunk.chunks.begin();
				while (it != chunk.chunks.end()) {
					switch ((*it)->type) {
					case Template::ControlWhen:
						if (auto v = exec.exec(*(*it)->expr, out)) {
							auto &v2 = v.readValue();
							if (val == v2) {
								return runWhenChunk(it);
							}
						} else {
							return false;
						}
						break;
					case Template::ControlDefault: def = *it; break;
					default: break;
					}
					++ it;
				}
				if (def) {
					return runChunk(*def, exec, out);
				} else {
					return true;
				}
			}
		}
		return false;
	};

	Context::VarScope scope;
	exec.pushVarScope(scope);
	auto ret = perform();
	exec.popVarScope();
	return ret;
}

void Template::pushWithPrettyFilter(memory::ostringstream &str, size_t indent, std::ostream &out) const {
	StringView r(str.weak());
	out << '\n';
	while (!r.empty()) {
		for (size_t i = 0; i < indent; ++ i) {
			out << '\t';
		}
		out << r.readUntil<StringView::Chars<'\r', '\n'>>();
		out << r.readChars<StringView::Chars<'\r'>>();
		if (r.is('\n')) {
			out << '\n';
			++ r;
		}
	}
}

NS_SP_EXT_END(pug)
