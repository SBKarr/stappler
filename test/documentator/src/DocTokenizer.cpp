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

#include "SPCommon.h"
#include "DocTokenizer.h"

namespace doc {

Token::Token(Class c, StringView t, Token *r, size_t idx) : tokClass(c), token(t), root(r), idx(idx) { }

void Token::describe(std::ostream &str, size_t depth) const {
	for (size_t i = 0; i < depth; ++ i) {
		str << "  ";
	}

	switch (tokClass) {
	case None: str << "None"; break;
	case File: str << "File"; break;
	case MultiLineComment: str << "MultiLineComment"; break;
	case LineComment: str << "LineComment: '" << token << "'"; break;
	case Preprocessor: str << "Preprocessor: '" << token << "'"; break;
	case PreprocessorCondition: str << "PreprocessorCondition: '" << token << "'"; break;
	case CharLiteral: str << "CharLiteral: '" << token << "'"; break;
	case StringLiteral: str << "StringLiteral: '" << token << "'"; break;
	case RawStringLiteral: str << "RawStringLiteral"; break;
	case Keyword: str << "Keyword: '" << token << "'"; break;
	case Word: str << "Word: '" << token << "'"; break;
	case NumWord: str << "NumWord: '" << token << "'"; break;
	case Operator: str << "Operator: '" << token << "'"; break;
	case ParentesisBlock: str << "ParentesisBlock"; break;
	case OperatorBlock: str << "OperatorBlock"; break;
	case CompositionBlock: str << "CompositionBlock"; break;
	}

	str << "\n";

	for (auto &it : tokens) {
		it->describe(str, depth + 1);
	}
}

void Token::makeFlow() {
	for (size_t i = 0; i < tokens.size(); ++ i) {
		if (i > 0) {
			tokens[i]->prev = tokens[i - 1];
		}
		if (i < tokens.size() - 1) {
			tokens[i]->next = tokens[i + 1];
		}

		tokens[i]->makeFlow();
	}
}

static const char *s_keywords[] = {
	"alignas",
	"alignof",
	"and",
	"and_eq",
	"asm",
	"atomic_cancel",
	"atomic_commit",
	"atomic_noexcept",
	"auto",
	"bitand",
	"bitor",
	"bool",
	"break",
	"case",
	"catch",
	"char",
	"char8_t",
	"char16_t",
	"char32_t",
	"class",
	"compl",
	"concept",
	"const",
	"consteval",
	"constexpr",
	"constinit",
	"const_cast",
	"continue",
	"co_await",
	"co_return",
	"co_yield",
	"decltype",
	"default",
	"delete",
	"do",
	"double",
	"dynamic_cast",
	"else",
	"enum",
	"explicit",
	"export",
	"extern",
	"false",
	"float",
	"for",
	"friend",
	"goto",
	"if",
	"inline",
	"int",
	"long",
	"mutable",
	"namespace",
	"new",
	"noexcept",
	"not",
	"not_eq",
	"nullptr",
	"operator",
	"or",
	"or_eq",
	"private",
	"protected",
	"public",
	"reflexpr",
	"register",
	"reinterpret_cast",
	"requires",
	"return",
	"short",
	"signed",
	"sizeof",
	"static",
	"static_assert",
	"static_cast",
	"struct",
	"switch",
	"synchronized",
	"template",
	"this",
	"thread_local",
	"throw",
	"true",
	"try",
	"typedef",
	"typeid",
	"typename",
	"union",
	"unsigned",
	"using",
	"virtual",
	"void",
	"volatile",
	"wchar_t",
	"while",
	"xor",
	"xor_eq",
	nullptr
};

static bool isKeyword(StringView r) {
	auto p = s_keywords;
	while (*p != nullptr) {
		if (StringView(*p) == r) {
			return true;
		}
		++ p;
	}
	return false;
}

Token * tokenize(StringView r, const Callback<void(StringView, StringView)> &errCb) {
	if (r.empty()) {
		return nullptr;
	}

	size_t idx = 0;
	Token *tok = new Token(Token::File, r, nullptr, 0);

	Vector<Token *> stack; stack.reserve(16);
	stack.emplace_back(tok);

	auto pushPreprocessor = [&] (StringView tok) {
		tok.trimChars<WhiteSpace>();

		auto tmp = tok;
		++ tmp;
		tmp.skipChars<WhiteSpace>();
		auto idf = stappler::string::tolower<Interface>(tmp.readChars<Identifier>());
		if (idf == "if" || idf == "ifdef" || idf == "ifndef") {
			stack.emplace_back(stack.back()->tokens.emplace_back(new Token(Token::PreprocessorCondition, tok, stack.back(), idx ++)));
		} else if (idf == "else" || idf == "elif") {
			auto it = stack.rbegin();
			while (it != stack.rend()) {
				if ((*it)->tokClass == Token::PreprocessorCondition) {
					stack.pop_back();
					break;
				} else {
					stack.pop_back();
					++ it;
				}
			}
			if (it == stack.rend()) {
				errCb(r, "Invalid preprocessor condition mark");
				r = StringView();
			}
			stack.emplace_back(stack.back()->tokens.emplace_back(new Token(Token::PreprocessorCondition, tok, stack.back(), idx ++)));
		} else if (idf == "endif") {
			auto it = stack.rbegin();
			while (it != stack.rend()) {
				if ((*it)->tokClass == Token::PreprocessorCondition) {
					stack.pop_back();
					break;
				} else {
					stack.pop_back();
					++ it;
				}
			}
			if (it == stack.rend()) {
				errCb(r, "Invalid preprocessor condition mark");
				r = StringView();
			}
			stack.back()->tokens.emplace_back(new Token(Token::Preprocessor, tok, stack.back(), idx ++));
		} else {
			stack.back()->tokens.emplace_back(new Token(Token::Preprocessor, tok, stack.back(), idx ++));
		}
	};

	while (!r.empty()) {
		r.skipChars<WhiteSpace>();
		if (r.is("//")) {
			auto tmp = r;
			auto comm = r.readUntil<NewLine>();
			while (!r.empty() && comm.size() > 0 && comm.back() == '\\') {
				r.skipChars<NewLine>();
				comm = r.readUntil<NewLine>();
			}
			stack.back()->tokens.emplace_back(new Token(Token::LineComment, StringView(tmp.data(), tmp.size() - r.size()), stack.back(), 0));
			r.skipChars<NewLine>();
		} else if (r.is("/*")) {
			auto tmp = r;
			r += "/*"_len;
			r.skipUntilString("*/", true);
			if (r.is("*/")) {
				r += "*/"_len;
				stack.back()->tokens.emplace_back(new Token(Token::MultiLineComment, StringView(tmp.data(), tmp.size() - r.size()), stack.back(), 0));
			}
		} else if (r.is("#")) {
			auto tmp = r;
			while (!r.is<NewLine>()) {
				auto prep = r.readUntil<NewLine, StringView::Chars<'/'>>();
				if (r.is<NewLine>()) {
					if (prep.size() > 0 && prep.back() == '\\') {
						r.skipChars<NewLine>();
					} else {
						pushPreprocessor(StringView(tmp.data(), tmp.size() - r.size()));
						break;
					}
				} else if (r.is('/')) {
					if (r.is("//")) {
						pushPreprocessor(StringView(tmp.data(), tmp.size() - r.size()));
						break;
					} else {
						++ r;
					}
				}
			}
			r.skipChars<NewLine>();
		} else if (r.is("\"") || r.is("L\"") || r.is("u\"") || r.is("U\"") || r.is("u8\"")
				|| r.is("R\"") || r.is("LR\"") || r.is("uR\"") || r.is("UR\"") || r.is("u8R\"")) {
			auto tmp = r;

			auto prefix = r.readUntil<StringView::Chars<'"'>>();
			if (prefix.size() > 0 && prefix.back() == 'R') {
				String delim = ")";
				auto del = r.readChars<Identifier>();
				if (!r.is('(')) {
					errCb(r, "No after delimiter '(' found for raw string literal");
					break;
				}
				if (!del.empty()) {
					delim.append(del.data(), del.size());
				}
				delim.push_back('"');

				r.skipUntilString(delim);
				if (!r.is(delim)) {
					errCb(r, "Closing delimiter for raw string not found");
					break;
				} else {
					r += delim.size();
					if (r.is('_')) {
						r.skipChars<Identifier>();
					}
					stack.back()->tokens.emplace_back(new Token(Token::RawStringLiteral, StringView(tmp.data(), tmp.size() - r.size()), stack.back(), idx ++));
				}
			} else {
				++ r;
				r.skipUntil<NewLine, StringView::Chars<'"', '\\'>>();
				while (!r.empty() && !r.is('"')) {
					if (r.is('\\')) {
						r += 2;
						r.skipUntil<NewLine, StringView::Chars<'"', '\\'>>();
					} else {
						errCb(r, "Line break within string literal");
						break;
					}
				}

				if (r.is('"')) {
					++ r;
					if (r.is('_')) {
						r.skipChars<Identifier>();
					}
					stack.back()->tokens.emplace_back(new Token(Token::StringLiteral, StringView(tmp.data(), tmp.size() - r.size()), stack.back(), idx ++));
				} else {
					errCb(r, "No closing quotes found for string literal");
					break;
				}
			}
		} else if (r.is("'") || r.is("L'") || r.is("u'") || r.is("U'") || r.is("u8'")) {
			auto tmp = r;
			r.skipUntil<StringView::Chars<'\''>>();
			if (r.is('\'')) {
				++ r;
			}
			r.skipUntil<NewLine, StringView::Chars<'\'', '\\'>>();
			while (!r.empty() && !r.is('\'')) {
				if (r.is('\\')) {
					r += 2;
					r.skipUntil<NewLine, StringView::Chars<'\'', '\\'>>();
				} else {
					errCb(r, "Line break within char literal");
					break;
				}
			}

			if (r.is('\'')) {
				++ r;
				if (r.is('_')) {
					r.skipChars<Identifier>();
				}
				stack.back()->tokens.emplace_back(new Token(Token::CharLiteral, StringView(tmp.data(), tmp.size() - r.size()), stack.back(), idx ++));
			} else {
				errCb(r, "No closing quote found for char literal");
				break;
			}
		} else if (r.is<Number>()) {
			auto idf = r.readChars<Identifier, StringView::Chars<'.', '\''>>();
			stack.back()->tokens.emplace_back(new Token(Token::NumWord, idf, stack.back(), idx ++));
		} else if (r.is<Identifier>()) {
			auto idf = r.readChars<Identifier>();
			if (isKeyword(idf)) {
				stack.back()->tokens.emplace_back(new Token(Token::Keyword, idf, stack.back(), idx ++));
			} else {
				stack.back()->tokens.emplace_back(new Token(Token::Word, idf, stack.back(), idx ++));
			}
		} else if (r.is('(')) {
			stack.emplace_back(stack.back()->tokens.emplace_back(new Token(Token::ParentesisBlock, r, stack.back(), idx ++)));
			++ r;
		} else if (r.is(')')) {
			if (stack.back()->tokClass == Token::ParentesisBlock) {
				++ r;
				stack.back()->token = StringView(stack.back()->token, stack.back()->token.size() - r.size());
				stack.pop_back();
			} else {
				if (stack.back()->tokClass != Token::PreprocessorCondition) {
					errCb(r, "Invalid ')' position");
					break;
				} else {
					++ r;
				}
			}

		} else if (r.is('[')) {
			stack.emplace_back(stack.back()->tokens.emplace_back(new Token(Token::CompositionBlock, r, stack.back(), idx ++)));
			++ r;
		} else if (r.is(']')) {
			if (stack.back()->tokClass == Token::CompositionBlock) {
				++ r;
				stack.back()->token = StringView(stack.back()->token, stack.back()->token.size() - r.size());
				stack.pop_back();
			} else {
				if (stack.back()->tokClass != Token::PreprocessorCondition) {
					errCb(r, "Invalid ']' position");
					break;
				} else {
					++ r;
				}
			}

		} else if (r.is('{')) {
			stack.emplace_back(stack.back()->tokens.emplace_back(new Token(Token::OperatorBlock, r, stack.back(), idx ++)));
			++ r;
		} else if (r.is('}')) {
			if (stack.back()->tokClass == Token::OperatorBlock) {
				++ r;
				stack.back()->token = StringView(stack.back()->token, stack.back()->token.size() - r.size());
				stack.pop_back();
			} else {
				if (stack.back()->tokClass != Token::PreprocessorCondition) {
					errCb(r, "Invalid '}' position");
					break;
				} else {
					++ r;
				}
			}
		} else if (r.is<Operator>()) {
			auto op = r.readChars<Operator>();
			while (!op.empty()) {
				if (op.is("%:%:")) {
					stack.back()->tokens.emplace_back(new Token(Token::Operator, op.sub(0, 4), stack.back(), idx ++));
					op += "%:%:"_len;
				} else if (op.is(">>=") || op.is("<<=") || op.is("...") || op.is("->*")) {
					stack.back()->tokens.emplace_back(new Token(Token::Operator, op.sub(0, 3), stack.back(), idx ++));
					op += 3;
				} else if (op.is("##") || op.is("::") || op.is(".*") || op.is("+=") || op.is("-=") || op.is("*=") || op.is("/=")
						|| op.is("%=") || op.is("^=") || op.is("&=") || op.is("|=") || op.is("<<") || op.is(">>") || op.is("==")
						|| op.is("!=") || op.is("<=") || op.is(">=") || op.is("&&") || op.is("||") || op.is("++") || op.is("--")
						|| op.is("->") || op.is("<%") || op.is("%>") || op.is("%:") || op.is("<:") || op.is(":>")) {
					stack.back()->tokens.emplace_back(new Token(Token::Operator, op.sub(0, 2), stack.back(), idx ++));
					op += 2;
				} else if (op.is('#') || op.is(';') || op.is(':') || op.is('?') || op.is('.') || op.is('+') || op.is('-')
						|| op.is('*') || op.is('/') || op.is('%') || op.is('^') || op.is('&') || op.is('|') || op.is('~')
						|| op.is('!') || op.is('=') || op.is('<') || op.is('>') || op.is('.') || op.is(',')) {
					stack.back()->tokens.emplace_back(new Token(Token::Operator, op.sub(0, 1), stack.back(), idx ++));
					++ op;
				} else {
					break;
				}
			}
			if (!op.empty()) {
				errCb(r, "Unknown operator");
				break;
			}
		} else if (!r.empty()) {
			errCb(r, "Unknown token");
			break;
		}
	}

	tok->makeFlow();
	return tok;
}

}
