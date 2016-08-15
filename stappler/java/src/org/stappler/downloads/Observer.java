package org.stappler.downloads;

import java.io.File;
import java.util.HashMap;
import java.util.Map;

import android.os.FileObserver;

public class Observer extends FileObserver {
	public HashMap<String, Info> downloads;
	public Manager manager = null;
	public String dir = null;
	
    public static final String LOG_TAG = Observer.class.getSimpleName();

    private static final int flags =
            FileObserver.CLOSE_WRITE
            | FileObserver.OPEN
            | FileObserver.MODIFY
            | FileObserver.DELETE
            | FileObserver.MOVED_FROM;

    public Observer(Manager _manager, String path) {
        super(path, flags);
        manager = _manager;
        downloads = new HashMap<String, Info>();
        dir = path;
    }

	public void dispose() {
		stopWatching();
		manager = null;
	}
    
    @Override
    public void onEvent(int event, String path) {
        if (path == null || manager == null) {
            return;
        }
    	
        Info download = getDownloadByPath(path);
        if (download == null) {
        	if (event == FileObserver.CLOSE_WRITE) {
        		File file = new File(dir, path);
        		file.delete();
        	}
        	return;
        }
        
        switch (event) {
        case FileObserver.CLOSE_WRITE:
            // Download complete, or paused when wifi is disconnected. Possibly reported more than once in a row.
            // Useful for noticing when a download has been paused. For completions, register a receiver for 
            // DownloadManager.ACTION_DOWNLOAD_COMPLETE.
        	manager.downloadClosed(download);
            break;
        case FileObserver.OPEN:
            // Called for both read and write modes.
            // Useful for noticing a download has been started or resumed.
        	manager.downloadOpened(download);
            break;
        case FileObserver.DELETE:
        case FileObserver.MOVED_FROM:
            // These might come in handy for obvious reasons.
        	manager.downloadRemoved(download);
            break;
        case FileObserver.MODIFY:
            // Called very frequently while a download is ongoing (~1 per ms).
            // This could be used to trigger a progress update, but that should probably be done less often than this.
        	if (download.isQueryAllowed()) {
        		manager.downloadModified(download);
        	}
        	break;
        }
    }
    
    public void addDownload(Info download) {
    	String path = download.getDownloadPath();
    	int index = path.lastIndexOf('/');
    	if (index >= 0) {
    		path = path.substring(index + 1);
    	}
    	
    	downloads.put(path, download);
    	startWatching();
    }
    
    public void removeDownload(Info download) {
    	String path = download.getDownloadPath();
    	int index = path.lastIndexOf('/');
    	if (index >= 0) {
    		path = path.substring(index + 1);
    	}
    	
    	downloads.remove(path);
    	if (downloads.size() == 0) {
    		stopWatching();
    	}
    }
    
    public void removeDownloadById(long id) {
    	Info download = getDownloadById(id);
    	if (download != null) {
    		removeDownload(download);
    	}
    }

    public Info getDownloadById(long id) {
    	for (Map.Entry<String, Info> entry : downloads.entrySet()) {
    		if (entry.getValue().getDownloadId() == id) {
    			return entry.getValue();
    		}
    	}
    	return null;
    }
    
    public Info getDownloadByName(String name) {
    	for (Map.Entry<String, Info> entry : downloads.entrySet()) {
    		if (entry.getValue().getIssueName().equals(name)) {
    			return entry.getValue();
    		}
    	}
    	return null;
    }
    
    public Info getDownloadByPath(String path) {
    	Info ret = downloads.get(path);
    	if (ret == null) {
        	for (Map.Entry<String, Info> entry : downloads.entrySet()) {
        		String s = String.valueOf(entry.getValue().getAssetId());
        		if (path.startsWith(s) == true) {
        			return entry.getValue();
        		}
        	}
    	}
    	return ret;
    }
}
