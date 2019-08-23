package org.stappler.downloads;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.nio.channels.FileChannel;

import org.stappler.Activity;
import android.app.DownloadManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.database.Cursor;
import android.net.Uri;
import android.os.Environment;
import android.util.Log;

import org.json.JSONException;
import org.json.JSONObject;

public class Manager {
	private Activity activity = null;
	private String packageName = null;
	private DownloadManager downloadManager = null;
	private Observer downloadObserver = null;
	private boolean isStarted = true;
	private boolean hasExternal = false;
	
	private File targetDir = null;
	private File documentsDir = null;
	
	private boolean isRecieversRegistred = false;

	BroadcastReceiver onComplete = new BroadcastReceiver() {
		public void onReceive(Context ctx, Intent intent) {
			try {
				long downloadId = intent.getLongExtra(DownloadManager.EXTRA_DOWNLOAD_ID, 0);
	
				String tmpFile = null;
				Info download = downloadObserver.getDownloadById(downloadId);
				if (download != null) {
					downloadObserver.removeDownload(download);
					tmpFile = download.getDownloadPath();
				}
				
				Cursor c = downloadManager.query(new DownloadManager.Query().setFilterById(downloadId));
	    		if (c != null && download != null && c.moveToFirst()) {
	    			int status = c.getInt(c.getColumnIndex(DownloadManager.COLUMN_STATUS));
	    			if (!hasExternal) {
	    				tmpFile = c.getString(c.getColumnIndex(DownloadManager.COLUMN_LOCAL_FILENAME));
	    			}
	    			
	    			if (status == DownloadManager.STATUS_SUCCESSFUL) {
	    				if (!fileMove(tmpFile, download.getDestPath())) {
	    					status =  DownloadManager.STATUS_FAILED;
	    				}
	    			}
	
					removeDownload(download);
				    new File(tmpFile).delete();
	    			
				    String downloadData = download.getData();
	    			if (status == DownloadManager.STATUS_SUCCESSFUL) {
	    				nativeDownloadCompleted(download.getAssetId(), true, downloadData);
	    			} else if (status == DownloadManager.STATUS_FAILED) {
	    				nativeDownloadCompleted(download.getAssetId(), false, downloadData);
	    			}
	    		}
	    		if (c != null) {
	        		c.close();
	    		}
			} catch (Exception e) {
				e.printStackTrace();
			}
		}
	};
	
	BroadcastReceiver onNotificationClick = new BroadcastReceiver() {
		public void onReceive(Context ctxt, Intent intent) {
			long ids[] = intent.getLongArrayExtra(DownloadManager.EXTRA_NOTIFICATION_CLICK_DOWNLOAD_IDS);
			long downloadId = intent.getLongExtra(DownloadManager.EXTRA_DOWNLOAD_ID, 0);
			Info download = null;
			if (downloadId != 0) {
				download = downloadObserver.getDownloadById(downloadId);
			} else if (ids[0] != 0) {
				download = downloadObserver.getDownloadById(ids[0]);
			}
			if (download != null) {
				nativeDownloadClicked(download.getAssetId());
			}

			Context context = activity.getApplicationContext();
			Intent newIntent = new Intent(context, Activity.class);
			newIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
			context.startActivity(newIntent);
		}
	};
	
	public Manager(Activity _activity) {
		activity = _activity;
		packageName = _activity.getPackageName();
		
		hasExternal = false;
	    if(Environment.getExternalStorageState().equals(Environment.MEDIA_MOUNTED)) {
	    	hasExternal = true;
			targetDir = activity.getExternalCacheDir();
	    	if (!targetDir.isDirectory() && !targetDir.mkdirs()) {
	    		targetDir = null;
	    	}
			documentsDir = activity.getExternalFilesDir(null);
	    	if (!documentsDir.isDirectory() && !documentsDir.mkdirs()) {
	    		documentsDir = null;
	    	}
	    }
	    
	    if (targetDir == null) {
	    	targetDir = activity.getCacheDir();
		    if (targetDir != null) {
		    	targetDir.mkdirs();
		    }
	    }
	    if (documentsDir == null) {
	    	documentsDir = activity.getFilesDir();
		    if (documentsDir != null) {
		    	documentsDir.mkdirs();
		    }
	    }
		
		downloadManager = (DownloadManager)activity.getSystemService(Context.DOWNLOAD_SERVICE);
	    downloadObserver = new Observer(this, targetDir.getAbsolutePath());
	    
	    if (isStarted) {
	    	//downloadObserver.startWatching();
	    }
	}
	
	public Activity getActivity() {
		return activity;
	}
	
	public void dispose() {
		if (activity != null) {
			if (isRecieversRegistred) {
				activity.unregisterReceiver(onComplete);
				activity.unregisterReceiver(onNotificationClick);
				isRecieversRegistred = false;
			}
			
			pause();
			
			downloadObserver.dispose();
			downloadObserver = null;
			activity = null;
		}
	}

	private void removeDownload(long downloadId) {
		if (activity != null) {
			downloadManager.remove(downloadId);
			downloadObserver.removeDownloadById(downloadId);

			SharedPreferences.Editor editor = activity.getPreferences(Context.MODE_PRIVATE).edit();
			editor.remove(String.valueOf(downloadId));
			editor.commit();
		}
	}
	
