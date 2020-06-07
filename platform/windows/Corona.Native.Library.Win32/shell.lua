------------------------------------------------------------------------------
--
-- This file is part of the Corona game engine.
-- For overview and more information on licensing please refer to README.md 
-- Home page: https://github.com/coronalabs/corona
-- Contact: support@coronalabs.com
--
------------------------------------------------------------------------------

-- Platform dependent initialization for Windows

-- Fetch the launch arguments passed to this script.
local launchArguments = ...

-- Notify the Corona runtime that this "shell.lua" script is done doing its work and it's time to run the "main.lua".
launchArguments.onShellComplete(nil)
