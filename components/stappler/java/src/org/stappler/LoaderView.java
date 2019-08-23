package org.stappler;

import android.content.Context;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.util.DisplayMetrics;
import android.view.Gravity;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.ProgressBar;
import android.widget.RelativeLayout;
import android.widget.TextView;

public class LoaderView extends FrameLayout {

	ImageView imageView = null;
	RelativeLayout progressLayout = null;
	
	public LoaderView(Context ctx) {
		super(ctx);

        ViewGroup.LayoutParams image_layout_params =
            new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
                                       ViewGroup.LayoutParams.MATCH_PARENT);

        imageView = new ImageView(ctx);
        imageView.setLayoutParams(image_layout_params);
        imageView.setAdjustViewBounds(true);

		addView(imageView);
		
		progressLayout = new RelativeLayout(ctx);
		addView(progressLayout);
	}

	protected void onLayout (boolean changed, int left, int top, int right, int bottom) {
		super.onLayout(changed, left, top, right, bottom);

        Resources res = this.getResources();
        DisplayMetrics metrics = res.getDisplayMetrics();
        
		int imageId;
		if (res.getConfiguration().orientation == Configuration.ORIENTATION_LANDSCAPE) {
			imageId = res.getIdentifier("default_landscape", "drawable", getContext().getPackageName());
		} else {
			imageId = res.getIdentifier("default_portrait", "drawable", getContext().getPackageName());
		}

		imageView.setImageResource(imageId);
		imageView.setScaleType(ImageView.ScaleType.CENTER_CROP);

		int width = getWidth();
		int height = getHeight();

		int vSizeWidth = (int)(metrics.density * 320);
		int vSizeHeight = (int)(metrics.density * 100);
		
		progressLayout.setPadding((width - vSizeWidth) / 2, (int)((height - vSizeHeight) * 0.9),
				(width - vSizeWidth) / 2, (int)((height - vSizeHeight) * 0.1));
	}
	
	protected void onSizeChanged (int w, int h, int oldw, int oldh) {
		super.onSizeChanged(w, h, oldw, oldh);
	}
	
}
