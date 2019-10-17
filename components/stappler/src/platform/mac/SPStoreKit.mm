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
#include "SPEventHandler.h"
#include "SPDevice.h"
#include "SPFilesystem.h"
#include "SPData.h"
#include "SPString.h"
#include "SPThread.h"
#include "SPTime.h"

#include "SPStorage.h"
#include "SPScheme.h"

#if (MACOSX)
#ifndef SP_RESTRICT
NS_SP_BEGIN

class StoreKitLinux;
static StoreKitLinux *s_storeKitLinux = nullptr;

class StoreKitLinux : public EventHandler {
	private:
	bool _init = false;
	
	storage::Handle *_handle;
	storage::Scheme _products;
	public:
	static StoreKitLinux *getInstance() {
		if (!s_storeKitLinux) {
			s_storeKitLinux = new StoreKitLinux();
		}
		return s_storeKitLinux;
	}
	
	StoreKitLinux() {
		_handle = storage::create("StoreKitEmulator", filesystem::documentsPath("storekit.emulator.sqlite"));
		_products = storage::Scheme("products", {
			storage::Scheme::Field("name", storage::Scheme::Type::Text, storage::Scheme::PrimaryKey),
			storage::Scheme::Field("price", storage::Scheme::Type::Text),
			storage::Scheme::Field("isSubscription", storage::Scheme::Type::Integer, 0, 1),
			storage::Scheme::Field("duration", storage::Scheme::Type::Integer),
			storage::Scheme::Field("purchased", storage::Scheme::Type::Integer, 0, 1),
			storage::Scheme::Field("purchaseTime", storage::Scheme::Type::Integer),
		}, {}, _handle);
		
		Thread::onMainThread([this] {
			init();
			return true;
		}, nullptr, true);
	}
	
	~StoreKitLinux() { }
	
	void init() {
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
		
		onSetupCompleted(true);
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
		
		auto features = new std::map<std::string, data::Value>();
		auto &thread = storage::thread(_handle);
		thread.perform([this, features] (const Task &) -> bool {
			_products.get([&] (data::Value && d) {
				if (d.isArray()) {
					for (auto &it : d.getArray()) {
						if (it.isDictionary() && it.getBool("purchased")) {
							features->insert(std::make_pair(it.getString("name"), it));
						}
					}
				}
			})->perform();
			return true;
		}, [this, features] (const Task &, bool) {
			auto &f = *features;
			for (auto &it : f) {
				onTransactionRestored(it.first, it.second);
			}
			onPurchaseRestored();
			delete features;
		});
		return true;
	}
	
