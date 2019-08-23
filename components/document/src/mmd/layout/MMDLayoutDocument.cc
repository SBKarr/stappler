// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2017 Roman Katuntsev <sbkarr@stappler.org>

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

#include "SPLayout.h"
#include "MMDLayoutDocument.h"
#include "MMDEngine.h"
#include "MMDLayoutProcessor.h"

#include "SPStringView.h"
#include "SLRendererTypes.h"

NS_MMD_BEGIN

bool LayoutDocument::isMmdData(const DataReader<ByteOrder::Network> &data) {
	StringView str((const char *)data.data(), data.size());
	str.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();

	if (str.is("#") || str.is("{{TOC}}") || str.is("Title:")) {
		return true;
	}

	str.skipUntilString("\n#", true);
	if (str.is("\n#")) {
		return true;
	}

	return false;
}

bool LayoutDocument::isMmdFile(const StringView &path) {
	auto ext = filepath::lastExtension(path);
	if (ext == "md" || ext == "markdown") {
		return true;
	}

	if (ext == "text" || ext == "txt" || ext.empty()) {
		if (auto file = filesystem::openForReading(path)) {
			StackBuffer<512> data;
			if (io::Producer(file).seekAndRead(0, data, 512) > 0) {
				return isMmdData(DataReader<ByteOrder::Network>(data.data(), data.size()));
			}
		}
	}

	return false;
}

static bool checkMmdFile(const StringView &path, const StringView &ct) {
	StringView ctView(ct);

	return ctView.is("text/markdown") || ctView.is("text/x-markdown") || LayoutDocument::isMmdFile(path);
}

static Rc<layout::Document> loadMmdFile(const StringView &path, const StringView &ct) {
	return Rc<LayoutDocument>::create(layout::FilePath(path), ct);
}

static bool checkMmdData(const DataReader<ByteOrder::Network> &data, const StringView &ct) {
	StringView ctView(ct);

	return ctView.is("text/markdown") || ctView.is("text/x-markdown") || LayoutDocument::isMmdData(data);
}

static Rc<layout::Document> loadMmdData(const DataReader<ByteOrder::Network> &data, const StringView &ct) {
	return Rc<LayoutDocument>::create(data, ct);
}

LayoutDocument::DocumentFormat LayoutDocument::MmdFormat(&checkMmdFile, &loadMmdFile, &checkMmdData, &loadMmdData);

bool LayoutDocument::init(const FilePath &path, const StringView &ct) {
	if (path.get().empty()) {
		return false;
	}

	_filePath = path.get().str();

	auto data = filesystem::readFile(path.get());

	return init(data, ct);
}

bool LayoutDocument::init(const DataReader<ByteOrder::Network> &data, const StringView &ct) {
	if (data.empty()) {
		return false;
	}

	Engine e; e.init(StringView((const char *)data.data(), data.size()), StapplerExtensions);
	e.process([&] (const Content &c, const StringView &s, const Token &t) {
		LayoutProcessor p; p.init(this);
		p.process(c, s, t);
	});

	return !_pages.empty();
}

layout::ContentPage *LayoutDocument::acquireRootPage() {
	if (_pages.empty()) {
		_pages.emplace(String(), layout::ContentPage{String(), layout::Node("body", String()), true});
	}

	auto page = &(_pages.begin()->second);
	auto mediaCssStringFn = [&] (layout::CssStringId strId, const StringView &string) {
		page->strings.insert(pair(strId, string.str()));
	};

	_minWidthQuery = page->queries.size();
	page->queries.emplace_back();
	page->queries.back().parse("all and (max-width:500px)", mediaCssStringFn);

	_mediumWidthQuery = page->queries.size();
	page->queries.emplace_back();
	page->queries.back().parse("all and (min-width:500px) and (max-width:750px)", mediaCssStringFn);

	_maxWidthQuery = page->queries.size();
	page->queries.emplace_back();
	page->queries.back().parse("all and (min-width:750px)", mediaCssStringFn);

	_imageViewQuery = page->queries.size();
	page->queries.emplace_back();
	page->queries.back().parse("all and (x-option:image-view)", mediaCssStringFn);

	return page;
}

