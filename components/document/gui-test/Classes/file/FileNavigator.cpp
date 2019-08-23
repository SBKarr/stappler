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

#include "Material.h"
#include "FileNavigator.h"
#include "EpubDocument.h"
#include "FileImageView.h"
#include "RTEpubView.h"
#include "RTSource.h"
#include "MaterialToolbar.h"
#include "MaterialScrollSlice.h"
#include "MaterialMetrics.h"
#include "MaterialMenuSource.h"
#include "MaterialLabel.h"
#include "MaterialButton.h"
#include "MaterialScene.h"
#include "MaterialIconSprite.h"
#include "SPFilesystem.h"
#include "SPDevice.h"
#include "SPStorage.h"
#include "FileDialog.h"
#include "MMDHtmlOutputProcessor.h"

NS_SP_EXT_BEGIN(app)

static String getFileName(const data::Value &d) {
	String ret;
	if (d.hasValue("fname")) {
		ret = d.getString("fname");
	} else if (d.hasValue("alias")) {
		ret = d.getString("alias");
	} else {
		ret = d.getString("name");
	}
	return ret;
}

static String formatFileSize(size_t size) {
	if (size < 1024) {
		return toString(size, "b");
	} else if (size < 10000 * 1024) {
		return toString(std::fixed, std::setprecision(2), size / 1024.0f, "Kb");
	} else {
		return toString(std::fixed, std::setprecision(2), size / (1024.0f * 1024), "Mb");
	}
}

struct FileNameComparator {
	bool operator() (const data::Value &l, const data::Value &r) {
		auto lName = getFileName( l );
		auto rName = getFileName( r );

		return (*this)(lName, rName);
	}
	bool operator() (const String &lName, const String &rName) {
		char *ptr;

		auto lNum = strtod(lName.c_str(), &ptr);
		auto lIsNum = (lName.c_str() != ptr);

		auto rNum = strtod(rName.c_str(), &ptr);
		auto rIsNum = (rName.c_str() != ptr);

		if (lIsNum && rIsNum && lNum != rNum) {
			return lNum < rNum;
		} else {
			string::toupper(lName);
			string::toupper(rName);

			return lName < rName;
		}
	}
};

