//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

package com.ansca.corona.listeners;

import com.ansca.corona.MailSettings;
import com.ansca.corona.permissions.PermissionsSettings;
import com.ansca.corona.SmsSettings;

/** The interface has the funcations that will show a new window/overlay */
public interface CoronaShowApiListener{
	/**
	 * Called from native.showPopup().  The lua script wants to send an email with the settings.
	 * @param mailSettings the settings the lua script passed made into an object.
	 */
	public void showSendMailWindowUsing(MailSettings mailSettings);

	/**
	 * Called from native.showPopup().  The lua script wants to send a sms with the settings.
	 * @param smsSettings the settings the lua script passed made into an object.
	 */
	public void showSendSmsWindowUsing(SmsSettings smsSettings);

	/**
	 * Called from native.showPopup().  The lua script wants to request permissions with the settings.
	 * @param permissionsSettings the settings the lua script passed made into an object.
	 */
	public void showRequestPermissionsWindowUsing(PermissionsSettings permissionsSettings);
}