void LayoutDocument::onTag(layout::Style &style, const StringView &tag, const StringView &parent, const MediaParameters &media) const {
	using namespace layout;
	using namespace layout::style;

	if (tag == "div") {
		style.set(Parameter::create<ParameterName::Display>(Display::Block), true);
	}

	if (tag == "p" || tag == "h1" || tag == "h2" || tag == "h3" || tag == "h4" || tag == "h5" || tag == "h6") {
		if (parent != "li" && parent != "blockquote") {
			style.set(Parameter::create<ParameterName::MarginTop>(Metric(0.5f, Metric::Units::Em)), true);
			style.set(Parameter::create<ParameterName::MarginBottom>(Metric(0.5f, Metric::Units::Em)), true);
			if (parent != "dd" && parent != "figcaption") {
				style.set(Parameter::create<ParameterName::TextIndent>(Metric(1.5f, Metric::Units::Rem)), true);
			}
		}
		style.set(Parameter::create<ParameterName::LineHeight>(Metric(1.2f, Metric::Units::Em)), true);
		style.set(Parameter::create<ParameterName::Display>(Display::Block), true);
	}

	if (tag == "span" || tag == "strong" || tag == "em" || tag == "nobr"
			|| tag == "sub" || tag == "sup" || tag == "inf" || tag == "b"
			|| tag == "i" || tag == "u" || tag == "code") {
		style.set(Parameter::create<ParameterName::Display>(Display::Inline), true);
	}

	if (tag == "h1") {
		style.set(Parameter::create<ParameterName::MarginTop>(Metric(0.8f, Metric::Units::Em)), true);
		style.set(Parameter::create<ParameterName::FontSize>(uint8_t(32)), true);
		style.set(Parameter::create<ParameterName::FontWeight>(FontWeight::W200), true);
		style.set(Parameter::create<ParameterName::Opacity>(uint8_t(222)), true);

	} else if (tag == "h2") {
		style.set(Parameter::create<ParameterName::MarginTop>(Metric(0.8f, Metric::Units::Em)), true);
		style.set(Parameter::create<ParameterName::FontSize>(uint8_t(28)), true);
		style.set(Parameter::create<ParameterName::FontWeight>(FontWeight::W400), true);
		style.set(Parameter::create<ParameterName::Opacity>(uint8_t(222)), true);

	} else if (tag == "h3") {
		style.set(Parameter::create<ParameterName::MarginTop>(Metric(0.8f, Metric::Units::Em)), true);
		style.set(Parameter::create<ParameterName::FontSize>(FontSize::XXLarge), true);
		style.set(Parameter::create<ParameterName::FontWeight>(FontWeight::W400), true);
		style.set(Parameter::create<ParameterName::Opacity>(uint8_t(200)), true);

	} else if (tag == "h4") {
		style.set(Parameter::create<ParameterName::MarginTop>(Metric(0.8f, Metric::Units::Em)), true);
		style.set(Parameter::create<ParameterName::FontSize>(FontSize::XLarge), true);
		style.set(Parameter::create<ParameterName::FontWeight>(FontWeight::W500), true);
		style.set(Parameter::create<ParameterName::Opacity>(uint8_t(188)), true);

	} else if (tag == "h5") {
		style.set(Parameter::create<ParameterName::MarginTop>(Metric(0.8f, Metric::Units::Em)), true);
		style.set(Parameter::create<ParameterName::FontSize>(uint8_t(18)), true);
		style.set(Parameter::create<ParameterName::FontWeight>(FontWeight::W400), true);
		style.set(Parameter::create<ParameterName::Opacity>(uint8_t(222)), true);

	} else if (tag == "h6") {
		style.set(Parameter::create<ParameterName::MarginTop>(Metric(0.8f, Metric::Units::Em)), true);
		style.set(Parameter::create<ParameterName::FontSize>(FontSize::Large), true);
		style.set(Parameter::create<ParameterName::FontWeight>(FontWeight::W500), true);
		style.set(Parameter::create<ParameterName::Opacity>(uint8_t(216)), true);

	} else if (tag == "p") {
		style.set(Parameter::create<ParameterName::TextAlign>(TextAlign::Justify), true);
		style.set(Parameter::create<ParameterName::Hyphens>(Hyphens::Auto), true);

	} else if (tag == "hr") {
		style.set(Parameter::create<ParameterName::MarginTop>(Metric(0.5f, Metric::Units::Em)), true);
		style.set(Parameter::create<ParameterName::MarginRight>(Metric(0.5f, Metric::Units::Em)), true);
		style.set(Parameter::create<ParameterName::MarginBottom>(Metric(0.5f, Metric::Units::Em)), true);
		style.set(Parameter::create<ParameterName::MarginLeft>(Metric(0.5f, Metric::Units::Em)), true);
		style.set(Parameter::create<ParameterName::Height>(Metric(2, Metric::Units::Px)), true);
		style.set(Parameter::create<ParameterName::BackgroundColor>(Color4B(0, 0, 0, 127)), true);

	} else if (tag == "a") {
		style.set(Parameter::create<ParameterName::TextDecoration>(TextDecoration::Underline), true);
		style.set(Parameter::create<ParameterName::Color>(Color3B(0x0d, 0x47, 0xa1)), true);

	} else if (tag == "b" || tag == "strong") {
		style.set(Parameter::create<ParameterName::FontWeight>(FontWeight::Bold), true);

	} else if (tag == "i" || tag == "em") {
		style.set(Parameter::create<ParameterName::FontStyle>(FontStyle::Italic), true);

	} else if (tag == "u") {
		style.set(Parameter::create<ParameterName::TextDecoration>(TextDecoration::Underline), true);

	} else if (tag == "nobr") {
		style.set(Parameter::create<ParameterName::WhiteSpace>(WhiteSpace::Nowrap), true);

	} else if (tag == "pre") {
		style.set(Parameter::create<ParameterName::MarginTop>(Metric(0.5f, Metric::Units::Em)), true);
		style.set(Parameter::create<ParameterName::MarginBottom>(Metric(0.5f, Metric::Units::Em)), true);
		style.set(Parameter::create<ParameterName::WhiteSpace>(WhiteSpace::Pre), true);
		style.set(Parameter::create<ParameterName::Display>(Display::Block), true);
		style.set(Parameter::create<ParameterName::BackgroundColor>(Color4B(228, 228, 228, 255)), true);

		style.set(Parameter::create<ParameterName::PaddingTop>(Metric(0.5f, Metric::Units::Em)), true);
		style.set(Parameter::create<ParameterName::PaddingLeft>(Metric(0.5f, Metric::Units::Em)), true);
		style.set(Parameter::create<ParameterName::PaddingBottom>(Metric(0.5f, Metric::Units::Em)), true);
		style.set(Parameter::create<ParameterName::PaddingRight>(Metric(0.5f, Metric::Units::Em)), true);

	} else if (tag == "code") {
		style.set(Parameter::create<ParameterName::FontFamily>(CssStringId("monospace"_hash)), true);
		style.set(Parameter::create<ParameterName::BackgroundColor>(Color4B(228, 228, 228, 255)), true);

	} else if (tag == "sub" || tag == "inf") {
		style.set(Parameter::create<ParameterName::VerticalAlign>(VerticalAlign::Sub), true);
		style.set(Parameter::create<ParameterName::FontSizeIncrement>(FontSizeIncrement::XSmaller), true);

	} else if (tag == "sup") {
		style.set(Parameter::create<ParameterName::VerticalAlign>(VerticalAlign::Super), true);
		style.set(Parameter::create<ParameterName::FontSizeIncrement>(FontSizeIncrement::XSmaller), true);

	} else if (tag == "body") {
		style.set(Parameter::create<ParameterName::MarginLeft>(Metric(0.8f, Metric::Units::Rem), MediaQuery::IsScreenLayout), true);
		style.set(Parameter::create<ParameterName::MarginRight>(Metric(0.8f, Metric::Units::Rem), MediaQuery::IsScreenLayout), true);

	} else if (tag == "br") {
		style.set(Parameter::create<ParameterName::WhiteSpace>(style::WhiteSpace::PreLine), true);
		style.set(Parameter::create<ParameterName::Display>(Display::Inline), true);

	} else if (tag == "li") {
		style.set(Parameter::create<ParameterName::Display>(Display::ListItem), true);
		style.set(Parameter::create<ParameterName::LineHeight>(Metric(1.2f, Metric::Units::Em)), true);
		style.set(Parameter::create<ParameterName::MarginTop>(Metric(0.25f, Metric::Units::Rem)), true);
		style.set(Parameter::create<ParameterName::MarginBottom>(Metric(0.25f, Metric::Units::Rem)), true);

	} else if (tag == "ol") {
		style.set(Parameter::create<ParameterName::Display>(Display::Block), true);
		style.set(Parameter::create<ParameterName::ListStyleType>(ListStyleType::Decimal), true);
		style.set(Parameter::create<ParameterName::PaddingLeft>(Metric(1.5f, Metric::Units::Rem)), true);
		style.set(Parameter::create<ParameterName::MarginTop>(Metric(0.4f, Metric::Units::Rem)), true);
		style.set(Parameter::create<ParameterName::MarginBottom>(Metric(0.4f, Metric::Units::Rem)), true);
		style.set(Parameter::create<ParameterName::XListStyleOffset>(Metric(0.7f, Metric::Units::Rem)), true);

	} else if (tag == "ul") {
		style.set(Parameter::create<ParameterName::Display>(Display::Block), true);
		if (parent == "li") {
			style.set(Parameter::create<ParameterName::ListStyleType>(ListStyleType::Circle), true);
		} else {
			style.set(Parameter::create<ParameterName::ListStyleType>(ListStyleType::Disc), true);
		}
		style.set(Parameter::create<ParameterName::PaddingLeft>(Metric(1.5f, Metric::Units::Rem)), true);
		style.set(Parameter::create<ParameterName::MarginTop>(Metric(0.4f, Metric::Units::Rem)), true);
		style.set(Parameter::create<ParameterName::MarginBottom>(Metric(0.4f, Metric::Units::Rem)), true);
		style.set(Parameter::create<ParameterName::XListStyleOffset>(Metric(0.7f, Metric::Units::Rem)), true);

	} else if (tag == "img") {
		if (parent == "figure") {
			style.set(Parameter::create<ParameterName::Display>(Display::Block), true);
		} else {
			style.set(Parameter::create<ParameterName::Display>(Display::InlineBlock), true);
		}

		style.set(Parameter::create<ParameterName::BackgroundSizeWidth>(Metric(1.0, Metric::Units::Contain)), true);
		style.set(Parameter::create<ParameterName::BackgroundSizeHeight>(Metric(1.0, Metric::Units::Contain)), true);
		style.set(Parameter::create<ParameterName::PageBreakInside>(PageBreak::Avoid), true);

		style.set(Parameter::create<ParameterName::MarginRight>(Metric(0.0f, Metric::Units::Auto)), true);
		style.set(Parameter::create<ParameterName::MarginLeft>(Metric(0.0f, Metric::Units::Auto)), true);

		style.set(Parameter::create<ParameterName::MaxWidth>(Metric(70.0f, Metric::Units::Vw)), true);
		style.set(Parameter::create<ParameterName::MaxHeight>(Metric(70.0f, Metric::Units::Vh)), true);
		style.set(Parameter::create<ParameterName::MinWidth>(Metric(100.8f, Metric::Units::Px)), true);
		style.set(Parameter::create<ParameterName::MinHeight>(Metric(88.8f, Metric::Units::Px)), true);

	} else if (tag == "table") {
		style.data.reserve(16);
		style.data.push_back(Parameter::create<ParameterName::MarginTop>(Metric(0.5f, Metric::Units::Rem)));
		style.data.push_back(Parameter::create<ParameterName::MarginBottom>(Metric(0.5f, Metric::Units::Rem)));

		style.data.push_back(Parameter::create<ParameterName::BorderTopStyle>(BorderStyle::Solid));
		style.data.push_back(Parameter::create<ParameterName::BorderTopWidth>(Metric(1.0f, Metric::Units::Px)));
		style.data.push_back(Parameter::create<ParameterName::BorderTopColor>(Color4B(168, 168, 168,255)));
		style.data.push_back(Parameter::create<ParameterName::BorderRightStyle>(BorderStyle::Solid));
		style.data.push_back(Parameter::create<ParameterName::BorderRightWidth>(Metric(1.0f, Metric::Units::Px)));
		style.data.push_back(Parameter::create<ParameterName::BorderRightColor>(Color4B(168, 168, 168,255)));
		style.data.push_back(Parameter::create<ParameterName::BorderBottomStyle>(BorderStyle::Solid));
		style.data.push_back(Parameter::create<ParameterName::BorderBottomWidth>(Metric(1.0f, Metric::Units::Px)));
		style.data.push_back(Parameter::create<ParameterName::BorderBottomColor>(Color4B(168, 168, 168,255)));
		style.data.push_back(Parameter::create<ParameterName::BorderLeftStyle>(BorderStyle::Solid));
		style.data.push_back(Parameter::create<ParameterName::BorderLeftWidth>(Metric(1.0f, Metric::Units::Px)));
		style.data.push_back(Parameter::create<ParameterName::BorderLeftColor>(Color4B(168, 168, 168,255)));

		style.set(Parameter::create<ParameterName::Display>(Display::Table), true);

	} else if (tag == "td" || tag == "th") {
		style.data.reserve(18);
		style.data.push_back(Parameter::create<ParameterName::PaddingTop>(Metric(0.3f, Metric::Units::Rem)));
		style.data.push_back(Parameter::create<ParameterName::PaddingLeft>(Metric(0.3f, Metric::Units::Rem)));
		style.data.push_back(Parameter::create<ParameterName::PaddingBottom>(Metric(0.3f, Metric::Units::Rem)));
		style.data.push_back(Parameter::create<ParameterName::PaddingRight>(Metric(0.3f, Metric::Units::Rem)));

		style.data.push_back(Parameter::create<ParameterName::BorderTopStyle>(BorderStyle::Solid));
		style.data.push_back(Parameter::create<ParameterName::BorderTopWidth>(Metric(1.0f, Metric::Units::Px)));
		style.data.push_back(Parameter::create<ParameterName::BorderTopColor>(Color4B(168, 168, 168,255)));
		style.data.push_back(Parameter::create<ParameterName::BorderRightStyle>(BorderStyle::Solid));
		style.data.push_back(Parameter::create<ParameterName::BorderRightWidth>(Metric(1.0f, Metric::Units::Px)));
		style.data.push_back(Parameter::create<ParameterName::BorderRightColor>(Color4B(168, 168, 168,255)));
		style.data.push_back(Parameter::create<ParameterName::BorderBottomStyle>(BorderStyle::Solid));
		style.data.push_back(Parameter::create<ParameterName::BorderBottomWidth>(Metric(1.0f, Metric::Units::Px)));
		style.data.push_back(Parameter::create<ParameterName::BorderBottomColor>(Color4B(168, 168, 168,255)));
		style.data.push_back(Parameter::create<ParameterName::BorderLeftStyle>(BorderStyle::Solid));
		style.data.push_back(Parameter::create<ParameterName::BorderLeftWidth>(Metric(1.0f, Metric::Units::Px)));
		style.data.push_back(Parameter::create<ParameterName::BorderLeftColor>(Color4B(168, 168, 168,255)));

		if (tag == "th") {
			style.set(Parameter::create<ParameterName::FontWeight>(FontWeight::Bold), true);
		}

	} else if (tag == "caption") {
		style.data.push_back(Parameter::create<ParameterName::TextAlign>(TextAlign::Center));

		style.data.push_back(Parameter::create<ParameterName::PaddingTop>(Metric(0.4f, Metric::Units::Rem)));
		style.data.push_back(Parameter::create<ParameterName::PaddingBottom>(Metric(0.4f, Metric::Units::Rem)));
		style.data.push_back(Parameter::create<ParameterName::PaddingLeft>(Metric(0.4f, Metric::Units::Rem)));
		style.data.push_back(Parameter::create<ParameterName::PaddingRight>(Metric(0.4f, Metric::Units::Rem)));

	} else if (tag == "blockquote") {
		style.set(Parameter::create<ParameterName::Display>(Display::Block), true);
		style.set(Parameter::create<ParameterName::MarginTop>(Metric(0.5f, Metric::Units::Rem)), true);
		style.set(Parameter::create<ParameterName::MarginBottom>(Metric(0.5f, Metric::Units::Rem)), true);

		if (parent == "blockquote") {
			style.set(Parameter::create<ParameterName::PaddingLeft>(Metric(0.8f, Metric::Units::Rem)), true);
		} else {
			style.set(Parameter::create<ParameterName::MarginLeft>(Metric(1.5f, Metric::Units::Rem)), true);
			style.set(Parameter::create<ParameterName::MarginRight>(Metric(1.5f, Metric::Units::Rem)), true);
			style.set(Parameter::create<ParameterName::PaddingLeft>(Metric(1.0f, Metric::Units::Rem)), true);
		}

		style.set(Parameter::create<ParameterName::PaddingTop>(Metric(0.1f, Metric::Units::Rem)), true);
		style.set(Parameter::create<ParameterName::PaddingBottom>(Metric(0.3f, Metric::Units::Rem)), true);

		style.set(Parameter::create<ParameterName::BorderLeftColor>(Color4B(0, 0, 0, 64)), true);
		style.set(Parameter::create<ParameterName::BorderLeftWidth>(Metric(3, Metric::Units::Px)), true);
		style.set(Parameter::create<ParameterName::BorderLeftStyle>(BorderStyle::Solid), true);

	} else if (tag == "dl") {
		style.set(Parameter::create<ParameterName::Display>(Display::Block), true);
		style.set(Parameter::create<ParameterName::MarginTop>(Metric(1.0f, Metric::Units::Em)), true);
		style.set(Parameter::create<ParameterName::MarginBottom>(Metric(1.0f, Metric::Units::Em)), true);
		style.set(Parameter::create<ParameterName::MarginLeft>(Metric(1.5f, Metric::Units::Rem)), true);

	} else if (tag == "dt") {
		style.set(Parameter::create<ParameterName::Display>(Display::Block), true);
		style.set(Parameter::create<ParameterName::FontWeight>(FontWeight::W700), true);

	} else if (tag == "dd") {
		style.set(Parameter::create<ParameterName::Display>(Display::Block), true);
		style.set(Parameter::create<ParameterName::PaddingLeft>(Metric(1.0f, Metric::Units::Rem)), true);
		style.set(Parameter::create<ParameterName::MarginTop>(Metric(0.5f, Metric::Units::Em)), true);
		style.set(Parameter::create<ParameterName::MarginBottom>(Metric(0.5f, Metric::Units::Em)), true);

		style.set(Parameter::create<ParameterName::BorderLeftColor>(Color4B(0, 0, 0, 64)), true);
		style.set(Parameter::create<ParameterName::BorderLeftWidth>(Metric(2, Metric::Units::Px)), true);
		style.set(Parameter::create<ParameterName::BorderLeftStyle>(BorderStyle::Solid), true);

	} else if (tag == "figure") {
		style.set(Parameter::create<ParameterName::Display>(Display::Block), true);
		style.set(Parameter::create<ParameterName::MarginTop>(Metric(1.0f, Metric::Units::Em)), true);
		style.set(Parameter::create<ParameterName::MarginBottom>(Metric(0.5f, Metric::Units::Em)), true);

	} else if (tag == "figcaption") {
		style.set(Parameter::create<ParameterName::Display>(Display::Block), true);
		style.set(Parameter::create<ParameterName::FontSize>(FontSize::Small), true);
		style.set(Parameter::create<ParameterName::FontWeight>(FontWeight::W500), true);
		if (media.hasOption("image-view")) {
			style.set(Parameter::create<ParameterName::MarginTop>(Metric(0.5f, Metric::Units::Em)), true);
			style.set(Parameter::create<ParameterName::TextAlign>(TextAlign::Justify), true);
		} else {
			style.set(Parameter::create<ParameterName::MarginTop>(Metric(0.5f, Metric::Units::Em)), true);
			style.set(Parameter::create<ParameterName::MarginBottom>(Metric(0.5f, Metric::Units::Em)), true);
			style.set(Parameter::create<ParameterName::TextAlign>(TextAlign::Center), true);
		}
	}
}

