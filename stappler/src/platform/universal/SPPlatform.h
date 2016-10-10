//
//  SPPlatform.h
//  stappler
//
//  Created by SBKarr on 7/25/14.
//  Copyright (c) 2014 SBKarr. All rights reserved.
//

#ifndef stappler_SPPlatform_h
#define stappler_SPPlatform_h

/* Do NOT include this header into public headers! */

#include "SPDefine.h"
#include "SPCommonPlatform.h"

/* Platform-depended functions interface */
NS_SP_PLATFORM_BEGIN

namespace network {
	void _setNetworkCallback(std::function<void(bool isOnline)> callback);
	bool _isNetworkOnline();
}

namespace device {
	bool _isTablet();

	std::string _userAgent();
	std::string _deviceIdentifier();
	std::string _bundleName();

	std::string _userLanguage();
	std::string _applicationName();
	std::string _applicationVersion();

	int _dpi();
	float _density();
	cocos2d::Size _screenSize();
	cocos2d::Size _viewportSize(const cocos2d::Size &screenSize, bool isTablet);

	float _statusBarHeight(const cocos2d::Size &screenSize, bool isTablet);
	float _designScale(const cocos2d::Size &screenSize, bool isTablet);

	const ScreenOrientation &_currentDeviceOrientation();

	std::pair<uint64_t, uint64_t> _diskSpace(); // <total, free>

	void _onDirectorStarted();
}

namespace interaction {
	void _goToUrl(const std::string &url, bool external);
	void _makePhoneCall(const std::string &number);
	void _mailTo(const std::string &address);

	void _backKey();
	void _notification(const std::string &title, const std::string &text);
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
	float _getHeight(const cocos2d::Size &screenSize, bool isTablet);
}

namespace clipboard {
	bool _isAvailable();
	std::string _getString();
	void _copyString(const std::string &value);
}

namespace ime {
	void _updateCursor(uint32_t pos, uint32_t len);
	void _updateText(const std::u16string &str, uint32_t pos, uint32_t len);
	void _runWithText(const std::u16string &str, uint32_t pos, uint32_t len);
	void _cancel();

	namespace native {
		bool hasText();
		void insertText(const std::u16string &str);
		void deleteBackward();
		void textChanged(const std::u16string &text, uint32_t cursorStart, uint32_t cursorLen);
		void onKeyboardShow(const cocos2d::Rect &rect, float duration);
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
}

#if (SP_INTERNAL_STOREKIT_ENABLED)
namespace storekit {
	void _saveProducts(const std::unordered_map<std::string, StoreProduct *> &);
	data::Value _loadProducts();

	void _updateProducts(const std::vector<StoreProduct *> &);
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
