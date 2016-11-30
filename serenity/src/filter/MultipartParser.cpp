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
#include "MultipartParser.h"

NS_SA_BEGIN

MultipartParser::MultipartParser(const InputConfig &c, size_t s, const apr::string &b)
: InputParser(c, s) {
	boundary.reserve(b.size() + 4);
	boundary.append("\r\n--");
	boundary.append(b);
	match = 2;
}

MultipartParser::MultipartParser(const InputConfig &c, size_t s, const char *b)
: InputParser(c, s) {
	boundary.reserve(strlen(b) + 4);
	boundary.append("\r\n--");
	boundary.append(b);
	match = 2;
}

data::Value * MultipartParser::flushVarName(Reader &r) {
	VarState cstate = VarState::Key;
	data::Value *current = nullptr;
	while (!r.empty()) {
		Reader str = r.readUntil<chars::Chars<char, '[', ']'>>();
		current = flushString(str, current, cstate);
		if (!current) {
			break;
		}
		if (!r.empty()) {
			switch (cstate) {
			case VarState::Key:
				switch (r[0]) {
				case '[': 			cstate = VarState::SubKey; break;
				default: 			cstate = VarState::End; break;
				}
				break;
			case VarState::SubKey:
				switch (r[0]) {
				case ']': 			cstate = VarState::SubKeyEnd; break;
				default: 			cstate = VarState::End; break;
				}
				break;
			case VarState::SubKeyEnd:
				switch (r[0]) {
				case '[': 			cstate = VarState::SubKey; break;
				default: 			cstate = VarState::End; break;
				}
				break;
			default:
				return nullptr;
				break;
			}
			++ r;
		}
	}
	return current;
}

void MultipartParser::flushLiteral(Reader &r, bool quoted) {
	switch (header) {
	case Header::ContentDispositionFileName:
		file.assign(r.data(), r.size());
		if (!quoted) {
			string::trim(file);
		}
		break;
	case Header::ContentDispositionName:
		name.assign(r.data(), r.size());
		if (!quoted) {
			string::trim(name);
		}
		break;
	case Header::ContentDispositionSize:
		size = strtol(r.data(), nullptr, 10);
		break;
	case Header::ContentType:
		type.assign(r.data(), r.size());
		string::trim(type);
		break;
	case Header::ContentEncoding:
		encoding.assign(r.data(), r.size());
		string::trim(encoding);
		break;
	default:
		break;
	}
}

void MultipartParser::flushData(Reader &r) {
	switch (data) {
	case Data::File:
		if (r.size() + files.back().writeSize >= getConfig().maxFileSize) {
			files.back().close();
			files.pop_back();
			data = Data::Skip;
		} else {
			files.back().write(r.data(), r.size());
		}
		break;
	case Data::FileAsData:
		if (r.size() + dataBuf.bytesWritten() >= getConfig().maxFileSize) {
			files.back().close();
			files.pop_back();
			data = Data::Skip;
		} else {
			dataBuf.sputn(r.data(), r.size());
		}
		break;
	case Data::Var:
		if (r.size() + buf.size() >= getConfig().maxVarSize) {
			buf.clear();
			data = Data::Skip;
		} else {
			buf.put(r.data(), r.size());
		}
		break;
	case Data::Skip:
		break;
	}
}

void MultipartParser::readBegin(Reader &r) {
	if (match == 0) {
		r.skipUntil<Reader::Chars<'-'>>();
	}
	while (match < boundary.length() && r.is(boundary.at(match))) {
		++ match; ++ r;
	}
	if (r.empty()) {
		return;
	} else if (match == boundary.length()) {
		state = State::BeginBlock;
		match = 0;
	} else {
		match = 0;
	}
	buf.clear();
}

