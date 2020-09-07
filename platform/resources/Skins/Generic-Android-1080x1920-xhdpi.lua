------------------------------------------------------------------------------
--
-- This file is part of the Corona game engine.
-- For overview and more information on licensing please refer to README.md 
-- Home page: https://github.com/coronalabs/corona
-- Contact: support@coronalabs.com
--
------------------------------------------------------------------------------

-- Based on the Sony Xperia C4, Xperia Z Ultra, and Xperia Z3 Tablet
-- https://material.io/devices/
simulator =
{
	device = "borderless-android-device",
	deviceWidth = 1920,
	deviceHeight = 1080,
	androidDisplayApproximateDpi = 320, -- xhdpi
	displayManufacturer = "Corona Labs Inc.",
	displayName = "GenericAndroidDevice",
	windowTitleBarName = "Android (xhdpi)",
}
simulator.defaultFontSize = 18.0 * (simulator.androidDisplayApproximateDpi / 160)
