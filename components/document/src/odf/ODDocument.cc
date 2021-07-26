
#include "ODDocument.h"
#include "SPZip.h"

#include <ios>
#include <iostream>

namespace opendocument {

static constexpr auto s_DocumentProducerName = "Stappler/3.0 ODF";

Document::Document(DocumentType t): _type(t) { }
Document::~Document() { }

void Document::setMetaGenerator(const StringView &str) {
	setMeta(Generator, str);
}
StringView Document::getMetaGenerator() const {
	return getMeta(Generator);
}

void Document::setMetaTitle(const StringView &val) {
	setMeta(Title, val);
}
StringView Document::getMetaTitle() const {
	return getMeta(Title);
}

void Document::setMetaDescription(const StringView &val) {
	setMeta(Description, val);
}
StringView Document::getMetaDescription() const {
	return getMeta(Description);
}

void Document::setMetaSubject(const StringView &val) {
	setMeta(Subject, val);
}
StringView Document::getMetaSubject() const {
	return getMeta(Subject);
}

void Document::setMetaKeywords(const StringView &val) {
	setMeta(Keywords, val);
}
StringView Document::getMetaKeywords() const {
	return getMeta(Keywords);
}

void Document::setMetaInitialCreator(const StringView &val) {
	setMeta(InitialCreator, val);
}
StringView Document::getMetaInitialCreator() const {
	return getMeta(InitialCreator);
}

void Document::setMetaCreator(const StringView &val) {
	setMeta(Creator, val);
}
StringView Document::getMetaCreator() const {
	return getMeta(Creator);
}

void Document::setMetaPrintedBy(const StringView &val) {
	setMeta(PrintedBy, val);
}
StringView Document::getMetaPrintedBy() const {
	return getMeta(PrintedBy);
}

void Document::setMetaCreationDate(const Time &val) {
	setMeta(CreationDate, formatMetaDateTime(val));
}
Time Document::getMetaCreationDate() const {
	return Time::fromHttp(getMeta(CreationDate));
}

void Document::setMetaDate(const Time &val) {
	setMeta(Date, formatMetaDateTime(val));
}
Time Document::getMetaDate() const {
	return Time::fromHttp(getMeta(Date));
}

void Document::setMetaPrintDate(const Time &val) {
	setMeta(PrintDate, formatMetaDateTime(val));
}
Time Document::getMetaPrintDate() const {
	return Time::fromHttp(getMeta(PrintDate));
}

void Document::setMetaLanguage(const StringView &val) {
	setMeta(Language, val);
}
StringView Document::getMetaLanguage() const {
	return getMeta(Language);
}

void Document::setUserMetaString(const StringView &key, const StringView &val) {
	setUserMetaType(key, val, MetaFormat::String);
}
void Document::setUserMetaDouble(const StringView &key, double val) {
	setUserMetaType(key, formatMetaDouble(val), MetaFormat::Double);
}
void Document::setUserMetaDateTime(const StringView &key, const Time &val) {
	setUserMetaType(key, formatMetaDateTime(val), MetaFormat::DateTime);
}
void Document::setUserMetaBoolean(const StringView &key, bool val) {
	setUserMetaType(key, formatMetaBoolean(val), MetaFormat::Boolean);
}

StringView Document::getUserMeta(const StringView &key) const {
	auto it = _userMeta.find(key);
	if (it != _userMeta.end()) {
		return it->second.first;
	}
	return StringView();
}

style::Style *Document::getDefaultStyle() {
	if (!_defaultStyle) {
		_defaultStyle = new style::Style(style::Style::Default, style::Style::Paragraph, StringView(), nullptr, [this] (const StringView &str) {
			_stringPool.emplace(style::StringId{stappler::hash::hash32(str.data(), str.size())}, str.str<Interface>());
		}, StringView());
	}
	return _defaultStyle;
}

style::Style *Document::addCommonStyle(const StringView &name, style::Style::Family f, const StringView &cl) {
	auto it = _commonStyles.find(name);
	if (it != _commonStyles.end()) {
		return it->second;
	} else {
		pool::push(_commonStyles.get_allocator());
		auto ret = _commonStyles.emplace(name.str<Interface>(), new style::Style(style::Style::Common, f, name, nullptr, [this] (const StringView &str) {
			_stringPool.emplace(style::StringId{stappler::hash::hash32(str.data(), str.size())}, str.str<Interface>());
		}, cl)).first->second;
		pool::pop();
		return ret;
	}
}

style::Style *Document::addCommonStyle(const StringView &name, const style::Style *parent, const StringView &cl) {
	auto it = _commonStyles.find(name);
	if (it != _commonStyles.end()) {
		return it->second;
	} else {
		pool::push(_commonStyles.get_allocator());
		auto ret = _commonStyles.emplace(name.str<Interface>(), new style::Style(style::Style::Common, parent->getFamily(), name, parent, [this] (const StringView &str) {
			_stringPool.emplace(style::StringId{stappler::hash::hash32(str.data(), str.size())}, str.str<Interface>());
		}, cl)).first->second;
		pool::pop();
		return ret;
	}
}

style::Style *Document::addAutoStyle(style::Style::Family f, const StringView &cl) {
	pool::push(_autoStyles.get_allocator());
	auto name = toString("__Auto", _autoStyleCount ++);
	auto ret = _autoStyles.emplace_back(new style::Style(style::Style::Automatic, f, name, nullptr, [this] (const StringView &str) {
		_stringPool.emplace(style::StringId{stappler::hash::hash32(str.data(), str.size())}, str.str<Interface>());
	}, cl));
	pool::pop();
	return ret;
}

style::Style *Document::addAutoStyle(const style::Style *parent, const StringView &cl) {
	pool::push(_autoStyles.get_allocator());
	auto name = toString("__Auto", _autoStyleCount ++);
	if (parent && parent->getType() != style::Style::Common) {
		std::cout << "Invalid style as parent\n";
	}
	auto ret = _autoStyles.emplace_back(new style::Style(style::Style::Automatic, parent->getFamily(), name, parent, [this] (const StringView &str) {
		_stringPool.emplace(style::StringId{stappler::hash::hash32(str.data(), str.size())}, str.str<Interface>());
	}, cl));
	pool::pop();
	return ret;
}

style::Style *Document::addContentStyle(style::Style::Family f, const StringView &cl) {
	pool::push(_contentStyles.get_allocator());
	auto name = toString("__content", _contentStyleCount ++);
	auto ret = _contentStyles.emplace_back(new style::Style(style::Style::Automatic, f, name, nullptr, [this] (const StringView &str) {
		_stringPool.emplace(style::StringId{stappler::hash::hash32(str.data(), str.size())}, str.str<Interface>());
	}, cl));
	pool::pop();
	return ret;
}

style::Style *Document::addContentStyle(const style::Style *s, const StringView &cl) {
	pool::push(_contentStyles.get_allocator());
	auto name = toString("__content", _contentStyleCount ++);
	auto ret = _contentStyles.emplace_back(new style::Style(style::Style::Automatic, s->getFamily(), name, s, [this] (const StringView &str) {
		_stringPool.emplace(style::StringId{stappler::hash::hash32(str.data(), str.size())}, str.str<Interface>());
	}, cl));
	pool::pop();
	return ret;
}

style::Style *Document::getCommonStyle(const StringView &name) {
	auto it = _commonStyles.find(name);
	if (it != _commonStyles.end()) {
		return it->second;
	}
	return nullptr;
}

MasterPage *Document::addMasterPage(const StringView &name, const style::Style *pageLayout) {
	auto it = _masterPages.find(name);
	if (it != _masterPages.end()) {
		return &it->second;
	} else {
		return &_masterPages.emplace(name.str<Interface>(), name, pageLayout).first->second;
	}
}

void Document::setUserMetaType(const StringView &key, const StringView &val, MetaFormat format) {
	auto it = _userMeta.find(key);
	if (it == _userMeta.end()) {
		_userMeta.emplace(key.str<Interface>(), val.str<Interface>(), format);
	} else {
		it->second.first = val.str<Interface>();
		it->second.second = format;
	}
}

void Document::setMeta(MetaType type, const StringView &val) {
	auto it = _meta.find(type);
	if (it == _meta.end()) {
		_meta.emplace(type, val.str<Interface>());
	} else {
		it->second = val.str<Interface>();
	}
}

StringView Document::getMeta(MetaType type) const {
	auto it = _meta.find(type);
	if (it != _meta.end()) {
		return it->second;
	}
	return StringView();
}

void Document::writeMeta(const Callback<void(const StringView &)> &writeCb, bool pretty) const {
	if (pretty) { writeCb << "\n"; }

	writeCb << "<office:meta>";
	if (pretty) { writeCb << "\n"; }

	bool hasGenerator = false;
	bool hasCreationDate = false;
	bool hasDate = false;

	for (auto &it : _meta) {
		if (pretty) { writeCb << "\t"; }
		switch (it.first) {
		case Generator:
			writeCb << "<meta:generator>" << Escaped(it.second) << "</meta:generator>";
			hasGenerator = true;
			break;
		case Title:
			writeCb << "<dc:title>" << Escaped(it.second) << "</dc:title>";
			break;
		case Description:
			writeCb << "<dc:description>" << Escaped(it.second) << "</dc:description>";
			break;
		case Subject:
			writeCb << "<dc:subject>" << Escaped(it.second) << "</dc:subject>";
			break;
		case Keywords:
			writeCb << "<meta:keyword>" << Escaped(it.second) << "</meta:keyword>";
			break;
		case InitialCreator:
			writeCb << "<meta:initial-creator>" << Escaped(it.second) << "</meta:initial-creator>";
			break;
		case Creator:
			writeCb << "<dc:creator>" << Escaped(it.second) << "</dc:creator>";
			break;
		case PrintedBy:
			writeCb << "<meta:printed-by>" << Escaped(it.second) << "</meta:printed-by>";
			break;
		case CreationDate:
			writeCb << "<meta:creation-date>" << it.second << "</meta:creation-date>";
			hasCreationDate = true;
			break;
		case Date:
			writeCb << "<dc:date>" << it.second << "</dc:date>";
			hasDate = true;
			break;
		case PrintDate:
			writeCb << "<meta:print-date>" << it.second << "</meta:print-date>";
			break;
		case Language:
			writeCb << "<dc:language>" << Escaped(it.second) << "</dc:language>";
			break;
		}
		if (pretty) { writeCb << "\n"; }
	}

	auto now = Time::now();

	if (!hasGenerator) {
		if (pretty) { writeCb << "\t"; }
		writeCb << "<meta:generator>" << Escaped(s_DocumentProducerName) << "</meta:generator>";
		if (pretty) { writeCb << "\n"; }
	}

	if (!hasCreationDate) {
		if (pretty) { writeCb << "\t"; }
		writeCb << "<meta:creation-date>" << now.toIso8601() << "</meta:creation-date>";
		if (pretty) { writeCb << "\n"; }
	}

	if (!hasDate) {
		if (pretty) { writeCb << "\t"; }
		writeCb << "<dc:date>" << now.toIso8601() << "</dc:date>";
		if (pretty) { writeCb << "\n"; }
	}

	for (auto &it : _userMeta) {
		if (pretty) { writeCb << "\t"; }
		writeCb << "<meta:user-defined meta:name=\"" << Escaped(it.first) << "\" meta:value-type=\"";
		switch (it.second.second) {
		case MetaFormat::String: writeCb << "string"; break;
		case MetaFormat::Double: writeCb << "float"; break;
		case MetaFormat::DateTime: writeCb << "time"; break;
		case MetaFormat::Boolean: writeCb << "boolean"; break;
		}
		writeCb << "\">" << it.second.first << "</meta:user-defined>";
		if (pretty) { writeCb << "\n"; }
	}
	writeCb << "</office:meta>";

	if (pretty) {
		writeCb << "\n";
	}
}

static void Document_writeNode(const WriteCallback &cb, const Node *node, bool pretty, size_t depth) {
	auto tag = node->getTag();
	if (tag.empty()) {
		if (pretty) { for (size_t i = 0; i < depth; ++ i) { cb << "\t"; } }
		auto v = node->getValue();
		if (!v.empty()) {
			cb << Escaped(v);
		}
		if (pretty) { cb << "\n"; }
	} else {
		if (pretty) { for (size_t i = 0; i < depth; ++ i) { cb << "\t"; } }
		cb << "<" << tag;

		if (auto style = node->getStyle()) {
			switch (style->getFamily()) {
			case style::Style::Table:
			case style::Style::TableCell:
			case style::Style::TableColumn:
			case style::Style::TableRow:
				cb << " table:style-name=\"" << Escaped(style->getName()) << "\"";
				break;
			case style::Style::Graphic:
				cb << " draw:style-name=\"" << Escaped(style->getName()) << "\"";
				break;
			case style::Style::Note:
				cb << " text:note-class=\"" << Escaped(style->getName()) << "\"";
				break;
			default:
				cb << " text:style-name=\"" << Escaped(style->getName()) << "\"";
				break;
			}
		}

		auto &attr = node->getAttribute();
		for (auto &it : attr) {
			cb << " " << it.first << "=\"" << Escaped(it.second) << "\"";
		}

		auto &nodes = node->getNodes();
		auto v = node->getValue();

		if (nodes.empty() && v.empty()) {
			cb << "/>";
			if (pretty) { cb << "\n"; }
		} else {
			cb << ">";
			if (!v.empty()) {
				cb << Escaped(v) << "</" << tag << ">";
				if (pretty) { cb << "\n"; }
			} else {
				if (pretty) { cb << "\n"; }

				for (auto &it : nodes) {
					Document_writeNode(cb, it, pretty, depth + 1);
				}

				if (pretty) { for (size_t i = 0; i < depth; ++ i) { cb << "\t"; } }
				cb << "</" << tag << ">";
				if (pretty) { cb << "\n"; }
			}
		}
	}
}

void Document::writeStyles(const WriteCallback &cb, bool pretty, bool withContent) const {
	auto stringCb = [strings = &_stringPool] (style::StringId id) -> StringView {
		auto it = strings->find(id);
		if (it != strings->end()) {
			return it->second;
		}
		return StringView();
	};

	if (pretty) { cb << "\n"; }
	if (_defaultStyle || !_commonStyles.empty()) {
		cb << "<office:styles>";
		if (pretty) { cb << "\n"; }

		if (_defaultStyle) {
			_defaultStyle->write(cb, stringCb, pretty);
		}

		for (auto &it : _commonStyles) {
			if (it.second->getType() == style::Style::Common) {
				it.second->write(cb, stringCb, pretty);
			}
		}

		cb << "</office:styles>";
	} else {
		cb << "<office:styles/>";
	}
	if (pretty) { cb << "\n"; }

	if (pretty) { cb << "\n"; }
	if (!_autoStyles.empty() || (withContent && !_contentStyles.empty())) {
		cb << "<office:automatic-styles>";
		if (pretty) { cb << "\n"; }

		for (auto &it : _autoStyles) {
			if (it->getType() == style::Style::Automatic) {
				it->write(cb, stringCb, pretty);
			}
		}

		if (withContent) {
			for (auto &it : _contentStyles) {
				if (it->getType() == style::Style::Automatic) {
					it->write(cb, stringCb, pretty);
				}
			}
		}

		cb << "</office:automatic-styles>";
	} else {
		cb << "<office:automatic-styles/>";
	}
	if (pretty) { cb << "\n"; }

	if (pretty) { cb << "\n"; }
	if (!_masterPages.empty()) {
		cb << "<office:master-styles>";
		if (pretty) { cb << "\n"; }

		for (auto &it : _masterPages) {

			if (pretty) { cb << "\t"; }

			if (it.second.getHeader().empty() && it.second.getFooter().empty()) {
				cb << "<style:master-page style:name=\"" << it.second.getName() << "\"";
				if (auto style = it.second.getPageLayout()) {
					cb << " style:page-layout-name=\"" << style->getName() << "\"";
				}
				cb << "/>";
			} else {
				cb << "<style:master-page style:name=\"" << it.second.getName() << "\"";
				if (auto style = it.second.getPageLayout()) {
					cb << " style:page-layout-name=\"" << style->getName() << "\"";
				}
				cb << ">";
				if (pretty) { cb << "\n"; }

				if (!it.second.getHeader().empty()) {
					Document_writeNode(cb, &it.second.getHeader(), pretty, 2);
				}

				if (!it.second.getFooter().empty()) {
					Document_writeNode(cb, &it.second.getFooter(), pretty, 2);
				}

				if (pretty) { cb << "\t"; }
				cb << "</style:master-page>";
				if (pretty) { cb << "\n"; }
			}

			if (pretty) { cb << "\n"; }
		}
		cb << "</office:master-styles>";
	} else {
		cb << "<office:master-styles/>";
	}
	if (pretty) { cb << "\n"; }
}

void Document::writeContentNode(const WriteCallback &cb, const Node &node, bool pretty) const {
	if (pretty) { cb << "\n"; }

	Document_writeNode(cb, &node, pretty, 0);

	if (pretty) { cb << "\n"; }
}

const File & Document::addTextFile(StringView name, StringView type, StringView data) {
	auto it = std::find_if(_files.begin(), _files.end(), [name] (const File &f) {
		return f.name == name;
	});
	if (it == _files.end()) {
		return _files.emplace_back(File::makeText(name, type, data));
	} else {
		return *it;
	}
}

const File & Document::addBinaryFile(StringView name, StringView type, Bytes &&data) {
	auto it = std::find_if(_files.begin(), _files.end(), [name] (const File &f) {
		return f.name == name;
	});
	if (it == _files.end()) {
		return _files.emplace_back(File::makeBinary(name, type, std::move(data)));
	} else {
		return *it;
	}
}

const File & Document::addFilesystemFile(StringView name, StringView type, StringView path) {
	auto it = std::find_if(_files.begin(), _files.end(), [name] (const File &f) {
		return f.name == name;
	});
	if (it == _files.end()) {
		return _files.emplace_back(File::makeFilesystem(name, type, path));
	} else {
		return *it;
	}
}

const File & Document::addFunctionalFile(StringView name, StringView type, FileReaderCallback &&fn) {
	auto it = std::find_if(_files.begin(), _files.end(), [name] (const File &f) {
		return f.name == name;
	});
	if (it == _files.end()) {
		return _files.emplace_back(File::makeFunctional(name, type, move(fn)));
	} else {
		return *it;
	}
}

const File & Document::addNetworkFile(StringView name, StringView url) {
	auto it = std::find_if(_files.begin(), _files.end(), [name] (const File &f) {
		return f.name == name;
	});
	if (it == _files.end()) {
		return _files.emplace_back(File::makeNetwork(name, url));
	} else {
		return *it;
	}
}

static auto s_manifestBegin = R"(<?xml version="1.0" encoding="UTF-8"?>
<manifest:manifest xmlns:manifest="urn:oasis:names:tc:opendocument:xmlns:manifest:1.0" manifest:version="1.2">
)";

