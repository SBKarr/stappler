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

#include "MaterialMultiViewLayout.h"
#include "SPGestureListener.h"
#include "SPScrollView.h"
#include "SPScrollController.h"
#include "SPActions.h"

NS_MD_BEGIN

bool MultiViewLayout::Generator::init() {
	if (!Component::init()) {
		return false;
	}

	return true;
}

bool MultiViewLayout::Generator::init(size_t count, const IndexViewCallback &gen) {
	if (!Component::init()) {
		return false;
	}

	_viewCount = count;
	_makeViewByIndex = gen;

	return true;
}
bool MultiViewLayout::Generator::init(const SequenceViewCallback &seq) {
	if (!Component::init()) {
		return false;
	}

	_makeViewSeq = seq;

	return true;
}

bool MultiViewLayout::Generator::isViewLocked(ScrollView *, int64_t index) {
	return !_enabled;
}

Rc<ScrollView> MultiViewLayout::Generator::makeIndexView(int64_t viewIndex) {
	if (_makeViewSeq) {
		return _makeViewSeq(nullptr, viewIndex, 0);
	} else if (viewIndex >= 0 && viewIndex < int64_t(_viewCount) && _makeViewByIndex) {
		return _makeViewByIndex(viewIndex);
	}
	return nullptr;
}

Rc<ScrollView> MultiViewLayout::Generator::makeNextView(ScrollView *view, int64_t viewIndex) {
	if (!_enabled) {
		return nullptr;
	}
	if (_makeViewSeq) {
		return _makeViewSeq(view, viewIndex, 1);
	} else if (_makeViewByIndex) {
		return _makeViewByIndex(viewIndex + 1);
	}
	return nullptr;
}

Rc<ScrollView> MultiViewLayout::Generator::makePrevView(ScrollView *view, int64_t viewIndex) {
	if (!_enabled) {
		return nullptr;
	}
	if (_makeViewSeq) {
		return _makeViewSeq(view, viewIndex, -1);
	} else if (_makeViewByIndex) {
		return _makeViewByIndex(viewIndex - 1);
	}
	return nullptr;
}

bool MultiViewLayout::Generator::isInfinite() const {
	return _viewCount == maxOf<size_t>();
}

int64_t MultiViewLayout::Generator::getViewCount() const {
	return _viewCount;
}

void MultiViewLayout::Generator::setViewSelectedCallback(const ViewCallback &cb) {
	_viewSelectedCallback = cb;
}

void MultiViewLayout::Generator::setViewCreatedCallback(const ViewCallback &cb) {
	_viewCreatedCallback = cb;
}

void MultiViewLayout::Generator::setApplyViewCallback(const ViewCallback &cb) {
	_applyViewCallback = cb;
}

void MultiViewLayout::Generator::setApplyProgressCallback(const ProgressCallback &cb) {
	_applyProgressCallback = cb;
}

void MultiViewLayout::Generator::onViewSelected(ScrollView *current, int64_t id) {
	if (_viewSelectedCallback) {
		_viewSelectedCallback(current, id);
	}
}
void MultiViewLayout::Generator::onViewCreated(ScrollView *current, int64_t id) {
	if (_viewCreatedCallback) {
		_viewCreatedCallback(current, id);
	}
}

void MultiViewLayout::Generator::onApplyView(ScrollView *current, int64_t id) {
	if (_applyViewCallback) {
		_applyViewCallback(current, id);
	}
}
void MultiViewLayout::Generator::onApplyProgress(ScrollView *current, ScrollView *n, float progress) {
	if (_applyProgressCallback) {
		_applyProgressCallback(current, n, progress);
	}
}

MultiViewLayout *MultiViewLayout::Generator::getLayout() {
	return dynamic_cast<MultiViewLayout *>(_owner);
}

bool MultiViewLayout::init(Generator *gen, material::ToolbarBase *toolbar) {
	if (!ToolbarLayout::init(toolbar)) {
		return false;
	}

	if (!gen) {
		auto g = Rc<Generator>::create();
		_generator = addComponentItem(g);
	} else {
		_generator = addComponentItem(gen);
	}

	auto g = Rc<gesture::Listener>::create();
	g->setSwipeCallback([this] (gesture::Event ev, const gesture::Swipe &s) -> bool {
		if (ev == gesture::Event::Began) {
			return beginSwipe(s.firstTouch.point - s.firstTouch.startPoint);
		} else if (ev == gesture::Event::Activated) {
			return setSwipeProgress(s.delta / screen::density());
		} else {
			return endSwipeProgress(s.delta / screen::density(), s.velocity / screen::density());
		}
		return true;
	}, 20.0f, false);
	_swipeListener = addComponentItem(g);

	return true;
}

