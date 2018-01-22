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

#ifdef IOS
#if (SP_INTERNAL_STOREKIT_ENABLED)

#include "SPStoreKit.h"
#include "SPEventHandler.h"
#include "SPDevice.h"
#include "SPFilesystem.h"
#include "SPData.h"
#include "SPThread.h"
#include "SPTime.h"
#include "SPString.h"
#include "SPNetworkTask.h"
#include "SPDataJsonBuffer.h"

#import <Foundation/Foundation.h>
#import <StoreKit/StoreKit.h>

//#define SPSTOREKIT_LOG(...) stappler::log::format("StoreKit", __VA_ARGS__)
#define SPSTOREKIT_LOG(...)

NS_SP_EXT_BEGIN(platform)

data::Value validateReceipt(const String &path);
data::Value validateReceipt(const Bytes &data);
data::Value validateReceipt(const DataReader<ByteOrder::Network> &data);

NS_SP_EXT_END(platform)

@interface SPStoreDelegate : NSObject<SKProductsRequestDelegate, SKPaymentTransactionObserver> @end

NS_SP_BEGIN

class StoreKitIOS : public EventHandler {
protected:
	bool _hasChanged = false;
	bool _init = false;
	SPStoreDelegate *_delegate;
	NSMutableDictionary *_skproducts; // we use Foundation container to follow ARC patterns

public:
	static StoreKitIOS *getInstance();
	StoreKitIOS();

	virtual void init();
	virtual bool restorePurchases();

public:
	/* iOS StoreKit bindings */
	void onProductsResponse(SKProductsRequest *req, SKProductsResponse *resp);
	void onTransactionCompleted(SKPaymentTransaction *transaction);
	void onTransactionFailed(SKPaymentTransaction *transaction);
	void onTransactionRestored(SKPaymentTransaction *transaction);

	void onTransactionsRestored(bool success);

	void save();
	void updateProductDictionary();

	data::Value loadProductDictionary();
	void saveProductDictionary(const std::unordered_map<std::string, StoreProduct *> &);

	bool purchaseProduct(StoreProduct *product);
	void updateProducts(const std::vector<StoreProduct *> &);

protected:
	void validateReceipts();
	void parseValidationReceipt(const data::Value & data, Set<String> &);

	NSString *getCloudName() const {
#if (DEBUG)
		return [NSString stringWithUTF8String:_debugCloudName.c_str()];
#else
		return [NSString stringWithUTF8String:_releaseCloudName.c_str()];
#endif
	}

	std::string getStoreFile() const {
#if (DEBUG)
		return _debugStoreFile;
#else
		return _releaseStoreFile;
#endif
	}
	
	std::string getUrl() const {
#if (DEBUG)
		return _debugUrl;
#else
		return _releaseUrl;
#endif
	}

	std::string _debugStoreFile = "store.debug.json";
	std::string _releaseStoreFile = "store.json";
	std::string _debugCloudName = "products-debug";
	std::string _releaseCloudName = "products";
	std::string _debugUrl = "https://sandbox.itunes.apple.com/verifyReceipt";
	std::string _releaseUrl = "https://buy.itunes.apple.com/verifyReceipt";
};

NS_SP_END

static stappler::data::Value ValueFromNSObject(NSObject *obj) {
	stappler::data::Value ret;
	if ([obj isKindOfClass:[NSDictionary class]]) {
		auto ptr = &ret;
		[((NSDictionary *)obj) enumerateKeysAndObjectsUsingBlock:^(id key, id obj, BOOL *stop) {
			ptr->setValue(ValueFromNSObject(obj), std::string(((NSString *)key).UTF8String));
		}];
	} else if ([obj isKindOfClass:[NSArray class]]) {
		auto ptr = &ret;
		[((NSArray *)obj) enumerateObjectsUsingBlock:^(id obj, NSUInteger idx, BOOL *stop) {
			if (auto arrobj = ValueFromNSObject(obj)) {
				ptr->addValue(std::move(arrobj));
			}
		}];
	} else if ([obj isKindOfClass:[NSString class]]) {
		ret = std::string(((NSString *)obj).UTF8String);
	} else if ([obj isKindOfClass:[NSData class]]) {
		ret = std::string((const char *)((NSData *)obj).bytes, ((NSData *)obj).length);
	} else if ([obj isKindOfClass:[NSNumber class]]) {
		double doubleValue = ((NSNumber *)obj).doubleValue;
		if (round(doubleValue) == doubleValue) {
			ret = (int64_t)((NSNumber *)obj).longLongValue;
		} else {
			ret = doubleValue;
		}
	}
	return ret;
}

