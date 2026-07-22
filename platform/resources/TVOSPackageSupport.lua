------------------------------------------------------------------------------
--
-- This file is part of the Solar2D game engine.
-- For overview and more information on licensing please refer to README.md 
-- Home page: https://github.com/coronalabs/corona
-- Contact: support@Solar2D.com
--
-- tvOS-specific hooks for AppleMobilePackageApp.lua.
--
------------------------------------------------------------------------------

local json = require("json")
local lfs = require("lfs")

local M = {}

function M.validateSettings()
end

local function writeJson(path, value)
	local file, err = io.open(path, "w")
	if not file then
		return err
	end
	file:write(json.prettify(value))
	file:close()
	return nil
end

local function createBundle(path, contents, context)
	local status, err = context.runScript("mkdir -p " .. context.quoteString(path))
	if status ~= 0 then
		return "Could not create asset bundle at " .. path .. ": " .. tostring(err)
	end
	local writeError = writeJson(context.makepath(path, "Contents.json"), contents)
	if writeError then
		return "Could not create asset descriptor at " .. path .. ": " .. writeError
	end
	return nil
end

local function copyAsset(source, destination, context)
	local status = context.runScript("cp " .. context.quoteString(source) .. " " .. context.quoteString(destination))
	if status ~= 0 then
		return "Could not find resource " .. source .. ". Please check settings.tvos in build.settings."
	end
	return nil
end

local function createImageStack(path, sourceDirectory, layers, defaultContents, context)
	local contents = { layers = {}, info = defaultContents.info }
	for index in ipairs(layers) do
		table.insert(contents.layers, { filename = "Layer" .. index .. ".imagestack" })
	end

	local err = createBundle(path, contents, context)
	if err then
		return err
	end

	for index, filename in ipairs(layers) do
		local layerPath = context.makepath(path, "Layer" .. index .. ".imagestack")
		err = createBundle(layerPath, defaultContents, context)
		if err then
			return err
		end

		local imageSetPath = context.makepath(layerPath, "Content.imageset")
		err = createBundle(imageSetPath, {
			images = {{ idiom = "tv", scale = "1x", filename = "asset.png" }},
			info = defaultContents.info,
		}, context)
		if err then
			return err
		end

		err = copyAsset(context.makepath(sourceDirectory, filename), context.makepath(imageSetPath, "asset.png"), context)
		if err then
			return err
		end
	end

	return nil
end

local function createLegacyAssetCatalog(options, context)
	local settings = options.settings.tvos
	local createIcons = settings.icon and settings.icon.small and settings.icon.large and settings.topShelfImage and settings.topShelfImageWide
	local createLaunchImage = type(settings.launchImage) == "string" and settings.launchImage ~= ""

	if settings.icon and settings.icon.small and settings.icon.large and settings.topShelfImage and not settings.topShelfImageWide then
		return nil, "settings.tvos.topShelfImageWide is required when legacy tvOS icon settings are used"
	end
	if not createIcons and not createLaunchImage then
		return nil
	end

	context.setStatus("Creating legacy tvOS asset catalog")
	local catalogPath = context.makepath(options.dstDir, options.dstFile .. ".app", ".build", "Corona.xcassets")
	local defaultContents = { info = { author = "Solar2D", version = 1 } }
	local err = createBundle(catalogPath, defaultContents, context)
	if err then
		return nil, err
	end

	if createLaunchImage then
		local launchPath = context.makepath(catalogPath, "tvOS Launch Image.launchimage")
		err = createBundle(launchPath, {
			images = {{
				idiom = "tv",
				extent = "full-screen",
				orientation = "landscape",
				scale = "1x",
				["minimum-system-version"] = context.minimumDeploymentTarget,
				filename = "LaunchImage1920.png",
			}},
			info = defaultContents.info,
		}, context)
		if not err then
			err = copyAsset(context.makepath(options.srcAssets, settings.launchImage), context.makepath(launchPath, "LaunchImage1920.png"), context)
		end
		if err then
			return nil, err
		end
	end

	if createIcons then
		local brandPath = context.makepath(catalogPath, "tvOS Icon.brandassets")
		err = createBundle(brandPath, {
			assets = {
				{ role = "primary-app-icon", idiom = "tv", size = "1280x768", filename = "App Icon - Large.imagestack" },
				{ role = "primary-app-icon", idiom = "tv", size = "400x240", filename = "App Icon - Small.imagestack" },
				{ role = "top-shelf-image-wide", idiom = "tv", size = "2320x720", filename = "Top Shelf Image Wide.imageset" },
				{ role = "top-shelf-image", idiom = "tv", size = "1920x720", filename = "Top Shelf Image.imageset" },
			},
			info = defaultContents.info,
		}, context)
		if err then
			return nil, err
		end

		local topShelfPath = context.makepath(brandPath, "Top Shelf Image.imageset")
		err = createBundle(topShelfPath, {
			images = {{ idiom = "tv", scale = "1x", filename = "TopShelf1920.png" }},
			info = defaultContents.info,
		}, context)
		if not err then
			err = copyAsset(context.makepath(options.srcAssets, settings.topShelfImage), context.makepath(topShelfPath, "TopShelf1920.png"), context)
		end
		if err then
			return nil, err
		end

		local topShelfWidePath = context.makepath(brandPath, "Top Shelf Image Wide.imageset")
		err = createBundle(topShelfWidePath, {
			images = {{ idiom = "tv", scale = "1x", filename = "TopShelf2320.png" }},
			info = defaultContents.info,
		}, context)
		if not err then
			err = copyAsset(context.makepath(options.srcAssets, settings.topShelfImageWide), context.makepath(topShelfWidePath, "TopShelf2320.png"), context)
		end
		if err then
			return nil, err
		end

		err = createImageStack(context.makepath(brandPath, "App Icon - Small.imagestack"), options.srcAssets, settings.icon.small, defaultContents, context)
		if not err then
			err = createImageStack(context.makepath(brandPath, "App Icon - Large.imagestack"), options.srcAssets, settings.icon.large, defaultContents, context)
		end
		if err then
			return nil, err
		end
	end

	return catalogPath
