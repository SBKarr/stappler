//
//  SPStoreKit.mm
//  stappler
//
//  Created by SBKarr on 8/24/14.
//  Copyright (c) 2014 SBKarr. All rights reserved.
//

#include "SPPlatform.h"

#if (SP_INTERNAL_STOREKIT_ENDBLED)

#include "SPStoreKit.h"
#include "SPEventHandler.h"
#include "SPDevice.h"
#include "SPFilesystem.h"
#include "SPData.h"
#include "SPThread.h"
#include "SPTime.h"

NS_SP_PLATFORM_BEGIN

namespace storekit {
	void _saveProducts(const std::unordered_map<std::string, StoreProduct *> &val) {

	}
	data::Value _loadProducts() {
		return data::Value();
	}

	void _updateProducts(const std::vector<StoreProduct *> &val) {
	}

	bool _purchaseProduct(StoreProduct *product) {
		return false;
	}
	bool _restorePurchases() {
		return false;
	}
}

NS_SP_PLATFORM_END

#endif
