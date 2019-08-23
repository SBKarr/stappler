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

#ifndef LIBS_STAPPLER_FEATURES_STOREKIT_SPSTOREKIT_H
#define LIBS_STAPPLER_FEATURES_STOREKIT_SPSTOREKIT_H

#include "SPDefine.h"

#include "SPEventHeader.h"
#include "SPEventHandler.h"
#include "SPStoreProduct.h"

NS_SP_BEGIN

class StoreKit : public Ref, public EventHandler {
public:
	// product data was updated
	static EventHeader onProductUpdate;

	// product purchasing was completed successfully
	static EventHeader onPurchaseCompleted;

	// product purchasing was not completed successfully
	static EventHeader onPurchaseFailed;

	// product restoration process was completed
	static EventHeader onTransactionsRestored;

public:
	static StoreKit *getInstance();

	StoreKit();

	// update products from recieved data
	void updateWithData(const data::Value &);

	// add product to store kit (if updateProduct is true, native store will be requested for purchase info)
	bool addProduct(const std::string &productId, bool subscription = false, uint32_t subMonth = 0, bool updateProduct = true);
	StoreProduct * updateProduct(const std::string &productId, bool subscription = false, uint32_t subMonth = 0, bool updateProduct = true);

	// updates not validated products with native store
	void updateProducts();

	// begin purchases restoration process (follow specific events to track process)
	bool restorePurchases();

	// is product available for purchasing?
	bool isProductAvailable(const std::string &productId);

	// is product already for purchased? (expired subscription returns true)
	bool isProductPurchased(const std::string &productId);

	// get subscription expiration time
	Time getSubscriptionExpiration(const std::string &productId);

	// is subscription purchased and not expired
	bool isSubscriptionActive(const std::string &productId);

	// begin purchase process (follow specific events to track process)
	bool purchaseProduct(const std::string &productId);

	// get product raw data
	StoreProduct *getProduct(const std::string &productId);

	void setSharedSecret(const std::string &str);
	const std::string &getSharedSecret() const { return _sharedSecret; }

	const std::unordered_map<std::string, StoreProduct *> & getProducts() const { return _products; }
protected:
	data::Value loadProductDictionary();
	void saveProductDictionary();

	void updateProduct(StoreProduct *product);
	bool purchaseProduct(StoreProduct *product);

    bool _purchaseProcess = false;
	std::string _sharedSecret;
	std::unordered_map<std::string, StoreProduct *> _products;
};

NS_SP_END

#endif
