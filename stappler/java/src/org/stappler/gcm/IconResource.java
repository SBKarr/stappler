package org.stappler.gcm;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Matrix;
import android.util.DisplayMetrics;

public class IconResource {
	static int iconId = 0;

	static public int getIconId(Context context) {
		if (iconId == 0) {
			iconId = context.getResources().getIdentifier("icon", "drawable", context.getPackageName());
		}
		return iconId;
	}
	
	static public Bitmap getIconBitmap(Context context) {
		int id = getIconId(context);
    	int iconSize = dpToPx(context, 64);
    	
    	Bitmap bm = BitmapFactory.decodeResource(context.getResources(), id);
    	return getResizedBitmap(bm , iconSize, iconSize);
	}
	
    static private int dpToPx(Context context, int dp) {
        DisplayMetrics displayMetrics = context.getResources().getDisplayMetrics();
        int px = Math.round(dp * (displayMetrics.xdpi / DisplayMetrics.DENSITY_DEFAULT));       
        return px;
    }

	static private Bitmap getResizedBitmap(Bitmap bm, int newHeight, int newWidth) {
	    int width = bm.getWidth();
	    int height = bm.getHeight();
	    float scaleWidth = ((float) newWidth) / width;
	    float scaleHeight = ((float) newHeight) / height;
	    
	    Matrix matrix = new Matrix();
	    matrix.postScale(scaleWidth, scaleHeight);
	
	    Bitmap resizedBitmap = Bitmap.createBitmap(bm, 0, 0, width, height, matrix, false);
	    return resizedBitmap;
	}
}