static void LayoutDocument_onClass(layout::Style &style, const StringView &name, const StringView &classStr, const layout::MediaParameters &media) {
	using namespace layout;
	using namespace layout::style;

	if (name == "div" && classStr == "footnotes") {
		style.set(Parameter::create<ParameterName::MarginTop>(Metric(16.0f, Metric::Units::Px)), true);
		style.set(Parameter::create<ParameterName::MarginBottom>(Metric(16.0f, Metric::Units::Px)), true);
	}

	if (classStr == "math") {
		style.set(Parameter::create<ParameterName::Display>(Display::None), true);
	}

	if (name == "figure") {
		if (classStr == "middle") {
			style.set(Parameter::create<ParameterName::Float>(Float::Right), true);
			style.set(Parameter::create<ParameterName::Width>(Metric(1.0f, Metric::Units::Percent)), true);
		}

	} else if (name == "img") {
		if (classStr == "middle") {
			style.set(Parameter::create<ParameterName::Display>(Display::Block), true);
			style.set(Parameter::create<ParameterName::MarginRight>(Metric(0.0f, Metric::Units::Auto)), true);
			style.set(Parameter::create<ParameterName::MarginLeft>(Metric(0.0f, Metric::Units::Auto)), true);
		}
	}

	if (classStr == "reversefootnote" && media.hasOption("tooltip")) {
		style.set(Parameter::create<ParameterName::Display>(Display::None), true);
	}
}