bool FileNavigator::init() {
	if (!ToolbarLayout::init()) {
		return false;
	}

	_hyphens = Rc<font::HyphenMap>::create();
	_hyphens->addHyphenDict(CharGroupId::Latin, "common/hyphen/hyph_en_GB.dic");
	_hyphens->addHyphenDict(CharGroupId::Cyrillic, "common/hyphen/hyph_ru_RU.dic");

	setToolbarColor(material::Color::Grey_300);
	setTitle("Files");
	setFlexibleToolbar(false);

	auto actions = getActionMenuSource();
	actions->addButton("refresh", material::IconName::Navigation_refresh, std::bind(&FileNavigator::refreshData, this));
	actions->addButton("open", material::IconName::File_folder, std::bind(&FileNavigator::openDirectory, this));
	_switchHiddenButton = actions->addButton("Показывать скрытые", material::IconName::Empty, std::bind(&FileNavigator::switchHidden, this));

	auto scroll = Rc<material::Scroll>::create();
	scroll->setHandlerCallback([] (material::Scroll *s) -> Rc<material::Scroll::Handler> {
		class Handler : public material::ScrollHandlerSlice {
		public:
			virtual bool init(material::Scroll *s) override {
				if (!ScrollHandlerSlice::init(s)) {
					return false;
				}
				return true;
			}
		protected:
			virtual Rc<material::Scroll::Item> onItem(data::Value &&d, const Vec2 &origin) override {
				auto name = d.getString("name");
				auto size = d.getString("size");
				if (!size.empty()) {
					name = name + "  " + size;
				}
				auto labelSize = material::Label::getLabelSize(material::FontType::Subhead, name, _size.width - 80.0f);

				return Rc<material::Scroll::Item>::create(std::move(d), origin,
						cocos2d::Size(_size.width, labelSize.height + 36.0f));
			}
		};

		return Rc<Handler>::create(s);
	});

	scroll->setItemCallback([this] (material::Scroll::Item *item) -> Rc<material::MaterialNode> {
		auto &d = item->getData();
		auto &size = item->getContentSize();

		auto name = d.getString("name");
		auto fsize = d.getString("size");
		if (!fsize.empty()) {
			name = name + "  ";

			auto w = d.getInteger("width");
			auto h = d.getInteger("height");
			if (w && h) {
				fsize += toString("  ", w, "x", h);
			}
		}
		auto label = Rc<material::Label>::create(material::FontType::System_Subhead);
		label->setColor(material::Color::Black);
		label->setString(name);
		label->appendTextWithStyle(fsize, DynamicLabel::Style(DynamicLabel::Opacity(127)));
		label->setWidth(size.width - 80.0f);
		label->setPosition(18.0f + 64.0f, 18.0f);
		label->tryUpdateLabel();

		auto node = Rc<material::MaterialNode>::create();
		node->setContentSize(size);
		node->setPadding(Padding().set(2, 2));
		node->setShadowZIndex(1.5f);
		node->addChild(label, 1);

		if (d.getBool("isDir")) {
			auto icon = Rc<material::IconSprite>::create(
					(d.getString("path") == _path) ? material::IconName::Navigation_arrow_back : material::IconName::File_folder);
			icon->setPosition(18.0f, node->getContentSize().height / 2.0f);
			icon->setAnchorPoint(cocos2d::Vec2(0.0f, 0.5f));
			icon->setColor(material::Color::Black);
			node->addChild(icon, 1);

			auto btn = Rc<material::Button>::create(std::bind(&FileNavigator::onDirButton, this, d.getString("path")));
			btn->setStyle(material::Button::Style::FlatBlack);
			btn->setAnchorPoint(cocos2d::Vec2(0, 0));
			btn->setContentSize(node->getContentSizeWithPadding());
			btn->setPosition(node->getAnchorPositionWithPadding());
			btn->setSwallowTouches(false);
			node->addChild(btn, 2);
		} else {
			auto btn = Rc<material::Button>::create(std::bind(&FileNavigator::onButton, this, d.getString("path")));
			btn->setStyle(material::Button::Style::FlatBlack);
			btn->setAnchorPoint(cocos2d::Vec2(0, 0));
			btn->setContentSize(node->getContentSizeWithPadding());
			btn->setPosition(node->getAnchorPositionWithPadding());
			btn->setSwallowTouches(false);
			node->addChild(btn, 2);
		}

		return node;
	});
	scroll->setMinLoadTime(TimeInterval::milliseconds(100));
	setBaseNode(scroll);
	_scroll = scroll;

	retain();
	storage::get("FileNavigator.Path", [this] (const StringView &, data::Value &&val) {
		std::string path;
		if (val.isDictionary()) {
			path = val.getString("path");
		} else {
			path = filepath::root(filepath::root(filepath::root(filesystem::writablePath()))) + "/Documents";
			filesystem::mkdir(path);
		}
		updateData(path);
		release();
	});

	return true;
}

void FileNavigator::refreshData() {
	updateData(_path);
}

void FileNavigator::updateData(const StringView &path) {
	data::Value dirs;
	data::Value files;
	filesystem::ftw(path, [this, &dirs, &files, path] (const StringView &p, bool isFile) {
		data::Value d;
		d.setString(p, "path");
		d.setString(filepath::replace(p, path, filepath::lastComponent(path)), "name");
		d.setString(filepath::lastComponent(p), "fname");
		d.setBool(!isFile, "isDir");
		if (isFile) {
			if (_showHidden || filepath::lastComponent(p).front() != '.') {
				d.setString(formatFileSize(filesystem::size(p)), "size");

				size_t w = 0, h = 0;
				CoderSource source(p);
				if (Bitmap::getImageSize(source, w, h)) {
					d.setInteger(w, "width");
					d.setInteger(h, "height");
				}

				files.addValue(std::move(d));
			}
		} else if (!p.empty() && p != "/") {
			if (p == path) {
				d.setString("", "fname");
			}
			auto n = filepath::lastComponent(p);
			if (_showHidden || (n.front() != '.' || n == "." || n == "..")) {
				dirs.addValue(std::move(d));
			}
		}
	}, 1, true);

	_data = data::Value();
	if (dirs.isArray()) {
		auto &arr = dirs.asArray();
		std::sort(arr.begin(), arr.end(), FileNameComparator());
		for (auto &it : dirs.asArray()) {
			_data.addValue(std::move(it));
		}
	}

	if (files.isArray()) {
		auto &arr = files.asArray();
		std::sort(arr.begin(), arr.end(), FileNameComparator());
		for (auto &it : files.asArray()) {
			_data.addValue(std::move(it));
		}
	}

	_toolbar->setTitle(filepath::lastComponent(path, 2).str());

	data::Value val;
	val.setString(path, "path");
	storage::set("FileNavigator.Path", std::move(val));
	if (!_source || path != _path) {
		_path = path.str();

		_source = Rc<data::Source>::create(data::Source::ChildsCount(_data.size()),
				[this] (const data::DataCallback &cb, data::Source::Id id) {
			cb(data::Value(_data.getValue((int)id.get())));
		});

		_scroll->setSource(_source);
		_scroll->reset();
	} else {
		_source->setChildsCount(_data.size());
	}
}

