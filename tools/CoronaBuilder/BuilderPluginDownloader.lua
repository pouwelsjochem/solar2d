------------------------------------------------------------------------------
--
-- This file is part of the Corona game engine.
-- For overview and more information on licensing please refer to README.md
-- Home page: https://github.com/coronalabs/corona
-- Contact: support@coronalabs.com
--
------------------------------------------------------------------------------


local json = require('json')
local lfs = require('lfs')
local builder = require('builder')

local verbosity = 3
local androidBuild = false
local alwaysQuery = false
local windows = (package.config:match("^.") == '\\')

-- in offline build `user` is nil
function DownloadPluginsMain(args, user, buildYear, buildRevision)
	if args[1] ~= 'download' then
		print("ERROR: unknows subcommand to 'plugins' command: '" .. tostring(args[1]) .. "'. Only 'download' is currently supported." )
		return 1
	end

	for i=#args,1,-1 do
		if args[i] == '--fetch-dependencies' then -- scans directory for dependencies
			table.remove(args, i)
			verbosity = 0
			fetchDependencies = true
		elseif args[i] == '--always-query' then -- forces always to query for available plugins, for test purposes mostly
			table.remove(args, i)
			alwaysQuery = true
		elseif args[i] == '--build' then --verrides buildYear and buildRevision
			table.remove(args, i)
			local build = args[i]
			table.remove(args, i)
			build = (build or ""):gmatch('(%d+)%.(%d+)')
			local y,b = build()
			if y and b then
				buildYear = y
				buildRevision = b
			end
		elseif args[i] == '--' then
			break
		end
	end

	local platform = args[2]


	if type(platform) ~= 'string' then
		print("ERROR: missing platform parameter to 'plugins download' subcommand.")
		return 1
	end
	platform = platform:lower()


	local buildSettingsFile = args[3]
	-- Parse the build settings to find the requested plugins.
	if not buildSettingsFile then
		print("ERROR: no build settings file specified.")
		return 1;
	end

	local settings
	if buildSettingsFile:sub(-#"build.properties") == "build.properties" then
		local props, err = io.open( buildSettingsFile, "r" )
		if not props then
			print("ERROR: unable to open build.properties file, error: " .. tostring(err))
			return 1
		end
		settings = json.decode(props:read("*a") or '{"buildSettings":{}}').buildSettings or {}

		props:close()
	else
		local oldSettings = _G['settings']
		_G['settings'] = nil
		pcall( function(  )
			dofile(buildSettingsFile)
		end  )
		settings = _G['settings']
		_G['settings'] = oldSettings
	end

	if type(settings) ~= 'table' then
		print("ERROR: Couldn't read 'build.settings' file at path: '" .. buildSettingsFile .. "'")
		return 1
	end

	if type(settings.plugins) ~= 'table' then
		settings.plugins = {}
	end

	if platform == 'win32' or platform == 'macos' then

		local destDir = args[4]
		if not destDir then
			print("ERROR: no output directory specified for '" .. platform .. "' plugin files.")
			return 1
		end

		-- Set up the native plugin cache directory.
		local pluginsDest
		if windows then
			pluginsDest = os.getenv('APPDATA') .. '\\Corona Labs'
			lfs.mkdir(pluginsDest)
			pluginsDest = pluginsDest .. '\\Corona Simulator'
			lfs.mkdir(pluginsDest)
			pluginsDest = pluginsDest .. '\\NativePlugins\\'
			lfs.mkdir(pluginsDest)
			pluginsDest = pluginsDest .. platform .. '\\'
			lfs.mkdir(pluginsDest)
		else
			pluginsDest = os.getenv('HOME') .. '/Library/Application Support/Corona'
			lfs.mkdir(pluginsDest)
			pluginsDest = pluginsDest .. '/Native Plugins/'
			lfs.mkdir(pluginsDest)
			pluginsDest = pluginsDest .. platform .. '/'
			lfs.mkdir(pluginsDest)
		end

		lfs.mkdir(destDir)

		-- Download plugin archives using CoronaBuilderPluginCollector.
		-- extractLocation causes all archives to be extracted flat into destDir,
		-- with any lua_51/ subdirectory merged into root automatically.
		local pluginCollector = require "CoronaBuilderPluginCollector"
		-- Pass only the revision number to the collector (e.g. 9999, not "2100.9999").
		-- CoronaBuilderPluginCollector's Solar2D directory checker does:
		--   entryBuildNumber <= tonumber(params.build)
		-- where entryBuildNumber is the revision extracted from version strings like "2020.3627".
		-- Passing the full "year.revision" float (~2101) would be less than revisions like
		-- 3627, making all version lookups fail.
		local collectorParams = {
			pluginPlatform = platform,
			plugins = settings.plugins,
			destinationDirectory = pluginsDest,
			extractLocation = destDir,
			build = tostring(buildRevision),
			download = builder.download,
			fetch = builder.fetch
		}
		local err = pluginCollector.collect(collectorParams)
		if err then
			print("ERROR collecting plugins for '" .. platform .. "': " .. tostring(err))
			return 1
		end

	else
		print("ERROR: unsupported platform '".. platform .."'.")
		return 1
	end

	if verbosity > 0 then
		print("Done downloading plugins!")
	end


	return 0
end

function DownloadAndroidOfflinePlugins(args, user, buildYear, buildRevision)
	local buildData
	table.remove(args, 1)
	local inputFile = string.match(args[1] or "", "^builderInput=(.+)")
	if inputFile
	then
	    local f = assert(io.open(inputFile, "rb"))
		table.remove(args, 1)
		buildData = json.decode(f:read("*all"))
		f:close()
  	else
		buildData = json.decode(io.read('*all'))
	end
	assert(buildData)
	buildData.build = buildData.build or buildRevision
	buildData.user = buildData.user or user
	for i=1, #args do
		local k,v = args[i]:match('(.+)=(.+)')
		if k and v then
			buildData[k] = v
		end
	end

	local pluginCollector = require "CoronaBuilderPluginCollector"
	local result = pluginCollector.collect(buildData)
	if type(result) == 'string' then
		print("ERROR: occured while collecting plugins for Android. ", result)
        return 1
	end
	return 0
end
