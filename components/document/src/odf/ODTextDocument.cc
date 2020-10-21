
#include "ODTextDocument.h"

namespace opendocument {

static constexpr auto s_xml_ns = R"(xmlns:office="urn:oasis:names:tc:opendocument:xmlns:office:1.0"
	xmlns:style="urn:oasis:names:tc:opendocument:xmlns:style:1.0"
	xmlns:text="urn:oasis:names:tc:opendocument:xmlns:text:1.0"
	xmlns:table="urn:oasis:names:tc:opendocument:xmlns:table:1.0"
	xmlns:draw="urn:oasis:names:tc:opendocument:xmlns:drawing:1.0"
	xmlns:fo="urn:oasis:names:tc:opendocument:xmlns:xsl-fo-compatible:1.0"
	xmlns:xlink="http://www.w3.org/1999/xlink"
	xmlns:dc="http://purl.org/dc/elements/1.1/"
	xmlns:meta="urn:oasis:names:tc:opendocument:xmlns:meta:1.0"
	xmlns:number="urn:oasis:names:tc:opendocument:xmlns:datastyle:1.0"
	xmlns:svg="urn:oasis:names:tc:opendocument:xmlns:svg-compatible:1.0"
	xmlns:chart="urn:oasis:names:tc:opendocument:xmlns:chart:1.0"
	xmlns:dr3d="urn:oasis:names:tc:opendocument:xmlns:dr3d:1.0"
	xmlns:math="http://www.w3.org/1998/Math/MathML"
	xmlns:form="urn:oasis:names:tc:opendocument:xmlns:form:1.0"
	xmlns:script="urn:oasis:names:tc:opendocument:xmlns:script:1.0"
	xmlns:ooo="http://openoffice.org/2004/office"
	xmlns:ooow="http://openoffice.org/2004/writer"
	xmlns:oooc="http://openoffice.org/2004/calc"
	xmlns:dom="http://www.w3.org/2001/xml-events"
	xmlns:xforms="http://www.w3.org/2002/xforms"
	xmlns:xsd="http://www.w3.org/2001/XMLSchema"
	xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
	xmlns:rpt="http://openoffice.org/2005/report"
	xmlns:of="urn:oasis:names:tc:opendocument:xmlns:of:1.2"
	xmlns:xhtml="http://www.w3.org/1999/xhtml"
	xmlns:grddl="http://www.w3.org/2003/g/data-view#"
	xmlns:officeooo="http://openoffice.org/2009/office"
	xmlns:tableooo="http://openoffice.org/2009/table"
	xmlns:drawooo="http://openoffice.org/2010/draw"
	xmlns:calcext="urn:org:documentfoundation:names:experimental:calc:xmlns:calcext:1.0"
	xmlns:loext="urn:org:documentfoundation:names:experimental:office:xmlns:loext:1.0"
	xmlns:field="urn:openoffice:names:experimental:ooo-ms-interop:xmlns:field:1.0"
	xmlns:formx="urn:openoffice:names:experimental:ooxml-odf-interop:xmlns:form:1.0"
	xmlns:css3t="http://www.w3.org/TR/css3-text/" office:version="1.2")";

static constexpr auto s_xml_ns_settings = R"(xmlns:office="urn:oasis:names:tc:opendocument:xmlns:office:1.0" xmlns:xlink="http://www.w3.org/1999/xlink" xmlns:config="urn:oasis:names:tc:opendocument:xmlns:config:1.0" xmlns:ooo="http://openoffice.org/2004/office" office:version="1.2")";

