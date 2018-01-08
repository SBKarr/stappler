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
#include "SPPlatform.h"
#include "SPStoreKit.h"

#include "SPFilesystem.h"
#include "SPTime.h"
#include "SPEventHandler.h"
#include "SPDevice.h"
#include "SPData.h"
#include "SPThread.h"
#include "SPString.h"
#include "SPJNI.h"

#if (ANDROID)

#if (SP_INTERNAL_STOREKIT_ENABLED)

NS_SP_BEGIN
class StoreKitAndroid;
static StoreKitAndroid *s_storeKitAndriod = nullptr;

Thread sStoreKitThread("AndroidStoreKit");

class StoreKitAndroid : public EventHandler {
private:
	bool _init = false;
public:
	static StoreKitAndroid *getInstance() {
		if (!s_storeKitAndriod) {
			s_storeKitAndriod = new StoreKitAndroid();
		}
		return s_storeKitAndriod;
	}

	StoreKitAndroid() {
		init();
	}

	virtual ~StoreKitAndroid() { }

	void init() {
		sStoreKitThread.perform([] (const Task & obj) -> bool {
			auto env = spjni::getJniEnv();
			auto storeKit = spjni::getService(spjni::Service::StoreKit, env);
			auto storeKitClass = spjni::getClassID(env, storeKit);
			if (!storeKitClass) {
				log::text("StoreKit", "test");
			} else {
				jmethodID startSetup = spjni::getMethodID(env, storeKitClass, "startSetup", "()V");
				if (startSetup) {
					env->CallVoidMethod(storeKit, startSetup);
				}
			}

			return true;
		});

		onEvent(Device::onNetwork, [this] (const Event &ev) {
			if (Device::getInstance()->isNetworkOnline()) {
				this->updateProducts();
			}
		});

		onEvent(Device::onBackground, [this] (const Event &ev) {
			if (!Device::getInstance()->isInBackground()) {
				this->restorePurchases();
			}
		});
	}

public:
	void onSetupCompleted(bool successful) {
		if (successful && !_init) {
			_init = true;
			updateProducts();
		}
	}

	void onDisposed() {
		if (_init) {
			_init = false;
		}
	}

	void onUpdateFinished() {
		save();

		/*auto allProducts = StoreKit::getInstance()->getProducts();
		std::vector<StoreProduct *> val;
		for (auto &it : allProducts) {
			if (!it.second->validated) {
				val.push_back(it.second);
			}
		}
		if (!val.empty()) {
			updateProducts(val);
		}*/
	}

	bool restorePurchases() {
		if (!_init) {
			return false;
		}
		sStoreKitThread.perform([] (const Task & ) -> bool {
			auto env = spjni::getJniEnv();
			auto storeKit = spjni::getService(spjni::Service::StoreKit, env);
			if (!storeKit) {
				return false;
			}
			auto storeKitClass = spjni::getClassID(env, storeKit);

			jmethodID restorePurchases = spjni::getMethodID(env, storeKitClass, "restorePurchases", "()V");
			if (restorePurchases) {
				env->CallVoidMethod(storeKit, restorePurchases);
				return true;
			}
			return false;
		});
		return true;
	}

