/**
Copyright (c) 2020 Roman Katuntsev <sbkarr@stappler.org>

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
#include "MaterialButton.h"

#include "SPClippingNode.h"

namespace stappler::material {

static constexpr auto ButtonHighLightSelectAction = 2;
static constexpr auto ButtonHighLightSpawnAction = 3;
static constexpr auto ButtonHighLightDuration = 0.2f;
static constexpr auto ButtonHighLightBasicOpacity = 16.0f / 255.0f;

bool ButtonHighLight::init(Node *stencil, const Function<void()> &b, const Function<void()> &e, const Function<void(float)> &p) {
	if (!Node::init()) {
		return false;
	}

	setCascadeOpacityEnabled(true);

	_beginCallback = b;
	_endCallback = e;
	_progressCallback = p;

	auto clip = Rc<ClippingNode>::create(stencil);
	clip->setInverted(false);
	clip->setEnabled(true);
	clip->setVisible(false);
	_clip = addChildNode(clip);

	auto background = Rc<RoundedSprite>::create(0, screen::density());
	background->setOpacity(0);
	background->setColor(Color::White);
	_background = _clip->addChildNode(background, 0);

	return true;
}

void ButtonHighLight::onContentSizeDirty() {
	Node::onContentSizeDirty();

	if (_animationNode) {
		float downscale = 4.0f;
		_animationNode->setPosition(0.0f, 0.0f);
		_animationNode->setContentSize(Size(_contentSize.width / downscale, _contentSize.height / downscale));
	}

	_clip->setPosition(0.0f, 0.0f);
	_clip->setContentSize(_contentSize);

	_background->setPosition(0.0f, 0.0f);
	_background->setContentSize(_contentSize);

	if (_sizeCallback) {
		_sizeCallback(_contentSize);
	}
}

void ButtonHighLight::animateSelection(const Vec2 &touchPoint) {
	if (!_selected) {
		stopActionByTag(ButtonHighLightSelectAction);
		auto a = cocos2d::EaseQuadraticActionInOut::create(Rc<ProgressAction>::create(
				progress(ButtonHighLightDuration, 0.0f, _animationProgress), 1.0f,
				[this] (ProgressAction *a, float progress) {
			updateSelection(progress, ButtonHighLightBasicOpacity * 255.0f);
		}, [this] (ProgressAction *a) {
			a->setSourceProgress(getSelectionProgress());
			beginSelection();
		}, [this] (ProgressAction *a) {
			endSelection();
		}));
		a->setTag(ButtonHighLightSelectAction);
		runAction(a);
	}

	_highlighted = true;
	_clip->setVisible(true);

	draw::Image::PathRef *path = new draw::Image::PathRef();
	Size *size = new Size();
	Vec2 *point = new Vec2();

	auto a = Rc<ProgressAction>::create(ButtonHighLightDuration, 1.0,
			[this, path, size, point] (ProgressAction *a, float progress) {
		updateSpawn(progress, (*path), (*size), (*point));
	}, [this, path, size, point, touchPoint] (ProgressAction *a) {
		(*path) = beginSpawn();
		if (_animationNode) {
			(*size) = _animationNode->getContentSize();
			(*point) = _animationNode->convertToNodeSpace(touchPoint);
		}
	}, [this, path, size, point] (ProgressAction *a) {
		if (_animationNode) {
			_animationNode->removePath((*path));
		}
		endSpawn();
		delete path;
		delete size;
		delete point;
	});
	a->setForceStopCallback(true);

	auto b = action::sequence(_spawnDelay, cocos2d::EaseOut::create(a, 3.0f));
	runAction(b, ButtonHighLightSpawnAction);
}

void ButtonHighLight::animateDeselection() {
	if (!_selected) {
		_highlighted = false;

		auto op = _background->getOpacity();

		stopActionByTag(ButtonHighLightSelectAction);
		auto a = cocos2d::EaseQuadraticActionInOut::create(Rc<ProgressAction>::create(
				progress(ButtonHighLightDuration, 0.0f, 1.0f - _animationProgress), 0.0f,
				[this, op] (ProgressAction *a, float progress) {
			updateSelection(progress, op);
		}, [this] (ProgressAction *a) {
			a->setSourceProgress(getSelectionProgress());
			beginSelection();
		}, [this, op] (ProgressAction *a) {
			updateSelection(0.0f, op);
			endSelection();
		}));
		a->setTag(ButtonHighLightSelectAction);
		runAction(a);
	}
}

void ButtonHighLight::setSelected(bool value, bool instant) {
	if (_selected != value) {
		_selected = value;
		if (_selected) {
			if (!instant) {
				stopActionByTag(2);
				auto a = cocos2d::EaseQuadraticActionInOut::create(Rc<ProgressAction>::create(
						progress(ButtonHighLightDuration, 0.0f, _animationProgress), 1.0f,
						[this] (ProgressAction *a, float progress) {
					updateSelection(progress, ButtonHighLightBasicOpacity * 255.0f);
				}, [this] (ProgressAction *a) {
					a->setSourceProgress(getSelectionProgress());
					beginSelection();
				}, [this] (ProgressAction *a) {
					endSelection();
				}));
				a->setTag(2);
				runAction(a);
			} else {
				beginSelection();
				updateSelection(1.0f, ButtonHighLightBasicOpacity * 255.0f);
				endSelection();
			}
		} else {
			animateDeselection();
		}
	}
}

bool ButtonHighLight::isSelected() const {
	return _selected;
}

void ButtonHighLight::setSpawnDelay(float s) {
	_spawnDelay = s;
}

void ButtonHighLight::setAnimationColor(Color c) {
	_background->setColor(c);
	_animationColor = c;
}

void ButtonHighLight::setAnimationOpacity(uint8_t op) {
	_animationOpacity = op;
}

void ButtonHighLight::setClipNode(Node *node, const Function<void(Size)> &cb) {
	_clip->setStencil(node);
	_sizeCallback = cb;
}

void ButtonHighLight::beginSelection() {
	_clip->setVisible(true);
	if (_animationProgress == 0.0f) {
		_beginCallback();
	}
}

void ButtonHighLight::endSelection() {
	if (_animationProgress == 0.0f) {
		_endCallback();
	}
}

void ButtonHighLight::updateSelection(float pr, uint8_t tar) {
	_animationProgress = pr;
	uint8_t op = (uint8_t)progress(0.0f, float(tar), pr);
	_background->setOpacity(op);
	_progressCallback(pr);

	if (pr == 0.0f && !_selected) {
		_clip->setVisible(false);
	} else {
		_clip->setVisible(true);
	}
}

float ButtonHighLight::getSelectionProgress() const {
	return _animationProgress;
}

draw::Image::PathRef ButtonHighLight::beginSpawn() {
	float downscale = 4.0f;
	uint32_t width = _contentSize.width / downscale;
	uint32_t height = _contentSize.height / downscale;

	if (_animationNode && _animationNode->getBaseWidth() == width && _animationNode->getBaseHeight() == height) {
		// do nothing, our node is perfect
	} else {
		if (_animationNode) {
			_animationNode->removeFromParent();
			_animationNode = nullptr;
		}
		auto animationNode = Rc<draw::PathNode>::create(width, height);
		animationNode->setContentSize(Size(_contentSize.width / downscale, _contentSize.height / downscale));
		animationNode->setPosition(Vec2(0, 0));
		animationNode->setAnchorPoint(Vec2(0, 0));
		animationNode->setScale(downscale);
		animationNode->setColor(_animationColor);
		animationNode->setOpacity(_animationOpacity);
		animationNode->setAntialiased(true);
		_animationNode = _clip->addChildNode(animationNode, 1);
	}

	auto path = _animationNode->addPath();
	_animationCount ++;

	return path;
}

void ButtonHighLight::updateSpawn(float pr, draw::Image::PathRef &path, const Size &size, const Vec2 &point) {
	if (_animationNode) {
		float rad = 0.0f;

		if (pr < 0.35f) {
			path.setFillOpacity(uint8_t(progress(0, 255, pr * 5.0f)));
		} else {
			path.setFillOpacity(255);
		}

		float a = point.length();
		float b = Vec2(point.x, size.height - point.y).length();
		float c = Vec2(size.width - point.x, size.height - point.y).length();
		float d = Vec2(size.width - point.x, point.y).length();

		float minRad = 0.125f;
		float maxRad = std::max(std::max(a, b), std::max(c, d));
		rad = progress(minRad, maxRad, pr);

		path.clear().addCircle(point.x, size.height - point.y, rad);
	}
}

void ButtonHighLight::endSpawn() {
	if (_animationCount > 0) {
		_animationCount --;
	}
	if (_animationCount == 0 && _animationNode) {
		if (_highlighted) {
			auto opf = 1.0f - (1.0f - ButtonHighLightBasicOpacity) * (1.0f - (_animationOpacity / 255.0f));
			auto op = uint8_t(opf * 255.0f);
			_background->setOpacity(op);
			stopActionByTag(ButtonHighLightSelectAction);
		}
		_animationNode->removeFromParent();
		_animationNode = nullptr;
	}
}

void ButtonHighLight::dropSpawn() {
	if (_animationNode) {
		_animationCount = 0;
		_animationNode->runAction(cocos2d::Sequence::createWithTwoActions(
				cocos2d::FadeTo::create(ButtonHighLightDuration / 3.0f, 0), cocos2d::RemoveSelf::create()));
		_animationNode = nullptr;
	} else {
		stopActionByTag(ButtonHighLightSpawnAction);
	}
}

}
