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
	deviceWidth = 1280,
	deviceHeight = 720,
	androidDisplayApproximateDpi = 320, -- xhdpi
	displayManufacturer = "samsung",
	displayName = "GT-I9300",
	windowTitleBarName = "Samsung Galaxy S3",
}
simulator.defaultFontSize = 18.0 * (simulator.androidDisplayApproximateDpi / 160)
