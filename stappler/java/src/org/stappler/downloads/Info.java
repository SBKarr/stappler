package org.stappler.downloads;

public class Info {
	private String issueName;
	private String downloadPath;
	private String destPath;
	private String data;
	private long assetId;
	private long downloadId;
	private long mtime;
	
	private float progress;
	
	public Info (long dId, long aId, String name, String downPath, String dstPath, String d) {
		issueName = name;
		downloadPath = downPath;
		destPath = dstPath;
		downloadId = dId;
		assetId = aId;
		progress = 0;
		mtime = System.currentTimeMillis();
		data = d;
	}
	
	public String getIssueName() {
		return issueName;
	}

	public String getDownloadPath() {
		return downloadPath;
	}

	public String getDestPath() {
		return destPath;
	}
	
	public String getData() {
		return data;
	}

	public long getAssetId() {
		return assetId;
	}
	
	public long getDownloadId() {
		return downloadId;
	}
	
	public void setProgress(float value) {
		mtime = System.currentTimeMillis();
		progress = value;
	}
	
	public float getProgress() {
		return progress;
	}
	
	public boolean isQueryAllowed() {
		if ((System.currentTimeMillis() - mtime) > 750) {
			return true;
		}
		return false;
	}
}
