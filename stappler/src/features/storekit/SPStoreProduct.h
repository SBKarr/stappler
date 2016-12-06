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

#if (ANDROID || LINUX || __MINGW32__ || MACOSX)
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
