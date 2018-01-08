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
