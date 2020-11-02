------------------------------------------------------------------------------
--
-- This file is part of the Corona game engine.
-- For overview and more information on licensing please refer to README.md 
-- Home page: https://github.com/coronalabs/corona
-- Contact: support@coronalabs.com
--
------------------------------------------------------------------------------

local json = require "json"
local lfs = require "lfs"
local pluginCollector = require "CoronaBuilderPluginCollector"

local debugBuildProcess = os.execute("exit $(defaults read com.coronalabs.Corona_Simulator debugBuildProcess 2>/dev/null || echo 0)")
function log(...)
    if debugBuildProcess > 1 then
        print(...)
    end
end

local function quoteString( str )
    str = str:gsub('\\', '\\\\')
    str = str:gsub('"', '\\"')
    return "\"" .. str .. "\""
end

local function exec(cmd)
    log('Running command', cmd)
    if debugBuildProcess < 1 then
        cmd = cmd .. ' &> /dev/null'
    end
    return (0 == os.execute(cmd))
end


function findTemplate(params)
    local template = params.resourceDir .. '/iostemplate/' .. params.template
    print("template", template, lfs.attributes(template, 'mode'))
    if lfs.attributes(template, 'mode') == 'file' then
        return template
    end
    template = params.resourceDir .. '/../../../../../../../Corona Simulator.app/Contents/Resources/iostemplate/' .. params.template
    print("template", template, lfs.attributes(template, 'mode'))
    if lfs.attributes(template, 'mode') == 'file' then
        return template
    end
    template = params.resourceDir .. '/../../../Corona Simulator.app/Contents/Resources/iostemplate/' .. params.template
    print("template", template, lfs.attributes(template, 'mode'))
    if lfs.attributes(template, 'mode') == 'file' then
        return template
    end
end

function findBuilder(params)
    local builder = params.resourceDir .. '/../MacOS/CoronaBuilder'
    print("builder", builder, lfs.attributes(builder, 'mode'))
    if lfs.attributes(builder, 'mode') == 'file' then
        return builder
    end
    builder = params.resourceDir .. '/../../../Native/Corona/mac/bin/CoronaBuilder.app/Contents/MacOS/CoronaBuilder'
    print("builder", builder, lfs.attributes(builder, 'mode'))
    if lfs.attributes(builder, 'mode') == 'file' then
        return builder
    end
    builder = params.resourceDir .. '/../../../CoronaBuilder.app/Contents/MacOS/CoronaBuilder'
    print("builder", builder, lfs.attributes(builder, 'mode'))
    if lfs.attributes(builder, 'mode') == 'file' then
        return builder
    end
end

function findLuac(params)
    local luac = params.resourceDir .. '/../../../luac'
    if lfs.attributes(luac, 'mode') == 'file' then
        return luac
    end
    luac = params.resourceDir .. '/../../../Native/Corona/mac/bin/luac'
    if lfs.attributes(luac, 'mode') == 'file' then
        return luac
    end
end

function CreateOfflinePackage(params)
    log("Building offline package with params", json.prettify(params))
    local template = findTemplate(params)
    if not template then
        return 'Unable to find template ' .. params.template
    end
    local builder = findBuilder(params)
    if not builder then
        return 'Unable to find Corona Builder.'
    end
    local luac = findLuac(params)
    if not luac then
        return 'Unable to find luac (lua compiler).'
    end

    local pluginsDir = params.tmpDir  .. '/plugins'
    params.destinationDirectory = pluginsDir
    local result = pluginCollector.collect(params)
    if type(result) == 'string' then
        return result
    end
    
    local tvOS = params.isAppleTV and 'YES' or 'NO'

    log("Packaging using template", template)
    exec('/usr/bin/tar -xvjf ' .. quoteString(template)  .. ' -C ' .. quoteString(params.tmpDir) ..  '/ --strip-components=2 libtemplate/build_output.sh'  )
    local success = exec(quoteString(params.tmpDir .. '/build_output.sh')
        .. ' ' .. quoteString(params.tmpDir)     -- 1
        .. ' ' .. quoteString(params.inputFile)  -- 2
        .. ' ' .. quoteString(template)          -- 3
        .. ' ' .. quoteString(params.appName)    -- 4
        .. ' ' .. quoteString(params.outputFile) -- 5
        .. ' ' .. quoteString(builder)           -- 6
        .. ' ' .. quoteString(luac)              -- 7
        .. ' ' .. quoteString(params.appPackage) -- 8
        .. ' ' .. quoteString(params.build)      -- 9
        .. ' ' .. quoteString(pluginsDir)        -- 10
        .. ' ' .. quoteString(tvOS)              -- 11
    )
    if not success then
        return 'Build script failed'
    end
    if lfs.attributes(params.outputFile, 'mode') ~= 'file' then
        return 'Build script succeeded but produced no result'
    end
end
