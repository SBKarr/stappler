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
