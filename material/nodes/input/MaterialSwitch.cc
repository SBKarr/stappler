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
#include "MaterialSwitch.h"

#include "MaterialNode.h"
#include "SPRoundedSprite.h"
#include "SPShadowSprite.h"
#include "SPActions.h"

#include "SPGestureListener.h"

NS_MD_BEGIN

static void createShadowSwitchGaussianData(uint8_t *data, uint32_t size) {
	float factor = 0.5f;
	float sigma = (size * factor) * sqrtf(1.0f / (-2.0f * logf(1.0f / 255.0f)));

	for (uint32_t i = 0; i < size; i++) {
		for (uint32_t j = 0; j < size; j++) {
			float dist = sqrtf( (i * i) + (j * j) );
			if (dist > size) {
				data[i * size + j] = 0;
			} else if (dist < size * (1.0f - factor)) {
				data[i * size + j] = 255;
			} else {
				dist -= size * (1.0f - factor);
				data[i * size + j] = (uint8_t)(255.0f * expf( -1.0f * (dist * dist) / (2.0f * sigma * sigma) ));
			}
		}
	}
}

class SwitchShadow : public CustomCornerSprite {
public:
	class ShadowTexture;

	static std::map<uint32_t, ShadowTexture *> s_textures;

	class ShadowTexture : public CustomCornerSprite::Texture {
	public:
		virtual bool init(uint32_t size) override{
			if (!Texture::init(size)) {
				return false;
			}

			s_textures.insert(std::make_pair(_size, this));
			return true;
		}

		virtual ~ShadowTexture(){
			s_textures.erase(_size);
		}

	    virtual uint8_t *generateTexture(uint32_t size) override {
	    	auto data = Texture::generateTexture(size);
	    	createShadowSwitchGaussianData(data, size);
	    	return data;
	    }
	};

protected:

	virtual Rc<Texture> generateTexture(uint32_t r) {
		auto it = s_textures.find(r);
		if (it == s_textures.end()) {
			return Rc<ShadowTexture>::create(r);
		} else {
			return it->second;
		}
	}
};

std::map<uint32_t, SwitchShadow::ShadowTexture *> SwitchShadow::s_textures;

bool Switch::init(const Callback &cb) {
	if (!Node::init()) {
		return false;
	}

	_callback = cb;

	auto listener = Rc<gesture::Listener>::create();
	listener->setPressCallback([this] (gesture::Event ev, const gesture::Press &p) -> bool {
		if (ev == gesture::Event::Began) {
			return onPressBegin(p.location());
		} else if (ev == gesture::Event::Ended) {
			return onPressEnded(p.location());
		} else if (ev == gesture::Event::Cancelled) {
			return onPressCancelled(p.location());
		}
		return true;
	});
	listener->setSwipeCallback([this] (gesture::Event ev, const gesture::Swipe &s) -> bool {
		if (ev == gesture::Event::Began) {
			return onSwipeBegin(s.location());
		} else if (ev == gesture::Event::Activated) {
			return onSwipe(s.delta);
		} else {
			return onSwipeEnded(s.location());
		}
		return true;
	});
	_listener = addComponentItem(listener);

	setCascadeOpacityEnabled(true);

	auto select = Rc<RoundedSprite>::create(20);
	select->setAnchorPoint(Vec2(0.5f, 0.5f));
	select->setContentSize(Size(40.0f, 40.0f));
	select->setColor(Color::Grey_500);
	select->setOpacity(32);
	select->setVisible(false);
	_select = addChildNode(select, 4);

	auto thumb = Rc<RoundedSprite>::create(8);
	thumb->setContentSize(Size(16.0f, 16.0f));
	thumb->setAnchorPoint(Vec2(0.5f, 0.5f));
	thumb->setColor(Color::Grey_50);
	_thumb = addChildNode(thumb, 3);

	auto shadow = Rc<SwitchShadow>::create(11);
	shadow->setContentSize(Size(22.0f, 22.0f));
	shadow->setAnchorPoint(Vec2(0.5f, 0.525f));
	shadow->setColor(Color::Black);
	shadow->setOpacity(168);
	_shadow = addChildNode(shadow, 2);

	auto track = Rc<RoundedSprite>::create(5);
	track->setAnchorPoint(Vec2(0.5f, 0.5f));
	track->setColor(Color::Black);
	track->setOpacity(64);
	track = addChildNode(track, 1);

	setContentSize(Size(42.0f, 24.0f));

	return true;
}