	void onFeatureInfo(const std::string &sku, const std::string &dataStr) {
		data::Value data = data::read(dataStr);
		if (data.isDictionary()) {
			auto sk = StoreKit::getInstance();
			StoreProduct *product = getProduct(sku);
			if (product) {
				product->price = data.getString("price");
				product->available = true;
				product->validated = true;

				StoreKit::onProductUpdate(sk, product->productId);
#if (DEBUG)
				log::format("StoreKit", "Feature: %s, Cost: %s", product->productId.c_str(), product->price.c_str());
#endif
			}
		}
	}
	void onFeatureInfoFailed(const std::string &sku) {
		auto sk = StoreKit::getInstance();
		StoreProduct *product = getProduct(sku);
		if (product) {
			product->available = false;
			StoreKit::onProductUpdate(sk, product->productId);
		}
	}
	void onTransactionRestored(const std::string &sku, const std::string &signature, const std::string &dataStr) {
		data::Value data = data::read(dataStr);
		StoreProduct *product = getProduct(sku);
		if (product && data.isDictionary()) {
			product->validated = true;
			product->confirmed = true;
			product->requestId = 0;
			auto status = data.getInteger("purchaseState");
			auto date = data.getInteger("purchaseTime");
			const auto &token = data.getString("developerPayload");
#if (DEBUG)
			log::text("StoreKit", data::toString(data, true));
			log::format("StoreKit", "developer token: %d %s", product->token.empty(), product->token.c_str());
#endif
			if (status == 0 && (product->token.empty() || product->token.compare(token) == 0)) {
				product->purchased = true;
				product->available = true;
				product->purchaseDate = Time::milliseconds(date);
				product->token = token;
				if (product->isSubscription) {
					product->expiration = Time::now() + TimeInterval::seconds(2 * 24 * 3600);
				}

				auto sk = StoreKit::getInstance();
				StoreKit::onProductUpdate(sk, product->productId);
				StoreKit::onPurchaseCompleted(sk, product->productId);
				save();
#if (DEBUG)
				log::format("StoreKit", "Feature restored: %s", product->productId.c_str());
#endif
			}
		}
	}
	void onTransactionCompleted(int requestId, const std::string &signature, const std::string &dataStr) {
		data::Value data = data::read(dataStr);
		StoreProduct *product = getProductByRequestId(requestId);
		if (product) {
			product->validated = true;
			product->confirmed = true;
			product->requestId = 0;
			auto status = data.getInteger("purchaseState");
			int64_t date = data.getInteger("purchaseTime");
			const auto &token = data.getString("developerPayload");
#if (DEBUG)
			log::text("StoreKit", data::toString(data, true));
			log::format("StoreKit", "developer token: %d %s", product->token.empty(), product->token.c_str());
#endif
			if (status == 0 && (product->token.empty() || product->token.compare(token) == 0)) {
				product->purchased = true;
				product->available = true;
				product->purchaseDate = Time::milliseconds(date);
				product->token = token;
				if (product->isSubscription) {
					product->expiration = Time::now() + TimeInterval::seconds(2 * 24 * 3600);
				}

				auto sk = StoreKit::getInstance();
				StoreKit::onProductUpdate(sk, product->productId);
				StoreKit::onPurchaseCompleted(sk, product->productId);
				save();
#if (DEBUG)
				log::format("StoreKit", "Feature purchased: %s", product->productId.c_str());
#endif
			}
		} else {
#if (DEBUG)
			log::format("StoreKit", "no product for id %d", requestId);
#endif
		}
	}
	void onTransactionFailed(int requestId) {
		StoreProduct *product = getProductByRequestId(requestId);
		if (product) {
			product->token = "";
			product->requestId = 0;
			StoreKit::onPurchaseFailed(StoreKit::getInstance(), product->productId);
			save();
#if (DEBUG)
			log::format("StoreKit", "Feature failed: %s", product->productId.c_str());
#endif
		} else {
#if (DEBUG)
			log::format("StoreKit", "no product for id %d", requestId);
#endif
		}
	}
	void onPurchaseRestored() {
		for (auto p : StoreKit::getInstance()->getProducts()) {
			if (!p.second->confirmed) {
				p.second->purchased = false;
			}
		}
		save();
		StoreKit::onTransactionsRestored(StoreKit::getInstance(), true);
	}
	data::Value loadProductDictionary() {
		return data::readFile(filesystem::documentsPath("store.json"));
	}
	void saveProductDictionary(const std::unordered_map<std::string, StoreProduct *> &dict) {
		data::Value val;
		for (const auto &it : dict) {
			val.setValue(it.second->encode(), it.first);
		}

		if (val) {
			data::save(val, filesystem::documentsPath("store.json"));
		}
	}

	String newToken() {
		int buf[16] = { 0 };
		for (int i = 0; i < 16; i++) {
			buf[i] = rand() - (RAND_MAX / 2);
		}

		return base64::encode(CoderSource((uint8_t *)(&buf), 16 * sizeof(int)));
	}

