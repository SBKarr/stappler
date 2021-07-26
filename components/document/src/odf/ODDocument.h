
#ifndef UTILS_SRC_ODDOCUMENT_H_
#define UTILS_SRC_ODDOCUMENT_H_

#include "OpenDocument.h"
#include "ODStyle.h"
#include "ODContent.h"

namespace opendocument {

class Document : public AllocBase {
public:
	Document(DocumentType);
	virtual ~Document();

public: // content
	const File & addTextFile(StringView name, StringView type, StringView data);
	const File & addBinaryFile(StringView name, StringView type, Bytes &&data);
	const File & addFilesystemFile(StringView name, StringView type, StringView path);
	const File & addFunctionalFile(StringView name, StringView type, FileReaderCallback &&);
	const File & addNetworkFile(StringView name, StringView url);

	Buffer save(bool pretty = false) const;

public: // meta
	void setMetaGenerator(const StringView &);
	StringView getMetaGenerator() const;

	void setMetaTitle(const StringView &);
	StringView getMetaTitle() const;

	void setMetaDescription(const StringView &);
	StringView getMetaDescription() const;

	void setMetaSubject(const StringView &);
	StringView getMetaSubject() const;

	void setMetaKeywords(const StringView &);
	StringView getMetaKeywords() const;

	void setMetaInitialCreator(const StringView &);
	StringView getMetaInitialCreator() const;

	void setMetaCreator(const StringView &);
	StringView getMetaCreator() const;

	void setMetaPrintedBy(const StringView &);
	StringView getMetaPrintedBy() const;

	void setMetaCreationDate(const Time &);
	Time getMetaCreationDate() const;

	void setMetaDate(const Time &);
	Time getMetaDate() const;

	void setMetaPrintDate(const Time &);
	Time getMetaPrintDate() const;

	void setMetaLanguage(const StringView &);
	StringView getMetaLanguage() const;

	void setUserMetaString(const StringView &, const StringView &);
	void setUserMetaDouble(const StringView &, double);
	void setUserMetaDateTime(const StringView &, const Time &);
	void setUserMetaBoolean(const StringView &, bool);

	StringView getUserMeta(const StringView &) const;

	style::Style *getDefaultStyle();

	style::Style *addCommonStyle(const StringView &, style::Style::Family, const StringView & = StringView());
	style::Style *addCommonStyle(const StringView &, const style::Style *, const StringView & = StringView());

	style::Style *addAutoStyle(style::Style::Family, const StringView & = StringView());
	style::Style *addAutoStyle(const style::Style *, const StringView & = StringView());

	style::Style *addContentStyle(style::Style::Family, const StringView & = StringView());
	style::Style *addContentStyle(const style::Style *, const StringView & = StringView());

	style::Style *getCommonStyle(const StringView &);

	MasterPage *addMasterPage(const StringView &, const style::Style *pageLayout);

protected:
	void setUserMetaType(const StringView &, const StringView &, MetaFormat);

	void setMeta(MetaType, const StringView &);
	StringView getMeta(MetaType) const;

	void writeMeta(const WriteCallback &, bool pretty) const;
	void writeStyles(const WriteCallback &, bool pretty, bool withContent = false) const;
	void writeContentNode(const WriteCallback &, const Node &, bool pretty) const;

	DocumentType _type;

	Map<MetaType, String> _meta;
	Map<String, stappler::Pair<String, MetaFormat>> _userMeta;

	Vector<File> _files;
	Map<style::StringId, String> _stringPool;

	style::Style *_defaultStyle = nullptr;
	Vector<style::Style *> _autoStyles;
	Vector<style::Style *> _contentStyles;
	Map<String, style::Style *> _commonStyles;

	Map<String, MasterPage> _masterPages;
	size_t _autoStyleCount = 0;
	size_t _contentStyleCount = 0;
};

}

#endif /* UTILS_SRC_ODDOCUMENT_H_ */
