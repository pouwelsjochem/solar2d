//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

package com.ansca.corona;

import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.util.Map;
import java.util.concurrent.TimeUnit;

import android.content.Context;
import android.os.Looper;

import com.google.android.gms.auth.blockstore.Blockstore;
import com.google.android.gms.auth.blockstore.BlockstoreClient;
import com.google.android.gms.auth.blockstore.RetrieveBytesRequest;
import com.google.android.gms.auth.blockstore.RetrieveBytesResponse;
import com.google.android.gms.auth.blockstore.StoreBytesData;
import com.google.android.gms.tasks.OnCompleteListener;
import com.google.android.gms.tasks.Task;
import com.google.android.gms.tasks.Tasks;

import org.json.JSONObject;

/**
 * Persists Corona's device identifier outside of the application's data directory.
 * <p>
 * Block Store can transfer its data to another device. The stored ANDROID_ID is therefore
 * validated before the persisted device identifier is reused.
 */
final class BlockStoreDeviceIdentifier implements Controller.DeviceIdentifierStore {
	private static final String BLOCK_STORE_KEY = "com.ansca.corona.deviceIdentifier";
	private static final String JSON_DEVICE_IDENTIFIER_KEY = "deviceIdentifier";
	private static final String JSON_OS_IDENTIFIER_KEY = "osIdentifier";
	private static final String JSON_VERSION_KEY = "version";
	private static final int JSON_VERSION = 1;
	private static final long WAIT_TIMEOUT_SECONDS = 5;

	private final BlockstoreClient fClient;
	private final Task<RetrieveBytesResponse> fRetrieveTask;
	private PersistedIdentifier fPersistedIdentifier;
	private boolean fRetrieveProcessed;
	private boolean fStoreInProgress;

	BlockStoreDeviceIdentifier(Context context) {
		fClient = Blockstore.getClient(context);
		RetrieveBytesRequest request = new RetrieveBytesRequest.Builder()
				.setKeys(Arrays.asList(BLOCK_STORE_KEY))
				.build();
		fRetrieveTask = fClient.retrieveBytes(request);
	}

	@Override
	public synchronized String getDeviceIdentifier(
			String currentDeviceIdentifier, String currentOSIdentifier) {
		if (!IsValid(currentDeviceIdentifier) || !IsValid(currentOSIdentifier)) {
			return currentDeviceIdentifier;
		}

		if ((fPersistedIdentifier != null) &&
				currentOSIdentifier.equals(fPersistedIdentifier.fOSIdentifier)) {
			return fPersistedIdentifier.fDeviceIdentifier;
		}

		if (!fRetrieveProcessed) {
			RetrieveBytesResponse response = GetRetrieveResponse();
			if (response == null) {
				return currentDeviceIdentifier;
			}

			fRetrieveProcessed = true;
			fPersistedIdentifier = Decode(response);
			if ((fPersistedIdentifier != null) &&
					currentOSIdentifier.equals(fPersistedIdentifier.fOSIdentifier)) {
				return fPersistedIdentifier.fDeviceIdentifier;
			}
		}

		if (!fStoreInProgress) {
			Store(new PersistedIdentifier(currentDeviceIdentifier, currentOSIdentifier));
		}

		return currentDeviceIdentifier;
	}

	private RetrieveBytesResponse GetRetrieveResponse() {
		try {
			if (!fRetrieveTask.isComplete()) {
				// system.getInfo() normally runs on Corona's runtime thread. Never block Android's
				// main thread if a Java caller requests the identifier before Block Store is ready.
				if (Looper.myLooper() == Looper.getMainLooper()) {
					return null;
				}
				return Tasks.await(fRetrieveTask, WAIT_TIMEOUT_SECONDS, TimeUnit.SECONDS);
			}
			if (fRetrieveTask.isSuccessful()) {
				return fRetrieveTask.getResult();
			}
		}
		catch (InterruptedException ex) {
			Thread.currentThread().interrupt();
		}
		catch (Exception ex) { }

		return null;
	}

	private static PersistedIdentifier Decode(RetrieveBytesResponse response) {
		if (response == null) {
			return null;
		}

		try {
			Map<String, RetrieveBytesResponse.BlockstoreData> dataMap =
					response.getBlockstoreDataMap();
			if (dataMap == null) {
				return null;
			}

			RetrieveBytesResponse.BlockstoreData data = dataMap.get(BLOCK_STORE_KEY);
			if ((data == null) || (data.getBytes() == null)) {
				return null;
			}

			JSONObject json =
					new JSONObject(new String(data.getBytes(), StandardCharsets.UTF_8));
			if (json.optInt(JSON_VERSION_KEY, 0) != JSON_VERSION) {
				return null;
			}

			String deviceIdentifier = json.optString(JSON_DEVICE_IDENTIFIER_KEY, null);
			String osIdentifier = json.optString(JSON_OS_IDENTIFIER_KEY, null);
			if (!IsValid(deviceIdentifier) || !IsValid(osIdentifier)) {
				return null;
			}

			return new PersistedIdentifier(deviceIdentifier, osIdentifier);
		}
		catch (Exception ex) { }

		return null;
	}

	private void Store(final PersistedIdentifier identifier) {
		try {
			JSONObject json = new JSONObject();
			json.put(JSON_VERSION_KEY, JSON_VERSION);
			json.put(JSON_DEVICE_IDENTIFIER_KEY, identifier.fDeviceIdentifier);
			json.put(JSON_OS_IDENTIFIER_KEY, identifier.fOSIdentifier);

			StoreBytesData request = new StoreBytesData.Builder()
					.setKey(BLOCK_STORE_KEY)
					.setBytes(json.toString().getBytes(StandardCharsets.UTF_8))
					.setShouldBackupToCloud(false)
					.build();
			fStoreInProgress = true;
			fClient.storeBytes(request).addOnCompleteListener(new OnCompleteListener<Integer>() {
				@Override
				public void onComplete(Task<Integer> task) {
					synchronized (BlockStoreDeviceIdentifier.this) {
						fStoreInProgress = false;
						if (task.isSuccessful()) {
							fPersistedIdentifier = identifier;
						}
					}
				}
			});
		}
		catch (Exception ex) {
			fStoreInProgress = false;
		}
	}

	private static boolean IsValid(String value) {
		return (value != null) && (value.length() > 0);
	}

	private static class PersistedIdentifier {
		final String fDeviceIdentifier;
		final String fOSIdentifier;

		PersistedIdentifier(String deviceIdentifier, String osIdentifier) {
			fDeviceIdentifier = deviceIdentifier;
			fOSIdentifier = osIdentifier;
		}
	}
}
