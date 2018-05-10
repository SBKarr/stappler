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

#ifndef stappler_SPPlatform_h
#define stappler_SPPlatform_h

/* Do NOT include this header into public headers! */

#include "SPDefine.h"
#include "SPCommonPlatform.h"

/* Platform-depended functions interface */
NS_SP_PLATFORM_BEGIN

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

	int _dpi();
	float _density();
	Size _screenSize();
	Size _viewportSize(const Size &screenSize, bool isTablet);

	float _statusBarHeight(const Size &screenSize, bool isTablet);
	float _designScale(const Size &screenSize, bool isTablet);

	const ScreenOrientation &_currentDeviceOrientation();

	Pair<uint64_t, uint64_t> _diskSpace(); // <total, free>

	void _onDirectorStarted();
}

namespace interaction {
	void _goToUrl(const StringView &url, bool external);
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

namespace clipboard {
	bool _isAvailable();
	String _getString();
	void _copyString(const String &value);
}

namespace ime {
	void _updateCursor(uint32_t pos, uint32_t len);
	void _updateText(const WideString &str, uint32_t pos, uint32_t len, int32_t);
	void _runWithText(const WideString &str, uint32_t pos, uint32_t len, int32_t);
	void _cancel();

	namespace native {
		bool hasText();
		void insertText(const WideString &str);
		void deleteBackward();
		void textChanged(const WideString &text, uint32_t cursorStart, uint32_t cursorLen);
		void cursorChanged(uint32_t cursorStart, uint32_t cursorLen);
		void onKeyboardShow(const Rect &rect, float duration);
		void onKeyboardHide(float duration);
		void setInputEnabled(bool enabled);
		void cancel();
	}
}

namespace render {
	void _init();
	void _requestRender();
	void _framePerformed();
	void _blockRendering();
	void _unblockRendering();

	bool _enableOffscreenContext();
	void _disableOffscreenContext();
}

#if (SP_INTERNAL_STOREKIT_ENABLED)
namespace storekit {
	void _saveProducts(const std::unordered_map<String, StoreProduct *> &);
	data::Value _loadProducts();

	void _updateProducts(const Vector<StoreProduct *> &);
	bool _purchaseProduct(StoreProduct *product);
	bool _restorePurchases();
}
#endif

namespace debug {
	void _init(const String &addr = String(), data::EncodeFormat fmt = data::EncodeFormat::Cbor);
	void _backtrace(StringStream &);
}

NS_SP_PLATFORM_END

#endif