void MultipartParser::readBlock(Reader &r) {
	while (!r.empty()) {
		if (buf.size() == 0 && r.is('-')) {
			buf.putc(r[0]);
		} else if (buf.size() == 1 && buf.get().is('-') && r.is('-')) {
			state = State::End;
			return;
		} else {
			r.skipUntil<Reader::Chars<'\n'>>();
			if (r.is('\n')) {
				state = State::HeaderLine;
				header = Header::Begin;
				name.clear();
				type.clear();
				encoding.clear();
				file.clear();
				size = 0;
				buf.clear();
				++r;
				return;
			}
		}
	}
}

void MultipartParser::readHeaderBegin(Reader &r) {
	Reader str = r.readUntil<Reader::Chars<'\n', ':'>>();
	if (r.is(':')) {
		Reader tmp;
		if (buf.empty()) {
			tmp = str;
		} else {
			buf.put(str.data(), str.size());
			tmp = buf.get();
		}

		tmp.skipChars<Reader::CharGroup<chars::CharGroupId::WhiteSpace>>();
		if (strncasecmp(tmp.data(), "Content-Disposition", "Content-Disposition"_len) == 0) {
			header = Header::ContentDisposition;
		} else if (strncasecmp(tmp.data(), "Content-Type", "Content-Type"_len) == 0) {
			header = Header::ContentType;
		} else if (strncasecmp(tmp.data(), "Content-Transfer-Encoding", "Content-Transfer-Encoding"_len) == 0) {
			header = Header::ContentEncoding;
		} else {
			header = Header::Unknown;
		}

		buf.clear();
		r ++;
	} else if (r.empty()) {
		buf.put(str.data(), str.size());
	} else if (r.is('\n')) {
		auto tmp = buf.get();
		str.skipChars<Reader::CharGroup<chars::CharGroupId::WhiteSpace>>();
		tmp.skipChars<Reader::CharGroup<chars::CharGroupId::WhiteSpace>>();
		if ((tmp.empty() || tmp.is('\r')) && (str.empty() || str.is('\r'))) {
			state = State::Data;
			if (!file.empty() || !type.empty()) {
				if ((config.required & InputConfig::Require::FilesAsData) != InputConfig::Require::None
						&& (size == 0 || size < getConfig().maxFileSize)
						&& ((type.compare(0, "application/cbor"_len, "application/cbor") == 0)
								|| (type.compare(0, "application/json"_len, "application/json") == 0))) {
					dataBuf.clear();
					data = Data::FileAsData;
				} else if ((config.required & InputConfig::Require::Files) != 0
						&& (size == 0 || size < getConfig().maxFileSize)) {
					files.emplace_back(std::move(name), std::move(type), std::move(encoding), std::move(file), size);
					data = Data::File;
				} else {
					data = Data::Skip;
				}
			} else {
				if ((config.required & InputConfig::Require::Data) != 0 &&
						(size == 0 || size < getConfig().maxVarSize)) {
					data = Data::Var;
				} else {
					data = Data::Skip;
				}
			}

			name.empty();
			type.empty();
			encoding.empty();
			file.empty();
			size = 0;
		}
		header = Header::Begin; // next header
		buf.clear();
		r ++;
	}
}

void MultipartParser::readHeaderContentDisposition(Reader &r) {
	Reader str = r.readUntil<Reader::Chars<'\n', ';'>>();
	if (r.is(';')) {
		Reader tmp;
		if (buf.empty()) {
			tmp = str;
		} else {
			buf.put(str.data(), str.size());
			tmp = buf.get();
		}

		tmp.skipChars<Reader::CharGroup<chars::CharGroupId::WhiteSpace>>();
		if (strncasecmp(tmp.data(), "form-data", "form-data"_len) == 0) {
			header = Header::ContentDispositionParams;
		} else {
			header = Header::Unknown;
		}

		buf.clear();
		r ++;
	} else if (r.empty()) {
		buf.put(str.data(), str.size());
	} else if (r.is('\n')) {
		header = Header::Begin; // next header
		buf.clear();
		r ++;
	}
}