end

function M.compileLegacyAssets(options, context)
	local catalogPath, err = createLegacyAssetCatalog(options, context)
	if err or not catalogPath then
		return err
	end

	local plistPath = os.tmpname()
	local command = "xcrun actool --output-format human-readable-text --warnings --notices --compress-pngs"
	command = command .. " --target-device tv"
	command = command .. " --minimum-deployment-target " .. context.minimumDeploymentTarget
	command = command .. " --output-partial-info-plist " .. context.quoteString(plistPath)
	command = command .. " --app-icon " .. context.quoteString("tvOS Icon")
	command = command .. " --launch-image " .. context.quoteString("tvOS Launch Image")
	command = command .. " --enable-on-demand-resources " .. (options.settings.tvos.enableOnDemandResources and "YES" or "NO")
	command = command .. " --platform " .. (options.sdkType == "appletvsimulator" and "appletvsimulator" or "appletvos")

	if options.settings.tvos.filterForDeviceModel then
		command = command .. " --filter-for-device-model " .. context.quoteString(options.settings.tvos.filterForDeviceModel)
	end
	if options.settings.tvos.filterForDeviceOSVersion then
		command = command .. " --filter-for-device-os-version " .. context.quoteString(options.settings.tvos.filterForDeviceOSVersion)
	end
	if options.settings.tvos.leaderboardIdentifierPrefix then
		command = command .. " --leaderboard-set-identifier-prefix " .. context.quoteString(options.settings.tvos.leaderboardIdentifierPrefix)
	end

	command = command .. " --compile " .. options.appBundleFile .. " " .. context.quoteString(catalogPath)
	context.setStatus("Compiling legacy tvOS assets")
	local status, commandError = context.runScript(command)
	if status ~= 0 then
		return "Error while compiling legacy tvOS assets: " .. tostring(commandError)
	end

	local plistJson = context.captureCommandOutput("plutil -convert json -o - -- " .. context.quoteString(plistPath) .. " && echo")
	local plistEntries, _, decodeError = json.decode(plistJson)
	if not plistEntries then
		return "Could not read actool output: " .. tostring(decodeError)
	end
	options.settings.tvos.plist = options.settings.tvos.plist or {}
	for key, value in pairs(plistEntries) do
		options.settings.tvos.plist[key] = value
	end

	context.runScript("rm -rf " .. context.quoteString(catalogPath) .. " " .. context.quoteString(plistPath))
	return nil
end

