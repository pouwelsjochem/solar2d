------------------------------------------------------------------------------
--
-- This file is part of the Corona game engine.
-- For overview and more information on licensing please refer to README.md 
-- Home page: https://github.com/coronalabs/corona
-- Contact: support@coronalabs.com
--
------------------------------------------------------------------------------

local json = require('json')
local builder = require('builder')

local function fileExists( path )
	local file = io.open( path, "rb" )
	if file then
		file:close()
		return true
	end
	return false
end

local function encodedSDKVersion( version )
	if type(version) ~= "string" or not version:match("^%d+%.%d+$") then
		return nil
	end
	return math.floor(tonumber(version) * 10000 + 0.5)
end

local function archiveSDKVersion( version )
	return string.format("%.1f", version / 10000)
end

local function usesSimulatorTemplate( params, simulatorTargets )
	return type(params.targetDevice) == "string" and simulatorTargets[params.targetDevice:lower()] == true
end

local function determineAppleMobileTargetVersion( params, bundleDir, deviceSDK, simulatorSDK, simulatorTargets )
	if type(params) ~= "table" then
		return false, "params must be a table when selecting an Apple mobile template"
	end

	local sdk = usesSimulatorTemplate(params, simulatorTargets) and simulatorSDK or deviceSDK
	local CoronaPListSupport = require("CoronaPListSupport")
	local activeVersion = CoronaPListSupport.captureCommandOutput("/usr/bin/xcrun --sdk '"..sdk.."' --show-sdk-version")
	local activeEncodedVersion = encodedSDKVersion(activeVersion)
	if not activeEncodedVersion then
		return false, "could not determine the active "..sdk.." SDK version"
	end

	local requestedVersion = params.platformVersion
	if requestedVersion ~= nil and type(requestedVersion) ~= "number" then
		return false, "'platformVersion' must be a number"
	end
	requestedVersion = requestedVersion or activeEncodedVersion

	if requestedVersion ~= activeEncodedVersion and not params.forceVersion then
		return false, "requested 'platformVersion' "..requestedVersion.." does not match active "..sdk.." SDK "..activeVersion
	end

	local customTemplate = params.customTemplate or ""
	if type(customTemplate) ~= "string" or (customTemplate ~= "" and not customTemplate:match("^%-%w[%w._-]*$")) then
		return false, "'customTemplate' must be empty or a filename suffix such as '-angle'"
	end

	local version = archiveSDKVersion(requestedVersion)
	local archiveName = sdk.."_"..version..customTemplate..".tar.bz"
	if not fileExists(bundleDir.."/iostemplate/"..archiveName) then
		return false, "CoronaBuilder does not contain template '"..archiveName.."'"
	end

	params.platformVersion = requestedVersion
	return true, "using bundled template '"..archiveName.."'"
end

-- These helper functions are called by CoronaBuilder/Rtt_AppPackagerFactory.cpp to select a
-- bundled template matching the active iOS or tvOS SDK.
function CoronaBuilderDetermineTargetiOSVersion( params, bundleDir )
	local simulatorTargets = { ["ios-simulator"] = true, ["iphone-simulator"] = true, ["ipad-simulator"] = true }
	return determineAppleMobileTargetVersion( params, bundleDir, "iphoneos", "iphonesimulator", simulatorTargets )
end

-- Determine we're running in a debug build
function isDebugBuild()
	local debugInfo = debug.getinfo(CoronaBuilderDetermineTargetiOSVersion)
	-- print("debugInfo: ", json.prettify(debugInfo))
	return (debugInfo ~= nil and debugInfo.short_src ~= "?")
end

function debug_print(...)
	if isDebugBuild() then
		print(...)
	end
end

function CoronaBuilderDetermineTargettvOSVersion( params, bundleDir )
	local simulatorTargets = { ["tvos-simulator"] = true, appletvsimulator = true }
	return determineAppleMobileTargetVersion( params, bundleDir, "appletvos", "appletvsimulator", simulatorTargets )
end

-- Called by CoronaBuilder/Rtt_BuildParams.cpp to load a JSON parameter file
function CoronaBuilderLoadJSONParams(path)

	debug_print("CoronaBuilderLoadJSONParams: "..tostring(path))
	return json.decodeFile( path )

end


--[[ local testJSONData = 
{
    "data": [
        {
            "bundles": null,
            "expires": "Never",
            "granted": 1481436000,
            "license_by_bundle": 0,
            "plugin_developer": "com.aaronsserver",
            "plugin_name": "plugin.awcolor",
            "status": 2,
            "title": "awcolor"
        },
        {
            "bundles": {
                "ios": "com.head-net.testgame",
                "android": "com.head-net.android.testgame"
            },
            "expires": "Never",
            "granted": 1503706792,
            "license_by_bundle": 1,
            "plugin_developer": "moti",
            "plugin_name": "test plugin-ignore",
            "status": 2,
            "title": "test plugin-ignore"
        },
        {
            "bundles": null,
            "expires": "Never",
            "granted": 1503708007,
            "license_by_bundle": 1,
            "plugin_developer": "prateek",
            "plugin_name": "testing",
            "status": 2,
            "title": "dfdsfds"
        },
    ],
    "status": "success"
}
]]

function CoronaBuilderDownloadFile(url, filename)

	debug_print("CoronaBuilderDownloadFile: ", tostring(url), tostring(filename))

	local headers = { } -- { ["Test-Header"] = "first value", second = "second value" }

	local result, errorMesg =  builder.download(url, filename, headers)

	return result, errorMesg
end
