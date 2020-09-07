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
	device = "android-tv",
	deviceWidth = 1280,
	deviceHeight = 720,
	androidDisplayApproximateDpi = 320, -- xhdpi
	displayManufacturer = "Amazon",
	displayName = "AFTB",
	windowTitleBarName = "Amazon Fire TV (Gen. 1)",

	safeScreenInsetTop = 64,
	safeScreenInsetLeft = 36,
	safeScreenInsetBottom = 64,
	safeScreenInsetRight = 36,
	safeLandscapeScreenInsetTop = 36,
	safeLandscapeScreenInsetLeft = 64,
	safeLandscapeScreenInsetBottom = 36,
	safeLandscapeScreenInsetRight = 64,
}
simulator.defaultFontSize = 18.0 * (simulator.androidDisplayApproximateDpi / 160)
