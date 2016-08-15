//
//  SPStoreKit.h
//  template
//
//  Created by SBKarr on 05.05.14.
//
//

#ifndef stappler_storekit_SPStoreKit
#define stappler_storekit_SPStoreKit

#include "SPDefine.h"

#include "SPEventHeader.h"
#include "SPEventHandler.h"
#include "SPStoreProduct.h"

NS_SP_BEGIN

class StoreKit : public cocos2d::Ref, public EventHandler {
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

#endif /* defined(__template__SPStoreKit__) */
