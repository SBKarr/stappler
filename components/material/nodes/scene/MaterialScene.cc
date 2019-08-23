// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2016 Roman Katuntsev <sbkarr@stappler.org>

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
#include "MaterialScene.h"

#include "SPBitmap.h"
#include "SPFilesystem.h"
#include "SPGestureListener.h"
#include "SPEventListener.h"

#include "base/CCDirector.h"
#include "base/CCMap.h"
#include "renderer/CCRenderer.h"
#include "renderer/ccGLStateCache.h"

#include "SPScreen.h"
#include "SPString.h"
#include "SPDevice.h"
#include "SPDrawPathNode.h"
#include "SPLayer.h"
#include "SPIME.h"

#include "MaterialBackgroundLayer.h"
#include "MaterialForegroundLayer.h"
#include "MaterialNavigationLayer.h"
#include "MaterialContentLayer.h"
#include "MaterialResourceManager.h"

#include "MaterialLabel.h"

#include "SPString.h"
#include "SPActions.h"

using namespace stappler::gesture;

NS_MD_BEGIN

SP_DECLARE_EVENT_CLASS(Scene, onBackKey);

Scene *Scene::getRunningScene() {
	return dynamic_cast<material::Scene *>(cocos2d::Director::getInstance()->getRunningScene());
}

void Scene::run(Scene *scene) {
	if (!scene) {
		cocos2d::Director::getInstance()->runWithScene(Rc<Scene>::create());
	} else {
		cocos2d::Director::getInstance()->runWithScene(scene);
	}
}

bool Scene::init() {
	if (!DynamicBatchScene::init()) {
		return false;
	}

	auto l = Rc<EventListener>::create();
	l->onEvent(stappler::Screen::onOrientation, [this] (const Event &) {
		layoutSubviews();
	});
	l->onEvent(stappler::Device::onBackKey, [this] (const Event &) {
		onBackKeyPressed();
	});
	l->onEvent(stappler::Device::onRegenerateTextures, [] (const Event &) {
		stappler::Device::getInstance()->onDirectorStarted();
	});
	l->onEvent(stappler::Device::onScreenshot, [this] (const Event &) {
		takeScreenshoot();
	});
	l->onEvent(ResourceManager::onLightLevel, std::bind(&Scene::onLightLevel, this));
	addComponentItem(l);

	if (!_background) {
		_background = addChildNode(createBackgroundLayer(), -1);
	}

	if (!_content) {
		_content = addChildNode(createContentLayer(), 1);
	}

	if (!_navigation) {
		_navigation = addChildNode(createNavigationLayer(), 3);
	}

	if (!_foreground) {
		_foreground = addChildNode(createForegroundLayer(), 4);
	}

	_captureCanvas = Rc<draw::Canvas>::create();

	return true;
}

void Scene::setAutoLightLevel(bool value) {
	_autoLightLevel = value;
	onLightLevel();
}

bool Scene::isAutoLightLevel() const {
	return _autoLightLevel;
}

void Scene::setDisplayStats(bool value) {
	if (_displayStats != value) {
		_displayStats = value;
		updateStatsLabel();
	}
}

bool Scene::isDisplayStats() const {
	return _displayStats;
}

