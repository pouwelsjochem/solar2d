------------------------------------------------------------------------------
--
-- This file is part of the Corona game engine.
-- For overview and more information on licensing please refer to README.md 
-- Home page: https://github.com/coronalabs/corona
-- Contact: support@coronalabs.com
--
------------------------------------------------------------------------------

-- fake out some things commonly found in config.lua
system = {}
system.getInfo = function(...) return ""; end
display = {}
display.pixelHeight = 480
display.pixelWidth = 320

-- These are items we recognize but cannot validate the contents of (generally because they're arbitrary)
stoplist = {"shaderPrecision"}

application = {
	isTransparent = false,
	backend = "gl",

	content = {
		minContentWidth = 1,
		minContentHeight = 1,
		maxContentWidth = 1,
		maxContentHeight = 1,
		fps = 60,
		audioPlayFrequency = 1,
		shaderPrecision = "",
	},

	license = {
		google = {
			key = "",
			policy = "serverManaged",
			mapsKey = "",
		}
	},

	notification = {
		iphone = {
			types = {
				"badge", "sound", "alert"
			}
		},

        google =
        {
            projectNumber = "",
        },
	},

	steamworks =
	{
		appId = "",
	},
}
