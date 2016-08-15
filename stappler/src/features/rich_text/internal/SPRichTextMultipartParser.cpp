/*
 * SPRichTextMultipartParser.cpp
 *
 *  Created on: 05 мая 2015 г.
 *      Author: sbkarr
 */

#include "SPDefine.h"
#include "SPRichTextMultipartParser.h"
#include "SPString.h"

NS_SP_EXT_BEGIN(rich_text)

/* read file width header */
bool MultipartParser::parse(const Bytes &vec, bool files) {
	return parse(vec.data(), vec.size(), files);
}
bool MultipartParser::parse(const uint8_t *d, size_t l, bool files) {
    data = Reader((const char *)d, l);
    origData = Reader((const char *)d, l);

	Reader str = readLine();
	while (!str.empty()) {
		if (str.is("Content-Type:")) {
            str.offset("Content-Type:"_len);
			if (!parseContentType(str)) {
				return false;
			}
		}
		str = readLine();
	}

	if (contentType.empty() || boundary.empty()) {
		return false;
	}

	readFiles = files;
	return parseBody();
}

/* read content only */
bool MultipartParser::parse(const Bytes &vec, const String &contentType, bool files) {
	return parse(vec.data(), vec.size(), contentType, files);
}
bool MultipartParser::parse(const uint8_t *d, size_t l, const String &contentType, bool files) {
    Reader r(contentType.c_str(), contentType.length());
	if (!parseContentType(r)) {
		return false;
	}

    readFiles = files;
    data = Reader((const char *)d, l);
    origData = Reader((const char *)d, l);
	return parseBody();
}

bool MultipartParser::parseBody() {
	Reader str = readLine();
	String eofMark = boundary + "--";
	while (!str.empty() && str != eofMark) {
		if (str != boundary) {
			return false;
		}
		if (!parseFile()) {
			return false;
		}
		str = readLine();
	}

	if (str == eofMark) {
		return true;
	} else {
		return false;
	}
}

bool MultipartParser::parseFile() {
	fileContentType.clear();
	fileName.clear();
	fileEncoding.clear();
	fileContentLength = 0;
	fileWidth = 0;
	fileHeight = 0;

	Reader str = readLine();
	while (!str.empty()) {
		if (str.is("Content-Type:")) {
            str.offset("Content-Type:"_len);
			if (!parseFileContentType(str)) {
				return false;
			}
        } else if (str.is("Content-Length:")) {
            str.offset("Content-Length:"_len);
			if (!parseFileContentLength(str)) {
				return false;
			}
        } else if (str.is("Content-Disposition:")) {
            str.offset("Content-Disposition:"_len);
			if (!parseFileContentDisposition(str)) {
				return false;
			}
        } else if (str.is("Content-Encoding:")) {
            str.offset("Content-Encoding:"_len);
			if (!parseFileContentEncoding(str)) {
				return false;
			}
		}

		str = readLine();
	}

	if (fileContentType.empty() || fileContentLength == 0) {
		return false;
	}

	String match = String("\r\n") + boundary;

    auto tmpData = data;
	tmpData.offset(fileContentLength);
	while(!tmpData.empty() && !tmpData.is(match.c_str())) {
        ++ tmpData;
		++ fileContentLength;
	}

	if (data.empty()) {
		return false;
	}

	if (fileContentType == "image/png" || fileContentType == "image/jpeg") {
		if (readFiles) {
			if (fileEncoding == "base64") {
				auto d = Reader(data.data(), fileContentLength);
				images.emplace_back(fileName, base64::decode(d), fileWidth, fileHeight);
			} else {
				images.emplace_back(fileName, (const uint8_t *)data.data(), fileContentLength, fileWidth, fileHeight);
			}
		} else {
			images.emplace_back(data.data() - origData.data(), fileContentLength, fileEncoding, fileName, fileWidth, fileHeight);
		}
	} else if (fileContentType == "application/x-font-ttf") {
		if (readFiles) {
            if (fileEncoding == "base64") {
                auto d = Reader(data.data(), fileContentLength);
				fonts.emplace_back(fileName, base64::decode(d));
			} else {
				fonts.emplace_back(fileName, (const uint8_t *)data.data(), fileContentLength);
			}
		} else {
			fonts.emplace_back(data.data() - origData.data(), fileContentLength, fileEncoding, fileName);
		}
	} else if (fileContentType == "text/html") {
		if (fileEncoding == "base64") {
			return false;
		}
		html = Reader(data.data(), fileContentLength);
	}
    data.offset(fileContentLength);

	if (data.is('\r')) {
        ++ data;
	}
    if (data.is('\n')) {
        ++ data;
    }

	return true;
}