	void onFeatureInfo(const std::string &sku, const data::Value &data) {
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
	void onTransactionRestored(const std::string &sku, const data::Value &data) {
		StoreProduct *product = getProduct(sku);
		if (product && data.isDictionary()) {
			product->validated = true;
			auto date = data.getInteger("purchaseTime");
			
			product->purchased = true;
			product->available = true;
			product->confirmed = true;
			product->purchaseDate = Time::milliseconds(date);
			
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
	
	void onTransactionCompleted(const std::string &sku, const data::Value &data) {
		StoreProduct *product = getProduct(sku);
		if (product) {
			product->validated = true;
			int64_t date = data.getInteger("purchaseTime");
			product->purchased = true;
			product->available = true;
			product->purchaseDate = Time::milliseconds(date);
			
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
		} else {
#if (DEBUG)
			log::format("StoreKit", "no product for id %s", sku.c_str());
#endif
		}
	}
	
	void onTransactionFailed(const std::string &sku) {
		StoreProduct *product = getProduct(sku);
		if (product) {
			StoreKit::onPurchaseFailed(StoreKit::getInstance(), product->productId);
			save();
#if (DEBUG)
			log::format("StoreKit", "Feature failed: %s", product->productId.c_str());
#endif
		} else {
#if (DEBUG)
			log::format("StoreKit", "no product for id %s", sku.c_str());
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
	
	bool purchaseProduct(StoreProduct *product) {
		if (!_init) {
			return false;
		}
		
		std::string productId = product->productId;
		auto data = new data::Value;
		
		auto &thread = storage::thread(_handle);
		thread.perform([this, productId, data] (const Task &) -> bool {
			bool ret = false;
			_products.get([&] (data::Value &&d) {
				if (d.isArray()) {
					auto &p = d.getValue(0);
					if (p.isDictionary()) {
						p.setBool(true, "purchased");
						p.setInteger(Time::now().toMilliseconds(), "purchaseTime");
						_products.insert(p)->perform();
						*data = std::move(p);
						ret = true;
					}
				}
			})->select(productId)->perform();
			return ret;
		}, [this, productId, data] (const Task &, bool success) {
			if (success) {
				onTransactionCompleted(productId, *data);
			} else {
				onTransactionFailed(productId);
			}
			delete data;
		});
		
#if (DEBUG)
		log::format("StoreKit", "start purchase with id %d", product->requestId);
#endif
		return true;
	}
	
	void updateProduct(StoreProduct *product) {
		std::vector<StoreProduct *> val;
		val.push_back(product);
		updateProducts(val);
	}
	
	void updateProducts() {
		auto allProducts = StoreKit::getInstance()->getProducts();
		std::vector<StoreProduct *> val;
		for (auto &it : allProducts) {
			val.push_back(it.second);
		}
		updateProducts(val);
	}
	
	void updateProducts(const std::vector<StoreProduct *> &val) {
		if (!_init) {
			return;
		}
		
		auto features = new std::map<std::string, data::Value>();
		auto &thread = storage::thread(_handle);
		thread.perform([this, val, features] (const Task &) -> bool {
			for (auto &it : val) {
				data::Value info;
				_products.get([&] (data::Value &&d) {
					if (d.isArray()) {
						info = std::move(d.getValue(0));
					}
				})->select(it->productId)->perform();
				
				if (info) {
					features->insert(std::make_pair(it->productId, std::move(info)));
				} else {
					info.setString(it->productId, "name");
					info.setString(it->isSubscription?toString(99 * it->duration, " руб."):"99 руб.", "price");
					info.setBool(it->isSubscription, "isSubscription");
					info.setInteger(it->duration, "duration");
					info.setBool(false, "purchased");
					info.setInteger(0, "purchaseTime");
					_products.insert(info)->perform();
					
					features->insert(std::make_pair(it->productId, std::move(info)));
				}
			}
			return true;
		}, [this, features] (const Task &, bool) {
			auto &f = *features;
			for (auto &it : f) {
				onFeatureInfo(it.first, it.second);
			}
			delete features;
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

namespace stappler::platform::storekit {
	void _saveProducts(const std::unordered_map<std::string, StoreProduct *> &val) {
		StoreKitLinux::getInstance()->saveProductDictionary(val);
	}
	data::Value _loadProducts() {
		return StoreKitLinux::getInstance()->loadProductDictionary();
	}
	
	void _updateProducts(const std::vector<StoreProduct *> &val) {
		StoreKitLinux::getInstance()->updateProducts(val);
	}
	
	bool _purchaseProduct(StoreProduct *product) {
		return StoreKitLinux::getInstance()->purchaseProduct(product);
	}
	
	bool _restorePurchases() {
		return StoreKitLinux::getInstance()->restorePurchases();
	}
}

#else

namespace stappler::platform::storekit {
	void _saveProducts(const std::unordered_map<std::string, StoreProduct *> &val) {
		
	}
	data::Value _loadProducts() {
		return data::Value();
	}
	
	void _updateProducts(const std::vector<StoreProduct *> &val) {
		
	}
	
	bool _purchaseProduct(StoreProduct *product) {
		return false;
	}
	
	bool _restorePurchases() {
		return false;
	}
}

#endif
#endif
