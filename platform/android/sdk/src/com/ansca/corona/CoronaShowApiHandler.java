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
	public void showRequestPermissionsWindowUsing(com.ansca.corona.permissions.PermissionsSettings permissionsSettings) {
		if (fActivity == null) {
			return;
		}
		fActivity.showRequestPermissionsWindowUsing(permissionsSettings);
	}
}
