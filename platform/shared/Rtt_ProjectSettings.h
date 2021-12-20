//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _Rtt_ProjectSettings_H__
#define _Rtt_ProjectSettings_H__

#include "Core/Rtt_Macros.h"
#include <map>
#include <set>
#include <string>
extern "C"
{
	struct lua_State;
}


namespace Rtt
{

class Runtime;
class NativeWindowMode;

/**
 * Stores a Corona project's "build.settings" and "config.lua" information.
 *
 * This class is only intended to be used on the Mac and Win32 platforms.
 */
class ProjectSettings
{
	Rtt_CLASS_NO_COPIES(ProjectSettings)

	public:
		/** Creates a new project settings initialized to its defaults. */
		ProjectSettings();

		/** Destroys this object and its owned resources. */
		virtual ~ProjectSettings();

		/**
		 * Attempts to load a Corona project's "build.settings" and "config.lua" from the given directory.
		 * If the given directory has a "resource.car" file, then it favor extracting this info from that file first.
		 * Information loaded from these files will then be provided by this object's getter methods.
		 * @param directoryPath
		 * Path to the Corona project directory that contains either a "resource.car"
		 * or the project's "build.settings" and "config.lua" files.
		 * @return
		 * Returns true if at least 1 file ("build.settings" or "config.lua") was loaded.
		 * 
		 * Returns false if failed to load any files or if given an invalid directory path.
		 */
		bool LoadFromDirectory(const char* directoryPath);

		/** Resets this object's "build.settings" related information back to their defaults. */
		virtual void ResetBuildSettings();

		/** Resets this object's "config.lua" related information back to their defaults. */
		virtual void ResetConfigLuaSettings();

		/**
		 * Determines if a "build.settings" file was successfully loaded by this object.
		 * @return
		 * Returns true if the object has successfully loaded the Corona project's "build.settings" file.
		 *
		 * Returns false if a "build.settings" file was not found.
		 */
		bool HasBuildSettings() const;

		/**
		 * Determines if a "config.lua" file was successfully loaded by this object.
		 * @return
		 * Returns true if the object has successfully loaded the Corona project's "config.lua" file.
		 *
		 * Returns false if a "config.lua" file was not found.
		 */
		bool HasConfigLua() const;

		/**
		 * Gets the default mode the window should be launch as such as kNormal, kFullscreen, etc.
		 * @return
		 * Returns a pointer to a window mode constant such as kNormal, kFullscreen, etc.
		 *
		 * Returns null if a default mode was not provided or if the "build.settings" file was not loaded.
		 */
		const Rtt::NativeWindowMode* GetDefaultWindowMode() const;
        
        /**
         * Determines if the Corona project should suspend when its window is minimized.
         * @return
         * Returns true if the Corona project should suspend when its window is minimized.
         *
         * Returns false if the Corona project should not suspend when its window is minimized.
         */
        bool SuspendWhenMinimized() const;

		/**
		 * Gets the default width in pixels the window's client/view area should be on startup.
		 * This is the region within the window's borders that Corona will render to.
		 * @return
		 * Returns the default width in pixels the window's client/view are should be on startup.
		 *
		 * Returns zero if a default was not provided.
		 */
		int GetDefaultWindowViewWidth() const;

		/**
		 * Gets the default height in pixels the window's client/view area should be on startup.
		 * This is the region within the window's borders that Corona will render to.
		 * @return
		 * Returns the default height in pixels the window's client/view are should be on startup.
		 *
		 * Returns zero if a default was not provided.
		 */
		int GetDefaultWindowViewHeight() const;

		/**
		 * Determines if the close button should be displayed by the window hosting the Corona runtime.
		 * @return
		 * Returns true if the close button should be displayed by the window.
		 *
		 * Returns false if it should be hidden.
		 */
		bool IsWindowCloseButtonEnabled() const;

		/**
		 * Determines if the minimize button should be displayed by the window hosting the Corona runtime.
		 * @return
		 * Returns true if the minimize button should be displayed by the window.
		 *
		 * Returns false if it should be hidden.
		 */
		bool IsWindowMinimizeButtonEnabled() const;

