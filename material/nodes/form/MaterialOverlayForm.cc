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
#include "MaterialFormController.h"
#include "MaterialOverlayForm.h"
#include "MaterialOverlayLayout.h"
#include "MaterialScene.h"

#include "SPIME.h"
#include "SPScrollView.h"
#include "SPScrollController.h"

NS_MD_BEGIN

bool OverlayForm::init(const data::Value &data, float width) {
	if (!MaterialNode::init()) {
		return false;
	}

	auto scroll = Rc<ScrollView>::create(ScrollView::Vertical);
	scroll->setController(Rc<ScrollController>::create());
	scroll->setVisible(false);
	addChild(scroll);

	_scroll = scroll;

	auto formController = Rc<FormController>::create(data);
	addComponent(formController);
	_formController = formController;

	auto c = _scroll->getController();
	c->setKeepNodes(true);

	_basicWidth = width;
	setShadowZIndex(2.0f);

	return true;
}

void OverlayForm::onContentSizeDirty() {
	MaterialNode::onContentSizeDirty();

	_scroll->setPosition(0.0f, 0.0f);
	_scroll->setContentSize(_contentSize);
}

void OverlayForm::setCompleteCallback(const CompleteCallback &cb) {
	_completeCallback = cb;
}
const OverlayForm::CompleteCallback &OverlayForm::getCompleteCallback() const {
	return _completeCallback;
}

void OverlayForm::setSubmitCallback(const SubmitCallback &cb) {
	_submitCallback = cb;
}
const OverlayForm::SubmitCallback &OverlayForm::getSubmitCallback() const {
	return _submitCallback;
}

void OverlayForm::setSaveCallback(const SaveCallback &cb) {
	_formController->setSaveCallback(cb);
}
const OverlayForm::SaveCallback &OverlayForm::getSaveCallback() const {
	return _formController->getSaveCallback();
}

bool OverlayForm::isOpen() const {
	return _isOpen;
}

void OverlayForm::push() {
	if (!_layout) {
		auto scene = material::Scene::getRunningScene();
		if (scene) {
			auto l = Rc<material::OverlayLayout>::create(this,
					Size(_basicWidth, _scroll->getController()->getNextItemPosition()),
					[this] {
				close();
			});
			l->setKeyboardTracked(true);

			_layout = l;
			scene->pushOverlayNode(_layout);
			onOpen();
		}
	}
}

void OverlayForm::push(const Vec2 &vec) {
	if (!_layout) {
		auto scene = material::Scene::getRunningScene();
		if (scene) {
			auto sceneLoc = scene->convertToScene(vec);
			auto l = Rc<material::OverlayLayout>::create(this,
					Size(_basicWidth, _scroll->getController()->getNextItemPosition()),
					[this] {
				close();
			});
			l->setKeyboardTracked(true);
			l->setOpenAnimation(sceneLoc, [this] {
				onOpen();
			});

			_layout = l;
			scene->pushOverlayNode(_layout);
		}
	}
}

void OverlayForm::close() {
	if (_layout) {
		_layout->cancel();
		_layout = nullptr;
	}
}

bool OverlayForm::submit() {
	ime::cancel();
	if (_submitCallback) {
		retain();
		_formController->setEnabled(false);

		class Target : public Ref {
		public:
			Target(OverlayForm *f) : form(f) { }
			virtual ~Target() { }

			bool called = false;
			Rc<OverlayForm> form;
		};

		Rc<Target> target = Rc<Target>::alloc(this);

		auto ret = _submitCallback(_formController->collect(true), [this, target] (bool success, data::Value && data) {
			auto ret = complete(success, move(data));
			target->called = true;
			return ret;
		});
		if (!ret) {
			_formController->setEnabled(true);
		}
		return true;
	}
	return false;
}

bool OverlayForm::complete(bool success, data::Value &&data) {
	bool ret = false;
	if (_completeCallback) {
		ret = _completeCallback(success, move(data));
	}
	_formController->setEnabled(true);
	if (ret) {
		close();
	}
	return ret;
}

void OverlayForm::onOpen() {
	_scroll->setVisible(true);
}

NS_MD_END
