package org.stappler;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.concurrent.atomic.AtomicInteger;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.egl.EGLSurface;

import android.opengl.GLSurfaceView.EGLContextFactory;

import org.cocos2dx.lib.Cocos2dxActivity;
import org.cocos2dx.lib.Cocos2dxGLSurfaceView;
import org.stappler.downloads.Manager;
import org.stappler.gcm.IconResource;

import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.support.v4.app.NotificationCompat;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.FrameLayout;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.content.res.Configuration;

public class Activity extends Cocos2dxActivity {
	protected final static String TAG = "Stappler Activity";

	protected LoaderView _splashScreen = null;
	protected boolean _directorStarted = false;
	protected boolean _isActive = false;
	protected boolean _statusBarVisible = true;

	// for Google cloud messaging support
	protected static final String EXTRA_MESSAGE = "message";
	protected int _notificationId = 1;

	protected  AtomicInteger _msgId = new AtomicInteger();
    protected Context _context;
    protected StoreKit _storeKit = null;
    protected Device _device = null;
    protected Manager _downloadManager = null;
    protected Clipboard _clipboard = null;

    protected native void setStatusBarHeight(int value);
    protected native void setInitialUrl(String url, String action);
    protected native void processUrl(String url, String action);
    
    protected native void nativeOnCreate();
    protected native void nativeOnDestroy();
    protected native void nativeOnStart();
    protected native void nativeOnRestart();
    protected native void nativeOnStop();
    protected native void nativeOnPause();
    protected native void nativeOnResume();

    @Override
    protected void onStart() {
    	super.onStart();
    	nativeOnStart();
    }
    
    @Override
    protected void onStop() {
    	nativeOnStop();
    	super.onStop();
    }

    @Override
    protected void onRestart() {
    	super.onRestart();
    	nativeOnRestart();
    }
    
	@Override
	protected void onResume() {
		_isActive = true;
		super.onResume();
		nativeOnResume();

		if (_device != null) {
			_device.updateDeviceToken(this);
		}

		final boolean visible = _statusBarVisible;
		runOnUiThread(new Runnable() {
			@Override
			public void run() {
				View decorView = getWindow().getDecorView();
				int uiOptions;
				if (visible) {
					uiOptions = decorView.getSystemUiVisibility() & ~View.SYSTEM_UI_FLAG_FULLSCREEN;
				} else {
					uiOptions = decorView.getSystemUiVisibility() | View.SYSTEM_UI_FLAG_FULLSCREEN;
				}
				decorView.setSystemUiVisibility(uiOptions);
			}
		});
	}

	@Override
	protected void onPause() {
		nativeOnPause();
		super.onPause();
		_isActive = false;
	}

	private void extractFile(String filename, String archivePath) {
		File caBundle = new File(getFilesDir() + "/" + filename);
		if (!caBundle.exists()) try {
		    InputStream is = getAssets().open(archivePath);
		    int size = is.available();
		    byte[] buffer = new byte[size];
		    is.read(buffer);
		    is.close();

		    FileOutputStream fos = new FileOutputStream(caBundle);
		    fos.write(buffer);
		    fos.close();
		} catch (IOException e) { }
	}

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		try {
			extractFile("ca_bundle.pem", "ca_bundle.pem");
			extractFile("app.json", "app.json");

			this.getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE
					| WindowManager.LayoutParams.SOFT_INPUT_STATE_HIDDEN);
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
				int statusBar = 0;
				int resourceId = getResources().getIdentifier("status_bar_height", "dimen", "android");
				if (resourceId > 0) {
					statusBar = getResources().getDimensionPixelSize(resourceId);
				}