static constexpr auto s_xml_ns_styles = R"(xmlns:office="urn:oasis:names:tc:opendocument:xmlns:office:1.0"
	xmlns:style="urn:oasis:names:tc:opendocument:xmlns:style:1.0"
	xmlns:text="urn:oasis:names:tc:opendocument:xmlns:text:1.0"
	xmlns:table="urn:oasis:names:tc:opendocument:xmlns:table:1.0"
	xmlns:draw="urn:oasis:names:tc:opendocument:xmlns:drawing:1.0"
	xmlns:fo="urn:oasis:names:tc:opendocument:xmlns:xsl-fo-compatible:1.0"
	xmlns:xlink="http://www.w3.org/1999/xlink"
	xmlns:dc="http://purl.org/dc/elements/1.1/"
	xmlns:meta="urn:oasis:names:tc:opendocument:xmlns:meta:1.0"
	xmlns:number="urn:oasis:names:tc:opendocument:xmlns:datastyle:1.0"
	xmlns:svg="urn:oasis:names:tc:opendocument:xmlns:svg-compatible:1.0"
	xmlns:chart="urn:oasis:names:tc:opendocument:xmlns:chart:1.0"
	xmlns:dr3d="urn:oasis:names:tc:opendocument:xmlns:dr3d:1.0"
	xmlns:math="http://www.w3.org/1998/Math/MathML"
	xmlns:form="urn:oasis:names:tc:opendocument:xmlns:form:1.0"
	xmlns:script="urn:oasis:names:tc:opendocument:xmlns:script:1.0"
	xmlns:ooo="http://openoffice.org/2004/office"
	xmlns:ooow="http://openoffice.org/2004/writer"
	xmlns:oooc="http://openoffice.org/2004/calc"
	xmlns:dom="http://www.w3.org/2001/xml-events"
	xmlns:rpt="http://openoffice.org/2005/report"
	xmlns:of="urn:oasis:names:tc:opendocument:xmlns:of:1.2"
	xmlns:xhtml="http://www.w3.org/1999/xhtml"
	xmlns:grddl="http://www.w3.org/2003/g/data-view#"
	xmlns:officeooo="http://openoffice.org/2009/office"
	xmlns:tableooo="http://openoffice.org/2009/table"
	xmlns:drawooo="http://openoffice.org/2010/draw"
	xmlns:calcext="urn:org:documentfoundation:names:experimental:calc:xmlns:calcext:1.0"
	xmlns:loext="urn:org:documentfoundation:names:experimental:office:xmlns:loext:1.0"
	xmlns:field="urn:openoffice:names:experimental:ooo-ms-interop:xmlns:field:1.0"
	xmlns:css3t="http://www.w3.org/TR/css3-text/")";

static constexpr auto s_xml_ns_meta = R"(xmlns:office="urn:oasis:names:tc:opendocument:xmlns:office:1.0" xmlns:xlink="http://www.w3.org/1999/xlink" xmlns:dc="http://purl.org/dc/elements/1.1/" xmlns:meta="urn:oasis:names:tc:opendocument:xmlns:meta:1.0" xmlns:ooo="http://openoffice.org/2004/office" xmlns:grddl="http://www.w3.org/2003/g/data-view#" office:version="1.2")";

TextDocument::TextDocument(bool singleDocument)
: Document(DocumentType::Text)
, _content("office:text"/*, Map<String, String>{ stappler::pair("text:use-soft-page-breaks", "true") }*/)
, _singleDocument(singleDocument) {
	if (_singleDocument) {
		addFunctionalFile("content.xml", "text/xml", [this] (const WriteCallback &cb, bool pretty) {
			writeSingleContentFile(cb, pretty);
		});
	} else {
		addFunctionalFile("settings.xml", "text/xml", [this] (const WriteCallback &cb, bool pretty) {
			writeSettingsFile(cb, pretty);
		});
		addFunctionalFile("meta.xml", "text/xml", [this] (const WriteCallback &cb, bool pretty) {
			writeMetaFile(cb, pretty);
		});
		addFunctionalFile("content.xml", "text/xml", [this] (const WriteCallback &cb, bool pretty) {
			writeContentFile(cb, pretty);
		});
		addFunctionalFile("styles.xml", "text/xml", [this] (const WriteCallback &cb, bool pretty) {
			writeStyleFile(cb, pretty);
		});
	}
}

