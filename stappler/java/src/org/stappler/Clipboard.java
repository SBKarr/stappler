package org.stappler;

import android.app.Activity;
import android.content.ClipData;
import android.content.ClipDescription;
import android.content.ClipboardManager;
import android.content.Context;

public class Clipboard {
	private Activity _activity = null;
	private ClipboardManager _clipboard = null;
	
	public Clipboard(Activity activity) {
		_activity = activity;
	    _clipboard = (ClipboardManager)_activity.getSystemService(Context.CLIPBOARD_SERVICE);
	}
	
	public void dispose() {
		_activity = null;
		_clipboard = null;
	}

	public boolean isClipboardAvailable() {
		if (_clipboard == null) {
			return false;
		}
		ClipData data = _clipboard.getPrimaryClip();
		if (data == null) {
			return false;
		}
		
		String clipMime = data.getDescription().getMimeType(0);
		if (clipMime.equals(ClipDescription.MIMETYPE_TEXT_PLAIN)  || clipMime.equals(ClipDescription.MIMETYPE_TEXT_HTML)) {
			return true;
		}

		return false;
	}
	
	public void copyStringToClipboard(String str) {
		if (_clipboard != null) {
			_clipboard.setPrimaryClip(ClipData.newPlainText("Bookmark Data", str));
		}
	}
	
	public String getStringFromClipboard() {
		if (_activity != null && _clipboard != null) {
			ClipData data = _clipboard.getPrimaryClip();
			if (data == null || data.getItemCount() < 1) {
				return null;
			}
			
			return data.getItemAt(0).coerceToText(_activity).toString();
		} else {
			return "";
		}
	}
}
