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

-- These helper functions are called by CoronaBuilder/Rtt_AppPackagerFactory.cpp to determine the
-- correct target SDK version for iOS and tvOS builds
function CoronaBuilderDetermineTargetiOSVersion( params, bundleDir, buildNum )
	local currentSDKsFile = bundleDir .. "/iOS-SDKs.json"
	return CoronaBuilderDetermineTargetSDKVersion( "iphoneos", "ios", currentSDKsFile, params, buildNum )
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

function CoronaBuilderDetermineTargettvOSVersion( params, bundleDir, buildNum )
	local currentSDKsFile = bundleDir .. "/tvOS-SDKs.json"
	return CoronaBuilderDetermineTargetSDKVersion( "appletvos", "tvos", currentSDKsFile, params, buildNum )
end

function CoronaBuilderDetermineTargetSDKVersion( sdkname, platformName, currentSDKsFile, params, buildNum )

	local CoronaPListSupport = require("CoronaPListSupport")
	local captureCommandOutput = CoronaPListSupport.captureCommandOutput

	if not params then
		return false, "params cannot be nil in determineTargetSDKVersion"
	end
	if type(params) ~= "table" then
		return false, "params must be a table in determineTargetSDKVersion (not a "..type(params)..")"
	end

	-- Find currently active version of Xcode
	local xcodeSDKVersion = captureCommandOutput("/usr/bin/xcrun --sdk '"..sdkname.."' --show-sdk-version")
	print("Active "..sdkname.." SDK version: ", xcodeSDKVersion)

	local targetVersion = params['targetPlatformVersion']
	local SDKs, lineno, errorMsg = json.decodeFile(currentSDKsFile) 

	if errorMsg then
		return false, errorMsg
	end

	local coronaVersion = false
	local failMessage = "cannot find a compatible CoronaSDK "..buildNum.." build target for "..sdkname.." SDK "..xcodeSDKVersion
	for idx, sdkParams in ipairs(SDKs[platformName]) do
		-- print(idx, json.prettify(sdkParams))
		if sdkParams['coronaVersion'] == params['platformVersion'] then
			failMessage = sdkParams['failMessage']
		end
		if sdkParams['xcodeVersion'] == xcodeSDKVersion then
			coronaVersion = sdkParams['coronaVersion']
		end
	end

	if coronaVersion then
		if not params['platformVersion'] then
			params['platformVersion'] = coronaVersion

			return true, "'platformVersion' defaulted to "..coronaVersion
		elseif params['platformVersion'] == coronaVersion then
			return true, "requested 'platformVersion' of "..coronaVersion.." available"
		else
			return false, "can't build with requested 'platformVersion' of "..params['platformVersion'].." ("..failMessage..")"
		end
	else
		return false, failMessage
	end

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
