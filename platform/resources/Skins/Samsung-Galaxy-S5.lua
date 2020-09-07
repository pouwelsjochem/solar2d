------------------------------------------------------------------------------
--
-- This file is part of the Corona game engine.
-- For overview and more information on licensing please refer to README.md 
-- Home page: https://github.com/coronalabs/corona
-- Contact: support@coronalabs.com
--
------------------------------------------------------------------------------

simulator =
{
	device = "android-phone",
	deviceWidth = 1920,
	deviceHeight = 1080,
	androidDisplayApproximateDpi = 480, -- xxhdpi
	displayManufacturer = "samsung",
	displayName = "SM-G900S",
	hasAccelerometer = true,
	windowTitleBarName = "Samsung Galaxy S5",
}
simulator.defaultFontSize = 18.0 * (simulator.androidDisplayApproximateDpi / 160)
