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
	device = "android-tablet",
	deviceWidth = 1280,
	deviceHeight = 800,
	androidDisplayApproximateDpi = 240, -- hdpi
	displayManufacturer = "Amazon",
	displayName = "KFTT",
	windowTitleBarName = 'Kindle Fire HD 7" (2012)',
}
simulator.defaultFontSize = 18.0 * (simulator.androidDisplayApproximateDpi / 160)
