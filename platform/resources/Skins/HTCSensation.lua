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
	deviceWidth = 960,
	deviceHeight = 540,
	androidDisplayApproximateDpi = 240, -- hdpi
	displayManufacturer = "HTC",
	displayName = "Sensation",
	windowTitleBarName = "HTC Sensation",
}
simulator.defaultFontSize = 18.0 * (simulator.androidDisplayApproximateDpi / 160)
