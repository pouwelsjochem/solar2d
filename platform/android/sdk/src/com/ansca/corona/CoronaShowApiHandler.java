//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

package com.ansca.corona;

import android.app.Activity;

/** The interface has all the funcations that are activity specific and aren't implemented by default. */
class CoronaShowApiHandler implements com.ansca.corona.listeners.CoronaShowApiListener{
	private CoronaActivity fActivity;
	private CoronaRuntime fCoronaRuntime;

	public CoronaShowApiHandler(CoronaActivity activity, CoronaRuntime runtime) {
		fActivity = activity;
		fCoronaRuntime = runtime;
	}

	@Override
	public void showSendMailWindowUsing(MailSettings mailSettings) {
		if (fActivity == null) {
			return;
		}
		fActivity.showSendMailWindowUsing(mailSettings);
	}

	@Override
	public void showSendSmsWindowUsing(SmsSettings smsSettings) {
		if (fActivity == null) {
			return;
		}
		fActivity.showSendSmsWindowUsing(smsSettings);
	}

	@Override
	public boolean showAppStoreWindow(java.util.HashMap<String, Object> settings) {
		// Keep a local reference to it in case it is nulled out on another thread.
		CoronaActivity activity = fActivity;
		if (activity == null) {
			return false;
		}

		// Fetch the store this app is targeting.
		String storeName = com.ansca.corona.purchasing.StoreServices.getTargetedAppStoreName();

		// If this app is not targeting an app store (which is common for Corona Enterprise builds),
		// then attempt to determine which store this app was purchased and installed from.
		// Note: This check will return store name "none" if the app was installed by hand.
		if (storeName.equals(com.ansca.corona.purchasing.StoreName.NONE)) {
			storeName = com.ansca.corona.purchasing.StoreServices.getStoreApplicationWasPurchasedFrom();
		}

		// If the targeted app store is still not known, then just pick one.
		if (storeName.equals(com.ansca.corona.purchasing.StoreName.NONE)) {
			if (settings != null) {
				String[] availableStores = com.ansca.corona.purchasing.StoreServices.getAvailableAppStoreNames();
				Object collection = settings.get("supportedAndroidStores");
				if ((availableStores != null) && (collection instanceof java.util.HashMap<?,?>)) {
					for (Object nextObject : ((java.util.HashMap<Object, Object>)collection).values()) {
						if (nextObject instanceof String) {
							String supportedStoreName = (String)nextObject;
							if (java.util.Arrays.binarySearch(availableStores, supportedStoreName) >= 0) {
								storeName = supportedStoreName;
								break;
							}
						}
					}
				}
			}
		}

		// Fetch the application's package name.
		String packageName = null;
		if (settings != null) {
			// Get the package name from settings, if provided.
			// This comes in handy if the app wants to advertise other apps in the app store.
			Object value = settings.get("androidAppPackageName");
			if (value instanceof String) {
				packageName = ((String)value).trim();
			}
		}
		if ((packageName == null) || (packageName.length() <= 0)) {
			// Package name was not provided in settings. Use this application's package name by default.
			packageName = activity.getPackageName();
		}

		// Display the requested window.
		if (storeName.equals(com.ansca.corona.purchasing.StoreName.GOOGLE)) {
			return fCoronaRuntime.getController().openUrl("market://details?id=" + packageName);
		}
		else if (storeName.equals(com.ansca.corona.purchasing.StoreName.AMAZON)) {
			return fCoronaRuntime.getController().openUrl("http://www.amazon.com/gp/mas/dl/android?p=" + packageName);
		}
		else if (storeName.equals(com.ansca.corona.purchasing.StoreName.SAMSUNG)) {
			return fCoronaRuntime.getController().openUrl("samsungapps://ProductDetail/" + packageName);
		}

		return false;
	}

	@Override
	public void showRequestPermissionsWindowUsing(com.ansca.corona.permissions.PermissionsSettings permissionsSettings) {
		if (fActivity == null) {
			return;
		}
		fActivity.showRequestPermissionsWindowUsing(permissionsSettings);
	}
}