void TextDocument::addFileFont(const StringView &name, Vector<Pair<File, String>> &&val, const StringView &family, Font::Generic gen, Font::Pitch pitch) {
	auto it = _fonts.find(name);
	if (it == _fonts.end()) {
		Vector<Pair<String, String>> files;
		for (auto &it : val) {
			if (std::find_if(_files.begin(), _files.end(), [&] (const File &f) -> bool {
				return f.name == it.first.name;
			}) != _files.end()) {
				stappler::log::vtext("TextDocument", "File already exists: ", it.first.name);
				continue;
			}

			files.emplace_back(it.first.name, it.second);
			_files.emplace_back(std::move(it.first));
		}

		_fonts.emplace(name.str<Interface>(), Font{family.empty() ? String() : family.str<Interface>(), gen, pitch, move(files)});
	} else {
		stappler::log::vtext("TextDocument", "Font already exists: ", name);
	}
}

Node &TextDocument::getContent() {
	return _content;
}

void TextDocument::addFamilyFont(const StringView &name, const StringView &family, Font::Generic gen, Font::Pitch pitch) {
	auto it = _fonts.find(name);
	if (it == _fonts.end()) {
		_fonts.emplace(name.str<Interface>(), Font{family.str<Interface>(), gen, pitch});
	} else {
		stappler::log::vtext("TextDocument", "Font already exists: ", name);
	}
}

void TextDocument::writeSingleContentFile(const WriteCallback &cb, bool pretty) {
	cb << R"(<?xml version="1.0" encoding="UTF-8"?>)" << "\n" << "<office:document " << s_xml_ns << ">";

	writeMeta(cb, pretty);

	cb << R"(<office:settings>
	<config:config-item-set config:name="ooo:configuration-settings">
		<config:config-item config:name="StylesNoDefault" config:type="boolean">true</config:config-item>
		<config:config-item config:name="LoadReadonly" config:type="boolean">true</config:config-item>
		<config:config-item config:name="EmbedFonts" config:type="boolean">true</config:config-item>
		<config:config-item config:name="PrintGraphics" config:type="boolean">true</config:config-item>
	</config:config-item-set>
</office:settings>
)";

	writeFonts(cb, pretty, true);
	writeStyles(cb, pretty, true);
	writeScripts(cb, pretty);

	cb << "\n<office:body>\n";

	writeContentNode(cb, _content, pretty);

	cb << "\n</office:body>\n";
	cb << "</office:document>";
}

void TextDocument::writeContentFile(const WriteCallback &cb, bool pretty) {
	cb << R"(<?xml version="1.0" encoding="UTF-8"?>)" << "\n" << "<office:document-content " << s_xml_ns << ">";

	writeScripts(cb, pretty);
	writeFonts(cb, pretty, true);

	auto stringCb = [strings = &_stringPool] (style::StringId id) -> StringView {
		auto it = strings->find(id);
		if (it != strings->end()) {
			return it->second;
		}
		return StringView();
	};

	if (pretty) { cb << "\n"; }
	if (!_contentStyles.empty()) {
		cb << "<office:automatic-styles>";
		if (pretty) { cb << "\n"; }

		for (auto &it : _contentStyles) {
			if (it->getType() == style::Style::Automatic) {
				it->write(cb, stringCb, pretty);
			}
		}

		cb << "</office:automatic-styles>";
	} else {
		cb << "<office:automatic-styles/>";
	}
	if (pretty) { cb << "\n"; }

	cb << "\n<office:body>\n";

	writeContentNode(cb, _content, pretty);

	cb << "\n</office:body>\n";
	cb << "</office:document-content>";
}

void TextDocument::writeStyleFile(const WriteCallback &cb, bool pretty) {
	cb << R"(<?xml version="1.0" encoding="UTF-8"?>)" << "\n" << "<office:document-styles " << s_xml_ns_styles << ">";

	writeFonts(cb, pretty, false);
	writeStyles(cb, pretty);

	cb << "</office:document-styles>";
}

