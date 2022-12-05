------------------------------------------------------------------------------
--
-- This file is part of the Corona game engine.
-- For overview and more information on licensing please refer to README.md
-- Home page: https://github.com/coronalabs/corona
-- Contact: support@coronalabs.com
--
------------------------------------------------------------------------------

--
-- Platform dependent initialization for Simulators
--


-- Tell luacheck not to flag our builtin globals
-- luacheck: globals system
-- luacheck: globals display
-- luacheck: globals native
-- luacheck: globals network
-- luacheck: globals easing
-- luacheck: globals Runtime
-- luacheck: globals graphics
-- luacheck: globals loadstring

local params = ...

local onShellComplete = params.onShellComplete
local exitCallback = params.exitCallback

--------------------------------------------------------------------------------

local PluginSync =
{
	queue = {},
	now = os.time(),
	-- This is the catalog of plugin manifest.
	clientCatalogFilename = 'catalog.json',
	clientCatalog = { Version = 3 },

	-- It's not mandatory to declare this here. We're doing it for the sake
	-- of making it clear that we're going to populate these values later.
	CatalogFilenamePath = "",

}

local json = require("json")

-- luacheck: push
-- luacheck: ignore 212 -- Unused argument.
function PluginSync:debugPrint(...)
	-- Uncomment to get verbose reporting on PluginSync activities
	-- print("PluginSync: ", ...)
end
-- luacheck: pop

function PluginSync:LoadCatalog()
	local f = io.open( self.CatalogFilenamePath )
	if not f then
		return
	end

	local content = f:read( "*a" )
	f:close()
	if not content then
		return
	end

	local catalog = json.decode( content )
	if not catalog then
		return
	end

	if not catalog.Version then
		-- This file isn't versioned.
		return
	end

	if catalog.Version ~= self.clientCatalog.Version then
		-- We want to use the catalog ONLY when the
		-- version number is an exact match.
		return
	end

	local catalogBuildNumber = catalog.CoronaBuild
	if catalogBuildNumber ~= system.getInfo("build") then
		return
	end

	self.clientCatalog = catalog
end

function PluginSync:initialize( platform )
	self.platform = platform

	self.CatalogFilenamePath = system.pathForFile( self.clientCatalogFilename,
														system.PluginsDirectory )

	self:LoadCatalog()
end

function PluginSync:UpdateClientCatalog()
	self.clientCatalog.CoronaBuild = system.getInfo("build")
	local content = json.encode( self.clientCatalog )

	local f, ioErrorMessage = io.open( self.CatalogFilenamePath, 'w' ) -- erase previous contents
	if f then
		f:write( content )
		f:close()
	else
		local message = "Error updating Corona's plugin catalog."
		if ( type( ioErrorMessage ) == "string" ) and ( string.len( ioErrorMessage ) > 0 ) then
			message = message .. "\nReason:   " .. ioErrorMessage
		end
		print( message )
	end
end


function PluginSync:addPluginToQueueIfRequired( required_plugin )

	local pluginName = required_plugin.pluginName
	local pluginVersion = required_plugin.version or ""
	local publisherId = required_plugin.publisherId
	local key = tostring(publisherId) .. '/' .. pluginName .. '@' .. pluginVersion
	required_plugin.clientCatalogKey = key

	-- Find reasons to queue the plugin for download.
	local should_queue = true

	local manifest = self.clientCatalog[ key ]
	if type(manifest) == 'table' and type(manifest.lastUpdate) == 'number'  then
		local age = os.difftime(self.now, manifest.lastUpdate)
		local maxAge = 24 * 60 * 60 -- update Simulator plugins every 24 hours
		should_queue = age > maxAge
	end

	if should_queue then -- Queue for download
		table.insert( self.queue, required_plugin )
	end
end

