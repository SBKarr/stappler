/* Copyright (c) 2012 Google Inc.
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

import com.android.vending.billing.IInAppBillingService;

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
    IInAppBillingService mService;
    ServiceConnection mServiceConn;

    // Billing response codes
    public static final int BILLING_RESPONSE_RESULT_OK = 0;
    public static final int BILLING_RESPONSE_RESULT_USER_CANCELED = 1;
    public static final int BILLING_RESPONSE_RESULT_BILLING_UNAVAILABLE = 3;
    public static final int BILLING_RESPONSE_RESULT_ITEM_UNAVAILABLE = 4;
    public static final int BILLING_RESPONSE_RESULT_DEVELOPER_ERROR = 5;
    public static final int BILLING_RESPONSE_RESULT_ERROR = 6;
    public static final int BILLING_RESPONSE_RESULT_ITEM_ALREADY_OWNED = 7;
    public static final int BILLING_RESPONSE_RESULT_ITEM_NOT_OWNED = 8;

    // IAB Helper error codes
    public static final int IABHELPER_ERROR_BASE = -1000;
    public static final int IABHELPER_REMOTE_EXCEPTION = -1001;
    public static final int IABHELPER_BAD_RESPONSE = -1002;
    public static final int IABHELPER_VERIFICATION_FAILED = -1003;
    public static final int IABHELPER_SEND_INTENT_FAILED = -1004;
    public static final int IABHELPER_USER_CANCELLED = -1005;
    public static final int IABHELPER_UNKNOWN_PURCHASE_RESPONSE = -1006;
    public static final int IABHELPER_MISSING_TOKEN = -1007;
    public static final int IABHELPER_UNKNOWN_ERROR = -1008;
    public static final int IABHELPER_SUBSCRIPTIONS_NOT_AVAILABLE = -1009;
    public static final int IABHELPER_INVALID_CONSUMPTION = -1010;

    // Keys for the responses from InAppBillingService
    public static final String RESPONSE_CODE = "RESPONSE_CODE";
    public static final String RESPONSE_GET_SKU_DETAILS_LIST = "DETAILS_LIST";
    public static final String RESPONSE_BUY_INTENT = "BUY_INTENT";
    public static final String RESPONSE_INAPP_PURCHASE_DATA = "INAPP_PURCHASE_DATA";
    public static final String RESPONSE_INAPP_SIGNATURE = "INAPP_DATA_SIGNATURE";
    public static final String RESPONSE_INAPP_ITEM_LIST = "INAPP_PURCHASE_ITEM_LIST";
    public static final String RESPONSE_INAPP_PURCHASE_DATA_LIST = "INAPP_PURCHASE_DATA_LIST";
    public static final String RESPONSE_INAPP_SIGNATURE_LIST = "INAPP_DATA_SIGNATURE_LIST";
    public static final String INAPP_CONTINUATION_TOKEN = "INAPP_CONTINUATION_TOKEN";

    // Item types
    public static final String ITEM_TYPE_INAPP = "inapp";
    public static final String ITEM_TYPE_SUBS = "subs";

    // some fields on the getSkuDetails response bundle
    public static final String GET_SKU_DETAILS_ITEM_LIST = "ITEM_ID_LIST";
    public static final String GET_SKU_DETAILS_ITEM_TYPE_LIST = "ITEM_TYPE_LIST";

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
            mService = null;
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
                mService = null;
				mServiceConn = null;
            }

            @Override
            public void onServiceConnected(ComponentName name, IBinder service) {
				if (mDisposed) {
					return;
				}
				
				logDebug("Billing service connected.");
				mService = IInAppBillingService.Stub.asInterface(service);
				String packageName = mContext.getPackageName();
				try {
					// check for in-app billing v3 support
					int response = mService.isBillingSupported(3, packageName,
							ITEM_TYPE_INAPP);
					if (response != BILLING_RESPONSE_RESULT_OK) {
						onSetupCompleted(false); // version 3 is not supported
						return;
					} else {
						// check for v3 subscriptions support
						response = mService.isBillingSupported(3, packageName,
								ITEM_TYPE_SUBS);
						if (response == BILLING_RESPONSE_RESULT_OK) {
							mSetupDone = true;
						}
					}
					if (mSetupDone) {
						getPurchases();
						onPurchaseRestored();
					}

					onSetupCompleted(mSetupDone);
				} catch (Exception e) {
					e.printStackTrace();
				}
            }
        };

		try {
			Intent serviceIntent = new Intent(
					"com.android.vending.billing.InAppBillingService.BIND");
			if (serviceIntent != null && mContext != null) {
				serviceIntent.setPackage("com.android.vending");
				PackageManager pm = mContext.getPackageManager();
				if (pm != null) {
					List<ResolveInfo> info = pm.queryIntentServices(
							serviceIntent, 0);
					if (info != null && !info.isEmpty()) {
						// service available to handle that Intent
						mContext.bindService(serviceIntent, mServiceConn,
								Context.BIND_AUTO_CREATE);
					} else {
						onSetupCompleted(false);
						mServiceConn = null;
					}
				}
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
    }
    
    private String processPurchases(String token, String itemType) throws RemoteException {
        logDebug("Process owned purchases... ");
    	Bundle ownedItems = mService.getPurchases(3, mContext.getPackageName(), itemType, null);
    	int response = ownedItems.getInt("RESPONSE_CODE");
    	if (response == BILLING_RESPONSE_RESULT_OK) {
    	   ArrayList<String> ownedSkus = ownedItems.getStringArrayList("INAPP_PURCHASE_ITEM_LIST");
    	   ArrayList<String>  purchaseDataList = ownedItems.getStringArrayList("INAPP_PURCHASE_DATA_LIST");
    	   ArrayList<String>  signatureList = ownedItems.getStringArrayList("INAPP_DATA_SIGNATURE_LIST");
    	   String continuationToken = ownedItems.getString("INAPP_CONTINUATION_TOKEN");
    	   for (int i = 0; i < purchaseDataList.size(); ++i) {
    	      String purchaseData = purchaseDataList.get(i);
    	      String signature = (signatureList != null)?signatureList.get(i):"";
    	      String sku = ownedSkus.get(i);
    	      logDebug("Owned purchase: " + itemType);
    	      onTransactionRestored(sku, purchaseData, signature);
    	   }
    	   return continuationToken;
    	}
    	return null;
    }
    
    private void getPurchases() {
        if (!mSetupDone) {
        	return;
        }
        
        try {
            logDebug("Get owned purchases...");
        	String token = null;
        	
        	token = processPurchases(null, ITEM_TYPE_INAPP);
        	while (token != null) {
        		token = processPurchases(token, ITEM_TYPE_INAPP);
        	}
        	
        	token = processPurchases(null, ITEM_TYPE_SUBS);
        	while (token != null) {
        		token = processPurchases(token, ITEM_TYPE_SUBS);
        	}
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
    
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

				Bundle skuDetails = mService.getSkuDetails(3, packageName,
						type, querySkus);
				int response = skuDetails.getInt("RESPONSE_CODE");
				if (response == BILLING_RESPONSE_RESULT_OK) {
					ArrayList<String> responseList = skuDetails
							.getStringArrayList(RESPONSE_GET_SKU_DETAILS_LIST);
					if (responseList.size() == 0) {
						logError("getSkuDetails() returned an empty bundle.");
					} else {
						for (String thisResponse : responseList) {
							JSONObject object = new JSONObject(thisResponse);
							String sku = object.getString("productId");
				            logDebug("Feature: " + sku);
							onFeatureInfoRecieved(sku, thisResponse);
							skuList.remove(sku);
						}
					}
				}
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
			if (mSetupDone) {
				final String f_sku = sku;
				final boolean f_isSubscription = isSubscription;
				final int f_requestId = requestId;
				final String f_token = token;

				activity.runOnUiThread(new Runnable() {
					@Override
					public void run() {
						String type = (f_isSubscription) ? "subs" : "inapp";
						try {
							Bundle buyIntentBundle = mService.getBuyIntent(3,
									mContext.getPackageName(), f_sku, type,
									f_token);
							int response = buyIntentBundle
									.getInt("RESPONSE_CODE");
							if (response == BILLING_RESPONSE_RESULT_OK) {
								PendingIntent pendingIntent = buyIntentBundle
										.getParcelable("BUY_INTENT");
								activity.startIntentSenderForResult(
										pendingIntent.getIntentSender(),
										f_requestId, new Intent(),
										Integer.valueOf(0), Integer.valueOf(0),
										Integer.valueOf(0));
								return;
							}
						} catch (Exception e) {
							e.printStackTrace();
						}
					}
				});
			} else {
				onTransactionFailed(requestId);
			}
		} catch (Exception e) {
			e.printStackTrace(System.out);
		}
    }

	public void onActivityResult(int requestCode, int resultCode, Intent data) {
		try {
			if (mSetupDone && resultCode == Activity.RESULT_OK) {
				int responseCode = data.getIntExtra("RESPONSE_CODE", 0);
				if (responseCode == BILLING_RESPONSE_RESULT_OK) {
					String purchaseData = data
							.getStringExtra("INAPP_PURCHASE_DATA");
					String dataSignature = data
							.getStringExtra("INAPP_DATA_SIGNATURE");
					onTransactionCompleted(requestCode, purchaseData,
							dataSignature);
					return;
				}
			}
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