void FileNavigator::onButton(const StringView &str) {
	if (epub::Document::isEpub(str)) {

		openFile(str);
		/*epub::Document doc;
		if (doc.init(FilePath(str))) {
			data::Value info;
			info.setString(doc.getTitle(), "title");
			info.setString(doc.getCreators(), "creators");
			info.setString(doc.getLanguage(), "language");
			//info.setValue(doc.encode(), "meta");
			log::text("Epub", data::toString(info, true));

			doc.prepare();
			auto &cfg = doc.getFontConfig();
			for (auto &it : cfg) {
				log::format("FontConfig", "%s: '%s'", it.first.c_str(), string::toUtf8(it.second.chars).c_str());
				if (it.second.face) {
					for (auto srcIt : it.second.face->src) {
						log::text("FontConfigSource", srcIt);
					}
				}
			}
		}*/
	} else {
		openFile(str);
	}
}

void FileNavigator::onDirButton(const StringView &str) {
	if (str == _path) {
		updateData(filepath::root(str));
	} else {
		updateData(str);
	}
}

void FileNavigator::openFile(const StringView &file) {
	if (filepath::lastExtension(file) == "json") {
		return;
	}

	if (Bitmap::isImage(file, true) || layout::Image::isSvg(FilePath(file))) {
		auto view = Rc<FileImageView>::create(file);
		material::Scene::getRunningScene()->getContentLayer()->pushNode(view);
	} else {
		if (layout::Document::canOpenDocumnt(file)) {
			auto data = filesystem::readTextFile(file);
			//mmd::HtmlOutputProcessor::run(&std::cout, data);

			auto source = Rc<rich_text::Source>::create( Rc<rich_text::SourceFileAsset>::create(FilePath(file)));
			source->setHyphens(_hyphens.get());

			auto view = Rc<rich_text::EpubView>::create(source, filepath::name(file));
			view->setLayout(ScrollView::Vertical);
			view->setTitle(filepath::name(file).str());
			rich_text::Source *s = source;

			view->getToolbar()->getActionMenuSource()->addButton("refresh", material::IconName::Navigation_refresh,
					[s] (material::Button *b, material::MenuSourceButton *) {
				s->refresh();
			});
			material::Scene::getRunningScene()->getContentLayer()->pushNode(view);
		}
	}
}

void FileNavigator::onForeground(material::ContentLayer *l, Layout *overlay) {
	ToolbarLayout::onForeground(l, overlay);
	updateData(_path);
}

void FileNavigator::openDirectory() {
	retain();
	dialog::open(_path, [this] (const std::string &path) {
		if (!path.empty()) {
			updateData(path);
		}
		release();
	});
}

void FileNavigator::switchHidden() {
	if (_showHidden) {
		_switchHiddenButton->setNameIcon(material::IconName::Empty);
		_showHidden = false;
		refreshData();
	} else {
		_switchHiddenButton->setNameIcon(material::IconName::Navigation_check);
		_showHidden = true;
		refreshData();
	}
}

NS_SP_EXT_END(app)