static NSObject *NSObjectFromValue(const stappler::data::Value &obj) {
	NSObject *object = nil;
	if (obj.isDictionary()) {
		NSMutableDictionary *dict = [NSMutableDictionary dictionaryWithCapacity:obj.size()];
        for (auto &it : obj.asDict()) {
			[dict setObject:NSObjectFromValue(it.second) forKey:[NSString stringWithUTF8String:it.first.c_str()]];
		}
		object = [NSDictionary dictionaryWithDictionary:dict];
	} else if (obj.isArray()) {
		NSMutableArray *array = [NSMutableArray arrayWithCapacity:obj.size()];
        for (auto &it : obj.asArray()) {
			[array addObject:NSObjectFromValue(it)];
		}
		object = [NSArray arrayWithArray:array];
	} else if (obj.isString()) {
		object = [NSString stringWithUTF8String:obj.asString().c_str()];
	} else if (obj.isDouble()) {
		object = [NSNumber numberWithDouble:obj.asDouble()];
	} else if (obj.isInteger()) {
		object = [NSNumber numberWithLongLong:obj.asInteger()];
	} else if (obj.isBool()) {
		object = [NSNumber numberWithBool:(obj.asBool()?YES:NO)];
	}
	return object;
}

@implementation SPStoreDelegate {
	NSOperationQueue *queue;
}

- (id) init {
	if (self = [super init]) {
		[[NSNotificationCenter defaultCenter] addObserver: self selector:@selector(iCloudChanged)
													 name: NSUbiquitousKeyValueStoreDidChangeExternallyNotification
												   object: [NSUbiquitousKeyValueStore defaultStore]];
	}
	return self;
}

+ (stappler::StoreKitIOS *) storeKit {
	return stappler::StoreKitIOS::getInstance();
}

- (void)productsRequest:(SKProductsRequest *)request didReceiveResponse:(SKProductsResponse *)response {
	auto storeKit = [SPStoreDelegate storeKit];
	if (storeKit) {
		storeKit->onProductsResponse(request, response);
	}
}

- (void)request:(SKRequest *)request didFailWithError:(NSError *)error {
    stappler::log::format("StoreKit","Fail to request product data: %s", error.localizedDescription.UTF8String);
	stappler::Thread::onMainThread([] () { });
}

- (void)paymentQueue:(SKPaymentQueue *)queue updatedTransactions:(NSArray *)transactions {
	auto storeKit = [SPStoreDelegate storeKit];

    for (SKPaymentTransaction *transaction in transactions) {
        switch (transaction.transactionState) {
            case SKPaymentTransactionStatePurchased:
				storeKit->onTransactionCompleted(transaction);
                break;

            case SKPaymentTransactionStateFailed:
                stappler::log::format("StoreKit", "Native response: %d : %s : %s", int(transaction.error.code), transaction.error.description.UTF8String, transaction.error.localizedDescription.UTF8String);
				if (transaction.error.localizedFailureReason) {
					stappler::log::text("StoreKit", transaction.error.localizedFailureReason.UTF8String);
				}
				if (transaction.error.userInfo) {
					stappler::log::text("StoreKit", transaction.error.userInfo.description.UTF8String);
				}
				storeKit->onTransactionFailed(transaction);
                break;

            case SKPaymentTransactionStateRestored:
				storeKit->onTransactionRestored(transaction);
                break;

            default:
                break;
        }
    }

	storeKit->save();
}

- (BOOL) iCloudAvailable {
	if(NSClassFromString(@"NSUbiquitousKeyValueStore")) { // is iOS 5?
		if([NSUbiquitousKeyValueStore defaultStore]) {  // is iCloud enabled
			return YES;
		}
	}
	return NO;
}

