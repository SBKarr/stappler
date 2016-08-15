/*
 * SPUrlencodeParser.cpp
 *
 *  Created on: 9 июня 2016 г.
 *      Author: sbkarr
 */

#include "SPCommon.h"
#include "SPUrlencodeParser.h"
#include "SPString.h"

NS_SP_BEGIN

#if SPAPR
static apr::string &apr_decodeString_inPlace(apr::string &str) {
	char code1 = 0, code2 = 0;
	char *writePtr = str.data(); size_t writeLen = 0;
	char *readPtr = str.data(); size_t readLen = str.size();

	while (readLen > 0) {
		char c = *readPtr;
		if (c != '%' || readLen <= 2) {
			if (readPtr != writePtr) {
				*writePtr = c;
			}
			++ writePtr; ++ writeLen;
			++ readPtr; -- readLen;
		} else {
			++ readPtr; -- readLen; code1 = * readPtr;
			++ readPtr; -- readLen; code2 = * readPtr;
			++ readPtr; -- readLen;

			*writePtr = string::hexToChar(code1) << 4 | string::hexToChar(code2);
			++ writePtr; ++ writeLen;
		}
	}

	str.resize(writeLen);
	return str;
}
#endif

using NextToken = chars::Chars<char, '=', '&', ';', '[', ']', '+', '%'>;
using NextKey = chars::Chars<char, '&', ';', '+'>;

UrlencodeParser::UrlencodeParser(data::Value &target, size_t len, size_t max) : target(&target), length(len), maxVarSize(max) { }

void UrlencodeParser::bufferize(Reader &r) {
	if (!skip) {
		if (buf.size() + r.size() > maxVarSize) {
			buf.clear();
			skip = true;
		} else {
			buf.put(r.data(), r.size());
		}
	}
}

void UrlencodeParser::bufferize(char c) {
	if (!skip) {
		if (buf.size() + 1 > maxVarSize) {
			buf.clear();
			skip = true;
		} else {
			buf.putc(c);
		}
	}
	buf.putc(c);
}

void UrlencodeParser::flush(Reader &r) {
	if (!skip) {
		if (r.size() < maxVarSize) {
			current = flushString(r, current, state);
		} else {
			skip = true;
		}
		buf.clear();
	}
}

size_t UrlencodeParser::read(const uint8_t * s, size_t count) {
	if (count >= length) {
		count = length;
		length = 0;
	}
	Reader r((const char *)s, count);

	while (!r.empty()) {
		Reader str;
		if (state == VarState::Value) {
			str = r.readUntil<NextKey>();
		} else {
			str = r.readUntil<NextToken>();
		}

		if (buf.empty() && (!r.empty() || length == 0) && !r.is('+')) {
			flush(str);
		} else {
			bufferize(str);
			if (!r.empty() && !r.is('+')) {
				Reader tmp = buf.get();
				flush(tmp);
			}
		}

		char c = r[0];
		if (c == '+') {
			bufferize(' ');
			++ r;
		} else {
			++ r;
			if (c == '%') {
				if (r.is("5B")) {
					c = '[';
					r += 2;
				} else if (r.is("5D")) {
					c = ']';
					r += 2;
				} else {
					bufferize('%');
					c = 0;
				}
			}

			if (c != 0) {
				switch (state) {
				case VarState::Key:
					switch (c) {
					case '[': 			state = VarState::SubKey; break;
					case '=': 			state = VarState::Value; break;
					case '&': case ';': state = VarState::Key; break;
					default: 			state = VarState::End; break;
					}
					break;
				case VarState::SubKey:
					switch (c) {
					case ']': 			state = VarState::SubKeyEnd; break;
					default: 			state = VarState::End; break;
					}
					break;
				case VarState::SubKeyEnd:
					switch (c) {
					case '[': 			state = VarState::SubKey; break;
					case '=': 			state = VarState::Value; break;
					case '&': case ';': state = VarState::Key; break;
					default: 			state = VarState::End; break;
					}
					break;
				case VarState::Value:
					switch (c) {
					case '&': case ';': state = VarState::Key; skip = false; break;
					default: 			state = VarState::End; break;
					}
					break;
				default:
					break;
				}
			}
		}
		if (skip) {
			break;
		}
	}

	return count;
}

data::Value *UrlencodeParser::flushString(CharReaderBase &r, data::Value *current, VarState state) {
	String str;
#if SPAPR
	str.assign_weak(r.data(), r.size());
	apr_decodeString_inPlace(str);
#else
	str = string::urldecode(r.str());
#endif

	switch (state) {
	case VarState::Key:
		if (!str.empty()) {
			if (target->hasValue(str)) {
				current = &target->getValue(str);
			} else {
				current = &target->setValue(data::Value(true), str);
			}
		}
		break;
	case VarState::SubKey:
		if (current) {
			if (str.empty()) {
				if (!current->isArray()) {
					current->setArray(data::Array());
				}
				current = &current->addValue(data::Value(true));
			} else {
				if (!current->isDictionary()) {
					current->setDict(data::Dictionary());
				}
				if (current->hasValue(str)) {
					current = &current->getValue(str);
				} else {
					current = &current->setValue(data::Value(true), str);
				}
			}
		}
		break;
	case VarState::Value:
		if (current) {
			if (!str.empty()) {
				current->setString(str);
			}
			current = nullptr;
		}
		break;
	default:
		break;
	}

	return current;
}

NS_SP_END