	private void removeDownload(Info download) {
		if (activity != null) {
			downloadManager.remove(download.getDownloadId());
			downloadObserver.removeDownload(download);

			SharedPreferences.Editor editor = activity.getPreferences(Context.MODE_PRIVATE).edit();
			editor.remove(String.valueOf(download.getDownloadId()));
			editor.commit();
		}
	}
	
	public void pause() {
		if (activity != null) {
		    if (isStarted) {
		    	downloadObserver.stopWatching();
		    	isStarted = false;
		    }
		}
	}
	
	public void resume() {
		if (activity != null) {
		    if (!isStarted) {
		    	//downloadObserver.startWatching();
		    	isStarted = true;
		    }
		}
	}
	
	public void completeDownload() {
		
	}
	
	public void restoreDownloads() {
		if (activity == null) {
			return;
		}
		//Log.d("DownloadMaster", "Begin Restore Downloads");
		Cursor c = downloadManager.query(new DownloadManager.Query());
		boolean b = c.moveToFirst();
		while (b) {
			String downloadPath = c.getString(c.getColumnIndex(DownloadManager.COLUMN_LOCAL_FILENAME));
			int status = c.getInt(c.getColumnIndex(DownloadManager.COLUMN_STATUS));
			long downloadId = c.getLong(c.getColumnIndex(DownloadManager.COLUMN_ID));
			
			SharedPreferences pref = activity.getPreferences(Context.MODE_PRIVATE);
			String data = pref.getString(String.valueOf(downloadId), null);
			JSONObject obj = null;
			String url = null;
			String path = null;
			String name = null;
			String downloadData = null;
			long assetId = 0;
			
			if (data != null) {
				try {
					obj = new JSONObject(data);
					url = obj.getString("url");
					path = obj.getString("path");
					name = obj.getString("name");
					assetId = obj.getLong("id");
					downloadData = obj.getString("data");
				} catch (JSONException e) {
					obj = null;
				}
			}
			
			if (obj != null) {
				if (nativeDownloadRestore(url, path, name, assetId, downloadData)) {
	    			if (status == DownloadManager.STATUS_SUCCESSFUL) {
	    				if (!fileMove(downloadPath, path)) {
	    					status =  DownloadManager.STATUS_FAILED;
	    				} else {
							removeDownload(downloadId);
	    					new File(downloadPath).delete();
	    				}
	    			}
					
					if (status == DownloadManager.STATUS_SUCCESSFUL) {
						/*Log.d("DownloadMaster", String.format("restore completed download: %d %s",
								id, name));*/
						nativeDownloadCompleted(assetId, true, downloadData);
					} else if (status == DownloadManager.STATUS_FAILED) {
						/*Log.d("DownloadMaster", String.format("restore failed download: %d %s",
								id, name));*/
						removeDownload(downloadId);
						nativeDownloadCompleted(assetId, false, downloadData);
					} else {
						Info download = new Info(downloadId, assetId, name, downloadPath, path, downloadData);
						nativeDownloadStarted(assetId);
						long dAll = c.getLong(c.getColumnIndex(DownloadManager.COLUMN_TOTAL_SIZE_BYTES));
						long dBytes = c.getLong(c.getColumnIndex(DownloadManager.COLUMN_BYTES_DOWNLOADED_SO_FAR));
						nativeDownloadProgress(assetId, (float)dBytes / (float)dAll);
						downloadObserver.addDownload(download);
					}
				} else {
					if (downloadPath.contains(packageName)) {
						downloadManager.remove(downloadId);
					}
				}
			}
			
			b = c.moveToNext();
		}
		c.close();
		
		nativeAllDownloadsRestored();
		enableRecievers();
	}

