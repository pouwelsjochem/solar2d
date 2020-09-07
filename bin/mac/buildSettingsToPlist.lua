------------------------------------------------------------------------------
--
-- This file is part of the Corona game engine.
-- For overview and more information on licensing please refer to README.md 
-- Home page: https://github.com/coronalabs/corona
-- Contact: support@coronalabs.com
--
------------------------------------------------------------------------------

---- Amend an app's Info.plist with CoronaSDK specific items.

-- add $pwd/../../shared/resource to lua module lookup path
package.path = package.path .. ";" .. arg[0]:match("(.+)/") .. "/../../shared/resource/?.lua"

local json = require "json"

local srcAssets = arg[1]
local appBundleFile = arg[2]
local deviceType = arg[3] or "iphone"

local function osExecute(...)
	print("osExecute: ".. ...)
	return os.execute(...)
end

-- Double quote a string escaping backslashes and any double quotes
local function quoteString( str )
	str = str:gsub('\\', '\\\\')
	str = str:gsub('"', '\\"')
	return "\"" .. str .. "\""
end

-- defaults
local targetDevice = nil -- default is in CoronaPlistSupport

-- Get the current build id from the environment (if set)
local corona_build_id = os.getenv("CORONA_BUILD_ID")

-- init options
local options = {
	appBundleFile = quoteString( appBundleFile ),
	dstDir = dstDir,
	bundleversion = bundleversion,
	signingIdentity = signingIdentity,
	sdkRoot = sdkRoot,
	targetDevice = targetDevice,
	targetPlatform = "iOS",
	verbose = verbose,
	corona_build_id = corona_build_id,
}

local function fileExists( filename )
	local f = io.open( filename, "r" )
	if ( f ) then
		io.close( f )
	end
	return ( nil ~= f )
end

local customSettingsFile = srcAssets .. "/build.settings"
if ( fileExists( customSettingsFile ) ) then
	local customSettings, msg = loadfile( customSettingsFile )
	if ( customSettings ) then
		local status, msg = pcall( customSettings )
		if status then
			print( "Using additional build settings from: " .. customSettingsFile )
			options.settings = _G.settings
		else
			print( "Error: Errors found in build.settings file:" )
			print( "\t".. msg ) 
			os.exit(1)
		end
	else
		print( "Error: Could not load build.settings file:" )
		print( "\t".. msg )
		os.exit(1)
	end
end

-- define modifyPlist()
local CoronaPListSupport = require("CoronaPListSupport")
CoronaPListSupport.modifyPlist( options )