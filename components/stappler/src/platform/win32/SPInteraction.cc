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
#include "SPThread.h"

#if (CYGWIN)

NS_SP_PLATFORM_BEGIN

namespace interaction {
	bool _dialogOpened = false;
	void _goToUrl(const std::string &url, bool external) {
		stappler::log::format("Intercation", "GoTo url: %s", url.c_str());
	}
	void _makePhoneCall(const std::string &str) {
		stappler::log::format("Intercation", "phone: %s", str.c_str());
	}
	void _mailTo(const std::string &address) {
		stappler::log::format("Intercation", "MailTo phone: %s", address.c_str());
	}
	void _backKey() { }
	void _notification(const std::string &title, const std::string &text) {

	}
	void _rateApplication() {
		stappler::log::text("Intercation", "Rate app");
	}

	void _openFileDialog(const std::string &path, const std::function<void(const std::string &)> &func) {
		if (func) {
			func("");
		}
	}
}

NS_SP_PLATFORM_END

#endif
