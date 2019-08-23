package org.stappler;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

public class NetworkReceiver extends BroadcastReceiver {
    private static final String TAG = "NetworkStateReceiver";

    private native void onConnectivityChanged();
    
    @Override
    public void onReceive(final Context context, final Intent intent) {
    	try {
    		onConnectivityChanged();
    	} catch (java.lang.UnsatisfiedLinkError e) {
    		Log.d("SP.NetworkReceiver", "Library is not loaded");
    	} catch (Exception e) {
    		Log.d("SP.NetworkReceiver", "Library is not loaded");
    	}
    }
}
