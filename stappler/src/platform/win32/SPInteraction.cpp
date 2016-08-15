//
//  SPInteraction.mm
//  stappler
//
//  Created by SBKarr on 7/25/14.
//  Copyright (c) 2014 SBKarr. All rights reserved.
//

#include "SPDefine.h"
#include "SPPlatform.h"
#include "SPThread.h"

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
