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
#include "SPStoreKit.h"

#include "SPEvent.h"
#include "SPDevice.h"
#include "SPPlatform.h"
#include "base/CCDirector.h"
#include "base/CCScheduler.h"

NS_SP_BEGIN

SP_DECLARE_EVENT_CLASS(StoreKit, onProductUpdate);
SP_DECLARE_EVENT_CLASS(StoreKit, onPurchaseCompleted);
SP_DECLARE_EVENT_CLASS(StoreKit, onPurchaseFailed);
SP_DECLARE_EVENT_CLASS(StoreKit, onTransactionsRestored);

static StoreKit *s_sharedStoreKit = nullptr;

StoreKit *StoreKit::getInstance() {
	if (!s_sharedStoreKit) {
		s_sharedStoreKit = new StoreKit();
	}
	return s_sharedStoreKit;
}

#if (SP_INTERNAL_STOREKIT_ENABLED)

StoreKit::StoreKit() {
	updateWithData(platform::storekit::_loadProducts());
	onEvent(Device::onNetwork, [this] (const Event &ev) {
		if (Device::getInstance()->isNetworkOnline()) {
			this->updateProducts();
		}
	});
}

void StoreKit::updateWithData(const data::Value &data) {
	if (!data.isDictionary()) {
		return;
	}

	for (auto &it : data.asDict()) {
		const auto &productId = it.first;
		if (auto product = getProduct(productId)) {
			if (product->updateWithData(it.second)) {
				onProductUpdate(this, productId);
			}
		} else if (auto product = StoreProduct::decode(it.second)) {
			_products.insert(std::make_pair(productId, product));
			onProductUpdate(this, productId);
		}
	}

	saveProductDictionary();
}

// add product to store kit (if updateProduct is true, native store will be requested for purchase info)
bool StoreKit::addProduct(const std::string &productId, bool subscription, uint32_t subDuration, bool update) {
	auto product = getProduct(productId);
	if (!product) {
		product = new StoreProduct(productId, subscription, subDuration);
		_products.insert(std::make_pair(productId, product));
		onProductUpdate(this, productId);
		if (update) {
			updateProduct(product);
		}
		return true;
	} else if (update) {
		updateProduct(product);
	}
	return false;
}

StoreProduct * StoreKit::updateProduct(const std::string &productId, bool subscription, uint32_t subDuration, bool update) {
	auto product = getProduct(productId);
	if (!product) {
		product = new StoreProduct(productId, subscription, subDuration);
		_products.insert(std::make_pair(productId, product));
		onProductUpdate(this, productId);
		if (update) {
			updateProduct(product);
		}
	}
	return product;
}

// updates not validated products with native store
void StoreKit::updateProducts() {
	std::vector<StoreProduct *> products;
	for (const auto &it : _products) {
		if (!it.second->validated) {
			products.push_back(it.second);
		}
	}

	platform::storekit::_updateProducts(products);
}

// begin purchases restoration process (follow specific events to track process)
bool StoreKit::restorePurchases() {
	return platform::storekit::_restorePurchases();
}

// is product available for purchasing?
bool StoreKit::isProductAvailable(const std::string &productId) {
	if (auto product = getProduct(productId)) {
		return (product->available && product->validated);
	}
	return false;
}

// is product already for purchased? (expired subscription returns true)
bool StoreKit::isProductPurchased(const std::string &productId) {
	if (auto product = getProduct(productId)) {
		return (product->purchased);
	}
	return false;
}

// get subscription expiration time
Time StoreKit::getSubscriptionExpiration(const std::string &productId) {
	if (auto product = getProduct(productId)) {
		return (product->isSubscription) ? product->expiration : Time();
	}
	return Time();
}

// is subscription purchased and not expired
bool StoreKit::isSubscriptionActive(const std::string &productId) {
	if (auto product = getProduct(productId)) {
		if (product->isSubscription && product->expiration) {
			auto now = Time::now();
			if (product->expiration > (int64_t)now) {
				return true;
			}
		}
	}
	return false;
}

// begin purchase process (follow specific events to track process)
bool StoreKit::purchaseProduct(const std::string &productId) {
	if (auto product = getProduct(productId)) {
		return purchaseProduct(product);
	}
	return false;
}

// get product raw data
StoreProduct *StoreKit::getProduct(const std::string &productId) {
	auto it = _products.find(productId);
	if (it != _products.end()) {
		return it->second;
	}
	return nullptr;
}

void StoreKit::setSharedSecret(const std::string &str) {
	_sharedSecret = str;
}

data::Value StoreKit::loadProductDictionary() {
	return platform::storekit::_loadProducts();
}

void StoreKit::saveProductDictionary() {
	platform::storekit::_saveProducts(_products);
}

void StoreKit::updateProduct(StoreProduct *product) {
	std::vector<StoreProduct *> products;
	products.push_back(product);
	platform::storekit::_updateProducts(products);
}

bool StoreKit::purchaseProduct(StoreProduct *product) {
	bool v = platform::storekit::_purchaseProduct(product);
	if (!v) {
		onPurchaseFailed(this, product->productId);
	}
	return v;
}

#else

StoreKit::StoreKit() { }
void StoreKit::updateWithData(const data::Value &data) {}
// add product to store kit (if updateProduct is true, native store will be requested for purchase info)
bool StoreKit::addProduct(const std::string &, bool, uint32_t, bool) { return false; }
StoreProduct * StoreKit::updateProduct(const std::string &, bool, uint32_t, bool) {return nullptr; }
// updates not validated products with native store
void StoreKit::updateProducts() { }
// begin purchases restoration process (follow specific events to track process)
bool StoreKit::restorePurchases() { return false; }
// is product available for purchasing?
bool StoreKit::isProductAvailable(const std::string &productId) { return false; }
// is product already for purchased? (expired subscription returns true)
bool StoreKit::isProductPurchased(const std::string &productId) { return false; }
// get subscription expiration time
Time StoreKit::getSubscriptionExpiration(const std::string &productId) { return Time(); }
// is subscription purchased and not expired
bool StoreKit::isSubscriptionActive(const std::string &productId) { return false; }
// begin purchase process (follow specific events to track process)
bool StoreKit::purchaseProduct(const std::string &productId) { return false; }
StoreProduct *StoreKit::getProduct(const std::string &productId) { return nullptr; }
void StoreKit::setSharedSecret(const std::string &str) { }
data::Value StoreKit::loadProductDictionary() { return data::Value(); }
void StoreKit::saveProductDictionary() { }
void StoreKit::updateProduct(StoreProduct *product) { }
bool StoreKit::purchaseProduct(StoreProduct *product) { return false; }

#endif

NS_SP_END