void MultiViewLayout::onPush(material::ContentLayer *l, bool replace) {
	ToolbarLayout::onPush(l, replace);

	if (!_currentView) {
		if (auto view = makeInitialView()) {
			_currentView = view;
			_nextView = nullptr;

			onViewSelected(_currentView, _currentViewIndex);
			setBaseNode(_currentView, 1);
			applyView(_currentView);
		}
	}
}

void MultiViewLayout::setGenerator(Generator *gen) {
	if (_generator) {
		removeComponent(_generator);
	}
	_generator = addComponentItem(gen);
}

MultiViewLayout::Generator *MultiViewLayout::getGenerator() const {
	return _generator;
}

int64_t MultiViewLayout::getCurrentIndex() const {
	return _currentViewIndex;
}

void MultiViewLayout::showNextView(float val) {
	setFlexibleLevelAnimated(1.0f, val);
	auto a = Rc<ProgressAction>::create(val, [this] (ProgressAction *a, float time) {
		_swipeProgress = _contentSize.width * time * -1.0f;
		onSwipeProgress();
	}, [] (ProgressAction *a) {

	}, [this] (ProgressAction *a) {
		endSwipeProgress(Vec2(0.0f, 0.0f), Vec2(0.0f, 0.0f));
	});
	runAction(cocos2d::EaseQuadraticActionInOut::create(a), "ListAction"_tag);
}

void MultiViewLayout::showPrevView(float val) {
	setFlexibleLevelAnimated(1.0f, val);
	auto a = Rc<ProgressAction>::create(val, [this] (ProgressAction *a, float time) {
		_swipeProgress = _contentSize.width * time;
		onSwipeProgress();
	}, [] (ProgressAction *a) {

	}, [this] (ProgressAction *a) {
		endSwipeProgress(Vec2(0.0f, 0.0f), Vec2(0.0f, 0.0f));
	});
	runAction(cocos2d::EaseQuadraticActionInOut::create(a), "ListAction"_tag);
}

void MultiViewLayout::showIndexView(int64_t idx, float val) {
	if (_currentViewIndex < idx) {
		_currentViewIndex = idx - 1;
		showNextView(val);
	} else if (_currentViewIndex > idx) {
		_currentViewIndex = idx + 1;
		showPrevView(val);
	}
}

Rc<ScrollView> MultiViewLayout::makeInitialView() {
	return _generator->makeIndexView(_currentViewIndex);
}

bool MultiViewLayout::onSwipeProgress() {
	if (!_currentView) {
		return false;
	}

	_currentView->setPositionX(_swipeProgress);

	if (_swipeProgress > 0.0f) {
		if (_nextView) {
			_nextView->removeFromParent();
			_nextView = nullptr;
		}

		if (!_prevView) {
			//log::format("MultiViewLayout", "prevView %d", _currentViewIndex - 1);
			if (auto view = _generator->makePrevView(_currentView, _currentViewIndex)) {
				onPrevViewCreated(_currentView, view);
				_prevView = addChildNode(view, _currentView->getLocalZOrder());
			}
		}

		if (!_prevView) {
			return false;
		}

		if (_prevView) {
			applyViewProgress(_currentView, _prevView, fabs(_swipeProgress) / _contentSize.width);
		}
	} else if (_swipeProgress < 0.0f) {
		if (_prevView) {
			_prevView->removeFromParent();
			_prevView = nullptr;
		}

		if (!_nextView) {
			if (auto view = _generator->makeNextView(_currentView, _currentViewIndex)) {
				onNextViewCreated(_currentView, view);
				_nextView = addChildNode(view, _currentView->getLocalZOrder());
			}
		}

		if (!_nextView) {
			return false;
		}

		if (_nextView) {
			applyViewProgress(_currentView, _nextView, fabs(_swipeProgress) / _contentSize.width);
		}
	} else {
		if (_nextView) {
			_nextView->removeFromParent();
			_nextView = nullptr;
		}

		if (_prevView) {
			_prevView->removeFromParent();
			_prevView = nullptr;
		}

		applyView(_currentView);
	}
	if (_nextView) {
		_nextView->setPositionX(_swipeProgress + _contentSize.width);
	}
	if (_prevView) {
		_prevView->setPositionX(_swipeProgress - _contentSize.width);
	}
	return true;
}

bool MultiViewLayout::beginSwipe(const Vec2 &diff) {
	if (_currentView) {
		if (_generator->isViewLocked(_currentView, _currentViewIndex)) {
			return false;
		}
	} else {
		return false;
	}
	if (_swipeProgress > _contentSize.width * 0.75f && _prevView) {
		setPrevView(_currentViewIndex - 1, _swipeProgress - _contentSize.width);
	} else if (_swipeProgress < -_contentSize.width * 0.75f && _nextView) {
		setNextView(_currentViewIndex + 1, _swipeProgress + _contentSize.width);
	}
	if (fabs(diff.x) < fabs(diff.y)) {
		return false;
	} else {
		setFlexibleLevelAnimated(1.0f, 0.25f);
	}
	return true;
}