		/**
		 * Fetches the UTF-8 encoded localized window title bar text for the given ISO language and country codes.
		 *
		 * If localized text was not found for the given country code, then this method fetches by language code.
		 *
		 * If localized text was not found by either language or country codes, then the default non-localized
		 * text is retrieved.
		 * @param languageCode
		 * The 2 character ISO 639-1 language code such as "en" for English, "fr" for French, etc.
		 *
		 * Can be null or empty, in which case, the "default" non-localized text will be retrieved.
		 * @param countryCode
		 * The 2 character ISO 3166-1 country code such as "us" for United States, "gb" for Great Britain, etc.
		 *
		 * Can be null or empty, in which case, only the language code is looked up.
		 * @return
		 * Returns the localized string for the given ISO langugae and/or country codes.
		 *
		 * Returns the "default" non-localized text if the given locale was not in the localization string table.
		 *
		 * Returns null if the "build.settings" file was not loaded or if it did not provide any title bar text.
		 */
		const char* GetWindowTitleTextForLocale(const char* languageCode, const char* countryCode) const;

		/**
		 * Fetches the UTF-8 encoded localized window title bar text for the given ISO language and country codes.
		 *
		 * This method will not fallback to language code only strings or a default non-localized text if the
		 * given language and country code combination was not found in the localization table.
		 * @param languageCode
		 * The 2 character ISO 639-1 language code such as "en" for English, "fr" for French, etc.
		 *
		 * Cannot be null or empty.
		 * @param countryCode
		 * The 2 character ISO 3166-1 country code such as "us" for United States, "gb" for Great Britain, etc.
		 *
		 * Can be null or empty, in which case, only the language code is looked up.
		 * @return
		 * Returns the localized string for the given ISO langugae and country codes.
		 *
		 * Returns null if a localized string was not found for the given arguments.
		 */
		const char* GetWindowTitleTextForLocaleWithoutFallback(const char* languageCode, const char* countryCode) const;

		int GetMinContentWidth() const;
		int GetMaxContentWidth() const;
		int GetMinContentHeight() const;
		int GetMaxContentHeight() const;

	protected:
		/**
		 * Called after the LoadFromDirectory() method has successfully loaded information from the
		 * the "build.settings" or "config.lua" files. Will not be called if neither file could be loaded.
		 *
		 * This method is intended to be overridden by derived classes to load platform specific information.
		 * @param luaStatePointer
		 * Pointer to the Lua state that the "build.settings" and "config.lua" global tables have been pushed to.
		 */
		virtual void OnLoadedFrom(lua_State* luaStatePointer);

	private:
		/**
		 * Set true if this object has successfully loaded the configuration from a "build.settings" file.
		 * Set false if not.
		 */
		bool fHasBuildSettings;

		/**
		 * Set true if this object has successfully loaded the configuration from a "config.lua" file.
		 * Set false if not.
		 */
		bool fHasConfigLua;

		/**
		 * Pointer to a window mode constant the app window should be launch as, such as kNormal, kFullscreen, etc.
		 *
		 * Set to null if a default window mode was not provided.
		 */
		const Rtt::NativeWindowMode* fDefaultWindowModePointer;
        
        /**
         * Set to true if the Corona project should suspend when its window is minimzed.
         */
        bool fSuspendWhenMinimized;

		/**
		 * The default width in pixels the window's client/view area should be launched in.
		 *
		 * This is the region within the window's borders that Corona will render to.
		 *
		 * Set to zero if there is no default.
		 */
		int fDefaultWindowViewWidth;

		/**
		 * The default height in pixels the window's client/view area should be launched in.
		 *
		 * This is the region within the window's borders that Corona will render to.
		 *
		 * Set to zero if there is no default.
		 */
		int fDefaultWindowViewHeight;

		/**
		 * The content dimensions read from the Corona project's "config.lua" file.
		 */
		int fMinContentWidth;
		int fMaxContentWidth;
		int fMinContentHeight;
		int fMaxContentHeight;

		/** Set to true if the Corona window should display a close button. False to not show it.*/
		bool fIsWindowCloseButtonEnabled;

		/** Set to true if the Corona window should display a minimize button. False to not show it.*/
		bool fIsWindowMinimizeButtonEnabled;
	
		/**
		 * Dictionary of localized window title bar strings read from the "build.settings" file.
		 *
		 * The key is an ASCII ISO locale ID such as "en", "en-us", "fr", "fr-ca", etc.
		 *
		 * The value is a UTF-8 localized string for the key's locale.
		 *
		 * This collection will be empty if failed to load the "build.settings" file or
		 * if it did not provide any localized title text.
		 */
		std::map<std::string, std::string> fLocalizedWindowTitleTextMap;
};

} // namespace Rtt

#endif // _Rtt_ProjectSettings_H__
