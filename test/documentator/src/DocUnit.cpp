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

#include "DocUnit.h"
#include "SPFilesystem.h"

namespace doc {


Unit::File::File(StringView k, StringView content, Token *t)
: key(k.str<Interface>())
, content(content)
, token(t) { }


void Unit::exclude(StringView str) {
	auto it = _files.find(str);
	if (it != _files.end()) {
		_files.erase(it);
	}
	_excludes.emplace(str.str<Interface>());
}

Unit::File * Unit::addFile(StringView path, StringView key) {
	if (_excludes.find(key) != _excludes.end()) {
		return nullptr;
	}
	StringView c;
	auto f = stappler::filesystem::openForReading(path);
	if (f) {
		auto fsize = f.size();
		auto b = (char *)pool::palloc(pool::acquire(), fsize);
		f.read((uint8_t *)b, fsize);
		f.close();
		c = StringView(b, fsize);
	}

	if (!c.empty()) {
		auto tok = tokenize(c, [&] (StringView pos, StringView err) {
			onError(key, c, pos, err);
		});
		if (tok) {
			return &_files.emplace(key.str<Interface>(), File(key, c, tok)).first->second;
		}
	}
	return nullptr;
}


auto Unit::getLineNumber(StringView f, StringView pos) -> Pair<size_t, StringView> {
	size_t ret = 0;
	StringView line;
	while (f.data() <= pos.data()) {
		line = f.readUntil<StringView::Chars<'\r', '\n'>>();
		++ ret;

		if (f.is('\r')) {
			++ f;
		}
		if (f.is('\n')) {
			++ f;
		}
	}
	return pair(ret, line);
};

void Unit::onError(StringView key, StringView file, StringView pos, StringView err) {
	auto filePos = getLineNumber(file, pos);
	std::cout << key << ":" << filePos.first << ":\n";
	std::cout << filePos.second << "\n";
	for (size_t i = 1; i < size_t(pos.data() - filePos.second.data()); ++ i) {
		std::cout << ((filePos.second[i - 1] == '\t') ? "\t" : " ");
	}
	std::cout << "^\n";
	std::cout << err << "\n";
}

void Unit::parseDefines() {
	// phase 1 - read defines
	for (auto &it : _files) {
		auto fileToken = it.second.token;
		parseDefinesTokens(&it.second, fileToken);
	}

	// phase 2 - read conditions
	for (auto &it : _files) {
		auto fileToken = it.second.token;
		parseConditionTokens(&it.second, fileToken);
	}
}

void Unit_skipSpaceComments(StringView &r) {
	while (!r.empty() && (r.is<WhiteSpace>() || r.is("/*"))) {
		if (r.is<WhiteSpace>()) {
			r.skipChars<WhiteSpace>();
		} else if (r.is("/*")) {
			r += 2; r.skipUntilString("*/");
			if (r.is("*/")) {
				r += 2;
			}
		} else {
			break;
		}
	}
}

void Unit::parseDefinesTokens(File *f, Token *tok) {
	switch (tok->tokClass) {
	case Token::Preprocessor: {
		auto r = tok->token;
		if (r.is("#")) {
			++ r;
		}
		r.skipChars<WhiteSpace>();
		auto idf = stappler::string::tolower<Interface>(r.readChars<Identifier>());
		Unit_skipSpaceComments(r);
		if (idf == "define") {
			auto name = r.readChars<Identifier>();
			auto it = _defines.find(name);
			if (it != _defines.end()) {
				it->second.refs.emplace_back(f, tok);
			} else {
				Define *def = nullptr;
				if (r.is('(')) {
					++ r;
					Vector<StringView> args;
					while (!r.empty() && !r.is(')')) {
						if (!args.empty()) {
							Unit_skipSpaceComments(r);
							if (!r.is(',')) {
								onError(f->key, f->content, r, "Invalid char in define argument list");
								break;
							} else {
								++ r;
							}
						}
						Unit_skipSpaceComments(r);
						if (args.empty() && r.is(")")) {
							break;
						}
						if (r.is("...")) {
							args.emplace_back(r.sub(0, 3));
							r += 3;
							Unit_skipSpaceComments(r);
							break;
						}
						auto arg = r.readChars<Identifier>();
						if (arg.empty()) {
							onError(f->key, f->content, r, "Invalid char in define argument list");
							break;
						} else {
							args.emplace_back(arg);
						}
					}

					if (r.is(')')) {
						++ r;
						def = &_defines.emplace(name, Define()).first->second;
						def->name = name;
						def->isFunctional = true;
						r.trimChars<WhiteSpace>();
						def->body = r;
						def->args = move(args);
						def->refs.emplace_back(f, tok);
					}
				} else {
					def = &_defines.emplace(name, Define()).first->second;
					def->name = name;
					def->isFunctional = false;
					r.trimChars<WhiteSpace>();
					def->body = r;
					def->refs.emplace_back(f, tok);

					if (tok->idx == 1 && tok->root->tokClass == Token::PreprocessorCondition) {
						auto r = tok->root->token;
						if (r.is("#")) {
							++ r;
						}
						r.skipChars<WhiteSpace>();
						auto idf = stappler::string::tolower<Interface>(r.readChars<Identifier>());
						r.skipChars<WhiteSpace>();
						if (idf == "ifndef") {
							auto n = r.readChars<Identifier>();
							if (n == name) {
								def->isGuard = true;
							}
						}
					}
				}
			}
		}
		break;
	}
	default:
		break;
	}

	if (!tok->tokens.empty()) {
		for (auto &it : tok->tokens) {
			parseDefinesTokens(f, it);
		}
	}
}

void Unit::parseConditionTokens(File *f, Token *tok) {
	auto emplaceDef = [&] (StringView name) {
		auto it = _defines.find(name);
		if (it != _defines.end()) {
			it->second.usage.emplace_back(f, tok);
		} else {
			auto def = &_defines.emplace(name, Define()).first->second;
			def->name = name;
			def->body = name;
			def->usage.emplace_back(f, tok);
		}
	};

	switch (tok->tokClass) {
	case Token::Preprocessor:
	case Token::PreprocessorCondition: {
		auto r = tok->token;
		if (r.is("#")) {
			++ r;
		}
		r.skipChars<WhiteSpace>();
		auto idf = stappler::string::tolower<Interface>(r.readChars<Identifier>());
		Unit_skipSpaceComments(r);
		if (idf == "ifdef" || idf == "ifndef") {
			auto name = r.readChars<Identifier>();
			emplaceDef(name);
		} else if (idf == "if" || idf == "elif") {
			while (!r.empty()) {
				Unit_skipSpaceComments(r);
				if (r.is<Identifier>()) {
					auto idfTok = r.readChars<Identifier>();
					if (!idfTok.is<Number>()) {
						emplaceDef(idfTok);
					}
				} else if (r.is<Operator>()) {
					r.skipChars<Operator>();
				} else {
					r.skipUntil<Identifier, Operator>();
				}
			}
		}
		break;
	}
	default:
		break;
	}

	if (!tok->tokens.empty()) {
		for (auto &it : tok->tokens) {
			parseConditionTokens(f, it);
		}
	}
}

const Map<StringView, Unit::Define> Unit::getDefines() const {
	return _defines;
}

}