void MultipartParser::readHeaderContentDispositionParam(Reader &r) {
	if (buf.empty()) {
		r.skipChars<Reader::Chars<';', ' '>>();
	}
	Reader str = r.readUntil<Reader::Chars<'\n', '='>>();
	if (r.is('=')) {
		Reader tmp;
		if (buf.empty()) {
			tmp = str;
		} else {
			buf.put(str.data(), str.size());
			tmp = buf.get();
		}

		if (strncasecmp(tmp.data(), "name", "name"_len) == 0) {
			header = Header::ContentDispositionName;
			literal = Literal::None;
		} else if (strncasecmp(tmp.data(), "filename", "filename"_len) == 0) {
			header = Header::ContentDispositionFileName;
			literal = Literal::None;
		} else if (strncasecmp(tmp.data(), "size", "size"_len) == 0) {
			header = Header::ContentDispositionSize;
			literal = Literal::None;
		} else {
			header = Header::ContentDispositionUnknown;
		}

		buf.clear();
		r ++;
	} else if (r.empty()) {
		buf.put(str.data(), str.size());
	} else if (r.is('\n')) {
		header = Header::Begin; // next header
		buf.clear();
		r ++;
	}
}

void MultipartParser::readHeaderValue(Reader &r) {
	auto &max = getConfig().maxVarSize;
	Reader str = r.readUntil<Reader::Chars<'\n'>>();
	if (r.empty()) {
		if (buf.size() + str.size() < max) {
			buf.put(str.data(), str.size());
		} else {
			header = Header::Unknown; // skip processing
		}
	} else if (r.is('\n')) {
		Reader tmp;
		if (buf.empty()) {
			if (str.size() < max) {
				tmp = str;
			}
		} else {
			if (str.size() + buf.size() < max) {
				buf.put(str.data(), str.size());
				tmp = buf.get();
			}
		}

		if (!tmp.empty()) {
			flushLiteral(tmp, false);
		}

		header = Header::Begin; // next header
		literal = Literal::None;
		buf.clear();
		r ++;
	}
}

void MultipartParser::readHeaderDummy(Reader &r) {
	r.skipUntil<Reader::Chars<'\n'>>();
	if (r.is('\n')) {
		header = Header::Begin; // next header
		literal = Literal::None;
		buf.clear();
		r ++;
	}
}

void MultipartParser::readPlainLiteral(Reader &r) {
	auto &max = getConfig().maxVarSize;
	Reader str = r.readUntil<Reader::Chars<'\n', ';'>>();
	if (r.is(';') || r.is('\n')) {
		Reader tmp;
		if (buf.empty()) {
			if (str.size() < max) {
				tmp = str;
			} else {
				header = Header::ContentDispositionUnknown;
			}
		} else {
			if (str.size() + buf.size() < max) {
				buf.put(str.data(), str.size());
				tmp = buf.get();
			} else {
				header = Header::ContentDispositionUnknown;
			}
		}

		if (!tmp.empty()) {
			flushLiteral(tmp, false);
		}

		header = r.is(';') ? Header::ContentDispositionParams : Header::Begin;
		literal = Literal::None;
		buf.clear();
		r ++;
	} else if (r.empty()) {
		if (str.size() + buf.size() < max) {
			buf.put(str.data(), str.size());
		} else {
			header = Header::ContentDispositionUnknown;
		}
	}
}