- (void) iCloudChanged {
	[SPStoreDelegate storeKit]->updateProductDictionary();
}

- (void)paymentQueue:(SKPaymentQueue *)queue restoreCompletedTransactionsFailedWithError:(NSError *)error {
	stappler::Thread::onMainThread([] () {
		[SPStoreDelegate storeKit]->onTransactionsRestored(false);
	});
}

- (void)paymentQueueRestoreCompletedTransactionsFinished:(SKPaymentQueue *)queue {
	stappler::Thread::onMainThread([] () {
		[SPStoreDelegate storeKit]->onTransactionsRestored(true);
	});
}

@end

NS_SP_BEGIN

static StoreKitIOS *s_sharedStoreKitIOS = nullptr;

StoreKitIOS *StoreKitIOS::getInstance() {
	if (!s_sharedStoreKitIOS) {
		Class klass = NSClassFromString(@"SKProduct");
		if (klass) {
			s_sharedStoreKitIOS = new StoreKitIOS;
		} else {
            stappler::log::text("StoreKit","StoreKit.framework is not available");
		}
	}
	return s_sharedStoreKitIOS;
}

StoreKitIOS::StoreKitIOS() {
	_delegate = [[SPStoreDelegate alloc] init];
	_skproducts = [[NSMutableDictionary alloc] init];
	[[SKPaymentQueue defaultQueue] addTransactionObserver:_delegate];
}

void StoreKitIOS::init() {
	validateReceipts();
}

void StoreKitIOS::updateProducts(const std::vector<StoreProduct *> &vec) {
	validateReceipts();

	NSMutableSet *products = [NSMutableSet setWithCapacity:vec.size()];
	for (const auto &it : vec) {
		SPSTOREKIT_LOG("updateProducts: %s", it->productId.c_str());
		[products addObject:[NSString stringWithUTF8String:it->productId.c_str()]];
	}

	if (products.count != 0) {
		SKProductsRequest *request = [[SKProductsRequest alloc] initWithProductIdentifiers:products];
		request.delegate = _delegate;
		[request start];
	}
}

void StoreKitIOS::updateProductDictionary() {
	StoreKit::getInstance()->updateWithData(loadProductDictionary());
	validateReceipts();
}

data::Value StoreKitIOS::loadProductDictionary() {
	data::Value ret;

	if ([_delegate iCloudAvailable] == YES) {
		// read from iCloud
		NSUbiquitousKeyValueStore *iCloudStore = [NSUbiquitousKeyValueStore defaultStore];
		NSDictionary *dataDict = [iCloudStore dictionaryForKey:getCloudName()];
		if (dataDict) {
			auto dataVal = ValueFromNSObject(dataDict);
			if (dataVal && dataVal.isDictionary()) {
				ret = std::move(dataVal);
			}
		}
	}

	if (!ret) {
		auto dataFile = data::readFile(filesystem::documentsPath(getStoreFile()));
		if (dataFile && dataFile.isDictionary()) {
			ret = std::move(dataFile);
		}
	}

	return ret;
}

void StoreKitIOS::saveProductDictionary(const std::unordered_map<std::string, StoreProduct *> &dict) {
	data::Value val;
	for (const auto &it : dict) {
		val.setValue(it.second->encode(), it.first);
	}

	if (val) {
		// SPSTOREKIT_LOG("save products: %s", data::toString(val, true).c_str());
		if ([_delegate iCloudAvailable] == YES) {
			NSUbiquitousKeyValueStore *iCloudStore = [NSUbiquitousKeyValueStore defaultStore];
			NSDictionary *dataDict = (NSDictionary *)NSObjectFromValue(val);
			[iCloudStore setDictionary:dataDict forKey:getCloudName()];
			[iCloudStore synchronize];
		}
        data::save(val, filesystem::documentsPath(getStoreFile()));
	}
}

void StoreKitIOS::save() {
	if (_hasChanged) {
		saveProductDictionary(StoreKit::getInstance()->getProducts());
		_hasChanged = false;
	}
}

