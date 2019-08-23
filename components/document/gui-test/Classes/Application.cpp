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
#include "Application.h"

#include "RTFontSizeMenu.h"
#include "RTLightLevelMenu.h"
#include "MaterialIconSprite.h"
#include "SPFilesystem.h"
#include "SPUrl.h"
#include "SPEvent.h"
#include "SPString.h"
#include "SPThread.h"
#include "SPDevice.h"
#include "SPScreen.h"
#include "SPRoundedSprite.h"
#include "SPStorage.h"

#include "MaterialMenuSource.h"
#include "MaterialResourceManager.h"
#include "MaterialResize.h"
#include "MaterialLabel.h"

#include "2d/CCActionInstant.h"

#include "base/ccUtils.h"
#include "MaterialScene.h"
#include "MaterialNavigationLayer.h"
#include "MaterialBackgroundLayer.h"
#include "MaterialContentLayer.h"
#include "MaterialFlexibleLayout.h"
#include "MaterialToolbar.h"
#include "MaterialMetrics.h"

#include "MaterialImage.h"

#include "FileNavigator.h"
#include "FileDialog.h"

#include "SLReader.h"
#include "SLImage.h"

NS_SP_EXT_BEGIN(app)

static Application *s_sharedApplication = nullptr;

Application *Application::getInstance() {
	return s_sharedApplication;
}

Application::Application() {
	s_sharedApplication = this;

	_appMenu = Rc<material::MenuSource>::create();

	onEvent(material::ResourceManager::onLoaded, [this] (const Event &) {
		init();
	});

	onEvent(material::Scene::onBackKey, [this] (const Event &) { });

	material::ResourceManager::getInstance();
}

Application::~Application() {
}

void Application::init() {
	rebuildMenu();

	storage::get("applicaton_layout", [this] (const StringView &key, data::Value &&val) {
		auto scene = Rc<material::Scene>::create();
		scene->getNavigationLayer()->setNavigationMenuSource(_appMenu);
		//scene->setDisplayStats(true);
		if (val) {
			_assetsDir = val.getString("assetsDir");
		}

		/*auto l = Rc<material::Layout>::create();

		auto vg = Rc<draw::PathNode>::create(draw::Format::RGBA8888);
		vg->setAutofit(draw::PathNode::Autofit::Contain);
		vg->setPosition(0, 0);
		vg->setAnchorPoint(Vec2(0, 0));
		vg->setImage(Rc<layout::Image>::create(FilePath("/home/sbkarr/android/StapplerNext/stappler/extensions/document/gui-test/tests/border.svg")));
		auto vgPtr = l->addChildNode(vg, 1);

		auto lPtr = l.get();
		l->setOnContentSizeDirtyCallback([lPtr, vgPtr] {
			vgPtr->setContentSize(lPtr->getContentSize());
		});

		scene->pushContentNode(l);*/
		scene->pushContentNode(Rc<FileNavigator>::create());

		material::Scene::run(scene);

		_data = std::move(val);
	});
}

void Application::openFile(const std::string &name) {

}

void Application::rebuildMenu() {
	_appMenu->clear();

	/*_appMenu->addCustom(120, [this] () -> cocos2d::Node * {
		auto img = Rc<material::MaterialImage>::create(_banner);
		img->setAutofit(material::MaterialImage::Autofit::Contain);
		return img;
	});*/

	_appMenu->addButton("Файлы", material::IconName::Action_description,
			[this] (material::Button *b, material::MenuSourceButton *i) {
		auto scene = material::Scene::getRunningScene();
		scene->replaceContentNode( Rc<FileNavigator>::create() );

		_data.setString("files", "layout");
		saveData();
	});

	_appMenu->addSeparator();
}


void Application::saveData() {
	storage::set("applicaton_layout", _data);
}

material::GridView *Application::createGridView() {
	return nullptr;
}

void Application::onArticleCallback(const std::string &filename) {
}

void Application::setAssetsDir(const std::string &str) {
	_assetsDir = str;
	_data.setString(_assetsDir, "assetsDir");
	saveData();
}
const std::string &Application::getAssetsDir() const {
	return _assetsDir;
}

void Application::setBanner(const std::string &str) {
	_banner = str;
	rebuildMenu();
}
const std::string &Application::getBanner() const {
	return _banner;
}

NS_SP_EXT_END(app)