void Switch::onContentSizeDirty() {
	Node::onContentSizeDirty();

	_track->setContentSize(Size(_contentSize.width - 16.0f, 10.0f));
	_track->setPosition(_contentSize.width / 2.0f, _contentSize.height / 2.0);

	_select->setPositionY(_contentSize.height / 2.0);
	_select->setPositionX(12.0f + (_contentSize.width - 24.0f) * _progress);

	_thumb->setPositionY(_contentSize.height / 2.0);
	_thumb->setPositionX(12.0f + (_contentSize.width - 24.0f) * _progress);

	_shadow->setPositionY(_contentSize.height / 2.0);
	_shadow->setPositionX(12.0f + (_contentSize.width - 24.0f) * _progress);

	if (_enabled) {
		_track->setColor(progress(Color::Black, _selectedColor, _progress));
		_track->setOpacity(progress(64, 128, _progress));
		_thumb->setColor(progress(Color::Grey_50, _selectedColor, _progress));
	} else {
		_track->setColor(Color::Black);
		_track->setOpacity(32);
		_thumb->setColor(Color::Grey_400);
	}
}

void Switch::toggle(float duration) {
	if (_enabled) {
		setSelected(!_selected, duration);
	}
}

void Switch::setSelected(bool value, float duration) {
	if (_selected != value) {
		_selected = value;
		if (_callback) {
			_callback(_selected);
		}
		stopAllActionsByTag("ProgressAnimation"_tag);
		if (duration == 0.0f) {
			onAnimation(_selected?1.0f:0.0f);
			_contentSizeDirty = true;
		} else {
			auto a = Rc<ProgressAction>::create(duration, _progress, _selected?1.0f:0.0f,
					std::bind(&Switch::onAnimation, this, std::placeholders::_2));
			runAction(a, "ProgressAnimation"_tag);
		}
	}
}

bool Switch::isSelected() const {
	return _selected;
}

void Switch::setEnabled(bool value) {
	if (_enabled != value) {
		_enabled = value;
		stopAllActions();
		_contentSizeDirty = true;
	}
}
bool Switch::isEnabled() const {
	return _enabled;
}

void Switch::setSelectedColor(const Color &c) {
	_selectedColor = c;
	_contentSizeDirty = true;
}
const Color &Switch::getSelectedColor() const {
	return _selectedColor;
}

void Switch::setCallback(const Callback &cb) {
	_callback = cb;
}

const Switch::Callback &Switch::getCallback() const {
	return _callback;
}

void Switch::onAnimation(float progress) {
	_progress = progress;
	_contentSizeDirty = true;
}

bool Switch::onPressBegin(const Vec2 &vec) {
	if (_enabled) {
		_select->stopAllActions();
		_select->setVisible(true);
		_select->setOpacity(0);
		_select->runAction(cocos2d::FadeTo::create(0.15, 48));
		return true;
	}
	return false;
}
bool Switch::onPressEnded(const Vec2 &vec) {
	toggle(0.20f);
	return onPressCancelled(vec);
}
bool Switch::onPressCancelled(const Vec2 &) {
	if (!_listener->hasGesture(gesture::Type::Swipe)) {
		_select->stopAllActions();
		_select->runAction(action::sequence(cocos2d::FadeTo::create(0.15, 48), cocos2d::Hide::create()));
	}
	return true;
}

bool Switch::onSwipeBegin(const Vec2 &vec) {
	return _enabled;
}
bool Switch::onSwipe(const Vec2 &delta) {
	float d = (delta.x / screen::density()) / (_contentSize.width - 16.0f);
	float newP = _progress + d;
	if (newP < 0.0f) {
		newP = 0.0f;
	} else if (newP > 1.0f) {
		newP = 1.0f;
	}

	_progress = newP;
	_contentSizeDirty = true;
	return true;
}
bool Switch::onSwipeEnded(const Vec2 &vec) {
	if (_progress < 0.5) {
		setSelected(false, progress(0.0f, 0.15f, _progress * 2.0f));
	} else {
		setSelected(true, progress(0.15f, 0.0f, (_progress - 0.5f) * 2.0f));
	}
	_select->stopAllActions();
	_select->runAction(action::sequence(cocos2d::FadeTo::create(0.15, 48), cocos2d::Hide::create()));
	return true;
}

NS_MD_END