	bool purchaseProduct(StoreProduct *product) {
		if (!_init) {
			return false;
		}
		std::string productId = product->productId;
		std::string token = newToken();
		bool isSubscription = product->isSubscription;
		int requestId = (rand() % 0x07ffffff) + 1;

		auto env = spjni::getJniEnv();
		auto storeKit = spjni::getService(spjni::Service::StoreKit, env);
		if (!storeKit) {
			return false;
		}
		auto storeKitClass = spjni::getClassID(env, storeKit);

		jmethodID purchaseFeature = spjni::getMethodID(env, storeKitClass, "purchaseFeature",
				"(Ljava/lang/String;ZILjava/lang/String;)V");
		if (purchaseFeature) {
#if (DEBUG)
			log::format("StoreKit", "start purchase with id %d", requestId);
#endif
			product->token = token.c_str();
			save();
			product->requestId = requestId;

			jstring jsku = env->NewStringUTF(productId.c_str());
			jstring jtoken = env->NewStringUTF(token.c_str());
			env->CallVoidMethod(storeKit, purchaseFeature, jsku, isSubscription, requestId, jtoken);
			env->DeleteLocalRef(jsku);
			env->DeleteLocalRef(jtoken);

#if (DEBUG)
			log::format("StoreKit", "start purchase with id %d", product->requestId);
#endif
			return true;
		}

		return false;
	}

	void updateProduct(StoreProduct *product) {
		std::vector<StoreProduct *> val;
		val.push_back(product);
		updateProducts(val);
	}

	void updateProducts() {
		log::text("StoreKit", "updateProducts");
		auto allProducts = StoreKit::getInstance()->getProducts();
		std::vector<StoreProduct *> val;
		for (auto &it : allProducts) {
			log::format("StoreKit", "updateProduct: %s", it.first.c_str());
			val.push_back(it.second);
		}
		updateProducts(val);
	}

	void updateProducts(const std::vector<StoreProduct *> &val) {
		if (!_init) {
			return;
		}
		std::vector<std::string> products;
		std::vector<std::string> subs;

		for (auto p : val) {
			if (p->isSubscription) {
				subs.push_back(p->productId);
			} else {
				products.push_back(p->productId);
			}
		}

		sStoreKitThread.perform([subs, products] (const Task & ) -> bool {
			auto env = spjni::getJniEnv();
			auto storeKit = spjni::getService(spjni::Service::StoreKit, env);
			if (!storeKit) {
				return false;
			}
			auto storeKitClass = spjni::getClassID(env, storeKit);

			jmethodID addProduct = spjni::getMethodID(env, storeKitClass, "addProduct", "(Ljava/lang/String;Z)V");
			if (addProduct) {
				for (auto &it : subs) {
					jstring jstr = env->NewStringUTF(it.c_str());
					env->CallVoidMethod(storeKit, addProduct, jstr, (jboolean)1);
					env->DeleteLocalRef(jstr);
				}

				for (auto &it : products) {
					jstring jstr = env->NewStringUTF(it.c_str());
					env->CallVoidMethod(storeKit, addProduct, jstr, (jboolean)0);
					env->DeleteLocalRef(jstr);
				}

				jmethodID getProductsInfo = spjni::getMethodID(env, storeKitClass, "getProductsInfo", "()V");
				if (getProductsInfo) {
					env->CallVoidMethod(storeKit, getProductsInfo);
					return true;
				}
			}

			return false;
		});
	}

	void save() {
		saveProductDictionary(StoreKit::getInstance()->getProducts());
	}
protected:
	StoreProduct *getProductByRequestId(int requestId) {
		auto products = StoreKit::getInstance()->getProducts();
		for (auto &it : products) {
			if (it.second->requestId == requestId) {
				return it.second;
			}
		}
		return nullptr;
	}

	StoreProduct *getProduct(const std::string &sku) {
		return StoreKit::getInstance()->getProduct(sku);
	}

};

NS_SP_END

NS_SP_PLATFORM_BEGIN

namespace storekit {
	void _saveProducts(const std::unordered_map<std::string, StoreProduct *> &val) {
		StoreKitAndroid::getInstance()->saveProductDictionary(val);
	}
	data::Value _loadProducts() {
		return StoreKitAndroid::getInstance()->loadProductDictionary();
	}

	void _updateProducts(const std::vector<StoreProduct *> &val) {
		StoreKitAndroid::getInstance()->updateProducts(val);
	}

	bool _purchaseProduct(StoreProduct *product) {
		return StoreKitAndroid::getInstance()->purchaseProduct(product);
	}

	bool _restorePurchases() {
		return StoreKitAndroid::getInstance()->restorePurchases();
	}
}

NS_SP_PLATFORM_END

NS_SP_EXTERN_BEGIN
stappler::StoreKitAndroid *getAndroidStoreKit() {
	return stappler::StoreKitAndroid::getInstance();
}

void Java_org_stappler_StoreKit_onSetupCompleted(JNIEnv *env, jobject storeKit, jboolean successful) {
	stappler::Thread::onMainThread([successful] () {
		getAndroidStoreKit()->onSetupCompleted(successful);
	});
}

