/*
 * SPRichTextMultipartParser.h
 *
 *  Created on: 05 мая 2015 г.
 *      Author: sbkarr
 */

#ifndef LIBS_STAPPLER_FEATURES_RICH_TEXT_SPRICHTEXTMULTIPARTPARSER_H_
#define LIBS_STAPPLER_FEATURES_RICH_TEXT_SPRICHTEXTMULTIPARTPARSER_H_

#include "SPRichTextParser.h"

NS_SP_EXT_BEGIN(rich_text)

struct MultipartParser {
    using Reader = CharReaderBase;

	struct Image {
		uint16_t width;
		uint16_t height;

		size_t offset;
		size_t length;
		String encoding;

		String name;
		Bytes data;

		Image(const String &name, const uint8_t *data, size_t len, uint16_t, uint16_t);
		Image(const String &name, Bytes && data, uint16_t, uint16_t);
		Image(size_t pos, size_t len, const String &enc, const String &name, uint16_t, uint16_t);
	};

	struct Font {
		String name;
		Bytes data;

		size_t offset;
		size_t length;
		String encoding;

		Font(const String &name, const uint8_t *data, size_t len);
		Font(const String &name, Bytes && data);
		Font(size_t pos, size_t len, const String &enc, const String &name);
	};

	bool readFiles = true;

	uint16_t fileWidth = 0;
	uint16_t fileHeight = 0;
	String fileContentType;
	String fileName;
	String fileEncoding;
	size_t fileContentLength = 0;

	String contentType;
	String boundary;

	Reader html;
	Vector<Image> images;
	Vector<Font> fonts;

    Reader data;
    Reader origData;

	/* read file width header */
	bool parse(const Bytes &, bool readFiles = true);
	bool parse(const uint8_t *, size_t, bool readFiles = true);

	/* read content only */
	bool parse(const Bytes &, const String &contentType, bool readFiles = true);
	bool parse(const uint8_t *, size_t, const String &contentType, bool readFiles = true);

	bool parseBody();
	bool parseFile();

	bool parseContentType(Reader &r);
	bool parseFileContentType(Reader &r);
	bool parseFileContentLength(Reader &r);
	bool parseFileContentDisposition(Reader &r);
	bool parseFileContentEncoding(Reader &r);
	Reader readLine();
};

NS_SP_EXT_END(rich_text)

#endif /* LIBS_STAPPLER_FEATURES_RICH_TEXT_SPRICHTEXTMULTIPARTPARSER_H_ */
