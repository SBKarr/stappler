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

#ifndef COMPONENTS_XENOLITH_PLATFORM_COMMON_XLPLATFORM_H_
#define COMPONENTS_XENOLITH_PLATFORM_COMMON_XLPLATFORM_H_

#include "XLDefine.h"
#include "SPCommonPlatform.h"

/* Platform-depended functions interface */
namespace stappler::xenolith::platform {

namespace network {
	void _setNetworkCallback(const Function<void(bool isOnline)> &callback);
	bool _isNetworkOnline();
}

namespace device {
	bool _isTablet();

	String _userAgent();
	String _deviceIdentifier();
	String _bundleName();

	String _userLanguage();
	String _applicationName();
	String _applicationVersion();

	Pair<uint64_t, uint64_t> _diskSpace(); // <total, free>

	void _onDirectorStarted();

	void _sleep(double value);
}

namespace interaction {
	bool _goToUrl(const StringView &url, bool external);
	void _makePhoneCall(const StringView &number);
	void _mailTo(const StringView &address);

	void _backKey();
	void _notification(const StringView &title, const StringView &text);
	void _rateApplication();
}

namespace statusbar {
	enum class _StatusBarColor : uint8_t {
        Light = 1,
        Black = 2,
	};
	void _setEnabled(bool enabled);
	bool _isEnabled();
	void _setColor(_StatusBarColor color);
	float _getHeight(const Size &screenSize, bool isTablet);
}

}


#if (MSYS || CYGWIN || LINUX || MACOS)

namespace stappler::xenolith::platform::desktop {
	void setScreenSize(const Size &size);
	Size getScreenSize();
	bool isTablet();
	bool isFixed();
	String getPackageName();
	float getDensity();
	String getUserLanguage();
	String getAppVersion();
}

#include "XLVkViewImpl-desktop.h"

#endif


#endif