void Java_org_stappler_StoreKit_onDisposed(JNIEnv *env, jobject storeKit) {
	stappler::Thread::onMainThread([] () {
		getAndroidStoreKit()->onDisposed();
	});
}

void Java_org_stappler_StoreKit_onUpdateFinished(JNIEnv *env, jobject storeKit) {
	stappler::Thread::onMainThread([] () {
		getAndroidStoreKit()->onUpdateFinished();
	});
}

void Java_org_stappler_StoreKit_onPurchaseRestored(JNIEnv *env, jobject storeKit) {
	stappler::Thread::onMainThread([] () {
		getAndroidStoreKit()->onPurchaseRestored();
	});
}

void Java_org_stappler_StoreKit_onFeatureInfoRecieved(JNIEnv *env, jobject storeKit,
		jstring sku, jstring data) {
	auto skuStr = stappler::spjni::jStringToStdString(env, sku);
	auto dataStr = stappler::spjni::jStringToStdString(env, data);

	stappler::Thread::onMainThread([dataStr, skuStr] () {
		getAndroidStoreKit()->onFeatureInfo(skuStr, dataStr);
	});
	stappler::platform::render::_requestRender();
}

void Java_org_stappler_StoreKit_onFeatureInfoFailed(JNIEnv *env, jobject storeKit,
		jstring sku) {
	const auto &skuStr = stappler::spjni::jStringToStdString(env, sku);
	stappler::Thread::onMainThread([skuStr] () {
		getAndroidStoreKit()->onFeatureInfoFailed(skuStr);
	});
	stappler::platform::render::_requestRender();
}

void Java_org_stappler_StoreKit_onTransactionRestored(JNIEnv *env, jobject storeKit,
		jstring sku, jstring data, jstring signature) {
	const auto &skuStr = stappler::spjni::jStringToStdString(env, sku);
	const auto &sigStr = stappler::spjni::jStringToStdString(env, signature);
	const auto &dataStr = stappler::spjni::jStringToStdString(env, data);
	stappler::Thread::onMainThread([dataStr, skuStr, sigStr] () {
		getAndroidStoreKit()->onTransactionRestored(skuStr, sigStr, dataStr);
	});
}

void Java_org_stappler_StoreKit_onTransactionCompleted(JNIEnv *env, jobject storeKit,
		jint requestId, jstring data, jstring signature) {
	const auto &sigStr = stappler::spjni::jStringToStdString(env, signature);
	const auto &dataStr = stappler::spjni::jStringToStdString(env, data);
	stappler::Thread::onMainThread([dataStr, requestId, sigStr] () {
		getAndroidStoreKit()->onTransactionCompleted(requestId, sigStr, dataStr);
	});
}

void Java_org_stappler_StoreKit_onTransactionFailed(JNIEnv *env, jobject storeKit, jint requestId) {
	stappler::Thread::onMainThread([requestId] () {
		getAndroidStoreKit()->onTransactionFailed(requestId);
	});
}

NS_SP_EXTERN_END

#else

NS_SP_EXTERN_BEGIN

void Java_org_stappler_StoreKit_onSetupCompleted(JNIEnv *env, jobject storeKit, jboolean successful) {

}

void Java_org_stappler_StoreKit_onDisposed(JNIEnv *env, jobject storeKit) {

}

void Java_org_stappler_StoreKit_onUpdateFinished(JNIEnv *env, jobject storeKit) {

}

void Java_org_stappler_StoreKit_onPurchaseRestored(JNIEnv *env, jobject storeKit) {

}

void Java_org_stappler_StoreKit_onFeatureInfoRecieved(JNIEnv *env, jobject storeKit,
		jstring sku, jstring data) {

}

void Java_org_stappler_StoreKit_onFeatureInfoFailed(JNIEnv *env, jobject storeKit,
		jstring sku) {

}

void Java_org_stappler_StoreKit_onTransactionRestored(JNIEnv *env, jobject storeKit,
		jstring sku, jstring data, jstring signature) {

}

void Java_org_stappler_StoreKit_onTransactionCompleted(JNIEnv *env, jobject storeKit,
		jint requestId, jstring data, jstring signature) {

}

void Java_org_stappler_StoreKit_onTransactionFailed(JNIEnv *env, jobject storeKit, jint requestId) {

}

NS_SP_EXTERN_END

#endif

#endif
