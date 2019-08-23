package org.stappler.gcm;

import com.google.firebase.iid.FirebaseInstanceId;
import com.google.firebase.iid.FirebaseInstanceIdService;

public class IdService extends FirebaseInstanceIdService {
    @Override
    public void onTokenRefresh() {
        // Get updated InstanceID token.
        String refreshedToken = FirebaseInstanceId.getInstance().getToken();
        // TODO: Implement this method to send any registration to your app's servers.
        setDeviceToken(refreshedToken);
    }

    private native void setDeviceToken(String value);
}