local function pluginDirectoryNames(pluginsDirectory)
	local result = {}
	if lfs.attributes(pluginsDirectory, "mode") ~= "directory" then
		return result
	end
	for filename in lfs.dir(pluginsDirectory) do
		if filename ~= "libtemplate" and filename:sub(1, 1) ~= "." then
			local path = pluginsDirectory .. "/" .. filename
			if lfs.attributes(path, "mode") == "directory" then
				table.insert(result, filename)
			end
		end
	end
	table.sort(result)
	return result
end

local function copyLuaPluginAssets(pluginDirectory, destination, context)
	local candidates = {
		pluginDirectory .. "/lua/lua_51",
		pluginDirectory .. "/lua_51",
	}
	for _, source in ipairs(candidates) do
		if lfs.attributes(source, "mode") == "directory" then
			local status, err = context.runScript("mkdir -p " .. context.quoteString(destination) .. " && cp -R " .. context.quoteString(source) .. "/* " .. context.quoteString(destination))
			if status ~= 0 then
				return "Could not copy Lua plugin assets: " .. tostring(err)
			end
		end
	end
	return nil
end

function M.integratePlugins(options, context)
	local appPath = context.makepath(options.dstDir, options.dstFile .. ".app")
	local buildDirectory = context.makepath(appPath, ".build")
	if lfs.attributes(buildDirectory, "mode") ~= "directory" then
		return nil
	end

	local frameworksDirectory = context.makepath(appPath, "Frameworks")
	local luaAssetsDirectory = context.makepath(appPath, "corona-plugins")
	context.runScript("mkdir -p " .. context.quoteString(frameworksDirectory))

	local embeddedFramework = false
	for _, pluginName in ipairs(pluginDirectoryNames(buildDirectory)) do
		local pluginDirectory = context.makepath(buildDirectory, pluginName)
		local frameworkName = "Corona_" .. pluginName:gsub("%.", "_") .. ".framework"
		local frameworkPath = context.makepath(pluginDirectory, frameworkName)
		if lfs.attributes(frameworkPath, "mode") == "directory" then
			local status, err = context.runScript("mv " .. context.quoteString(frameworkPath) .. " " .. context.quoteString(frameworksDirectory))
			if status ~= 0 then
				return "Could not embed tvOS plugin framework " .. frameworkName .. ": " .. tostring(err)
			end
			embeddedFramework = true

			local executable = context.makepath(frameworksDirectory, frameworkName, frameworkName:gsub("%.framework$", ""))
			local swiftLibraries = context.captureCommandOutput("xcrun otool -L " .. context.quoteString(executable) .. " 2>/dev/null | grep '/libswift' || true")
			if swiftLibraries and swiftLibraries ~= "" then
				options.usesSwift = true
			end
		end

		local copyError = copyLuaPluginAssets(pluginDirectory, luaAssetsDirectory, context)
		if copyError then
			return copyError
		end
	end

	if lfs.attributes(luaAssetsDirectory, "mode") == "directory" then
		local status, err = context.runScript("find " .. context.quoteString(luaAssetsDirectory) .. " \\( -name '*.lu' -o -name '*.lua' \\) -delete")
		if status ~= 0 then
			return "Could not prune Lua plugin sources: " .. tostring(err)
		end

		status, err = context.runScript("COPYPNG=$(xcrun -f copypng); find " .. context.quoteString(luaAssetsDirectory) .. " -name '*.png' -exec \"$COPYPNG\" -compress {} {}.copypng \\; -exec mv {}.copypng {} \\;")
		if status ~= 0 then
			return "Could not optimize tvOS plugin PNGs: " .. tostring(err)
		end
	end

	if embeddedFramework then
		local sdkVersion = tonumber((context.captureCommandOutput("xcrun --sdk appletvos --show-sdk-version") or ""):match("%d+"))
		if sdkVersion and sdkVersion >= 16 then
			local stripCommand = "cd " .. context.quoteString(frameworksDirectory) .. " && for F in *.framework; do B=\"$F/${F%.*}\"; if xcrun otool -l \"$B\" | grep LLVM -q; then xcrun bitcode_strip -r \"$B\" -o \"$B.tmp\" && mv \"$B.tmp\" \"$B\"; fi; done"
			local status, err = context.runScript(stripCommand)
			if status ~= 0 then
				return "Could not strip tvOS plugin bitcode: " .. tostring(err)
			end
		end
	end

	local status, err = context.runScript("mv " .. context.quoteString(buildDirectory) .. " " .. context.quoteString(options.tmpDir))
	if status ~= 0 then
		return "Could not move tvOS plugin build files: " .. tostring(err)
	end
	return nil
end

return M
