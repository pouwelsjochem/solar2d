------------------------------------------------------------------------------
--
-- This file is part of the Solar2D game engine.
-- For overview and more information on licensing please refer to README.md 
-- Home page: https://github.com/coronalabs/corona
-- Contact: support@Solar2D.com
--
-- iOS-specific hooks for AppleMobilePackageApp.lua.
--
------------------------------------------------------------------------------

local json = require("json")

local M = {}

function M.validateSettings(options)
	local settings = options.settings.ios
	if not settings.plist then
		print("WARNING: missing ios / ios.plist section in build.settings")
	elseif settings.plist.UILaunchStoryboardName == nil and settings.plist.UILaunchImages == nil then
		print("WARNING: iOS builds require ios.plist.UILaunchStoryboardName or ios.plist.UILaunchImages in build.settings")
	end
end

function M.compileLegacyAssets()
	return nil
end

function M.integratePlugins(options, context)
	local appPath = context.makepath(options.dstDir, options.dstFile .. ".app")
	local buildDirectory = context.makepath(appPath, ".build")
	local libtemplateDirectory = context.makepath(buildDirectory, "libtemplate")
	local builderConstructor = loadfile(context.makepath(libtemplateDirectory, "Builder.lua"))

	-- Builder.lua only exists when native plugins need to be linked.
	if not builderConstructor then
		return nil
	end

	local oldPackagePath = package.path or ""
	package.path = libtemplateDirectory .. "/?.lua;" .. oldPackagePath

	local builder = builderConstructor()
	local buildOptions = {
		dstDir = appPath,
		dstName = options.bundleexecutable,
		dstPath = context.makepath(appPath, options.bundleexecutable),
		librarySearchPaths = { libtemplateDirectory },
		pluginsDir = buildDirectory,
		verbose = options.verbose == nil and true or options.verbose,
		sdkType = options.sdkType,
		tmpDir = options.tmpDir,
		settings = options.settings,
	}

	context.setStatus("Adding plugins")
	print("builder: buildOptions: " .. json.prettify(buildOptions))

	local buildCallSucceeded, buildResult = pcall(function() builder:build(buildOptions) end)
	package.path = oldPackagePath
	options.usesSwift = buildOptions.usesSwift

	print("builder: buildCallSucceeded: " .. tostring(buildCallSucceeded) .. "; buildResult: " .. tostring(buildResult))
	if not buildCallSucceeded then
		local buildError = json.decode(buildResult:gsub(".*<error>(.*)</error>", "%1"))
		if buildError and buildError.message then
			print("ERROR: Builder failed: " .. tostring(buildError.message))
		else
			print("ERROR: Builder failed: " .. tostring(buildResult))
		end
	end

	if not buildCallSucceeded or (buildResult ~= nil and buildResult ~= 0) then
		return "There was a problem linking the app.\n\nCheck the console for more information."
	end

	buildResult = context.runScript("mv " .. context.quoteString(buildDirectory) .. " " .. context.quoteString(options.tmpDir))
	if buildResult ~= 0 then
		return "ERROR: build failed to move helper files/plugin libs"
	end

	return nil
end

return M
