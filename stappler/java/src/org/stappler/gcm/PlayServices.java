package org.stappler.gcm;

import java.io.IOException;

import com.google.android.gms.common.ConnectionResult;
import com.google.android.gms.common.GooglePlayServicesUtil;
import com.google.android.gms.gcm.GoogleCloudMessaging;

import org.stappler.Activity;
import org.stappler.Device;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager.NameNotFoundException;
import android.os.AsyncTask;
import android.os.Build;
import android.util.Log;

public class PlayServices {
	private final static String TAG = "PlayServices";

	private String SENDER_ID = "81996293845";
	
	private static final String PROPERTY_REG_ID = "registration_id";
	private static final String PROPERTY_APP_VERSION = "appVersion";
	private final static int PLAY_SERVICES_RESOLUTION_REQUEST = 9000;
	
	private Activity _activity = null;
	private GoogleCloudMessaging _gcm = null;
	private String _regid = null;
	private boolean _errorShown = false;

	public PlayServices(Activity activity) {
		_activity = activity;
	}
	
	public void dispose() {
		_activity = null;
	}
	
	public void requestToken() {
		if (_activity == null) {
			return;
		}
        // Check device for Play Services APK. If check succeeds, proceed with GCM registration.
        if (checkPlayServices()) {
            _gcm = GoogleCloudMessaging.getInstance(_activity);
            _regid = getRegistrationId(_activity);

            if (_regid != null && !_regid.isEmpty()) {
            	_activity.getDevice().setDeviceToken(_regid);
            } else {
                registerInBackground();
            }
        } else {
            Log.i(TAG, "No valid Google Play Services APK found.");
        }
	}

	// Google cloud messaging implementation

    private void storeRegistrationId(Context context, String regId) {
        final SharedPreferences prefs = getGcmPreferences();
        if (prefs != null) {
            int appVersion = getAppVersion(_activity);
            SharedPreferences.Editor editor = prefs.edit();
            editor.putString(PROPERTY_REG_ID, regId);
            editor.putInt(PROPERTY_APP_VERSION, appVersion);
            editor.commit();
        }
    }

    private String getRegistrationId(Context context) {
        final SharedPreferences prefs = getGcmPreferences();
        String registrationId = prefs.getString(PROPERTY_REG_ID, "");
        if (registrationId.isEmpty()) {
            return null;
        }
        int registeredVersion = prefs.getInt(PROPERTY_APP_VERSION, Integer.MIN_VALUE);
        int currentVersion = getAppVersion(context);
        if (registeredVersion != currentVersion) {
            return null;
        }
        return registrationId;
    }

    private void registerInBackground() {
    	final Activity activity = _activity;
    	if (activity != null) {
    		activity.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    new AsyncTask<Void, Void, String>() {
                        @Override
                        protected String doInBackground(Void... params) {
                            String msg = "";
                            try {
                            	if (activity != null) {
                                    if (_gcm == null) {
                                        _gcm = GoogleCloudMessaging.getInstance(activity);
                                    }
                                    _regid = _gcm.register(SENDER_ID);
                                    msg = "Device registered, registration ID=" + _regid;
                                    storeRegistrationId(activity, _regid);
                            	}
                            } catch (IOException ex) {
                                msg = "Error :" + ex.getMessage();
                            }
                            return msg;
                        }

                        @Override
                        protected void onPostExecute(String msg) {
                            if (_regid != null && !_regid.isEmpty()) {
                            	Device dev = activity.getDevice();
                            	if (dev != null) {
                                	dev.setDeviceToken(_regid);
                            	}
                            }
                        }
                    }.execute(null, null, null);
                }
    		});
    	}
    }

	public boolean checkPlayServices() {
		if (_activity == null) {
			return false;
		}
	    final int resultCode = GooglePlayServicesUtil.isGooglePlayServicesAvailable(_activity);
	    if (resultCode != ConnectionResult.SUCCESS) {
	        if (GooglePlayServicesUtil.isUserRecoverableError(resultCode)) {
	        	_activity.runOnUiThread(new Runnable() {
	        		@Override
	                public void run() {
	        			if (!_errorShown && !isAndroidEmulator()) {
		    	            GooglePlayServicesUtil.getErrorDialog(resultCode, _activity,
		    	                    PLAY_SERVICES_RESOLUTION_REQUEST).show();
		    	            _errorShown = true;
	        			}
	        		}
	        	});
	        } else {
	            Log.i(TAG, "This device is not supported.");
	        }
	        return false;
	    }
	    return true;
	}

    private int getAppVersion(Context context) {
        try {
            PackageInfo packageInfo = context.getPackageManager().getPackageInfo(context.getPackageName(), 0);
            return packageInfo.versionCode;
        } catch (NameNotFoundException e) {
            Log.i(TAG, "Could not get package name: " + e);
        }
        return 0;
    }
    
    private SharedPreferences getGcmPreferences() {
    	if (_activity == null) {
    		return null;
    	}
        return _activity.getSharedPreferences(Activity.class.getSimpleName(), Context.MODE_PRIVATE);
    }
    
    private final static boolean isAndroidEmulator() {
        String model = Build.MODEL;
        Log.d(TAG, "model=" + model);
        String product = Build.PRODUCT;
        Log.d(TAG, "product=" + product);
        boolean isEmulator = false;
        if (product != null) {
           isEmulator = product.equals("sdk") || product.contains("_sdk") || product.contains("sdk_");
        }
        Log.d(TAG, "isEmulator=" + isEmulator);
        return isEmulator;
     }
}