void StoreKitIOS::onProductsResponse(SKProductsRequest *req, SKProductsResponse *response) {
	SPSTOREKIT_LOG("onProductsResponse");
	for(NSString *invalidProduct in response.invalidProductIdentifiers) {
        stappler::log::format("StoreKit","Problem in iTunes connect configuration for product: %s", invalidProduct.UTF8String);
	}

	auto sk = StoreKit::getInstance();
	for(SKProduct *skproduct in response.products) {
		StoreProduct *product = sk->getProduct(skproduct.productIdentifier.UTF8String);
		if (product && skproduct.localizedTitle) {
			NSNumberFormatter *numberFormatter = [[NSNumberFormatter alloc] init];
			[numberFormatter setFormatterBehavior:NSNumberFormatterBehavior10_4];
			[numberFormatter setNumberStyle:NSNumberFormatterCurrencyStyle];
			[numberFormatter setLocale:skproduct.priceLocale];
			NSString *formattedPrice = [numberFormatter stringFromNumber:skproduct.price];
			formattedPrice = [formattedPrice stringByReplacingOccurrencesOfString:@" " withString:@" "];
			formattedPrice = [formattedPrice stringByReplacingOccurrencesOfString:@"₽" withString:@"руб."];

			product->price = formattedPrice.UTF8String;
			product->available = true;
			product->validated = true;

			StoreKit::onProductUpdate(sk, product->productId);
			SPSTOREKIT_LOG("Feature: %s, Cost: %s", product->productId.c_str(), product->price.c_str());
			[_skproducts setObject:skproduct forKey:[NSString stringWithUTF8String:product->productId.c_str()]];
		}
	}

	saveProductDictionary(StoreKit::getInstance()->getProducts());
}

bool StoreKitIOS::purchaseProduct(StoreProduct *product) {
	if (product->available && product->validated) {
		if ([SKPaymentQueue canMakePayments]) {
			SPSTOREKIT_LOG("purchaseProduct: add to queue: %s", product->productId.c_str());
			SKProduct *thisProduct = [_skproducts objectForKey:[NSString stringWithUTF8String:product->productId.c_str()]];
			if (thisProduct) {
				SKPayment *payment = [SKPayment paymentWithProduct:thisProduct];
				[[SKPaymentQueue defaultQueue] addPayment:payment];
				SPSTOREKIT_LOG("purchaseProduct: added: %s", product->productId.c_str());
				return true;
			}
		}
	}
	return false;
}

bool StoreKitIOS::restorePurchases() {
	SPSTOREKIT_LOG("restoreCompletedTransactions");
	[[SKPaymentQueue defaultQueue] restoreCompletedTransactions];
	return true;
}

void StoreKitIOS::onTransactionCompleted(SKPaymentTransaction *transaction) {
	auto sk = StoreKit::getInstance();
	if (auto product = sk->getProduct(transaction.payment.productIdentifier.UTF8String)) {
		SPSTOREKIT_LOG("onTransactionCompleted: %s", product->productId.c_str());
		product->purchased = true;
		if (product->isSubscription) {
			validateReceipts();
		}
		StoreKit::onProductUpdate(sk, product->productId);
		StoreKit::onPurchaseCompleted(sk, product->productId);
		_hasChanged = true;
	}
	[[SKPaymentQueue defaultQueue] finishTransaction: transaction];
}

void StoreKitIOS::onTransactionFailed(SKPaymentTransaction *transaction) {
	[[SKPaymentQueue defaultQueue] finishTransaction: transaction];
	SPSTOREKIT_LOG("onTransactionFailed: %s", transaction.payment.productIdentifier.UTF8String);
	StoreKit::onPurchaseFailed(StoreKit::getInstance(), std::string(transaction.payment.productIdentifier.UTF8String));
}

void StoreKitIOS::onTransactionRestored(SKPaymentTransaction *transaction) {
	auto sk = StoreKit::getInstance();
	if (auto product = sk->getProduct(transaction.payment.productIdentifier.UTF8String)) {
		SPSTOREKIT_LOG("onTransactionRestored: %s", product->productId.c_str());
		product->purchased = true;
		if (product->isSubscription) {
			validateReceipts();
		}
		StoreKit::onProductUpdate(sk, product->productId);
		StoreKit::onPurchaseCompleted(sk, product->productId);
		_hasChanged = true;
	}

	[[SKPaymentQueue defaultQueue] finishTransaction: transaction];
}

