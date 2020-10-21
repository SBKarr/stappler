
#ifndef UTILS_SRC_ODTEXTDOCUMENT_H_
#define UTILS_SRC_ODTEXTDOCUMENT_H_

#include "ODDocument.h"

namespace opendocument {

class TextDocument : public Document {
public:
	TextDocument(bool singleDocument = false);

	void addFileFont(const StringView &, Vector<Pair<File, String>> &&, const StringView &family = StringView(), Font::Generic = Font::Generic::None, Font::Pitch = Font::Pitch::None);
	void addFamilyFont(const StringView &, const StringView &, Font::Generic = Font::Generic::None, Font::Pitch = Font::Pitch::None);

	Node &getContent();

protected:
	void writeSingleContentFile(const WriteCallback &, bool pretty);
	void writeContentFile(const WriteCallback &, bool pretty);
	void writeStyleFile(const WriteCallback &, bool pretty);
	void writeSettingsFile(const WriteCallback &, bool pretty);
	void writeMetaFile(const WriteCallback &, bool pretty);

	void writeScripts(const WriteCallback &, bool pretty);
	void writeFonts(const WriteCallback &, bool pretty, bool full);

	Map<String, Font> _fonts;
	Node _content;
	bool _singleDocument = false;
};

}

#endif /* UTILS_SRC_ODTEXTDOCUMENT_H_ */