// Default style, that can be redefined with css
layout::Style LayoutDocument::beginStyle(const Node &node, const Vector<const Node *> &stack, const MediaParameters &media) const {
	using namespace layout;
	using namespace layout::style;

	const Node *parent = nullptr;
	if (stack.size() > 1) {
		parent = stack.at(stack.size() - 2);
	}

	Style style;
	onTag(style, node.getHtmlName(), parent ? StringView(parent->getHtmlName()) : StringView(), media);

	if (parent && node.getHtmlName() == "tr") {
		if (parent->getHtmlName() == "tbody") {
			if (parent->getChildIndex(node) % 2 == 1) {
				style.set(Parameter::create<ParameterName::BackgroundColor>(Color4B::WHITE), true);
			} else {
				style.set(Parameter::create<ParameterName::BackgroundColor>(Color4B(232, 232, 232, 255)), true);
			}
		} else {
			style.set(Parameter::create<ParameterName::BackgroundColor>(Color4B::WHITE), true);
		}
	}

	auto &attr = node.getAttributes();
	for (auto &it : attr) {
		if (it.first == "class") {
			StringView r(it.second);
			r.split<StringView::CharGroup<CharGroupId::WhiteSpace>>([&] (const StringView &classStr) {
				LayoutDocument_onClass(style, node.getHtmlName(), classStr, media);
			});
		} else if ((node.getHtmlName() == "img" || node.getHtmlName() == "figcaption") && it.first == "type") {
			LayoutDocument_onClass(style, node.getHtmlName(), it.second, media);
		} else {
			onStyleAttribute(style, node.getHtmlName(), it.first, it.second, media);
		}
	}

	return style;
}

// Default style, that can NOT be redefined with css
layout::Style LayoutDocument::endStyle(const Node &node, const Vector<const Node *> &stack, const MediaParameters &media) const {
	return Document::endStyle(node, stack, media);
}

NS_MMD_END