void Scene::setVisualizeTouches(bool value) {
	if (_visualizeTouches != value) {
		_visualizeTouches = value;
		if (_visualizeTouches) {
			_touchesNode = cocos2d::Node::create();
			_touchesNode->setPosition(0, 0);
			_touchesNode->setAnchorPoint(cocos2d::Vec2(0, 0));
			_touchesNode->setContentSize(_contentSize);

			auto touches = Rc<gesture::Listener>::create();
			touches->setTouchFilter([] (const cocos2d::Vec2 &loc, const stappler::gesture::Listener::DefaultTouchFilter &) {
				return true;
			});
			touches->setTouchCallback([this] (stappler::gesture::Event ev, const stappler::gesture::Touch &t) -> bool {
				if (_visualizeTouches) {
					if (ev == gesture::Event::Began) {
						auto d = screen::density();
						auto node = Rc<draw::PathNode>::create(24 * d, 24 * d);
						node->setAnchorPoint(Vec2(0.5, 0.5));
						node->setPosition(t.location());
						node->setContentSize(Size(24 * d, 24 * d));
						node->setColor(Color::Black);
						_touchesNode->addChild(node);
						_touchesNodes.insert(std::make_pair(t.id, node));

						draw::Path p;
						p.setStyle(stappler::draw::Path::Style::Fill)
								.addOval(Rect(0, 0, 24 * d, 24 * d))
								.setFillOpacity(127);
						node->addPath(std::move(p));
					} else if (ev == gesture::Event::Activated) {
						auto it = _touchesNodes.find(t.id);
						if (it != _touchesNodes.end()) {
							it->second->setPosition(t.location());
						}
					} else {
						auto it = _touchesNodes.find(t.id);
						if (it != _touchesNodes.end()) {
							it->second->removeFromParent();
							_touchesNodes.erase(it);
						}
					}
				}
				return true;
			});
			_touchesNode->addComponent(touches);
			addChild(_touchesNode, 100500);
		} else {
			if (_touchesNode) {
				_touchesNode->removeFromParent();
			}
			_touchesNodes.clear();
		}
	}
}
bool Scene::isVisualizeTouches() const {
	return _visualizeTouches;
}

void Scene::setExitProtection(bool value) {
	_exitProtection = value;
}

bool Scene::isExitProtection() const {
	return _exitProtection;
}

void Scene::onEnter() {
	DynamicBatchScene::onEnter();
	if (_autoLightLevel) {
		onLightLevel();
	}
}

void Scene::onEnterTransitionDidFinish() {
	DynamicBatchScene::onEnterTransitionDidFinish();
	layoutSubviews();
	updateStatsLabel();
	stappler::Device::getInstance()->onDirectorStarted();
}

void Scene::onExitTransitionDidStart() {
	DynamicBatchScene::onExitTransitionDidStart();
	updateStatsLabel();
}

const Size &Scene::getViewSize() const {
	return _background->getContentSize();
}

ForegroundLayer *Scene::getForegroundLayer() const {
	return _foreground;
}
NavigationLayer *Scene::getNavigationLayer() const {
	return _navigation;
}
ContentLayer *Scene::getContentLayer() const {
	return _content;
}
BackgroundLayer *Scene::getBackgroundLayer() const {
	return _background;
}

void Scene::setNavigationEnabled(bool value) {
	_navigation->setEnabled(value);
}

bool Scene::isNavigationEnabled() const {
	return _navigation->isEnabled();
}

Rc<ForegroundLayer> Scene::createForegroundLayer() {
	return Rc<ForegroundLayer>::create();
}
Rc<NavigationLayer> Scene::createNavigationLayer() {
	return Rc<NavigationLayer>::create();
}
Rc<ContentLayer> Scene::createContentLayer() {
	return Rc<ContentLayer>::create();
}
Rc<BackgroundLayer> Scene::createBackgroundLayer() {
	return Rc<BackgroundLayer>::create();
}

void Scene::layoutSubviews() {
	auto frameSize = cocos2d::Director::getInstance()->getOpenGLView()->getFrameSize();
	setContentSize(frameSize);

	layoutLayer(_background);
	layoutLayer(_content);
	layoutLayer(_navigation);
	layoutLayer(_foreground);

	if (_contentSizeDirty) {
		updateCapturedContent();
	}

	if (_touchesNode) {
		_touchesNode->setContentSize(_contentSize);
	}
}