void MultipartParser::readQuotedLiteral(Reader &r) {
	auto &max = getConfig().maxVarSize;
	Reader str = r.readUntil<Reader::Chars<'\n', '"'>>();
	if (r.is('"')) {
		Reader tmp;
		if (buf.empty()) {
			if (str.size() < max) {
				tmp = str;
			} else {
				header = Header::ContentDispositionUnknown;
			}
		} else {
			if (buf.size() + str.size() < max) {
				buf.put(str.data(), str.size());
				tmp = buf.get();
			} else {
				header = Header::ContentDispositionUnknown;
			}
		}

		flushLiteral(tmp, true);
		buf.clear();
		r ++;
		header = Header::ContentDispositionParams;
		literal = Literal::None;
	} else if (r.empty()) {
		if (buf.size() + str.size() < max) {
			buf.put(str.data(), str.size());
		} else {
			header = Header::ContentDispositionUnknown;
		}
	} else if (r.is('\n')) {
		header = Header::Begin; // next header
		literal = Literal::None;
		buf.clear();
		r ++;
	}
}

void MultipartParser::readHeaderContentDispositionValue(Reader &r) {
	switch (literal) {
	case Literal::None:
		if (r.is('"')) {
			literal = Literal::Quoted;
			r ++;
		} else if (r.is('\n')) {
			header = Header::Begin; // next header
			buf.clear();
			r ++;
		} else if (r.is<Reader::CharGroup<chars::CharGroupId::WhiteSpace>>()) {
			literal = Literal::Plain;
		}
		break;
	case Literal::Plain:
		readPlainLiteral(r);
		break;
	case Literal::Quoted:
		readQuotedLiteral(r);
		break;
	}
}

void MultipartParser::readHeaderContentDispositionDummy(Reader &r) {
	r.skipUntil<Reader::Chars<'\n', ';'>>();
	if (r.is(';')) {
		header = Header::ContentDispositionParams;
		literal = Literal::None;
		buf.clear();
		r ++;
	} else if (r.is('\n')) {
		header = Header::Begin; // next header
		literal = Literal::None;
		buf.clear();
		r ++;
	}
}

void MultipartParser::readHeader(Reader &r) {
	switch (header) {
	case Header::Begin:
		readHeaderBegin(r);
		break;
	case Header::ContentDisposition:
		readHeaderContentDisposition(r);
		break;
	case Header::ContentDispositionParams:
		readHeaderContentDispositionParam(r);
		break;
	case Header::ContentDispositionName:
	case Header::ContentDispositionFileName:
	case Header::ContentDispositionSize:
		readHeaderContentDispositionValue(r);
		break;
	case Header::ContentDispositionUnknown:
		readHeaderContentDispositionDummy(r);
		break;
	case Header::ContentType:
	case Header::ContentEncoding:
		readHeaderValue(r);
		break;
	case Header::Unknown:
		readHeaderDummy(r);
		break;
	}
}

void MultipartParser::readData(Reader &r) {
	if (match == 0) {
		auto str = r.readUntil<Reader::Chars<'\r'>>();
		flushData(str);
		if (r.empty()) {
			return;
		} else {
			match = 1;
			r ++;
		}
	} else {
		while (!r.empty() && r[0] == boundary[match] && match < boundary.length()) {
			match ++;
			r ++;
		}

		if (match == boundary.length()) {
			state = State::BeginBlock;
			if (data == Data::Var) {
				Reader tmp(name);
				auto current = flushVarName(tmp);
				if (current) {
					current->setString(buf.str());
				}
				buf.clear();
			} else if (data == Data::FileAsData) {
				root.setValue(std::move(dataBuf.data()), std::move(name));
				dataBuf.clear();
			}
			match = 0;
		} else if (!r.empty() && r[0] != boundary[match]) {
			Reader tmp(boundary.data(), match);
			flushData(tmp);
			match = 0;
		}
	}
}

void MultipartParser::run(const char *str, size_t len) {
	Reader r(str, len);
	while (!r.empty()) {
		switch (state) {
		case State::Begin: // skip preambula
			readBegin(r);
			break;
		case State::BeginBlock: // wait for CRLF then headers or "--" then EOF
			readBlock(r);
			break;
		case State::HeaderLine:
			readHeader(r);
			break;
		case State::Data:
			readData(r);
			break;
		default:
			return;
			break;
		}
	}
}

void MultipartParser::finalize() {

}

NS_SA_END