void TextDocument::writeSettingsFile(const WriteCallback &cb, bool pretty) {
	cb << R"(<?xml version="1.0" encoding="UTF-8"?>)" << "\n" << "<office:document-settings " << s_xml_ns_settings << ">";

	//bool pretty = true;

	cb << R"(<office:settings>
	<config:config-item-set config:name="ooo:configuration-settings">
		<config:config-item config:name="StylesNoDefault" config:type="boolean">true</config:config-item>
		<config:config-item config:name="LoadReadonly" config:type="boolean">true</config:config-item>
		<config:config-item config:name="EmbedFonts" config:type="boolean">true</config:config-item>
		<config:config-item config:name="PrintGraphics" config:type="boolean">true</config:config-item>
	</config:config-item-set>
</office:settings>
)";

	cb << "</office:document-settings>";
}

void TextDocument::writeMetaFile(const WriteCallback &cb, bool pretty) {
	cb << R"(<?xml version="1.0" encoding="UTF-8"?>)" << "\n" << "<office:document-meta " << s_xml_ns_meta << ">";

	writeMeta(cb, pretty);

	cb << "</office:document-meta>";
}

void TextDocument::writeScripts(const WriteCallback &cb, bool pretty) {
	if (pretty) { cb << "\n"; }
	cb << "<office:scripts/>";
	if (pretty) { cb << "\n"; }
}

void TextDocument::writeFonts(const WriteCallback &cb, bool pretty, bool full) {
	if (pretty) { cb << "\n"; }

	cb << "<office:font-face-decls>";
	if (pretty) { cb << "\n"; }

	auto writeFontFaceInfo = [&] (const StringView &name, const Font &font) {
		cb << "<style:font-face style:name=\"" << Escaped(name) << "\"";

		if (!font.family.empty()) {
			cb << " svg:font-family=\"" << Escaped(font.family) << "\"";
		}

		switch (font.generic) {
		case Font::Generic::None: break;
		case Font::Generic::Decorative: cb << " style:font-family-generic=\"decorative\""; break;
		case Font::Generic::Modern: cb << " style:font-family-generic=\"modern\""; break;
		case Font::Generic::Roman: cb << " style:font-family-generic=\"roman\""; break;
		case Font::Generic::Script: cb << " style:font-family-generic=\"script\""; break;
		case Font::Generic::Swiss: cb << " style:font-family-generic=\"swiss\""; break;
		case Font::Generic::System: cb << " style:font-family-generic=\"system\""; break;
		}

		switch (font.pitch) {
		case Font::Pitch::None: break;
		case Font::Pitch::Variable: cb << " style:font-pitch=\"variable\""; break;
		case Font::Pitch::Fixed: cb << " style:font-pitch=\"fixed\""; break;
		}
	};

	for (auto &it : _fonts) {
		if (full && !it.second.files.empty()) {
			if (pretty) { cb << "\t"; }
			writeFontFaceInfo(it.first, it.second);
			cb << "><svg:font-face-src>";
			if (pretty) { cb << "\n"; }

			for (auto &iit : it.second.files) {
				if (pretty) { cb << "\t\t"; }
				cb << "<svg:font-face-uri xlink:href=\"" << Escaped(iit.first) << "\" xlink:type=\"simple\"><svg:font-face-format svg:string=\""
					<< Escaped(iit.second) << "\"/></svg:font-face-uri>";
				if (pretty) { cb << "\n"; }
			}

			if (pretty) { cb << "\t"; }
			cb << "</svg:font-face-src></style:font-face>";
			if (pretty) { cb << "\n"; }
		} else if (!it.second.family.empty()) {
			if (pretty) { cb << "\t"; }
			writeFontFaceInfo(it.first, it.second);
			cb << " />";
			if (pretty) { cb << "\n"; }
		}
	}

	cb << "</office:font-face-decls>";
	if (pretty) { cb << "\n"; }
}

}
