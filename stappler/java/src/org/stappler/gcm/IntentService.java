package org.stappler.gcm;

import org.stappler.Activity;

import com.google.android.gms.gcm.GoogleCloudMessaging;

import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.support.v4.app.NotificationCompat;
import android.util.Log;

public class IntentService extends android.app.IntentService {
    public static final int NOTIFICATION_ID = 1;
    private NotificationManager mNotificationManager;
    NotificationCompat.Builder builder;

    public IntentService() {
        super("GcmIntentService");
    }
    public static final String TAG = "Stappler-GCM";

    @Override
    protected void onHandleIntent(Intent intent) {
        Bundle extras = intent.getExtras();
        GoogleCloudMessaging gcm = GoogleCloudMessaging.getInstance(this);
        String messageType = gcm.getMessageType(intent);

        if (!extras.isEmpty()) {
            if (GoogleCloudMessaging.MESSAGE_TYPE_MESSAGE.equals(messageType)) {
                sendNotification(extras.getString("header"), extras.getString("message"));
            }
        }
        BroadcastReceiver.completeWakefulIntent(intent);
    }

    private void sendNotification(String header, String message) {
    	if (header == null || message == null || header.isEmpty() || message.isEmpty()) {
    		return;
    	}

    	Context context = getApplicationContext();

        mNotificationManager = (NotificationManager)
                this.getSystemService(Context.NOTIFICATION_SERVICE);

        PendingIntent contentIntent = PendingIntent.getActivity(this, 0,
                new Intent(this, Activity.class), 0);

        NotificationCompat.Builder mBuilder =
                new NotificationCompat.Builder(this)
        .setLargeIcon(IconResource.getIconBitmap(context))
        .setSmallIcon(IconResource.getIconId(context))
        .setContentTitle(header)
        .setStyle(new NotificationCompat.BigTextStyle()
        .bigText(message))
        .setContentText(message);

        mBuilder.setContentIntent(contentIntent);
        mNotificationManager.notify(NOTIFICATION_ID, mBuilder.build());
        
        try {
        	onRemoteNotification();
    	} catch (java.lang.UnsatisfiedLinkError e) {
    		Log.d("SP.NetworkReceiver", "Library is not loaded");
    	} catch (Exception e) {
    		Log.d("SP.NetworkReceiver", "Library is not loaded");
    	}
    }
    
    protected native void onRemoteNotification();
}
