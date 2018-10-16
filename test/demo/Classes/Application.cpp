// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2018 Roman Katuntsev <sbkarr@stappler.org>

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

#include "MaterialResourceManager.h"
#include "MaterialScene.h"

#include "SPStorage.h"

#include "HelloWorldLayout.h"

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

	storage::get("applicaton_layout", [this] (data::Value &&val) {
		auto scene = Rc<material::Scene>::create();
		scene->getNavigationLayer()->setNavigationMenuSource(_appMenu);

		scene->pushContentNode(Rc<HelloWorldLayout>::create(val));

		material::Scene::run(scene);

		_data = std::move(val);
	});
}

void Application::rebuildMenu() {
	_appMenu->clear();

	_appMenu->addButton("Hello world layout", material::IconName::Action_description,
			[this] (material::Button *b, material::MenuSourceButton *i) {
		auto scene = material::Scene::getRunningScene();
		scene->replaceContentNode( Rc<HelloWorldLayout>::create(_data) );
	});

	_appMenu->addSeparator();
}

void Application::setData(const data::Value &data) {
	_data = data;
	saveData();
}

void Application::saveData() {
	storage::set("applicaton_layout", _data);
}

NS_SP_EXT_END(app)
