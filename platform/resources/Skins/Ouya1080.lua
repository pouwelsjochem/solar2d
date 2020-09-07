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
	deviceWidth = 1920,
	deviceHeight = 1080,
	androidDisplayApproximateDpi = 320, -- xhdpi
	displayManufacturer = "OUYA",
	displayName = "OUYA Console",
	windowTitleBarName = "Ouya",

	safeScreenInsetTop = 96,
	safeScreenInsetLeft = 54,
	safeScreenInsetBottom = 96,
	safeScreenInsetRight = 54,
	safeLandscapeScreenInsetTop = 54,
	safeLandscapeScreenInsetLeft = 96,
	safeLandscapeScreenInsetBottom = 54,
	safeLandscapeScreenInsetRight = 96,
}
simulator.defaultFontSize = 18.0 * (simulator.androidDisplayApproximateDpi / 160)
