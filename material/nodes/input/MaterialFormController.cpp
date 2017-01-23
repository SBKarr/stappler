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
#include "MaterialFormController.h"

NS_MD_BEGIN

SP_DECLARE_EVENT_CLASS(FormController, onForceCollect);
SP_DECLARE_EVENT_CLASS(FormController, onForceUpdate);

bool FormController::init(const data::Value &data, const SaveCallback &cb) {
	return init(data.asDict(), cb);
}
bool FormController::init(const Map<String, data::Value> &data, const SaveCallback &cb) {
	if (!Component::init()) {
		return false;
	}

	_data = data;
	_saveCallback = cb;

	return true;
}

void FormController::onExit() {
	Component::onExit();
	save(false);
}

void FormController::setSaveCallback(const SaveCallback &cb) {
	_saveCallback = cb;
}
const FormController::SaveCallback &FormController::getSaveCallback() const {
	return _saveCallback;
}

data::Value FormController::getValue(const String &key) const {
	auto it = _data.find(key);
	if (it != _data.end()) {
		return it->second;
	}
	return data::Value();
}
void FormController::setValue(const String &key, const data::Value &value) {
	log::format("FormController", "%s - %s", key.c_str(), data::toString(value, true).c_str());
	auto it = _data.find(key);
	if (it != _data.end()) {
		it->second = value;
	} else {
		_data.emplace(key, value);
	}
}

void FormController::reset() {
	onForceUpdate(this);
}

void FormController::reset(const data::Value &data) {
	reset(data.asDict());
}

void FormController::reset(const Map<String, data::Value> &data) {
	_data = data;
	reset();
}

data::Value FormController::collect(bool force) {
	if (force) {
		onForceCollect(this);
	}
	return data::Value(_data);
}

void FormController::save(bool force) {
	if (_saveCallback) {
		_saveCallback(collect(force));
	}
}

NS_MD_END
