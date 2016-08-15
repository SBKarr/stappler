//
//  SPStoreProduct.h
//  stappler
//
//  Created by SBKarr on 8/23/14.
//  Copyright (c) 2014 SBKarr. All rights reserved.
//

#ifndef __stappler__SPStoreProduct__
#define __stappler__SPStoreProduct__

#include "SPDefine.h"
#include "SPData.h"

NS_SP_BEGIN

class StoreProduct : public cocos2d::Ref {
public:
	data::Value encode();
	data::Value info();
	bool updateWithData(const data::Value &);
	static StoreProduct *decode(const data::Value &);

public:
	std::string productId;
	std::string price;

	bool isSubscription = false;

	int64_t duration; // month on subscription (12 - annual)
	Time expiration; // by default - in milliseconds
	Time purchaseDate; // undefined

	bool purchased = false;
	bool available = false;
	bool validated = false;

#if (ANDROID || LINUX || __MINGW32__)
	std::string token = "";
	std::string signature = "";

	int requestId = 0;
	bool confirmed = false;
#endif

	StoreProduct(const std::string &product, bool subscription = false, uint32_t subMonth = 0) {
		productId = product;
		isSubscription = subscription;
		duration = subMonth;
	}
};

NS_SP_END

#endif /* defined(__stappler__SPStoreProduct__) */