void Scene::layoutLayer(cocos2d::Node *layer) {
	if (!layer) {
		return;
	}

	float density = stappler::Screen::getInstance()->getDensity();

	layer->ignoreAnchorPointForPosition(false);
	layer->setAnchorPoint(Vec2(0.5, 0.5));
	layer->setPosition(_contentSize.width / 2, _contentSize.height / 2);
	layer->setScale(density);
	layer->setContentSize(Size(_contentSize.width / density, _contentSize.height / density));
}

void Scene::update(float dt) {
	updateStatsValue();
}

void Scene::captureContentForNode(cocos2d::Node *node) {
	if (_contentCaptureNodes.find(node) != _contentCaptureNodes.end()) {
		return;
	}

	_contentCaptureNodes.insert(node);
	if (_contentCaptured) {
		return;
	}

	updateCapturedContent();
}

void Scene::releaseContentForNode(cocos2d::Node *node) {
	_contentCaptureNodes.erase(node);
	if (_contentCaptureNodes.size() == 0) {
	    _content->setVisible(true);
	    _background->setTexture(nullptr);
	    _contentCaptured = false;
		return;
	}
}
bool Scene::isContentCaptured() {
	return _contentCaptured;
}

void Scene::updateStatsLabel() {
	if (_displayStats && _running) {
		if (!_registred) {
			scheduleUpdate();
			_registred = true;
		}

		if (!_stats) {
			auto stats = Rc<Label>::create(FontType::Caption);
			stats->setAnchorPoint(Vec2(0, 0));
			stats->setScale(stappler::screen::density());
			_stats = addChildNode(stats, INT_MAX - 1);
		}

		if (!_statsColor) {
			auto d = stappler::screen::density();
			auto statsColor = Rc<Layer>::create();
			statsColor->setContentSize(Size(42 * d, 58 * d));
			statsColor->setOpacity(168);
			statsColor->setColor(Color::White);
			_statsColor = addChildNode(statsColor, INT_MAX - 2);
		}
	}
	if (!_displayStats || !_running) {
		if (_registred) {
			unscheduleUpdate();
			_registred = false;
		}
	}

	if (!_displayStats && _stats) {
		_stats->removeFromParent();
		_stats = nullptr;
	}

	if (!_displayStats && _statsColor) {
		_statsColor->removeFromParent();
		_statsColor = nullptr;
	}
}

void Scene::updateStatsValue() {
	if (_stats) {
		auto d = cocos2d::Director::getInstance();
		auto r = d->getRenderer();

		auto fps = d->getFrameRate();
		auto spf = d->getSecondsPerFrame();

		auto drawCalls = r->getDrawnBatches();
		auto drawVerts = r->getDrawnVertices();

		std::string str = toString(std::fixed, std::setprecision(3),
				fps, "\n",
				spf, "\n",
				drawCalls, "\n",
				drawVerts);
		_stats->setString(str);
	}
}

void Scene::updateCapturedContent() {
	if (_contentCaptureNodes.empty()) {
		return;
	}

	_contentCapturing = true;

	_content->setVisible(true);
	_navigation->setVisible(false);
	_foreground->setVisible(false);
	_background->setTexture(nullptr);

	auto tex = _captureCanvas->captureContents(this, cocos2d::Texture2D::PixelFormat::RGB888, 1.0f);

	_navigation->setVisible(true);
	_foreground->setVisible(true);

	_content->setVisible(false);

	_background->setTexture(tex);
	_contentCaptured = true;
	_contentCapturing = false;
}

void Scene::onBackKeyPressed() {
	if (!_backButtonStack.empty()) {
		auto top = _backButtonStack.back();
		_backButtonStack.pop_back();
		top.second();
	} else if (stappler::ime::isInputEnabled()) {
		stappler::ime::cancel();
	} else if (_foreground->isActive()) {
		_foreground->clear();
	} else if (_navigation->isNodeEnabled()) {
		_navigation->hide();
	} else if (_content->isActive() && _content->onBackButton()) {
	} else if (_exitProtection && !_shouldExit) {
		showSnackbar(move(SnackbarData("SystemTapExit"_locale.to_string()).delayFor(3.0f)));
		_shouldExit = true;
		runAction(action::sequence(3.0f, [this] {
			_shouldExit = false;
		}));
	} else {
		_shouldExit = false;
		_foreground->clearSnackbar();
		stappler::Device::getInstance()->keyBackClicked();
	}
	onBackKey(this);
}

