/* Copyright (c) 2012 Google Inc.
 * Copyright (c) 2016-2019 Roman Katuntsev <sbkarr@stappler.org>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package org.stappler;

import android.app.Activity;
import android.app.PendingIntent;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.os.Bundle;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.Log;

import org.json.JSONObject;

import java.util.ArrayList;
import java.util.List;

public class StoreKit {
    // Is debug logging enabled?
    boolean mDebugLog = false;
    String mDebugTag = "StoreKit";

    boolean mSetupDone = false;
    boolean mDisposed = false;
    
    Activity activity;
    Context mContext;
    ServiceConnection mServiceConn;

    public StoreKit(Activity _activity) {
    	activity = _activity;
        mContext = activity.getApplicationContext();
    }
    
    public void dispose() {
    	if (!mDisposed) {
    		try {
    			onDisposed();
    		} catch (Exception e) {
				e.printStackTrace();
    		}
            mSetupDone = false;
            if (mServiceConn != null) {
                if (mContext != null) mContext.unbindService(mServiceConn);
            }
            mDisposed = true;
            mContext = null;
            mServiceConn = null;
            activity = null;
    	}
    }
    
    public void startSetup() {
        if (mSetupDone) {
        	return;
        }

        // Connection to IAB service
        logDebug("Starting in-app billing setup.");
        mServiceConn = new ServiceConnection() {
            @Override
            public void onServiceDisconnected(ComponentName name) {
                logDebug("Billing service disconnected.");
				mServiceConn = null;
            }

            @Override
            public void onServiceConnected(ComponentName name, IBinder service) {
				if (mDisposed) {
					return;
				}

				try {
					onSetupCompleted(mSetupDone);
				} catch (Exception e) {
					e.printStackTrace();
				}
            }
        };

		try {

		} catch (Exception e) {
			e.printStackTrace();
		}
    }
    
    private String processPurchases(String token, String itemType) throws RemoteException {
    	return null;
    }
    
    private void getPurchases() { }
    
    public void restorePurchases() {
    	getPurchases();
    	onPurchaseRestored();
    }

    private ArrayList<String> _featuresList = null;
    private ArrayList<String> _subscriptionList = null;
    
    public synchronized void addProduct(String sku, boolean isSubscription) {
    	if (isSubscription == true) {
    		if (_subscriptionList == null) {
    			_subscriptionList = new ArrayList<String>();
    		}

			if (_subscriptionList.indexOf(sku) == -1) {
				logDebug("Add subscription to update: " + sku);
				_subscriptionList.add(sku);
			}
    	} else {
    		if (_featuresList == null) {
    			_featuresList = new ArrayList<String>();
    		}

			if (_featuresList.indexOf(sku) == -1) {
				logDebug("Add product to update: " + sku);
				_featuresList.add(sku);
			}
    	}
    }

	public synchronized void getProductsInfo() {
		try {
			if (_subscriptionList != null) {
				if (_subscriptionList.size() <= 20) {
					getProductsInfo(_subscriptionList, true);
				} else {
					ArrayList<ArrayList<String>> packs = new ArrayList<ArrayList<String>>();
					ArrayList<String> tempList;
					int n = _subscriptionList.size() / 20;
					int mod = _subscriptionList.size() % 20;
					for (int i = 0; i < n; i++) {
						tempList = new ArrayList<String>();
						for (String s : _subscriptionList.subList(i * 20, i * 20 + 20)) {
							tempList.add(s);
						}
						packs.add(tempList);
					}
					if (mod != 0) {
						tempList = new ArrayList<String>();
						for (String s : _subscriptionList.subList(n * 20, n * 20 + mod)) {
							tempList.add(s);
						}
						packs.add(tempList);
					}

					for (ArrayList<String> skuPartList : packs) {
						getProductsInfo(skuPartList, true);
					}
				}
			}
			if (_featuresList != null) {
				if (_featuresList.size() <= 20) {
					getProductsInfo(_featuresList, false);
				} else {
					ArrayList<ArrayList<String>> packs = new ArrayList<ArrayList<String>>();
					ArrayList<String> tempList;
					int n = _featuresList.size() / 20;
					int mod = _featuresList.size() % 20;
					for (int i = 0; i < n; i++) {
						tempList = new ArrayList<String>();
						for (String s : _featuresList.subList(i * 20, i * 20 + 20)) {
							tempList.add(s);
						}
						packs.add(tempList);
					}
					if (mod != 0) {
						tempList = new ArrayList<String>();
						for (String s : _featuresList.subList(n * 20, n * 20 + mod)) {
							tempList.add(s);
						}
						packs.add(tempList);
					}

					for (ArrayList<String> skuPartList : packs) {
						getProductsInfo(skuPartList, false);
					}
				}
			}

			_featuresList = null;
			_subscriptionList = null;

			onUpdateFinished();
		} catch (Exception e) {
			e.printStackTrace(System.out);
		}
    }
    
    private void getProductsInfo(ArrayList<String> skuList, boolean isSubscriptions) {
		try {
			if (mSetupDone) {
	            logDebug("begin products update...");
	            
				Bundle querySkus = new Bundle();
				querySkus.putStringArrayList("ITEM_ID_LIST", skuList);
				String type = (isSubscriptions) ? "subs" : "inapp";
				String packageName = mContext.getPackageName();
			}
			if (skuList.size() > 0) {
				for (String sku : skuList) {
					onFeatureInfoFailed(sku);
				}
			}
		} catch (Exception e) {
			e.printStackTrace(System.out);
		}
    }

    public void purchaseFeature(String sku, boolean isSubscription, int requestId, String token) {
		try {
			onTransactionFailed(requestId);
		} catch (Exception e) {
			e.printStackTrace(System.out);
		}
    }

	public void onActivityResult(int requestCode, int resultCode, Intent data) {
		try {
			onTransactionFailed(requestCode);
		} catch (Exception e) {
			e.printStackTrace(System.out);
		}
	}
    
    void logDebug(String msg) {
        if (mDebugLog) Log.d(mDebugTag, msg);
    }

    void logError(String msg) {
        Log.e(mDebugTag, "In-app billing error: " + msg);
    }

    void logWarn(String msg) {
        Log.w(mDebugTag, "In-app billing warning: " + msg);
    }

    private native void onSetupCompleted(boolean successful);
    private native void onDisposed();
    private native void onUpdateFinished();
    private native void onPurchaseRestored();

    private native void onTransactionRestored(String sku, String purchaseData, String signature);
    private native void onFeatureInfoRecieved(String sku, String data);
    private native void onFeatureInfoFailed(String sku);
    
    private native void onTransactionCompleted(int requestId, String data, String signature);
    private native void onTransactionFailed(int requestId);
}