static auto s_manifestEnd = R"(</manifest:manifest>
)";

Buffer Document::save(bool pretty) const {
	stappler::ZipArchive<Interface> archive;

	auto writeData = [&] () -> bool {
		Buffer tmp;

		auto cb = [&] (const StringView &bytes) {
			tmp.put(bytes.data(), bytes.size());
		};

		cb << s_manifestBegin <<"<manifest:file-entry manifest:full-path=\"/\" manifest:version=\"1.2\" manifest:media-type=\"";
		switch (_type) {
		case DocumentType::Text: cb << "application/vnd.oasis.opendocument.text"; break;
		}
		cb << "\"/>\n";

		for (auto &it : _files) {
			cb << "<manifest:file-entry manifest:full-path=\"" << Escaped(it.name) << "\" manifest:media-type=\""
				<< Escaped(it.type) << "\"/>\n";
		}

		cb << s_manifestEnd;
		archive.addFile("META-INF/manifest.xml", tmp.get());
		tmp.clear();

		for (auto &it : _files) {
			switch (it.fileType) {
			case Text:
			case Binary:
				if (!archive.addFile(it.name, BytesView((const uint8_t *)it.data.data(), it.data.size()))) {
					return false;
				}
				break;
			case Filesystem: {
				auto bytes = stappler::filesystem::readIntoMemory(StringView((const char *)it.data.data(), it.data.size() - 1));
				if (!archive.addFile(it.name, BytesView((const uint8_t *)bytes.data(), bytes.size()))) {
					return false;
				}
				break;
			}
			case Functional:
				it.callback([&] (const StringView &bytes) {
					tmp.put(bytes.data(), bytes.size());
				}, pretty);
				archive.addFile(it.name, tmp.get());
				tmp.clear();
				break;
			}
		}
		return true;
	};

	if (archive.addFile("mimetype", "application/vnd.oasis.opendocument.text", true)) {
		if (writeData()) {
			return archive.save();
		}
	}
	return Buffer();
}

}
