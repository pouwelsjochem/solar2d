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
	deviceWidth = 1920,
	deviceHeight = 1200,
	androidDisplayApproximateDpi = 240, -- hdpi
	displayManufacturer = "Amazon",
	displayName = "KFJWI",
	windowTitleBarName = 'Kindle Fire HD 8.9" (2012)',
}
simulator.defaultFontSize = 18.0 * (simulator.androidDisplayApproximateDpi / 160)
