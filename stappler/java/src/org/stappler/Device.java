package org.stappler;

import java.lang.reflect.Constructor;
import java.util.Locale;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.graphics.Point;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.Build;
import android.os.Environment;
import android.os.StatFs;
import android.provider.Settings.Secure;
import android.util.DisplayMetrics;
import android.view.Display;
import android.webkit.WebSettings;
import android.webkit.WebView;
import com.google.firebase.iid.FirebaseInstanceId;
import com.google.android.gms.common.GoogleApiAvailability;
import com.google.android.gms.common.ConnectionResult;

public class Device {
	private Activity _activity = null;
	private boolean _init = false;

	private native void setIsTablet(boolean isTablet);
	private native void setUserAgent(String value);
	private native void setDeviceIdentifier(String value);
	private native void setBundleName(String value);
	private native void setUserLanguage(String value);
	private native void setApplicationName(String value);
	private native void setApplicationVersion(String value);
	private native void setScreenSize(int width, int height);
	private native void setCurrentSize(int width, int height);
	private native void setDensity(float value);

	public  native void setDeviceToken(String value);

	private boolean isTablet(Activity activity) {
        DisplayMetrics metrics = new DisplayMetrics();
        activity.getWindowManager().getDefaultDisplay().getMetrics(metrics);

        int widthPixels = metrics.widthPixels;
        int heightPixels = metrics.heightPixels;

        float scaleFactor = metrics.density;

        float widthDp = widthPixels / scaleFactor;
        float heightDp = heightPixels / scaleFactor;

        float dp = (widthDp > heightDp)?heightDp:widthDp;

        if (dp >= 450) {
        	return true;
        } else {
        	return false;
        }
	}

	@SuppressLint("NewApi")
	private String getUserAgent(Activity activity) {
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1) {
			return WebSettings.getDefaultUserAgent(activity);
		}

		String userAgent = System.getProperty("http.agent");
		if (userAgent != null) {
			return userAgent;
		}