bool MultiViewLayout::setSwipeProgress(const Vec2 &delta) {
	_currentView->stopActionByTag("ListAction"_tag);
	float diff = delta.x;
	if (!_generator->isInfinite()) {
		if (_currentViewIndex == 0 && _swipeProgress + diff > 0.0f) {
			_swipeProgress = 0.0f;
			onSwipeProgress();
		} else if (_currentViewIndex == _generator->getViewCount() - 1 && _swipeProgress + diff < 0.0f) {
			_swipeProgress = 0.0f;
			onSwipeProgress();
		} else {
			_swipeProgress += diff;
			onSwipeProgress();
		}
	} else {
		_swipeProgress += diff;
		if (!onSwipeProgress()) {
			_swipeProgress = 0.0f;
			onSwipeProgress();
			return false;
		}
	}
	return true;
}

bool MultiViewLayout::endSwipeProgress(const Vec2 &delta, const Vec2 &velocity) {
	if (!_currentView) {
		return false;
	}
	/*if (fabsf(_swipeProgress) < 20.0f) {
		_swipeProgress = 0.0f;
		onSwipeProgress();
		return false;
	}*/

	float v = fabsf(velocity.x);
	float a = 5000;
	float t = v / a;
	float dx = v * t - (a * t * t) / 2;

	float pos = _swipeProgress + ((velocity.x > 0) ? (dx) : (-dx));

	cocos2d::FiniteTimeAction *action = nullptr;
	if (pos > _contentSize.width / 2 && _prevView) {
		action = action::sequence(
				Accelerated::createBounce(5000, Vec2(_swipeProgress, 0), Vec2(_contentSize.width, 0.0f), Vec2(velocity.x, 0),
						200000, std::bind(&MultiViewLayout::onSwipeAction, this, std::placeholders::_1)),
				[this] {
			setPrevView(_currentViewIndex - 1);
		});
	} else if (pos < -_contentSize.width / 2 && _nextView) {
		action = action::sequence(
				Accelerated::createBounce(5000, Vec2(_swipeProgress, 0), Vec2(-_contentSize.width, 0.0f), Vec2(velocity.x, 0),
						200000, std::bind(&MultiViewLayout::onSwipeAction, this, std::placeholders::_1)),
				[this] {
			setNextView(_currentViewIndex + 1);
		});
	} else {
		action = action::sequence(
				Accelerated::createBounce(5000, Vec2(_swipeProgress, 0), Vec2(0, 0), Vec2(velocity.x, 0), 50000,
						std::bind(&MultiViewLayout::onSwipeAction, this, std::placeholders::_1)),
				[this] {
			_swipeProgress = 0;
			onSwipeProgress();
		});
	}
	_currentView->runAction(action, "ListAction"_tag);
	return true;
}

void MultiViewLayout::onSwipeAction(cocos2d::Node *node) {
	if (node == _currentView) {
		_swipeProgress = _currentView->getPositionX();
		onSwipeProgress();
	}
}

void MultiViewLayout::setNextView(int64_t id, float newProgress) {
	_currentViewIndex = id;
	if (_nextView) {
		_currentView = _nextView;
		_nextView = nullptr;

		onViewSelected(_currentView, id);
		setBaseNode(_currentView, 1);
	}
	_swipeProgress = newProgress;
	onSwipeProgress();
}

void MultiViewLayout::setPrevView(int64_t id, float newProgress) {
	_currentViewIndex = id;
	if (_prevView) {
		_currentView = _prevView;
		_prevView = nullptr;

		onViewSelected(_currentView, id);
		setBaseNode(_currentView, 1);
	}
	_swipeProgress = newProgress;
	onSwipeProgress();
}

void MultiViewLayout::onViewSelected(ScrollView *current, int64_t id) {
	_generator->onViewSelected(current, id);
}

void MultiViewLayout::onPrevViewCreated(ScrollView *current, ScrollView *prev) {
	prev->setContentSize(current->getContentSize());
	prev->setPosition(current->getPosition());
	prev->setPadding(current->getPadding());
	_generator->onViewCreated(prev, _currentViewIndex - 1);
}

void MultiViewLayout::onNextViewCreated(ScrollView *current, ScrollView *next) {
	next->setContentSize(current->getContentSize());
	next->setPosition(current->getPosition());
	next->setPadding(current->getPadding());
	_generator->onViewCreated(next, _currentViewIndex + 1);
}

void MultiViewLayout::applyView(ScrollView *current) {
	_generator->onApplyView(current, _currentViewIndex);
}

void MultiViewLayout::applyViewProgress(ScrollView *current, ScrollView *n, float p) {
	_generator->onApplyProgress(current, n, p);
}

NS_MD_END