				if (statusBar == 0) {
					this.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_TRANSLUCENT_STATUS);
				} else {
					setStatusBarHeight(statusBar);
					this.getWindow().addFlags(WindowManager.LayoutParams.FLAG_TRANSLUCENT_STATUS);
				}
			}

			super.onCreate(savedInstanceState);
			_context = getApplicationContext();

	    	_downloadManager = new Manager(this);
	    	_device = new Device(this);
	    	_clipboard = new Clipboard(this);
		
			_device.updateDevice(this);
			_storeKit = new StoreKit(this);
		
			cancelNotifications();

			Intent intent = getIntent();
			if (intent != null) {
				String action = intent.getAction();
				Uri data = intent.getData();

				if (data != null) {
					setInitialUrl(data.toString(), action);
				}
			}

			nativeOnCreate();
		} catch (Exception e) {
			e.printStackTrace(System.out);
		}
	}

	@Override
	public void onDestroy() {
		nativeOnDestroy();
		
		_clipboard.dispose();
		_clipboard = null;
		
		_downloadManager.dispose();
		_downloadManager = null;
		
		_storeKit.dispose();
		_storeKit = null;

		_device.dispose();
		_device = null;
		super.onDestroy();
	}

	protected EGLContext _offscreenContext = null;
	protected EGL10 _egl = null;
	protected EGLSurface _localSurface = null;
	protected EGLDisplay _display = null;

	private static int EGL_CONTEXT_CLIENT_VERSION = 0x3098;

	@Override
	public Cocos2dxGLSurfaceView onCreateView() {
		Cocos2dxGLSurfaceView view = new Cocos2dxGLSurfaceView(this);

		view.setEGLContextFactory(new EGLContextFactory() {
			@Override
			public void destroyContext(EGL10 egl, EGLDisplay display, EGLContext context) {
				egl.eglDestroySurface(display, _localSurface);
				egl.eglDestroyContext(display, _offscreenContext);
				egl.eglDestroyContext(display, context);

				_display = null;
				_offscreenContext = null;
				_localSurface = null;
			}
			@Override
			public EGLContext createContext(final EGL10 egl, final EGLDisplay display,
											final EGLConfig eglConfig) {

				EGLContext renderContext = egl.eglCreateContext(display, eglConfig,
						EGL10.EGL_NO_CONTEXT, new int[] { EGL_CONTEXT_CLIENT_VERSION, 2, EGL10.EGL_NONE } );

				int pbufferAttribs[] = { EGL10.EGL_WIDTH, 1, EGL10.EGL_HEIGHT, 1, EGL10.EGL_NONE };
				_localSurface = egl.eglCreatePbufferSurface(display, eglConfig, pbufferAttribs);
				_offscreenContext = egl.eglCreateContext(display, eglConfig, renderContext,
						new int[] { EGL_CONTEXT_CLIENT_VERSION, 2, EGL10.EGL_NONE } );

				_egl = egl;
				_display = display;
				return renderContext;
			}
		});

		return view;
	}

	@Override
	public void setContentView(View view) {
		FrameLayout layout = (FrameLayout)view;

		if (_directorStarted == false) {
			_splashScreen = new LoaderView(this);
			layout.addView(_splashScreen);
		}

		super.setContentView(layout);
	}

	void enableOffscreenContext() {
		try {
			_egl.eglMakeCurrent(_display, _localSurface, _localSurface, _offscreenContext);
		} catch (Exception e) {
			e.printStackTrace(System.out);
		}
	}
	
	void disableOffscreenContext() {
		try {
			_egl.eglMakeCurrent(_display, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_CONTEXT);
		} catch (Exception e) {
			e.printStackTrace(System.out);
		}
	}

	public void removeSplashScreen() {
		if (_splashScreen != null) {
			ViewGroup parent = (ViewGroup)_splashScreen.getParent();
			parent.removeView(_splashScreen);
			_splashScreen = null;
		}
	}

	@Override
	public void onConfigurationChanged (Configuration newConfig) {
		super.onConfigurationChanged(newConfig);
		_device.updateDevice(this);
	}

	@Override
	public void onNewIntent(Intent intent) {
		if (intent != null) {
		    String action = intent.getAction();
		    Uri data = intent.getData();
		    
		    if (data != null) {
		    	processUrl(data.toString(), action);
		    }
		}
		
		cancelNotifications();
	}

	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		_storeKit.onActivityResult(requestCode, resultCode, data);
	}

	public StoreKit getStoreKit() {
		return _storeKit;
	}
	
	public Device getDevice() {
		return _device;
	}

	public Clipboard getClipboard() {
		return _clipboard;
	}
	
	public Manager getDownloadManager() {
		return _downloadManager;
	}

	public boolean isActive() {
		return _isActive;
	}
	
	public void openWebURL( String inURL ) {
	    Intent browse = new Intent( Intent.ACTION_VIEW , Uri.parse( inURL ) );
	    browse.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
	    startActivity( browse );
	}
	
	public void perfromBackPressed() {
		runOnUiThread(new Runnable() {
            @Override
            public void run() {
    			onBackPressed();
            }
		});
	}
	
	public void showNotificationRunnable(String title, String text) {
		NotificationCompat.Builder mBuilder =
		        new NotificationCompat.Builder(this)
				.setLargeIcon(IconResource.getIconBitmap(this))
		        .setSmallIcon(IconResource.getIconId(this))
		        .setContentTitle(title)
		        .setContentText(text);

		Intent resultIntent = new Intent(this, Activity.class);
		mBuilder.setContentIntent(PendingIntent.getActivity(this, 0,
				resultIntent, Intent.FLAG_ACTIVITY_NEW_TASK));
		NotificationManager mNotificationManager =
		    (NotificationManager) this.getSystemService(Context.NOTIFICATION_SERVICE);
		mNotificationManager.notify(_notificationId, mBuilder.build());
		_notificationId ++;
	}

	public void cancelNotifications() {
		runOnUiThread(new Runnable() {
			@Override
			public void run() {
				NotificationManager mNotificationManager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
				mNotificationManager.cancelAll();
			}
		});
	}
	
	public void showNotification(String title, String text) {
		final String fTitle = title;
		final String fText = text;
		if (_isActive == false) {
			runOnUiThread(new Runnable() {
	            @Override
	            public void run() {
	            	showNotificationRunnable(fTitle, fText);
	            }
			});
		}
	}
	
	public void directorStarted() {
		_device.updateDeviceToken(this);
		runOnUiThread(new Runnable() {
            @Override
            public void run() {
				_directorStarted = true;
            	removeSplashScreen();
            }
		});
	}
	
	public void renderFrame() {
		org.cocos2dx.lib.Cocos2dxGLSurfaceView.staticRenderFrame();
	}
	
	public void setRenderContinuously(boolean value) {
		org.cocos2dx.lib.Cocos2dxGLSurfaceView.staticSetRenderContinuously(value);
	}

	public void runInput(String text, int cursorStart, int cursorLen, int type) {
		final String ftext = text;
		final int fcursorStart = cursorStart;
		final int fcursorLen = cursorLen;
		final int ftype = type;
		runOnUiThread(new Runnable() {
			@Override
			public void run() {
				if (mGLSurfaceView != null && _isActive) {
					mGLSurfaceView.runInput(ftext, fcursorStart, fcursorLen, ftype);
				}

				Activity.this.getWindow().addFlags(WindowManager.LayoutParams.FLAG_TRANSLUCENT_STATUS);
			}
		});
	}
	
	public void updateText(String text, int cursorStart, int cursorLen, int type) {
		final String ftext = text;
		final int fcursorStart = cursorStart;
		final int fcursorLen = cursorLen;
		final int ftype = type;
		runOnUiThread(new Runnable() {
			@Override
			public void run() {
				if (mGLSurfaceView != null && _isActive) {
					mGLSurfaceView.updateInput(ftext, fcursorStart, fcursorLen, ftype);
				}
			}
		});
	}
	
	public void updateCursor(int cursorStart, int cursorLen) {
		final int fcursorStart = cursorStart;
		final int fcursorLen = cursorLen;
		runOnUiThread(new Runnable() {
			@Override
			public void run() {
				if (mGLSurfaceView != null && _isActive) {
					mGLSurfaceView.updateCursor(fcursorStart, fcursorLen);
				}
			}
		});
	}
	
	public void cancelInput() {
		runOnUiThread(new Runnable() {
			@Override
			public void run() {
				if (mGLSurfaceView != null && _isActive) {
					mGLSurfaceView.cancelInput();
				}
			}
		});
	}

	public synchronized String getWritablePath() {
		return getFilesDir().getAbsolutePath();
	}

	protected String _externalStorageState = null;
	protected File _documentsDir = null;
	public synchronized String getCachesPath() {
		if (_externalStorageState == null || !Environment.getExternalStorageState().equals(_externalStorageState)) {
			_externalStorageState = Environment.getExternalStorageState();
			if (_externalStorageState.equals(Environment.MEDIA_MOUNTED)) {
				_documentsDir = getExternalFilesDir(null);
				if (!_documentsDir.isDirectory() && !_documentsDir.mkdirs()) {
					_documentsDir = null;
				}
			} else {
				_documentsDir = null;
			}

		    if (_documentsDir == null) {
		    	_documentsDir = getFilesDir();
		    	_documentsDir.mkdirs();
		    }
		}
	    
		return _documentsDir.getAbsolutePath();
	}
	
	public boolean isStatusBarEnabled() {
		return _statusBarVisible;
	}
	
	public void setStatusBarEnabled(boolean value) {
		if (value != _statusBarVisible) {
			if (_statusBarVisible) {
				runOnUiThread(new Runnable() {
					@Override
					public void run() {
						View decorView = getWindow().getDecorView();
						int uiOptions = decorView.getSystemUiVisibility() | View.SYSTEM_UI_FLAG_FULLSCREEN;
						decorView.setSystemUiVisibility(uiOptions);
					}
				});
			} else {
				runOnUiThread(new Runnable() {
					@Override
					public void run() {
						View decorView = getWindow().getDecorView();
						int uiOptions = decorView.getSystemUiVisibility() & ~View.SYSTEM_UI_FLAG_FULLSCREEN;
						decorView.setSystemUiVisibility(uiOptions);
					}
				});
			}
			_statusBarVisible = value;
		}
	}
	
	public void setStatusBarLight(boolean value) {
		final boolean v = value;
		runOnUiThread(new Runnable() {
			@Override
			public void run() {
				View decorView = getWindow().getDecorView();
				int uiOptions = decorView.getSystemUiVisibility();
				if (v) {
					uiOptions = uiOptions | View.SYSTEM_UI_FLAG_LIGHT_STATUS_BAR;
				} else {
					uiOptions = uiOptions & ~View.SYSTEM_UI_FLAG_LIGHT_STATUS_BAR;
				}
				decorView.setSystemUiVisibility(uiOptions);
			}
		});
	}
	
    static {
         System.loadLibrary("application");
    }
}
