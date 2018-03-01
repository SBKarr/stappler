package org.stappler.gcm;

import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.support.v4.app.NotificationCompat;
import android.util.Log;

import com.google.firebase.messaging.FirebaseMessagingService;
import com.google.firebase.messaging.RemoteMessage;

import org.stappler.Activity;

import java.util.Map;

public class MessagingService extends FirebaseMessagingService {
    public static final String TAG = "Stappler-GCM";
    public static final int NOTIFICATION_ID = 1;
    private NotificationManager mNotificationManager;
    NotificationCompat.Builder builder;

    @Override
    public void onMessageReceived(RemoteMessage remoteMessage) {
        // ...

        // TODO(developer): Handle FCM messages here.
        // Not getting messages here? See why this may be: https://goo.gl/39bRNJ
        Log.d(TAG, "From: " + remoteMessage.getFrom());

        // Check if message contains a notification payload.
        if (remoteMessage.getNotification() != null) {
            sendNotification(remoteMessage.getNotification().getTitle(), remoteMessage.getNotification().getBody());
        } else {
            try {
                Map<String, String> data = remoteMessage.getData();
                String title = data.get("title");
                String alert = data.get("alert");

                sendNotification(title, alert);
            } catch (Exception e) {

            }
        }
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