local function collectPlugins(localQueue, extractLocation, platform, continueOnError, asyncOnComplete)
	local plugins = {}
	for i=1,#localQueue do
		local pluginInfo = localQueue[i]
		plugins[pluginInfo.pluginName] = {}
		if pluginInfo.json then
			plugins[pluginInfo.pluginName] = json.decode(pluginInfo.json)
		end
		plugins[pluginInfo.pluginName].publisherId = pluginInfo.publisherId
		if continueOnError and asyncOnComplete then
			if type(plugins[pluginInfo.pluginName].supportedPlatforms) == 'table' then
				if not plugins[pluginInfo.pluginName].supportedPlatforms[platform] then
					if plugins[pluginInfo.pluginName].supportedPlatforms[platform] ~= false then
						plugins[pluginInfo.pluginName].supportedPlatforms[platform] = true
					end
				end
			end
		end
	end
	local _, sim_build_number = string.match( system.getInfo( "build" ), '(%d+)%.(%d+)' )

	local collectorParams = {
		pluginPlatform = platform,
		plugins = plugins,
		destinationDirectory = system.pathForFile("", system.PluginsDirectory),
		build = sim_build_number,
		extractLocation = extractLocation,
		continueOnError = continueOnError,
	}

	if #plugins > 0 then
		print("Cache expired for " .. #plugins .. " plugins, recollecting now...")
	end
	return params.shellPluginCollector(json.encode(collectorParams), asyncOnComplete)
end

function PluginSync:downloadQueuedPlugins( onComplete )

	-- Only download if there have been keys added
	if #self.queue == 0 then
		-- Nothing to do.
		return
	end

	self.onComplete = onComplete

	collectPlugins(self.queue, system.pathForFile("", system.PluginsDirectory), self.platform, true, function(result)
		local updateTime = self.now
		if type(result.result) == 'string' then
			updateTime = nil
			local res = result.result:gsub('%[(.-)%]%((https?://.-)%)', '%1 (%2)')
			print("WARNING: there was an issue while downloading simulator plugin placeholders:\n" .. res)
		end
		for i=1,#self.queue do
			local key = self.queue[i].clientCatalogKey
			self.clientCatalog[ key ] = { lastUpdate = updateTime }
		end
		self:UpdateClientCatalog()

		self.onComplete()
	end)
end

local function onInternalRequestUnzipPlugins( event )
	-- Verify that a destination path was provided.
	local destinationPath = event.destinationPath
	if type( destinationPath ) ~= "string" then
		return "onInternalRequestUnzipPlugins() Destination path argument was not provided."
	end

	-- Do not continue if this app does not use any plugins.
	if #params.plugins <= 0 then
		return true
	end
	local result = collectPlugins(params.plugins, destinationPath, event.platform or params.platform, false, nil)
	if result == nil then
		return true
	else
		return result
	end
end
Runtime:addEventListener( "_internalRequestUnzipPlugins", onInternalRequestUnzipPlugins )

local function onInternalQueryAreAllPluginsAvailable( _ )
	return true
end
Runtime:addEventListener( "_internalQueryAreAllPluginsAvailable", onInternalQueryAreAllPluginsAvailable )

local function loadMain( onComplete )

	PluginSync:initialize( params.platform )
	if not params.shellPluginCollector then
		-- No way to download.
		onComplete( )
		return
	end

	local required_plugins = params.plugins
	if ( not required_plugins ) or
		( #required_plugins == 0 ) then
		-- Nothing to download.
		onComplete( )
		return
	end

	-- Find what needs to be downloaded.
	for i=1,#required_plugins do
		PluginSync:addPluginToQueueIfRequired( required_plugins[i] )
	end

	if #PluginSync.queue == 0 then
		-- Nothing to download.
		onComplete( )
	else
		-- Download.
		PluginSync:downloadQueuedPlugins( onComplete )
	end
end

loadMain( onShellComplete )

-- Only override os.exit if a function is provided
if exitCallback then
	_G.os.exit = function( code )
		print( code )
		exitCallback( code )
	end
end

--------------------------------------------------------------------------------
