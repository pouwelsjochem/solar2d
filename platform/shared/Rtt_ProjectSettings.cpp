//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Rtt_ProjectSettings.h"
#include "Core/Rtt_Build.h"
#include "Core/Rtt_FileSystem.h"
#include "Display/Rtt_Display.h"
#include "Rtt_Archive.h"
#include "Rtt_LuaFile.h"
#include "Rtt_NativeWindowMode.h"
#include "Rtt_Runtime.h"
#include <algorithm>
#include <string>
extern "C"
{
#	include "lua.h"
}


namespace Rtt
{

ProjectSettings::ProjectSettings()
{
	ResetBuildSettings();
	ResetConfigLuaSettings();
}

ProjectSettings::~ProjectSettings()
{
}

bool ProjectSettings::LoadFromDirectory(const char* directoryPath)
{
	bool wasBuildSettingsFound = false;
	bool wasConfigLuaFound = false;

	// Validate argument.
	if (Rtt_StringIsEmpty(directoryPath))
	{
		return false;
	}

	// Check if the directory exists.
	if (!Rtt_FileExists(directoryPath))
	{
		return false;
	}

	// Create a new Lua state to load the "build.settings" and "config.lua" into.
	lua_State* luaStatePointer = luaL_newstate();
	if (!luaStatePointer)
	{
		return false;
	}
	luaL_openlibs(luaStatePointer);

	// Make a directory path with a trailing slash.
	std::string directoryPathWithSlash(directoryPath);
	char lastCharacter = directoryPathWithSlash.c_str()[directoryPathWithSlash.size() - 1];
#ifdef Rtt_WIN_ENV
	if ((lastCharacter != '\\') && (lastCharacter != '/'))
	{
		directoryPathWithSlash += '\\';
	}
#else
	if (lastCharacter != '/')
	{
		directoryPathWithSlash += '/';
	}
#endif

	// Attempt to load the Corona project's "build.settings" and "config.lua" files into the Lua state as globals.
	std::string filePath = directoryPathWithSlash;
	filePath += "resource.car";
	if (Rtt_FileExists(filePath.c_str()))
	{
		// Load the "build.settings" and "config.lua" from the directory's "resource.car" file.
		Rtt_Allocator* allocatorPointer = Rtt_AllocatorCreate();
		if (allocatorPointer)
		{
			{
				Rtt::Archive archive(*allocatorPointer, filePath.c_str());
				archive.DoResource(luaStatePointer, "build.settings", 0);

				// archive.DoResource(luaStatePointer, Rtt_LUA_OBJECT_FILE("config"), 0);
				// Sometimes config.lua uses display.* APIs. It will fail here: not a big deal
				// It will be loaded again in Rtt::Runtime (similar logic implemented below)
				int status = archive.LoadResource(luaStatePointer, Rtt_LUA_OBJECT_FILE("config"));
				if (!status)
				{
					status = lua_pcall(luaStatePointer, 0, 0, 0);
				}
			}
			Rtt_AllocatorDestroy(allocatorPointer);
		}
	}
	else
	{
		// *** A "resource.car" file was not found. ***

		// Attempt to load the "build.settings" file.
		filePath = directoryPathWithSlash;
		filePath += "build.settings";
		if (Rtt_FileExists(filePath.c_str()))
		{
			Rtt::Lua::DoFile(luaStatePointer, filePath.c_str(), 0, true);
		}

		// Attempt to load the "config.lua" file.
		// Note: This could fail if the script contains Corona's Lua APIs.
		//       In this case, we have to load this configuration via the Rtt::Runtime instead.
		filePath = directoryPathWithSlash;
		filePath += "config.lua";
		if (Rtt_FileExists(filePath.c_str()))
		{
			int status = luaL_loadfile(luaStatePointer, filePath.c_str());
			if (!status)
			{
				status = lua_pcall(luaStatePointer, 0, 0, 0);
			}
		}
	}

	// Fetch the "build.settings" information, if successfully loaded into Lua up above.
	lua_getglobal(luaStatePointer, "settings");
	if (lua_istable(luaStatePointer, -1))
	{
		// Reset "build.settings" related info and flag that this file was found.
		ResetBuildSettings();
		fHasBuildSettings = true;
		wasBuildSettingsFound = true;

		// Fetch the project's window settings.
		lua_getfield(luaStatePointer, -1, "window");
		if (lua_istable(luaStatePointer, -1))
		{
			// Fetch the window's default window mode such as "normal", "fullscreen", etc.
			lua_getfield(luaStatePointer, -1, "defaultMode");
			if (lua_type(luaStatePointer, -1) == LUA_TSTRING)
			{
				fDefaultWindowModePointer = Rtt::NativeWindowMode::FromStringId(lua_tostring(luaStatePointer, -1));
			}
			lua_pop(luaStatePointer, 1);
            
            // Fetch the window's suspend when minimized setting.
            lua_getfield(luaStatePointer, -1, "suspendWhenMinimized");
            if (lua_type(luaStatePointer, -1) == LUA_TBOOLEAN)
            {
                fSuspendWhenMinimized = lua_toboolean(luaStatePointer, -1) ? true : false;
            }
            lua_pop(luaStatePointer, 1);

			// Fetch the width/height the window's client/view area should default to on startup.
			lua_getfield(luaStatePointer, -1, "defaultViewWidth");
			if (lua_type(luaStatePointer, -1) == LUA_TNUMBER)
			{
				fDefaultWindowViewWidth = (int)lua_tointeger(luaStatePointer, -1);
				if (fDefaultWindowViewWidth < 0)
				{
					fDefaultWindowViewWidth = 0;
				}
			}
			lua_pop(luaStatePointer, 1);
			lua_getfield(luaStatePointer, -1, "defaultViewHeight");
			if (lua_type(luaStatePointer, -1) == LUA_TNUMBER)
			{
				fDefaultWindowViewHeight = (int)lua_tointeger(luaStatePointer, -1);
				if (fDefaultWindowViewHeight < 0)
				{
					fDefaultWindowViewHeight = 0;
				}
			}
			lua_pop(luaStatePointer, 1);

			// Fetch the window close, minimize, and maximize button enable settings.
			lua_getfield(luaStatePointer, -1, "enableCloseButton");
			if (lua_type(luaStatePointer, -1) == LUA_TBOOLEAN)
			{
				fIsWindowCloseButtonEnabled = lua_toboolean(luaStatePointer, -1) ? true : false;
			}
			lua_pop(luaStatePointer, 1);

			lua_getfield(luaStatePointer, -1, "enableMinimizeButton");
			if (lua_type(luaStatePointer, -1) == LUA_TBOOLEAN)
			{
				fIsWindowMinimizeButtonEnabled = lua_toboolean(luaStatePointer, -1) ? true : false;
			}
			lua_pop(luaStatePointer, 1);

			// Fetch the window's title bar text.
			lua_getfield(luaStatePointer, -1, "titleText");
			if (lua_istable(luaStatePointer, -1))
			{
				// A localized string table was provided.
				// The keys are expected to be the ISO "language-country" codes and values are the localized strings.
				int luaTableIndex = lua_gettop(luaStatePointer);
				for (lua_pushnil(luaStatePointer);
				     lua_next(luaStatePointer, luaTableIndex) != 0;
				     lua_pop(luaStatePointer, 1))
				{
					// Validate the key/value pair.
					// Note: Value can be an empty string, but not null.
					if ((lua_type(luaStatePointer, -1) != LUA_TSTRING) || (lua_type(luaStatePointer, -2) != LUA_TSTRING))
					{
						continue;
					}
					auto stringKey = lua_tostring(luaStatePointer, -2);
					auto stringValue = lua_tostring(luaStatePointer, -1);
					if (Rtt_StringIsEmpty(stringKey) || !stringValue)
					{
						continue;
					}

					// Convert the ASCII string key to lowercase.
					std::string lowercaseStringKey(stringKey);
					std::transform(
							lowercaseStringKey.begin(), lowercaseStringKey.end(), lowercaseStringKey.begin(), ::tolower);

					// Add the localized text to the dictionary.
					std::pair<std::string, std::string> pair(lowercaseStringKey, std::string(stringValue));
					fLocalizedWindowTitleTextMap.insert(pair);
				}
			}
			else if (lua_isstring(luaStatePointer, -1))
			{
				// Only one string was provided. Use it as the default, regardless of current system locale.
				auto titleText = lua_tostring(luaStatePointer, -1);
				if (titleText)
				{
					std::pair<std::string, std::string> pair(std::string("default"), std::string(titleText));
					fLocalizedWindowTitleTextMap.insert(pair);
				}
			}
			lua_pop(luaStatePointer, 1);
		}
		lua_pop(luaStatePointer, 1);
	}
	lua_pop(luaStatePointer, 1);

	// Fetch the "config.lua" information, if successfully loaded into Lua up above.
	lua_getglobal(luaStatePointer, "application");
	if (lua_istable(luaStatePointer, -1))
	{
		// Reset "config.lua" related info and flag that this file was found.
		ResetConfigLuaSettings();
		fHasConfigLua = true;
		wasConfigLuaFound = true;

		// Fetch the project's transparency setting.
		lua_getfield(luaStatePointer, -1, "isTransparent");
		if (lua_type(luaStatePointer, -1) == LUA_TBOOLEAN)
		{
			fIsWindowTransparent = lua_toboolean(luaStatePointer, -1) ? true : false;
		}
		lua_pop(luaStatePointer, 1);
		
		lua_getfield(luaStatePointer, -1, "backend");
		if (lua_isstring(luaStatePointer, -1))
		{
			const char * backend = lua_tostring(luaStatePointer, -1);

			if (strcmp(backend, "wantVulkan") == 0 || strcmp(backend, "requireVulkan") == 0)
			{
				fBackend = backend;
			}
		}
		lua_pop(luaStatePointer, 1);

		// Fetch the project's content scaling settings.
		lua_getfield(luaStatePointer, -1, "content");
		if (lua_istable(luaStatePointer, -1))
		{
			// Fetch the content width and height restrictions.
			lua_getfield(luaStatePointer, -1, "minContentWidth");
			if (lua_isnumber(luaStatePointer, -1))
			{
				fMinContentWidth = (int)lua_tointeger(luaStatePointer, -1);
			}
			lua_pop(luaStatePointer, 1);

			lua_getfield(luaStatePointer, -1, "maxContentWidth");
			if (lua_isnumber(luaStatePointer, -1))
			{
				fMaxContentWidth = (int)lua_tointeger(luaStatePointer, -1);
			}
			lua_pop(luaStatePointer, 1);

			lua_getfield(luaStatePointer, -1, "minContentHeight");
			if (lua_isnumber(luaStatePointer, -1))
			{
				fMinContentHeight = (int)lua_tointeger(luaStatePointer, -1);
			}
			lua_pop(luaStatePointer, 1);

			lua_getfield(luaStatePointer, -1, "maxContentHeight");
			if (lua_isnumber(luaStatePointer, -1))
			{
				fMaxContentHeight = (int)lua_tointeger(luaStatePointer, -1);
			}
			lua_pop(luaStatePointer, 1);
		}
		lua_pop(luaStatePointer, 1);
	}
	lua_pop(luaStatePointer, 1);

	// Let a derived version of this class load its custom fields, but only if at least 1 of the files were found.
	if (wasBuildSettingsFound || wasConfigLuaFound)
	{
		OnLoadedFrom(luaStatePointer);
	}

	// Destroy the Lua state created up above.
	lua_close(luaStatePointer);

	// Return an error result if no project files were found.
	if (!wasBuildSettingsFound && !wasConfigLuaFound)
	{
		return false;
	}

	// Load was successful.
	return true;
}

void ProjectSettings::ResetBuildSettings()
{
	fHasBuildSettings = false;
	fDefaultWindowModePointer = NULL;
    fSuspendWhenMinimized = false;
	fIsWindowCloseButtonEnabled = true;
	fIsWindowMinimizeButtonEnabled = true;
	fLocalizedWindowTitleTextMap.clear();
	fBackend = "gl";
}

void ProjectSettings::ResetConfigLuaSettings()
{
	fHasConfigLua = false;
	fMinContentWidth = 0;
	fMaxContentWidth = 0;
	fMinContentHeight = 0;
	fMaxContentHeight = 0;
}

bool ProjectSettings::HasBuildSettings() const
{
	return fHasBuildSettings;
}

bool ProjectSettings::HasConfigLua() const
{
	return fHasConfigLua;
}

const Rtt::NativeWindowMode* ProjectSettings::GetDefaultWindowMode() const
{
	return fDefaultWindowModePointer;
}

bool ProjectSettings::SuspendWhenMinimized() const
{
    return fSuspendWhenMinimized;
}

int ProjectSettings::GetDefaultWindowViewWidth() const
{
	return fDefaultWindowViewWidth;
}

int ProjectSettings::GetDefaultWindowViewHeight() const
{
	return fDefaultWindowViewHeight;
}

bool ProjectSettings::IsWindowCloseButtonEnabled() const
{
	return fIsWindowCloseButtonEnabled;
}

bool ProjectSettings::IsWindowMinimizeButtonEnabled() const
{
	return fIsWindowMinimizeButtonEnabled;
}

const char* ProjectSettings::GetWindowTitleTextForLocale(
	const char* languageCode, const char* countryCode) const
{
	// Optimization: Do not continue if there are no localized strings available.
	if (fLocalizedWindowTitleTextMap.size() <= 0)
	{
		return NULL;
	}

	// Attempt to fetch a localized string in the following order:
	// 1) By language and country.
	// 2) By language. (Supports all countries.)
	// 3) Fallback to the "default" string if all else fails.
	const char* titleText = NULL;
	if (!Rtt_StringIsEmpty(countryCode))
	{
		titleText = GetWindowTitleTextForLocaleWithoutFallback(languageCode, countryCode);
	}
	if (!titleText)
	{
		titleText = GetWindowTitleTextForLocaleWithoutFallback(languageCode, NULL);
	}
	if (!titleText)
	{
		titleText = GetWindowTitleTextForLocaleWithoutFallback("default", NULL);
	}

	// Return the localized string.
	return titleText;
}

const char* ProjectSettings::GetWindowTitleTextForLocaleWithoutFallback(
	const char* languageCode, const char* countryCode) const
{
	// Do not continue if given an invalid language code. Only the country code is optional.
	if (Rtt_StringIsEmpty(languageCode))
	{
		return NULL;
	}

	// Create a lowercase locale string key with the given ISO language code and country code.
	std::string stringKey(languageCode);
	if (!Rtt_StringIsEmpty(countryCode))
	{
		stringKey.append("-");
		stringKey.append(countryCode);
	}
	std::transform(stringKey.begin(), stringKey.end(), stringKey.begin(), ::tolower);

	// Attempt to fetch the localized string.
	const char* titleText = NULL;
	auto iter = fLocalizedWindowTitleTextMap.find(stringKey);
	if (iter != fLocalizedWindowTitleTextMap.end())
	{
		titleText = iter->second.c_str();
	}
	return titleText;
}

int ProjectSettings::GetMinContentWidth() const
{
	return fMinContentWidth;
}
int ProjectSettings::GetMaxContentWidth() const
{
	return fMaxContentWidth;
}

int ProjectSettings::GetMinContentHeight() const
{
	return fMinContentHeight;
}
int ProjectSettings::GetMaxContentHeight() const
{
	return fMaxContentHeight;
}

const std::string & ProjectSettings::Backend() const
{
	return fBackend;
}

void ProjectSettings::OnLoadedFrom(lua_State* luaStatePointer)
{
}

}	// namespace Rtt
