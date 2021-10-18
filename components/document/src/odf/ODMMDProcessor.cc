
#include "ODMMDProcessor.h"
#include "MMDEngine.h"
#include "MMDToken.h"
#include "MMDCore.h"

namespace opendocument {

MmdProcessor::Config::Config(Document *doc, Node *n,
		Function<ImageData(const StringView &)> &&img,
		Function<ImageData(const StringView &)> &&video,
		Function<String(const StringView &)> &&link,
		Function<String(const StringView &)> &&id,
		Function<void(Node *, const StringView &)> &&insert,
		Function<void(Node *, const StringView &)> &&math,
		Function<style::Style *(style::Style *, StringView, StringView)> &&cl,
		StringStream *stream)
: document(doc), root(n), onImage(move(img)), onVideo(move(video)), onLink(move(link))
, onId(move(id)), onInsert(move(insert)), onMath(move(math)), onClass(move(cl)), errorStream(stream) { }

void MmdProcessor::run(const Config &cfg, pool_t *pool, const StringView &str, const Value *meta, const Extensions &ext) {
	Engine e; e.init(pool, str, ext);

	e.process([&] (const Content &c, const StringView &s, const Token &t) {
		MmdProcessor p; p.init(cfg, meta);
		p.process(c, s, t);
	});
}

bool MmdProcessor::init(const Config &cfg, const Value *meta) {
	_config = cfg;
	_nodeStack.push_back(_config.root);
	_meta = meta;
	safeMath = true;

	return true;
}

void MmdProcessor::processHtml(const Content &c, const StringView &str, const Token &t) {
	source = str;
	exportTokenTree(buffer, t);
	flushBuffer();
}

void MmdProcessor::exportMath(std::ostream &, token *t) {
	flushBuffer();
	StringStream out;
	exportTokenTreeMath(out, t->child);
	if (_meta) {
		auto &math = _meta->getValue("math");
		if (math && math.isDictionary()) {
			auto val = math.getValue(out.weak());
			if (val.isString()) {
				_config.onMath(_nodeStack.back(), val.getString());
				return;
			}
		}
	}

	(_config.errorStream ? *_config.errorStream : std::cout) << "Invalid math: " << out.weak() << "\n";
}

Node *MmdProcessor::makeNode(const StringView &name, InitList &&attr, VecList &&vec) {
	if (_nodeStack.back() == nullptr) {
		return nullptr;
	}

	if (name == "div" || name == "blockquote") {
		StringView cl;
		for (auto &it : vec) {
			if (it.first == "class") {
				cl = it.second;
			}
		}
		if (!cl.empty() && _config.onClass) {
			auto s = _config.document->getDefaultStyle();
			if (s) {
				s = _config.onClass(s, name, cl);
				if (!s) {
					return nullptr;
				}
			}
		}
		return _nodeStack.back();
	}
	if (name == "p" && attr.size() == 0 && vec.empty()) {
		if (_nodeStack.back()->getTag() == "text:p") {
			return _nodeStack.back();
		}
		return _nodeStack.back()->addNode("text:p", _inFootnote ? getNoteStyle() : getParagraphStyle());
	}
	if (name == "p" && attr.size() == 0 && vec.size() == 1 && vec.front().first == "class") {
		if (_nodeStack.back()->getTag() == "text:p") {
			return _nodeStack.back();
		}

		auto c = _inFootnote ? getNoteStyle() : getParagraphStyle();

		auto cl = vec.front().second;
		if (!cl.empty() && _config.onClass) {
			c = _config.onClass(c, name, cl);
		}

		if (c) {
			return _nodeStack.back()->addNode("text:p", c);
		}
		return nullptr;
	}
	if ((name == "h1" || name == "h2" || name == "h3" || name == "h4" || name == "h5" || name == "h6") && attr.size() <= 1) {
		StringView id;
		for (auto &it : attr) {
			if (it.first == "id") {
				id = it.second;
			}
		}

		StringView cl;
		for (auto &it : vec) {
			if (it.first == "class") {
				cl = it.second;
			}
		}

		auto n = name.sub(1).readInteger().get() + 1 - base_header_level;
		auto c = getHeaderStyle(n);
		if (!cl.empty() && _config.onClass) {
			c = _config.onClass(c, name, cl);
		}

		if (c) {
			if (attr.size() == 1 && attr.begin()->first == "id") {
				auto node = _nodeStack.back()->addNode("text:p", c);
				node->addNode("text:bookmark", { stappler::pair("text:name", _config.onId(id)) });
				return node;
			} else if (attr.size() == 0) {
				return _nodeStack.back()->addNode("text:p", c);
			}
		}
		return nullptr;
	}
	if (name == "ol" && attr.size() <= 1 && vec.empty()) {
		if (attr.size() == 0) {
			return _nodeStack.back()->addNode("text:list", getOrderedListStyle());
		} else {
			for (auto &it : attr) {
				if (it.first == "start") {
					return &_nodeStack.back()->addNode("text:list", getOrderedListStyle())->setNote("text:start-value", it.second);
				}
			}
		}
	}
	if (name == "ul" && attr.size() == 0 && vec.empty()) {
		return _nodeStack.back()->addNode("text:list", getUnorderedListStyle());
	}
	if (name == "li" && attr.size() == 0 && vec.empty()) {
		auto start = _nodeStack.back()->popNote("text:start-value");
		if (!start.empty()) {
			return _nodeStack.back()->addNode("text:list-item", {
					stappler::pair("text:start-value", start)
			})->addNode("text:p", getParagraphStyle());
		} else {
			return _nodeStack.back()->addNode("text:list-item")->addNode("text:p", getParagraphStyle());
		}
	}
	if ((name == "em" || name == "i") && attr.size() == 0 && vec.empty()) {
		return _nodeStack.back()->addNode("text:span", getEmStyle());
	}
	if ((name == "strong" || name == "b") && attr.size() == 0 && vec.empty()) {
		return _nodeStack.back()->addNode("text:span", getStrongStyle());
	}
	if (name == "span" && attr.size() == 0) {
		StringView cl;
		StringView id;
		for (auto &it : vec) {
			if (it.first == "class") {
				cl = it.second;
			} else if (it.first == "id") {
				id = it.second;
			}
		}

		if (cl == "anchor") {
			return _nodeStack.back()->addNode("text:bookmark", { stappler::pair("text:name", _config.onId(id)) });
		}
	}
	if (name == "a") {
		if (auto node = makeLinkNode(name, std::move(attr), std::move(vec))) {
			return node;
		}
	}
	if (name == "figure") {
		auto p = _nodeStack.back();
		if (p->getTag() == "text:p") {
			return p->addNode("draw:frame", {
				stappler::pair("draw:z-index", "0"),
				//stappler::pair("svg:width", "100%")
				stappler::pair("svg:width", "190.0mm")
			}, getFigureStyle())->addNode("draw:text-box")->addNode("text:p", getFigureContentStyle());
		} else {
			return p->addNode("text:p")->addNode("draw:frame", {
				stappler::pair("draw:z-index", "0"),
				//stappler::pair("svg:width", "100%")
				stappler::pair("svg:width", "190.0mm")
			}, getFigureStyle())->addNode("draw:text-box")->addNode("text:p", getFigureContentStyle());
		}
	}
	if (name == "img") {
		if (auto node = makeImageNode(name, std::move(attr), std::move(vec))) {
			return node;
		}
	}
	if (name == "figcaption") {
		if (_config.videoAlt) {
			StringView type;
			StringView url;

			for (auto &it : attr) {
				if (it.first == "data-type") {
					type = it.second;
				} else if (it.first == "data-src") {
					url = it.second;
				}
			}

			if (type == "video" && !url.empty()) {
				auto ret = _nodeStack.back()->addNode("text:span", getFigureLabelStyle())->addNode("text:a", {
					stappler::pair("xlink:href", url.str<Interface>()),
					stappler::pair("xlink:type", "simple")
				}, getLinkStyle());
				ret->addNode("text:line-break");
				return ret;
			} else {
				auto ret = _nodeStack.back()->addNode("text:span", getFigureLabelStyle());
				ret->addNode("text:line-break");
				return ret;
			}
		} else {
			auto ret = _nodeStack.back()->addNode("text:span", getFigureLabelStyle());
			ret->addNode("text:line-break");
			return ret;
		}
	}

	if (name == "table") {
		return _nodeStack.back()->addNode("table:table", { stappler::pair("table:name", "Таблица1") }, getTableStyle());
	} else if (name == "colgroup") {
		//return _nodeStack.back()->addNode("table:table-columns");
		return  _nodeStack.back();
	} else if (name == "col") {
		return _nodeStack.back()->addNode("table:table-column");
	} else if (name == "thead") {
		//return _nodeStack.back()->addNode("table:table-rows");
		return _nodeStack.back();
	} else if (name == "tbody") {
		//return _nodeStack.back()->addNode("table:table-rows");
		return _nodeStack.back();
	} else if (name == "tr") {
		return _nodeStack.back()->addNode("table:table-row", getTableRowStyle());
	} else if (name == "th") {
		return _nodeStack.back()->addNode("table:table-cell", { stappler::pair("office:value-type", "string") }, getTableHeaderCellStyle())->addNode("text:p");
	} else if (name == "td") {
		return _nodeStack.back()->addNode("table:table-cell", { stappler::pair("office:value-type", "string") },
				getTableBodyCellStyle(_nodeStack.at(_nodeStack.size() - 2)->getNodes().size()))->addNode("text:p");
	} else if (name == "br") {
		return _nodeStack.back()->addNode("text:line-break");
	} else if (name == "caption") {
		if (_nodeStack.back()->getTag() == "table:table") {
			StringView id;
			StringView style;
			for (auto &it : attr) {
				if (it.first == "id") {
					id = it.second;
				} else if (it.first == "style") {
					style = it.second;
				}
			}

			if (style.find("caption-side: bottom;") != stappler::maxOf<size_t>()) {
				auto node = _nodeStack.at(_nodeStack.size() - 2)->addNode("text:p", getTableCaptionStyle());
				if (!id.empty()) {
					node->addNode("text:bookmark", { stappler::pair("text:name", _config.onId(id)) });
				}
				return node;
			}
		}
	}

	auto &stream = (_config.errorStream ? *_config.errorStream : std::cout);

	stream << name;
	stream << " ATTR ";
	for (auto &it : attr) {
		stream << " " << it.first << "=" << it.second;
	}
	stream << " VEC ";
	for (auto &it : vec) {
		stream << " " << it.first << "=" << it.second;
	}

	stream << "\n";

	return &_tmpNode;
}

Node *MmdProcessor::makeLinkNode(const StringView &name, InitList &&attr, VecList &&vec) {
	StringView href;
	StringView type;

	for (auto &it : vec) {
		if (it.first == "href") {
			href = it.second;
		} else if (it.first == "type") {
			type = it.second;
		}
	}

	for (auto &it : attr) {
		if (it.first == "href") {
			href = it.second;
		}
	}

	if (href.starts_with("(") && href.ends_with(")")) {
		href.trimChars<StringView::Chars<'(', ')'>>();
	}

	if (type == "insert") {
		_config.onInsert(_nodeStack.back(), href);
		return &_tmpNode;
	} else if (href.starts_with("http://") || href.starts_with("https://")) {
		auto link = _config.onLink(href);
		if (!link.empty()) {
			return _nodeStack.back()->addNode("text:a", {
				stappler::pair("xlink:href", link),
				stappler::pair("xlink:type", "simple")
			}, getLinkStyle());
		} else {
			return nullptr;
		}
	}

	if (!href.empty() && attr.size() == 0 && vec.size() == 1) {
		auto link = _config.onLink(href);
		if (!link.empty()) {
			return _nodeStack.back()->addNode("text:a", {
				stappler::pair("xlink:href", link),
				stappler::pair("xlink:type", "simple")
			}, getLinkStyle());
		}
	}
	return nullptr;
}

Node *MmdProcessor::makeImageNode(const StringView &name, InitList &&attr, VecList &&vec) {
	String type;
	String src;
	for (auto &it : vec) {
		if (it.first == "type") {
			type = it.second.str<Interface>();
		}
		if (it.first == "src") {
			src = it.second.str<Interface>();
		}
	}
	if (type == "image") {
		if (auto img = _config.onImage(src)) {
			float width = 150.0f;
			float height = 100.0f;

			float scale = std::min(width / img.width, height / img.height);

			if (_config.mimeExt) {
				return _nodeStack.back()->addNode("draw:frame", {
					stappler::pair("draw:z-index", "1"),
					stappler::pair("svg:width", toString(img.width * scale, "mm")),
					stappler::pair("svg:height", toString(img.height * scale, "mm")),
					stappler::pair("text:anchor-type" ,"frame")
				}, getImageStyle())->addNode("draw:image", {
					stappler::pair("xlink:href", img.name),
					stappler::pair("loext:mime-type", img.type),
					stappler::pair("xlink:type", "simple")
				});
			} else {
				return _nodeStack.back()->addNode("draw:frame", {
					stappler::pair("draw:z-index", "1"),
					stappler::pair("svg:width", toString(img.width * scale, "mm")),
					stappler::pair("svg:height", toString(img.height * scale, "mm")),
					stappler::pair("text:anchor-type" ,"frame")
				}, getImageStyle())->addNode("draw:image", {
					stappler::pair("xlink:href", img.name),
					stappler::pair("xlink:type", "simple")
				});
			}
		}
	} else if (type == "video" || StringView(src).starts_with("https://www.youtube.com") || StringView(src).starts_with("https://youtube.com")) {
		if (auto img = _config.onVideo(src)) {
			float width = 150.0f;
			float height = 100.0f;

			float scale = std::min(width / img.width, height / img.height);

			auto wStr = toString(img.width * scale, "mm");
			auto hStr = toString(img.height * scale, "mm");

			StringView url(src);
			if (url.starts_with("(") && url.ends_with(")")) {
				url.trimChars<StringView::Chars<'(', ')'>>();
			}

			auto ref = _nodeStack.back()->addNode("draw:a", {
				stappler::pair("xlink:href", url.str<Interface>()),
				stappler::pair("xlink:type", "simple")
			});

			Node *imageNode = nullptr;
			if (_config.videoAlt) {
				imageNode = ref->addNode("draw:frame", {
					stappler::pair("draw:z-index", "1"),
					stappler::pair("svg:width", wStr),
					stappler::pair("svg:height", hStr),
					stappler::pair("text:anchor-type" ,"paragraph")
				}, getImageStyle())->addNode("draw:image", {
					stappler::pair("xlink:href", img.name),
					stappler::pair("loext:mime-type", img.type),
					stappler::pair("xlink:type", "simple")
				});
			} else {
				if (_config.mimeExt) {
					imageNode = ref->addNode("draw:frame", {
						stappler::pair("draw:z-index", "1"),
						stappler::pair("svg:width", wStr),
						stappler::pair("svg:height", hStr),
						stappler::pair("text:anchor-type" ,"paragraph")
					}, getImageStyle())->addNode("draw:image", {
						stappler::pair("xlink:href", img.name),
						stappler::pair("loext:mime-type", img.type),
						stappler::pair("xlink:type", "simple")
					});
				} else {
					imageNode = ref->addNode("draw:frame", {
						stappler::pair("draw:z-index", "1"),
						stappler::pair("svg:width", wStr),
						stappler::pair("svg:height", hStr),
						stappler::pair("text:anchor-type" ,"paragraph")
					}, getImageStyle())->addNode("draw:image", {
						stappler::pair("xlink:href", img.name),
						stappler::pair("xlink:type", "simple")
					});
				}

				float iconScale = 20.0f / 1024.0f;
				float iconWidth = 1024.0f * iconScale;
				float iconHeight = 721.0f * iconScale;
				float iconY = (img.height * scale - iconHeight) / 2.0f;
				float iconX = (img.width * scale - iconWidth) / 2.0f;

				if (_config.mimeExt) {
					_nodeStack.back()->addNode("draw:a", {
						stappler::pair("xlink:href", src),
						stappler::pair("xlink:type", "simple")
					})->addNode("draw:frame", {
						stappler::pair("draw:z-index", "2"),
						stappler::pair("svg:width", toString(iconWidth, "mm")),
						stappler::pair("svg:height", toString(iconHeight, "mm")),
						stappler::pair("svg:x", toString(iconX, "mm")),
						stappler::pair("svg:y", toString(iconY, "mm")),
						stappler::pair("text:anchor-type" ,"paragraph")
					}, getImageStyle())->addNode("draw:image", {
						stappler::pair("xlink:href", "Pictures/youtube.svg"),
						stappler::pair("loext:mime-type", "image/svg+xml"),
						stappler::pair("xlink:type", "simple")
					});
				} else {
					_nodeStack.back()->addNode("draw:a", {
						stappler::pair("xlink:href", src),
						stappler::pair("xlink:type", "simple")
					})->addNode("draw:frame", {
						stappler::pair("draw:z-index", "2"),
						stappler::pair("svg:width", toString(iconWidth, "mm")),
						stappler::pair("svg:height", toString(iconHeight, "mm")),
						stappler::pair("svg:x", toString(iconX, "mm")),
						stappler::pair("svg:y", toString(iconY, "mm")),
						stappler::pair("text:anchor-type" ,"paragraph")
					}, getImageStyle())->addNode("draw:image", {
						stappler::pair("xlink:href", "Pictures/youtube.svg"),
						stappler::pair("xlink:type", "simple")
					});
				}
			}
			return imageNode;
		} else {
			(_config.errorStream ? *_config.errorStream : std::cout) << "Invalid url:" << src << "\n";
			return &_tmpNode;
		}
	} else {
		(_config.errorStream ? *_config.errorStream : std::cout) << "Unknown type:" << type << " " << src << "\n";
	}
	return nullptr;
}

void MmdProcessor::pushNode(token *, const StringView &name, InitList &&attr, VecList &&vec) {
	flushBuffer();
	auto node = makeNode(name, move(attr), move(vec));
	_nodeStack.push_back(node);
}

void MmdProcessor::pushInlineNode(token *, const StringView &name, InitList &&attr, VecList &&vec) {
	flushBuffer();
	makeNode(name, move(attr), move(vec));
}

void MmdProcessor::popNode() {
	flushBuffer();
	_nodeStack.pop_back();
}

void MmdProcessor::flushBuffer() {
	if (_nodeStack.back()) {
		auto str = buffer.str();
		StringView r(str);
		if (!r.empty()) {
			r.skipChars<StringView::CharGroup<stappler::CharGroupId::WhiteSpace>>();
			if (!r.empty()) {
				r = StringView(str);
				if (r.back() == '\n') {
					size_t s = r.size();
					while (s > 0 && (r[s - 1] == '\n' || r[s - 1] == '\r')) {
						-- s;
					}
					r = StringView(r.data(), s);
				}
				_nodeStack.back()->addContent(r);
			}
		}
	}
	buffer.clear();
}


void MmdProcessor::exportPairBracketCitation(std::ostream &out, token *t) {
	auto temp_note = parseFootnoteBracket(t);
	auto temp_short = temp_note->count;

	if (!temp_note) {
		out << "[^";
		exportTokenTree(out, t->child->next);
		out << "]";
		return;
	}

	auto s = _config.document->getCommonStyle("footnote");
	if (!s) {
		s = &_config.document->addCommonStyle("footnote", style::Style::Note)
			->set<style::Name::NoteStartNumberingAt>(style::StartNumberingAt::Page)
			.set<style::Name::NoteFootnotesPosition>(style::FootnotesPosition::Page)
			.set<style::Name::NoteNumFormat>(StringView("1"))
			.set<style::Name::NoteNumSuffix>(StringView("."));
	}

	flushBuffer();
	auto note = _nodeStack.back()->addNode("text:note", s);
	char str = 'a' + temp_short - 1;
	note->addNode("text:note-citation")->addContent(StringView(&str, 1));
	auto body = note->addNode("text:note-body");

	_inFootnote = true;
	_nodeStack.push_back(body);
	exportTokenTree(out, temp_note->content);
	flushBuffer();
	_nodeStack.pop_back();
	_inFootnote = false;
}

void MmdProcessor::exportPairBracketFootnote(std::ostream &out, token *t) {
	auto temp_note = parseFootnoteBracket(t);
	auto temp_short = temp_note->count;

	if (!temp_note) {
		out << "[^";
		exportTokenTree(out, t->child->next);
		out << "]";
		return;
	}

	auto s = _config.document->getCommonStyle("footnote");
	if (!s) {
		s = &_config.document->addCommonStyle("footnote", style::Style::Note)
			->set<style::Name::NoteStartNumberingAt>(style::StartNumberingAt::Page)
			.set<style::Name::NoteFootnotesPosition>(style::FootnotesPosition::Page)
			.set<style::Name::NoteNumFormat>(StringView("1"))
			.set<style::Name::NoteNumSuffix>(StringView("."));
	}

	flushBuffer();
	auto note = _nodeStack.back()->addNode("text:note", s);
	note->addNode("text:note-citation")->addContent(toString(temp_short));
	auto body = note->addNode("text:note-body");

	_inFootnote = true;
	_nodeStack.push_back(body);
	exportTokenTree(out, temp_note->content);
	flushBuffer();
	_nodeStack.pop_back();
	_inFootnote = false;
}

style::Style *MmdProcessor::getParagraphStyle() {
	auto s = _config.document->getCommonStyle("MMDParagraph");
	if (!s) {
		s = &_config.document->addCommonStyle("MMDParagraph", style::Style::Paragraph)
			->set<style::Name::ParagraphMarginTop>(1.0_mm)
			.set<style::Name::ParagraphMarginBottom>(1.0_mm)
			.set<style::Name::ParagraphLineHeightAtLeast>(17_pt)
			.set<style::Name::TextHyphenate>(true)
			.set<style::Name::ParagraphTextAlign>(style::TextAlign::Justify);
	}
	return s;
}

style::Style *MmdProcessor::getHeaderStyle(size_t idx) {
	auto name = toString("MMDHeader", idx);
	auto s = _config.document->getCommonStyle(name);
	if (!s) {
		s = &_config.document->addCommonStyle(name, style::Style::Paragraph)
			->set<style::Name::ParagraphMarginTop>(6_pt)
			.set<style::Name::ParagraphMarginBottom>(3_pt)
			.set<style::Name::ParagraphLineHeight>(19_pt)
			.set<style::Name::ParagraphKeepWithNext>(style::KeepMode::Always);

		switch (idx) {
		case 1:
			s->set<style::Name::TextFontSize>(15.0_pt)
				.set<style::Name::TextFontWeight>(style::FontWeight::W600)
				.set<style::Name::TextColor>(style::Color::Grey_700.asColor3B());
			break;
		case 2:
			s->set<style::Name::TextFontSize>(14.0_pt)
				.set<style::Name::TextFontWeight>(style::FontWeight::W600)
				.set<style::Name::TextColor>(style::Color::Grey_700.asColor3B());
			break;
		case 3:
		case 4:
		case 5:
		case 6:
			s->set<style::Name::TextFontSize>(14.0_pt)
				.set<style::Name::TextFontWeight>(style::FontWeight::W500)
				.set<style::Name::TextColor>(style::Color::Grey_700.asColor3B());
			break;
		}
	}
	return s;
}

style::Style *MmdProcessor::getNoteStyle() {
	auto s = _config.document->getCommonStyle("MMDNote");
	if (!s) {
		s = &_config.document->addCommonStyle("MMDNote", style::Style::Paragraph)
			->set<style::Name::ParagraphLineHeight>(15_pt)
			.set<style::Name::TextHyphenate>(true)
			.set<style::Name::TextFontSize>(11_pt)
			.set<style::Name::ParagraphTextAlign>(style::TextAlign::Justify)
			.set<style::Name::ParagraphMarginLeft>(7_mm)
			.set<style::Name::ParagraphTextIndent>(-7_mm);
	}
	return s;
}

style::Style *MmdProcessor::getEmStyle() {
	auto s = _config.document->getCommonStyle("MMDSpanEm");
	if (!s) {
		s = &_config.document->addCommonStyle("MMDSpanEm", style::Style::Text)
			->set<style::Name::TextFontStyle>(style::FontStyle::Italic);
	}
	return s;
}

style::Style *MmdProcessor::getStrongStyle() {
	auto s = _config.document->getCommonStyle("MMDSpanStrong");
	if (!s) {
		s = &_config.document->addCommonStyle("MMDSpanStrong", style::Style::Text)
			->set<style::Name::TextFontWeight>(style::FontWeight::W600);
	}
	return s;
}

style::Style *MmdProcessor::getOrderedListStyle() {
	auto s = _config.document->getCommonStyle("MMDOrderedList");
	if (!s) {
		s = _config.document->addCommonStyle("MMDOrderedList", style::Style::List);
		s->addListLevel(style::Style::ListLevel::Number)
			.set<style::Name::ListStyleNumSuffix>(StringView("."))
			.set<style::Name::ListStyleNumFormat>(StringView("1"))
			.set<style::Name::ListLevelPositionMode>(style::ListLevelPositionMode::LabelAlignment)
			.set<style::Name::ListLevelLabelFollowedBy>(style::LabelFollowedBy::Listtab)
			.set<style::Name::ListLevelLabelTabStopPosition>(10_mm)
			.set<style::Name::ListLevelLabelTextIndent>(-7_mm)
			.set<style::Name::ListLevelLabelMarginLeft>(10_mm);

		s->addListLevel(style::Style::ListLevel::Number)
			.set<style::Name::ListStyleNumSuffix>(StringView("."))
			.set<style::Name::ListStyleNumFormat>(StringView("1"))
			.set<style::Name::ListLevelPositionMode>(style::ListLevelPositionMode::LabelAlignment)
			.set<style::Name::ListLevelLabelFollowedBy>(style::LabelFollowedBy::Listtab)
			.set<style::Name::ListLevelLabelTabStopPosition>(15_mm)
			.set<style::Name::ListLevelLabelTextIndent>(-7_mm)
			.set<style::Name::ListLevelLabelMarginLeft>(15_in);

		s->addListLevel(style::Style::ListLevel::Number)
			.set<style::Name::ListStyleNumSuffix>(StringView("."))
			.set<style::Name::ListStyleNumFormat>(StringView("1"))
			.set<style::Name::ListLevelPositionMode>(style::ListLevelPositionMode::LabelAlignment)
			.set<style::Name::ListLevelLabelFollowedBy>(style::LabelFollowedBy::Listtab)
			.set<style::Name::ListLevelLabelTabStopPosition>(20_mm)
			.set<style::Name::ListLevelLabelTextIndent>(-7_mm)
			.set<style::Name::ListLevelLabelMarginLeft>(20_in);

		s->addListLevel(style::Style::ListLevel::Number)
			.set<style::Name::ListStyleNumSuffix>(StringView("."))
			.set<style::Name::ListStyleNumFormat>(StringView("1"))
			.set<style::Name::ListLevelPositionMode>(style::ListLevelPositionMode::LabelAlignment)
			.set<style::Name::ListLevelLabelFollowedBy>(style::LabelFollowedBy::Listtab)
			.set<style::Name::ListLevelLabelTabStopPosition>(25_mm)
			.set<style::Name::ListLevelLabelTextIndent>(-7_mm)
			.set<style::Name::ListLevelLabelMarginLeft>(25_mm);

		s->addListLevel(style::Style::ListLevel::Number)
			.set<style::Name::ListStyleNumSuffix>(StringView("."))
			.set<style::Name::ListStyleNumFormat>(StringView("1"))
			.set<style::Name::ListLevelPositionMode>(style::ListLevelPositionMode::LabelAlignment)
			.set<style::Name::ListLevelLabelFollowedBy>(style::LabelFollowedBy::Listtab)
			.set<style::Name::ListLevelLabelTabStopPosition>(30_in)
			.set<style::Name::ListLevelLabelTextIndent>(-7_mm)
			.set<style::Name::ListLevelLabelMarginLeft>(30_in);

		s->addListLevel(style::Style::ListLevel::Number)
			.set<style::Name::ListStyleNumSuffix>(StringView("."))
			.set<style::Name::ListStyleNumFormat>(StringView("1"))
			.set<style::Name::ListLevelPositionMode>(style::ListLevelPositionMode::LabelAlignment)
			.set<style::Name::ListLevelLabelFollowedBy>(style::LabelFollowedBy::Listtab)
			.set<style::Name::ListLevelLabelTabStopPosition>(35_in)
			.set<style::Name::ListLevelLabelTextIndent>(-7_mm)
			.set<style::Name::ListLevelLabelMarginLeft>(35_in);
	}
	return s;
}

style::Style *MmdProcessor::getUnorderedListStyle() {
	auto s = _config.document->getCommonStyle("MMDUnorderedList");
	if (!s) {
		s = _config.document->addCommonStyle("MMDUnorderedList", style::Style::List);
		s->addListLevel(style::Style::ListLevel::Bullet)
			.set<style::Name::ListStyleNumSuffix>(StringView("."))
			.set<style::Name::ListStyleBulletChar>(StringView("•"))
			.set<style::Name::ListStyleBulletRelativeSize>(1.2f)
			.set<style::Name::ListLevelPositionMode>(style::ListLevelPositionMode::LabelAlignment)
			.set<style::Name::ListLevelLabelFollowedBy>(style::LabelFollowedBy::Listtab)
			.set<style::Name::ListLevelLabelTabStopPosition>(10_mm)
			.set<style::Name::ListLevelLabelTextIndent>(-7_mm)
			.set<style::Name::ListLevelLabelMarginLeft>(10_mm);

		s->addListLevel(style::Style::ListLevel::Bullet)
			.set<style::Name::ListStyleNumSuffix>(StringView("."))
			.set<style::Name::ListStyleBulletChar>(StringView("○"))
			.set<style::Name::ListLevelPositionMode>(style::ListLevelPositionMode::LabelAlignment)
			.set<style::Name::ListLevelLabelFollowedBy>(style::LabelFollowedBy::Listtab)
			.set<style::Name::ListLevelLabelTabStopPosition>(15_mm)
			.set<style::Name::ListLevelLabelTextIndent>(-7_mm)
			.set<style::Name::ListLevelLabelMarginLeft>(15_in);

		s->addListLevel(style::Style::ListLevel::Bullet)
			.set<style::Name::ListStyleNumSuffix>(StringView("."))
			.set<style::Name::ListStyleBulletChar>(StringView("■"))
			.set<style::Name::ListLevelPositionMode>(style::ListLevelPositionMode::LabelAlignment)
			.set<style::Name::ListLevelLabelFollowedBy>(style::LabelFollowedBy::Listtab)
			.set<style::Name::ListLevelLabelTabStopPosition>(20_mm)
			.set<style::Name::ListLevelLabelTextIndent>(-7_mm)
			.set<style::Name::ListLevelLabelMarginLeft>(20_in);

		s->addListLevel(style::Style::ListLevel::Bullet)
			.set<style::Name::ListStyleNumSuffix>(StringView("."))
			.set<style::Name::ListStyleBulletChar>(StringView("□"))
			.set<style::Name::ListStyleBulletRelativeSize>(0.8f)
			.set<style::Name::ListLevelPositionMode>(style::ListLevelPositionMode::LabelAlignment)
			.set<style::Name::ListLevelLabelFollowedBy>(style::LabelFollowedBy::Listtab)
			.set<style::Name::ListLevelLabelTabStopPosition>(25_mm)
			.set<style::Name::ListLevelLabelTextIndent>(-7_mm)
			.set<style::Name::ListLevelLabelMarginLeft>(25_mm);

		s->addListLevel(style::Style::ListLevel::Bullet)
			.set<style::Name::ListStyleNumSuffix>(StringView("."))
			.set<style::Name::ListStyleBulletChar>(StringView("▶"))
			.set<style::Name::ListLevelPositionMode>(style::ListLevelPositionMode::LabelAlignment)
			.set<style::Name::ListLevelLabelFollowedBy>(style::LabelFollowedBy::Listtab)
			.set<style::Name::ListLevelLabelTabStopPosition>(30_in)
			.set<style::Name::ListLevelLabelTextIndent>(-7_mm)
			.set<style::Name::ListLevelLabelMarginLeft>(30_in);

		s->addListLevel(style::Style::ListLevel::Bullet)
			.set<style::Name::ListStyleNumSuffix>(StringView("."))
			.set<style::Name::ListStyleBulletChar>(StringView("▷"))
			.set<style::Name::ListLevelPositionMode>(style::ListLevelPositionMode::LabelAlignment)
			.set<style::Name::ListLevelLabelFollowedBy>(style::LabelFollowedBy::Listtab)
			.set<style::Name::ListLevelLabelTabStopPosition>(35_in)
			.set<style::Name::ListLevelLabelTextIndent>(-7_mm)
			.set<style::Name::ListLevelLabelMarginLeft>(35_in);
	}
	return s;
}

style::Style *MmdProcessor::getFigureStyle() {
	auto s = _config.document->getCommonStyle("MMDFigure");
	if (!s) {
		s = &_config.document->addCommonStyle("MMDFigure", style::Style::Graphic)
			->set<style::Name::DrawMargin>(0_mm)
			.set<style::Name::DrawHorizontalPos>(style::HorizontalPos::Center)
			.set<style::Name::DrawHorizontalRel>(style::HorizontalRel::Paragraph)
			.set<style::Name::DrawWrap>(style::Wrap::None)
			.set<style::Name::DrawAnchorType>(style::AnchorType::Paragraph)
			.set<style::Name::DrawPadding>(1.5_mm);
	}
	return s;
}

style::Style *MmdProcessor::getFigureContentStyle() {
	auto s = _config.document->getCommonStyle("MMDFigureContent");
	if (!s) {
		s = &_config.document->addCommonStyle("MMDFigureContent", style::Style::Paragraph)
			->set<style::Name::ParagraphTextAlign>(style::TextAlign::Center);
	}
	return s;
}

style::Style *MmdProcessor::getFigureLabelStyle() {
	auto s = _config.document->getCommonStyle("MMDFigureLabel");
	if (!s) {
		s = &_config.document->addCommonStyle("MMDFigureLabel", style::Style::Text)
			->set<style::Name::TextFontSize>(12_pt)
			.set<style::Name::TextFontStyle>(style::FontStyle::Italic)
			.set<style::Name::TextColor>(style::Color::Grey_800.asColor3B());
	}
	return s;
}

style::Style *MmdProcessor::getImageStyle() {
	auto s = _config.document->getCommonStyle("MMDImage");
	if (!s) {
		s = &_config.document->addCommonStyle("MMDImage", style::Style::Graphic)
			->set<style::Name::DrawAnchorType>(style::AnchorType::Paragraph)
			.set<style::Name::DrawWrap>(style::Wrap::None)
			.set<style::Name::DrawHorizontalPos>(style::HorizontalPos::Center)
			.set<style::Name::DrawHorizontalRel>(style::HorizontalRel::Paragraph)
			.set<style::Name::DrawVerticalPos>(style::VerticalPos::FromTop)
			.set<style::Name::DrawVerticalRel>(style::VerticalRel::Baseline)
			.set<style::Name::DrawColorMode>(style::ColorMode::Standard);
	}
	return s;
}

style::Style *MmdProcessor::getLinkStyle() {
	auto s = _config.document->getCommonStyle("MMDLink");
	if (!s) {
		s = &_config.document->addCommonStyle("MMDLink", style::Style::Text)
			->set<style::Name::TextColor>(style::Color::Teal_500.asColor3B())
			.set<style::Name::TextUnderlineMode>(style::LineMode::Continuous)
			.set<style::Name::TextUnderlineStyle>(style::LineStyle::Solid)
			.set<style::Name::TextUnderlineType>(style::LineType::Single);
	}

	return s;
}

style::Style *MmdProcessor::getTableStyle() {
	if (!_tableStyle) {
		_tableStyle = &_config.document->addContentStyle(style::Style::Table)
			->set<style::Name::TableMarginTop>(3_mm)
			.set<style::Name::TableMarginBottom>(3_mm)
			.set<style::Name::TableAlign>(style::TableAlign::Margins)
			.set<style::Name::TableBorderModel>(style::BorderModel::Collapsing);
	}
	return _tableStyle;
}
style::Style *MmdProcessor::getTableRowStyle() {
	if (!_rowStyle) {
		_rowStyle = &_config.document->addContentStyle(style::Style::TableRow)
			->set<style::Name::TableRowKeepTogether>(style::KeepMode::Always);
	}
	return _rowStyle;
}
style::Style *MmdProcessor::getTableHeaderCellStyle() {
	if (!_headerStyle) {
		_headerStyle = &_config.document->addContentStyle(style::Style::TableCell)
			->set<style::Name::TableCellBackgroundColor>(style::Color::getColorByName(StringView("#97b05a")).asColor3B())
			.set<style::Name::TableCellBorder>(style::Border{0.5_mm, style::LineStyle::Solid, style::Color3B::WHITE})
			.set<style::Name::TableCellPadding>(1.5_mm)
			.set<style::Name::TextColor>(style::Color3B::WHITE)
			.set<style::Name::TextFontWeight>(style::FontWeight::W600)
			.set<style::Name::ParagraphTextAlign>(style::TextAlign::Center);
	}
	return _headerStyle;
}
style::Style *MmdProcessor::getTableBodyCellStyle(size_t n) {
	if (n % 2 == 0) {
		if (!_cellStyle2) {
			_cellStyle2 = &_config.document->addContentStyle(style::Style::TableCell)
				->set<style::Name::TableCellBackgroundColor>(style::Color::getColorByName(StringView("#d9e0ed")).asColor3B())
				.set<style::Name::TableCellBorder>(style::Border{0.5_mm, style::LineStyle::Solid, style::Color3B::WHITE})
				.set<style::Name::TableCellPadding>(1.5_mm);
		}
		return _cellStyle2;
	} else {
		if (!_cellStyle1) {
			_cellStyle1 = &_config.document->addContentStyle(style::Style::TableCell)
				->set<style::Name::TableCellBackgroundColor>(style::Color::getColorByName(StringView("#e5eaf2")).asColor3B())
				.set<style::Name::TableCellBorder>(style::Border{0.5_mm, style::LineStyle::Solid, style::Color3B::WHITE})
				.set<style::Name::TableCellPadding>(1.5_mm);
		}
		return _cellStyle1;
	}
}

style::Style *MmdProcessor::getTableCaptionStyle() {
	auto s = _config.document->getCommonStyle("MMDTableCaption");
	if (!s) {
		s = &_config.document->addCommonStyle("MMDTableCaption", style::Style::Paragraph)
			->set<style::Name::ParagraphTextAlign>(style::TextAlign::Center)
			.set<style::Name::TextFontStyle>(style::FontStyle::Italic)
			.set<style::Name::ParagraphMarginBottom>(5_mm);
	}
	return s;
}

}