	public void enableRecievers() {
		if (activity == null) {
			return;
		}

		activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
        		if (!isRecieversRegistred) {
        			activity.registerReceiver(onComplete, new IntentFilter(DownloadManager.ACTION_DOWNLOAD_COMPLETE));
        			activity.registerReceiver(onNotificationClick, new IntentFilter(DownloadManager.ACTION_NOTIFICATION_CLICKED));
        			isRecieversRegistred = true;
        		}
            }
		});
	}
	
	public void downloadIssueContentRunnable(String url, String path, String name, long assetId, String secure, String magName, String downloadData) {
		if (activity == null) {
			return;
		}
		
		Uri uri = Uri.parse(url);

		/*Log.d("DownloadMaster", String.format("create request: \"%s\" \"%s\" \"%s\" \"%s\"",
				issueName, issueUrl, issueTitle, issueDesc));*/

		String absPath = targetDir.getAbsolutePath() + "/" + assetId + ".asset";
		Uri downloadPath = Uri.fromFile(new File(absPath));
		
		DownloadManager.Request req = new DownloadManager.Request(uri)
			.setAllowedNetworkTypes(DownloadManager.Request.NETWORK_WIFI | 
					DownloadManager.Request.NETWORK_MOBILE)
			.setAllowedOverRoaming(false)
			.setTitle(name)
			.setDescription(magName)
			.setMimeType("application/octet-stream");

		req.addRequestHeader("X-Stappler", "IssueRequest");
		req.addRequestHeader("X-Stappler-Issue", secure);
		
		if (hasExternal) {
			req.setDestinationUri(downloadPath);
		}

		//Log.d("DownloadMaster", String.format("start download with path: %s", path.toString()));
		
		long downloadId = downloadManager.enqueue(req);
		
		JSONObject data = null;
		try {
			data = new JSONObject()
				.put("path", path)
				.put("name", name)
				.put("url", url)
				.put("id", assetId)
				.put("data", downloadData);
		} catch (JSONException e) {
	    	Log.w("DownloadMaster", e.getLocalizedMessage());
		}
		
		if (data != null) {
			SharedPreferences.Editor editor = activity.getPreferences(Context.MODE_PRIVATE).edit();
			editor.putString(String.valueOf(downloadId), data.toString());
			editor.commit();
		}
		
		Info download = new Info(downloadId, assetId, name, absPath, path, downloadData);
		downloadObserver.addDownload(download);
		nativeDownloadStarted(assetId);
	}

	public void downloadIssueContent(String url, String path, String name, long id, String secure, String magName, String data) {
		if (activity == null) {
			return;
		}
		
		final String f_url = url;
		final String f_path = path;
		final String f_name = name;
		final long f_id = id;
		final String f_secure = secure;
		final String f_magName = magName;
		final String f_data = data;
		
		activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
            	downloadIssueContentRunnable(f_url, f_path, f_name, f_id, f_secure, f_magName, f_data);
            }
		});
	}

	public void downloadModified(Info download) {
		Cursor c = downloadManager.query(new DownloadManager.Query().setFilterById(download.getDownloadId()));
      	if (c == null) {
      		return;
      	}
		if (c.moveToFirst()) {
			long dAll = c.getLong(c.getColumnIndex(DownloadManager.COLUMN_TOTAL_SIZE_BYTES));
			long dBytes = c.getLong(c.getColumnIndex(DownloadManager.COLUMN_BYTES_DOWNLOADED_SO_FAR));
			float progress = (float)dBytes / (float)dAll;

			if (dAll == dBytes) {
				nativeDownloadProgress(download.getAssetId(), 1);
			} else if (progress - download.getProgress() > 0.01) {
				download.setProgress(progress);
				nativeDownloadProgress(download.getAssetId(), progress);
			}
		}
		c.close();
	}

	public void downloadRemoved(Info download) {
		removeDownload(download);
		nativeDownloadCompleted(download.getAssetId(), false, download.getData());
		Log.d("DownloadManager", "downloadRemoved");
	}

	public void downloadOpened(Info download) {
		
	}

	public void downloadClosed(Info download) {
		
	}

	native public boolean nativeDownloadRestore(String url, String path, String name, long id, String data);
	native public boolean nativeDownloadStarted(long id);
	native public boolean nativeDownloadCompleted(long id, boolean successful, String data);
	native public boolean nativeDownloadProgress(long id, float progress);
	native public boolean nativeDownloadClicked(long id);
	native public void nativeAllDownloadsRestored();
	
	public boolean fileMove(String from, String to) {
    	boolean ret = true;
	    try {
	    	(new File(to.substring(0, to.lastIndexOf("/")))).mkdirs();
    	
	    	File sourceFile = new File(from);
	    	File destFile = new File(to); destFile.delete();
		
	    	if (sourceFile.renameTo(destFile)) {
	    		return true;
	    	}
		
	    	// or make it HARD WAY!

			FileInputStream sourceStream = null;
			FileOutputStream destStream = null;
			
		    FileChannel sourceChannel = null;
		    FileChannel destChannel = null;
		    
		    try {
		    	sourceStream = new FileInputStream(sourceFile);
		    	destStream = new FileOutputStream(destFile);
		    	
		    	sourceChannel = sourceStream.getChannel();
		    	destChannel = destStream.getChannel();

		        // previous code: destination.transferFrom(source, 0, source.size());
		        // to avoid infinite loops, should be:
		        long count = 0;
		        long size = sourceChannel.size();

		    	//Log.w("DownloadMaster", String.format("File size %d", sourceChannel.size()));
		        while(count < size) {
		        	count += destChannel.transferFrom(sourceChannel, count, size - count);
			    	//Log.w("DownloadMaster", String.format("Transfer %d", count));
		        }
		    } finally {
		        if (sourceChannel != null) {
		        	sourceChannel.close();
		        }
		        if (destChannel != null) {
		        	destChannel.close();
		        }
		        if (sourceStream != null) {
		        	sourceStream.close();
		        }
		        if (destStream != null) {
		        	destStream.close();
		        }
		    }
	    } catch (Exception e) {
	    	Log.w("DownloadMaster", "Exception on move file");
	    	Log.w("DownloadMaster", "From: " + from);
	    	Log.w("DownloadMaster", "To: " + to);
	    	Log.w("DownloadMaster", e.getLocalizedMessage());
	    	ret = false;
	    }
	    return ret;
	}
}
