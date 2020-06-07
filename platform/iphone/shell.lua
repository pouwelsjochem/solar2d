------------------------------------------------------------------------------
--
-- This file is part of the Corona game engine.
-- For overview and more information on licensing please refer to README.md 
-- Home page: https://github.com/coronalabs/corona
-- Contact: support@coronalabs.com
--
------------------------------------------------------------------------------


local params = ...

local onShellComplete = params.onShellComplete
local onShellCompleteCalled = false

-- Arrange to call onShellComplete() only once
local function callOnShellComplete(param)
	if not onShellCompleteCalled then
		onShellCompleteCalled = true
		onShellComplete(param)
	end
end

-- Tell the runtime that shell.lua has completed
callOnShellComplete(nil)