		try {
			Constructor<WebSettings> constructor = WebSettings.class
					.getDeclaredConstructor(Context.class, WebView.class);
			constructor.setAccessible(true);
			try {
				WebSettings settings = constructor.newInstance(activity, null);
				return settings.getUserAgentString();
			} finally {
				constructor.setAccessible(false);
			}
		} catch (Exception e) {
			return new WebView(activity).getSettings().getUserAgentString();
		}
	}

	private String getDeviceIdentifier(Activity activity) {
		 return Secure.getString(activity.getContentResolver(), Secure.ANDROID_ID);
	}

	private String getApplicationName(Activity activity) {
		final PackageManager pm = activity.getPackageManager();
		ApplicationInfo ai;
		try {
			ai = pm.getApplicationInfo( activity.getPackageName(), 0);
		} catch (final NameNotFoundException e) {
			ai = null;
		}
		return (String) (ai != null ? pm.getApplicationLabel(ai) : "(unknown)");
	}

	private String getApplicationVersion(Activity activity) {
		String version = null;
		try {
			PackageInfo pInfo = activity.getPackageManager().getPackageInfo(activity.getPackageName(), 0);
			version = pInfo.versionName;
		} catch (final NameNotFoundException e) {
			version = null;
		}
		return (String) (version != null ? version : "(unknown)");
	}

	private String getUserLanguage(Activity activity) {
		String l = Locale.getDefault().getLanguage() + "-" + Locale.getDefault().getCountry();
		return l.toLowerCase(Locale.getDefault());
	}
	
	
	public Device(Activity activity) {
		_activity = activity;
		updateDevice(_activity);
	}
	
	public void dispose() {
		_activity = null;
	}

	private boolean checkPlayService(int PLAY_SERVICE_STATUS) {
		switch (PLAY_SERVICE_STATUS) {
			case ConnectionResult.SUCCESS:
				return true;
			case ConnectionResult.SERVICE_MISSING:
			case ConnectionResult.SERVICE_UPDATING:
			case ConnectionResult.SERVICE_VERSION_UPDATE_REQUIRED:
			case ConnectionResult.SERVICE_DISABLED:
			case ConnectionResult.SERVICE_INVALID:
				break;
		}
		return false;
	}

	public void updateDeviceToken(final Activity activity) {
		GoogleApiAvailability api = GoogleApiAvailability.getInstance();
		if (checkPlayService(api.isGooglePlayServicesAvailable(activity))) {
			String refreshedToken = FirebaseInstanceId.getInstance().getToken();
			if (refreshedToken != null) {
				setDeviceToken(refreshedToken);
			}
		} else {
			activity.runOnUiThread(new Runnable() {
				@Override
				public void run() {
					GoogleApiAvailability api = GoogleApiAvailability.getInstance();
					api.makeGooglePlayServicesAvailable(activity);
				}
			});
		}
	}

	public void updateDevice(Activity activity) {
		if (!_init) {
			setIsTablet(isTablet(activity));
			setUserAgent(getUserAgent(activity));
			setDeviceIdentifier(getDeviceIdentifier(activity));
			setBundleName(activity.getPackageName());
			setApplicationName(getApplicationName(activity));
			setApplicationVersion(getApplicationVersion(activity));
			setUserLanguage(getUserLanguage(activity));
			_init = true;
		}

		updateDeviceToken(activity);

		Display d = activity.getWindowManager().getDefaultDisplay();
		DisplayMetrics metrics = new DisplayMetrics();
		d.getMetrics(metrics);
		setDensity(metrics.density);
		int widthPixels = metrics.widthPixels;
		int heightPixels = metrics.heightPixels;
		if (Build.VERSION.SDK_INT >= 14 && Build.VERSION.SDK_INT < 17) {
			try {
			    widthPixels = (Integer) Display.class.getMethod("getRawWidth").invoke(d);
			    heightPixels = (Integer) Display.class.getMethod("getRawHeight").invoke(d);
			} catch (Exception ignored) { }
		} else if (Build.VERSION.SDK_INT >= 17) {
			try {
			    Point realSize = new Point();
			    Display.class.getMethod("getRealSize", Point.class).invoke(d, realSize);
			    widthPixels = realSize.x;
			    heightPixels = realSize.y;
			} catch (Exception ignored) { }
		}
		
		if (widthPixels > heightPixels) {
			setScreenSize(heightPixels, widthPixels);
		} else {
			setScreenSize(widthPixels, heightPixels);
		}
		
		int width = metrics.widthPixels;
		int height = metrics.heightPixels;
		setCurrentSize(width, height);
	}
	
	@SuppressWarnings("deprecation")
	@SuppressLint("NewApi")
	public long getTotalSpace() {
		if (Build.VERSION.SDK_INT >= 14 && Build.VERSION.SDK_INT < 18) {
	        StatFs statFs = new StatFs(Environment.getExternalStorageDirectory().getAbsolutePath());   
	        long   total  = (statFs.getBlockCount() * statFs.getBlockSize());
	        return total;
		} else {
	        StatFs statFs = new StatFs(Environment.getExternalStorageDirectory().getAbsolutePath());   
	        long   total  = (statFs.getBlockCountLong() * statFs.getBlockSizeLong());
	        return total;
		}
	}
	
	@SuppressWarnings("deprecation")
	@SuppressLint("NewApi")
	public long getFreeSpace() {
		if (Build.VERSION.SDK_INT >= 14 && Build.VERSION.SDK_INT < 18) {
	        StatFs statFs = new StatFs(Environment.getExternalStorageDirectory().getAbsolutePath());
	        long   free   = (statFs.getAvailableBlocks() * statFs.getBlockSize());
	        return free;
		} else {
	        StatFs statFs = new StatFs(Environment.getExternalStorageDirectory().getAbsolutePath());
	        long   free   = (statFs.getAvailableBlocksLong() * statFs.getBlockSizeLong());
	        return free;
		}
	}
	
	public boolean isNetworkOnline() {
		if (_activity == null) {
			return false;
		}
	    ConnectivityManager cm = (ConnectivityManager) _activity.getSystemService(Context.CONNECTIVITY_SERVICE);
	    NetworkInfo netInfo = cm.getActiveNetworkInfo();
	    if (netInfo != null && netInfo.isConnectedOrConnecting()) {
	        return true;
	    }
	    return false;
	}
}