void Scene::replaceContentNode(Layout *l, Transition *enter) {
	_content->replaceNode(l, enter);
}
void Scene::replaceTopContentNode(Layout *l, Transition *enter, Transition *exit) {
	_content->replaceTopNode(l, enter, exit);
}

void Scene::pushContentNode(Layout *l, Transition *enter, Transition *exit) {
	_content->pushNode(l, enter, exit);
}
void Scene::popContentNode(Layout *l) {
	_content->popNode(l);
}

void Scene::pushOverlayNode(OverlayLayout *l, Transition *enter, Transition *exit) {
	_content->pushOverlayNode(l, enter, exit);
}
void Scene::popOverlayNode(OverlayLayout *l) {
	_content->popOverlayNode(l);
}

void Scene::pushFloatNode(cocos2d::Node *n, int z) {
	_foreground->pushFloatNode(n, z);
}
void Scene::popFloatNode(cocos2d::Node *n) {
	_foreground->popFloatNode(n);
}

void Scene::pushForegroundNode(cocos2d::Node *n, const std::function<void()> &cb) {
	_foreground->pushNode(n, cb);
}
void Scene::popForegroundNode(cocos2d::Node *n) {
	_foreground->popNode(n);
}

void Scene::popNode(cocos2d::Node *node) {
	if (node->getParent() == _foreground) {
		_foreground->popNode(node);
	} else if (node->getParent() == _content) {
		if (auto l = dynamic_cast<OverlayLayout *>(node)) {
			if (!_content->popOverlayNode(l)) {
				_content->popNode(l);
			}
		} else if (auto l = dynamic_cast<Layout *>(node)) {
			_content->popNode(l);
		}
	}
}

void Scene::setNavigationMenuSource(material::MenuSource *source) {
	_navigation->setNavigationMenuSource(source);
}

void Scene::showSnackbar(SnackbarData &&data) {
	_foreground->showSnackbar(move(data));
}

Vec2 Scene::convertToScene(const Vec2 &vec) const {
	return _foreground->convertToNodeSpace(vec);
}

void Scene::pushBackButtonCallback(stappler::Ref *ref, const Function<void()> &cb) {
	_backButtonStack.emplace_back(ref, cb);
}
void Scene::popBackButtonCallback(stappler::Ref *ref) {
	auto it = _backButtonStack.begin();
	while (it != _backButtonStack.end()) {
		if (it->first == ref) {
			it = _backButtonStack.erase(it);
		} else {
			++ it;
		}
	}
}

void Scene::takeScreenshoot() {
	auto path = stappler::filesystem::writablePath("screenshots");
	stappler::filesystem::mkdir(path);
	path += toString("/", stappler::Time::now().toMilliseconds(), ".png");

	auto tex = _captureCanvas->captureContents(this, cocos2d::Texture2D::PixelFormat::RGBA8888, 1.0f);
	saveScreenshot(path, tex);

	_foreground->showSnackbar("Screenshot saved to " + path);
}

void Scene::saveScreenshot(const String &filename, cocos2d::Texture2D *tex) {
	auto bmp = _captureCanvas->captureTexture(tex);
	bmp.save(filename, true);
}

void Scene::onLightLevel() {
	if (_autoLightLevel) {
		auto level = material::ResourceManager::getInstance()->getLightLevel();
		switch(level) {
		case LightLevel::Dim:
			_background->setColor(Color(0x303030));
			break;
		case LightLevel::Normal:
		case LightLevel::Washed:
			_background->setColor(Color::Grey_50);
			break;
		};
	}
}

NS_MD_END
