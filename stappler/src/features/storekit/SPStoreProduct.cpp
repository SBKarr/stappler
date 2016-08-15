//
//  SPStoreProduct.cpp
//  stappler
//
//  Created by SBKarr on 8/23/14.
//  Copyright (c) 2014 SBKarr. All rights reserved.
//

#include "SPDefine.h"
#include "SPStoreProduct.h"

NS_SP_BEGIN

data::Value StoreProduct::info() {
	data::Value ret;
	ret.setString(productId, "productId");
	ret.setString(price, "price");
	ret.setBool(purchased, "purchased");
	ret.setBool(available, "available");
	ret.setBool(isSubscription, "isSubscription");
	ret.setBool(validated, "validated");
	ret.setInteger(duration, "duration");
	ret.setInteger(expiration.toMilliseconds(), "expiration");
	ret.setInteger(purchaseDate.toMilliseconds(), "purchaseDate");
	return ret;
}

data::Value StoreProduct::encode() {
	data::Value ret;
	ret.setString(productId, "productId");
	ret.setString(price, "price");
	ret.setBool(purchased, "purchased");
	ret.setBool(available, "available");
	ret.setBool(isSubscription, "isSubscription");
	ret.setInteger(duration, "duration");
	ret.setInteger(expiration.toMilliseconds(), "expiration");
	ret.setInteger(purchaseDate.toMilliseconds(), "purchaseDate");
#if (ANDROID)
	ret.setString(token, "token");
	ret.setString(signature, "signature");
#endif
	return ret;
}

bool StoreProduct::updateWithData(const data::Value &val) {
	if (val.getString("productId") == productId && val.getBool("isSubscription") == isSubscription) {
		price = val.getString("price");
		purchased = val.getBool("purchased");
		duration = val.getInteger("duration");
		expiration = Time::milliseconds(val.getInteger("expiration"));
		purchaseDate = Time::milliseconds(val.getInteger("purchaseDate"));
		available = val.getBool("available");
#if (ANDROID)
		token = val.getString("token");
		signature = val.getString("signature");
#endif
		validated = false;
		return true;
	}
	return false;
}

StoreProduct *StoreProduct::decode(const data::Value &val) {
	std::string productId = val.getString("productId");
	if (!productId.empty()) {
		StoreProduct *product = new StoreProduct(productId);
		product->price = val.getString("price");
		product->purchased = val.getBool("purchased");
		product->available = val.getBool("available");
		product->isSubscription = val.getBool("isSubscription");
		product->duration = val.getInteger("duration");
		product->expiration = Time::milliseconds(val.getInteger("expiration"));
		product->purchaseDate = Time::milliseconds(val.getInteger("purchaseDate"));
#if (ANDROID)
		product->token = val.getString("token");
		product->signature = val.getString("signature");
#endif
		return product;
	}
	return nullptr;
}

NS_SP_END