MultipartParser::Reader MultipartParser::readLine() {
    auto line = data.readUntil<Reader::Chars<'\r', '\n'>>();
    if (data.is('\r')) { ++ data; }
    if (data.is('\n')) { ++ data; }
	return line;
}

bool MultipartParser::parseContentType(Reader &r) {
    r.skipChars<Reader::CharGroup<chars::CharGroupId::WhiteSpace>>();
	if (r.empty()) {
		return false;
	}

    auto ct = r.readUntil<Reader::CharGroup<chars::CharGroupId::WhiteSpace>, Reader::Chars<';'>>();
    if (!ct.is("multipart/mixed") && !ct.is("multipart/form-data")) {
        return false;
    }

    r.skipChars<Reader::CharGroup<chars::CharGroupId::WhiteSpace>, Reader::Chars<';'>>();
    if (!r.is("boundary=")) {
        return false;
    }
    r.offset("boundary="_len);

	contentType = ct.str();
	boundary = String("--") + r.str();
	return true;
}

bool MultipartParser::parseFileContentType(Reader &r) {
    r.skipChars<Reader::CharGroup<chars::CharGroupId::WhiteSpace>>();

    auto ct = r.readUntil<Reader::CharGroup<chars::CharGroupId::WhiteSpace>, Reader::Chars<';'>>();
    r.skipChars<Reader::CharGroup<chars::CharGroupId::WhiteSpace>, Reader::Chars<';'>>();
	if (r.empty()) {
		fileContentType = ct.str();
		return true;
	}

	uint16_t width = 0, height = 0;
	while (!r.empty()) {
        if (r.is("width=")) {
            r.offset("width="_len);
            auto val = r.readInteger();
            if (val < maxOf<uint16_t>()) {
                width = (uint16_t)val;
            }
        } else if (r.is("height=")) {
            r.offset("height="_len);
            auto val = r.readInteger();
            if (val < maxOf<uint16_t>()) {
                height = (uint16_t)val;
            }
        }
        r.skipChars<Reader::CharGroup<chars::CharGroupId::WhiteSpace>, Reader::Chars<';'>>();
	}

	if (width && height) {
		fileContentType = ct.str();
		fileWidth = width;
		fileHeight = height;
		return true;
	}
	return false;
}

bool MultipartParser::parseFileContentLength(Reader &r) {
    int64_t len = r.readInteger();
    if (len != maxOf<int64_t>()) {
        fileContentLength = (size_t)len;
        return true;
    }
    return false;
}

bool MultipartParser::parseFileContentDisposition(Reader &r) {
	r.skipChars<Reader::CharGroup<chars::CharGroupId::WhiteSpace>, Reader::Chars<';'>>();
	if (r.empty()) { return true; }

	Reader name;

	while (!r.empty()) {
		if (r.is("name=")) {
			r.offset(5);
			if (r.is('"')) {
				++ r;
				name = r.readUntil<Reader::Chars<'"'>>();
			} else {
				name = r.readUntil<Reader::CharGroup<chars::CharGroupId::WhiteSpace>, Reader::Chars<';'>>();
			}
		}
		r.skipUntil<Reader::Chars<';'>>();
		r.skipChars<Reader::CharGroup<chars::CharGroupId::WhiteSpace>, Reader::Chars<';'>>();
	}

	if (!name.empty()) {
		fileName = name.str();
		return true;
	}
	return false;
}

bool MultipartParser::parseFileContentEncoding(Reader &r) {
	r.skipChars<Reader::CharGroup<chars::CharGroupId::WhiteSpace>>();
	auto enc = r.readUntil<Reader::CharGroup<chars::CharGroupId::WhiteSpace>, Reader::Chars<';'>>();
	if (!enc.empty()) {
		fileEncoding = enc.str();
		return true;
	}
	return false;
}

MultipartParser::Image::Image(const String &name, const uint8_t *data, size_t len, uint16_t w, uint16_t h)
: width(w), height(h), offset(0), length(0), name(name), data(data, data + len) { }

MultipartParser::Image::Image(const String &name, Bytes && d, uint16_t w, uint16_t h)
: width(w), height(h), offset(0), length(0), name(name) {
	data.swap(d);
}

MultipartParser::Image::Image(size_t pos, size_t len, const String &enc, const String &name, uint16_t w, uint16_t h)
: width(w), height(h), offset(pos), length(len), encoding(enc), name(name) { }

MultipartParser::Font::Font(const String &name, const uint8_t *data, size_t len)
: name(name), data(data, data + len), offset(0), length(0) { }
MultipartParser::Font::Font(const String &name, Bytes && data)
: name(name), data(std::move(data)), offset(0), length(0)  { }

MultipartParser::Font::Font(size_t pos, size_t len, const String &enc, const String &name)
: name(name), data(), offset(pos), length(len), encoding(enc) { }

NS_SP_EXT_END(rich_text)
