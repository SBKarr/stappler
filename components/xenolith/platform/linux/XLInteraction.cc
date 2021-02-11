/**
Copyright (c) 2021 Roman Katuntsev <sbkarr@stappler.org>

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

#include "XLPlatform.h"

#if (LINUX)

namespace stappler::xenolith::platform::interaction {
	bool _goToUrl(const StringView &url, bool external) {
		log::format("Interaction", "GoTo url: %s", url.data());
		::system(toString("xdg-open ", url).data());
		return true;
	}
	void _makePhoneCall(const StringView &str) {
		log::format("Interaction", "phone: %s", str.data());
		::system(toString("xdg-open ", str).data());
	}
	void _mailTo(const StringView &address) {
		log::format("Interaction", "MailTo: %s", address.data());
		::system(toString("xdg-open ", address).data());
	}
	void _backKey() { }
	void _notification(const StringView &title, const StringView &text) {

	}
	void _rateApplication() {
		log::text("Interaction", "Rate app");
	}

	void _openFileDialog(const String &path, const Function<void(const String &)> &func) {
		if (func) {
			func("");
		}
	}
}

#endif
