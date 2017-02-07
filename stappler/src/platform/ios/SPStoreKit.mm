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

//#define SPSTOREKIT_LOG(...) stappler::logTag("StoreKit", __VA_ARGS__)
#define SPSTOREKIT_LOG(...)

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
	void saveReceipt(StoreProduct *product, NSData *receipt);
	NSData *getReceipt(StoreProduct *product);

	void validateReceipts();
	void validateSubscriptionReceipt(StoreProduct *product);
	void validateSubscriptionReceipt(StoreProduct *product, bool sandbox);
	
	void parseValidationResponse(const data::Value & data);
	StoreProduct * parseValidationReceipt(const data::Value & data, bool override);

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
                stappler::log::format("Store: ", "%s", transaction.error.localizedDescription.UTF8String);
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
		SPSTOREKIT_LOG("save products: %s", val.write(true).c_str());
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
//#pragma GCC diagnostic push
//#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
			saveReceipt(product, transaction.transactionReceipt);
//#pragma GCC diagnostic pop
			validateSubscriptionReceipt(product);
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
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
			saveReceipt(product, transaction.transactionReceipt);
#pragma GCC diagnostic pop
			validateSubscriptionReceipt(product);
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

void StoreKitIOS::saveReceipt(StoreProduct *product, NSData *receipt) {
	std::string path = filesystem::documentsPath("receipts");
	filesystem::mkdir(path);
	path += "/" + product->productId + ".receipt";

	[receipt writeToFile:[NSString stringWithUTF8String:path.c_str()] atomically:NO];

	if ([_delegate iCloudAvailable] == YES) {
		NSUbiquitousKeyValueStore *iCloudStore = [NSUbiquitousKeyValueStore defaultStore];
		[iCloudStore setData:receipt forKey:[NSString stringWithUTF8String:product->productId.c_str()]];
		[iCloudStore synchronize];
	}
}

NSData *StoreKitIOS::getReceipt(StoreProduct *product) {
	if ([_delegate iCloudAvailable] == YES) {
		NSUbiquitousKeyValueStore *iCloudStore = [NSUbiquitousKeyValueStore defaultStore];
		NSData *data = [iCloudStore dataForKey:[NSString stringWithUTF8String:product->productId.c_str()]];
		if (data) {
			return data;
		}
	}

	std::string path = filesystem::documentsPath("receipts/");
	path.append(product->productId).append(".receipt");
	if (filesystem::exists(path)) {
		return [NSData dataWithContentsOfFile:[NSString stringWithUTF8String:path.c_str()]];
	}
	return nil;
}

void StoreKitIOS::validateReceipts() {
	const auto &products = StoreKit::getInstance()->getProducts();
	for (const auto & it : products) {
		if (it.second->isSubscription) {
			validateSubscriptionReceipt(it.second);
		}
	}
}

void StoreKitIOS::validateSubscriptionReceipt(StoreProduct *product, bool sandbox) {
	NSData *receipt = getReceipt(product);
	if (!receipt) {
		return;
	}
	
	SPSTOREKIT_LOG("validateSubscriptionReceipt: %s", product->productId.c_str());
	
	auto task = Rc<NetworkTask>::create(NetworkTask::Method::Post, (sandbox?_debugUrl:_releaseUrl));
	auto buf = new data::JsonBuffer;
	
	std::string data;
	data.append("{\"receipt-data\":\"");
	data.append(base64::encode((const uint8_t *)receipt.bytes, receipt.length));
	data.append("\" \"password\":\"");
	data.append(StoreKit::getInstance()->getSharedSecret());
	data.append("\"}");
	
	task->setSendData(std::move(data));
	task->addHeader("Content-Type", "application/json; charset=utf-8");
	task->setReceiveCallback([buf] (char *data, size_t size) -> size_t {
		buf->read((const uint8_t *)data, size);
		return size;
	});
	
	task->addCompleteCallback([this, buf, product] (const Task &, bool) {
		auto & value = buf->data();
		if (value) {
			auto status = value.getInteger("status");
			if (status == 21007) {
				validateSubscriptionReceipt(product, true); // retry with sandbox server
			} else if (status == 21008) {
				validateSubscriptionReceipt(product, false); // retry with production server
			} else {
				parseValidationResponse(value);
			}
		}
	});
	
	task->run();
}
void StoreKitIOS::validateSubscriptionReceipt(StoreProduct *product) {
#if (DEBUG)
	validateSubscriptionReceipt(product, true);
#else
	validateSubscriptionReceipt(product, false);
#endif
}

void StoreKitIOS::parseValidationResponse(const data::Value &data) {
	SPSTOREKIT_LOG("parseValidationResponse: %s", data.write(true).c_str());
	auto status = data.getInteger("status");

	StoreProduct *product = nullptr;
	std::set<std::string> products;
	if (status == 0 || status == 21006) {

		product = parseValidationReceipt(data.getValue("receipt"), false);
		if (product) {
			products.insert(product->productId);
		}

		product = parseValidationReceipt(data.getValue((status == 0)?"latest_receipt_info":"latest_expired_receipt_info"), true);
		if (product) {
			products.insert(product->productId);
		}
	}

	if (product) {
		const auto &latestRecept = data.getString("latest_receipt");
		if (!latestRecept.empty()) {
			const auto &receiptData = base64::decode(latestRecept);
			saveReceipt(product, [NSData dataWithBytes:receiptData.data() length:receiptData.size()]);
		}
	}

	for (auto &v : products) {
		StoreKit::onProductUpdate(StoreKit::getInstance(), v);
	}

	saveProductDictionary(StoreKit::getInstance()->getProducts());
}

StoreProduct * StoreKitIOS::parseValidationReceipt(const data::Value & info, bool override) {
	SPSTOREKIT_LOG("parseValidationReceipt: %s", info.write(true).c_str());
	if (!info.isDictionary()) {
		return nullptr;
	}

	const auto &ret = info.getString("product_id");
	StoreProduct *product = StoreKit::getInstance()->getProduct(ret);

	if (!product) {
		return nullptr;
	}
	
	if (info.getValue("cancellation_date")) {
		if (product->purchased) {
			product->purchased = false;
			product->expiration = 0;
			saveProductDictionary(StoreKit::getInstance()->getProducts());
			StoreKit::onProductUpdate(StoreKit::getInstance(), product->productId);
		}
		return product; // subscription was cancelled by apple support
	}

	if (product->purchased) {
		auto purchaseDate = info.getInteger("original_purchase_date_ms");
		if (purchaseDate == 0) {
			purchaseDate = info.getInteger("purchase_date_ms");
		}
		if (purchaseDate != 0) {
			product->purchaseDate = Time::milliseconds(purchaseDate);
		}

		auto expires = Time::milliseconds(info.getInteger("expires_date")); // to seconds
		if (expires && product->expiration != expires) {
			product->expiration = expires;
		}
	}

	return product;
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
