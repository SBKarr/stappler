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

#include "SPDefine.h"
#include "SPPlatform.h"

#if (CYGWIN || MSYS)

namespace stappler::platform::ime {
	Function<void(bool)> _enabledCallback;

	void setEnabledCallback(const Function<void(bool)> &f) {
		_enabledCallback = f;
	}
	void _updateCursor(uint32_t pos, uint32_t len) {

	}

	void _updateText(const WideString &str, uint32_t pos, uint32_t len, int32_t) {

	}

	void _runWithText(const WideString &str, uint32_t pos, uint32_t len, int32_t) {
		if (_enabledCallback) {
			_enabledCallback(true);
		}
#ifndef SP_RESTRICT
		auto size = Screen::getInstance()->getFrameSize();
		native::onKeyboardShow(Rect(0, 0, size.width, size.height * 0.35f), 0.0f);
#endif
	}

	void _cancel() {
		if (_enabledCallback) {
			_enabledCallback(false);
		}
#ifndef SP_RESTRICT
		native::onKeyboardHide(0.0f);
#endif
	}
}

#endif