void StoreKitIOS::onTransactionsRestored(bool success) {
	StoreKit::onTransactionsRestored(StoreKit::getInstance(), success);
}

void StoreKitIOS::validateReceipts() {
	auto url = [[NSBundle mainBundle] appStoreReceiptURL];
	auto receipt = [NSData dataWithContentsOfURL:url];
	if (receipt) {
		Set<String> updatedProducts;
		Map<String, data::Value> latestReceipts;
		auto data = platform::validateReceipt(DataReader<ByteOrder::Network>((uint8_t *)receipt.bytes, receipt.length));
		SPSTOREKIT_LOG("appReceipt: %s", data::toString(data, true).c_str());
		if (data.isArray("in_app")) {
			auto &inApp = data.getArray("in_app");
			for (auto &it : inApp) {
				auto &productId = it.getString("product_id");
				
				auto latestIt = latestReceipts.find(productId);
				if (latestIt == latestReceipts.end()) {
					latestReceipts.emplace(productId, it);
				} else {
					auto date = it.getInteger("purchase_date_ms");
					auto ldate = latestIt->second.getInteger("purchase_date_ms");
					if (date > ldate) {
						latestIt->second = it;
					}
				}
			}
		}

		for (auto &it : latestReceipts) {
			parseValidationReceipt(it.second, updatedProducts);
		}
		
		saveProductDictionary(StoreKit::getInstance()->getProducts());
		
		for (auto &it : updatedProducts) {
			StoreKit::onProductUpdate(StoreKit::getInstance(), it);
		}
	}
}

void StoreKitIOS::parseValidationReceipt(const data::Value & info, Set<String> &updated) {
	SPSTOREKIT_LOG("parseValidationReceipt: %s", data::toString(info, true).c_str());
	if (!info.isDictionary()) {
		return;
	}

	const auto &ret = info.getString("product_id");
	StoreProduct *product = StoreKit::getInstance()->getProduct(ret);

	if (!product) {
		return;
	}

	if (info.getInteger("cancellation_date_ms") > 0) {
		if (product->purchased) {
			product->purchased = false;
			product->expiration = 0;
			updated.emplace(product->productId);
		}
		return; // subscription was cancelled by apple support
	}
	
	if (!product->isSubscription && !product->purchased) {
		product->purchased = true;
		product->purchaseDate = Time::milliseconds(info.getInteger("purchase_date_ms"));
		updated.emplace(product->productId);
	} else if (product->purchased) {
		auto purchaseDate = info.getInteger("original_purchase_date_ms");
		if (purchaseDate == 0) {
			purchaseDate = info.getInteger("purchase_date_ms");
		}

		if (purchaseDate != 0 && purchaseDate != product->purchaseDate) {
			product->purchaseDate = Time::milliseconds(purchaseDate);
			updated.emplace(product->productId);
		}

		auto expires = Time::milliseconds(info.getInteger("expires_date_ms"));
		if (expires) {
			if (product->expiration < expires) {
				product->expiration = expires;
				updated.emplace(product->productId);
			}
		}
	}
}

NS_SP_END


NS_SP_PLATFORM_BEGIN

namespace storekit {
	void _saveProducts(const std::unordered_map<std::string, StoreProduct *> &val) {
		if (auto sk = StoreKitIOS::getInstance()) {
			sk->saveProductDictionary(val);
		}
	}
	data::Value _loadProducts() {
		if (auto sk = StoreKitIOS::getInstance()) {
			return sk->loadProductDictionary();
		}
		return data::Value();
	}

	void _updateProducts(const std::vector<StoreProduct *> &val) {
		if (auto sk = StoreKitIOS::getInstance()) {
			sk->updateProducts(val);
		}
	}

	bool _purchaseProduct(StoreProduct *product) {
		if (auto sk = StoreKitIOS::getInstance()) {
			return sk->purchaseProduct(product);
		}
		return false;
	}
	bool _restorePurchases() {
		if (auto sk = StoreKitIOS::getInstance()) {
			return sk->restorePurchases();
		}
		return false;
	}
}

NS_SP_PLATFORM_END

#endif
#endif
