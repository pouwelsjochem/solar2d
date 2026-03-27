//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Rtt_ReadOnlyProjectSettings.h"
#include "Rtt_ProjectSettings.h"
#include "Rtt_NativeWindowMode.h"


namespace Rtt
{

ReadOnlyProjectSettings::ReadOnlyProjectSettings(const ProjectSettings& settings)
:	fSettings(settings)
{
}

ReadOnlyProjectSettings::~ReadOnlyProjectSettings()
{
}

bool ReadOnlyProjectSettings::HasBuildSettings() const
{
	return fSettings.HasBuildSettings();
}

bool ReadOnlyProjectSettings::HasConfigLua() const
{
	return fSettings.HasConfigLua();
}

const Rtt::NativeWindowMode* ReadOnlyProjectSettings::GetDefaultWindowMode() const
{
	return fSettings.GetDefaultWindowMode();
}

int ReadOnlyProjectSettings::GetDefaultWindowViewWidth() const
{
	return fSettings.GetDefaultWindowViewWidth();
}

int ReadOnlyProjectSettings::GetDefaultWindowViewHeight() const
{
	return fSettings.GetDefaultWindowViewHeight();
}

bool ReadOnlyProjectSettings::IsWindowCloseButtonEnabled() const
{
	return fSettings.IsWindowCloseButtonEnabled();
}

bool ReadOnlyProjectSettings::IsWindowMinimizeButtonEnabled() const
{
	return fSettings.IsWindowMinimizeButtonEnabled();
}
const char* ReadOnlyProjectSettings::GetWindowTitleTextForLocale(
	const char* languageCode, const char* countryCode) const
{
	return fSettings.GetWindowTitleTextForLocale(languageCode, countryCode);
}

const char* ReadOnlyProjectSettings::GetWindowTitleTextForLocaleWithoutFallback(
	const char* languageCode, const char* countryCode) const
{
	return fSettings.GetWindowTitleTextForLocaleWithoutFallback(languageCode, countryCode);
}

const char * ReadOnlyProjectSettings::Backend() const
{
	return fSettings.Backend().c_str();
}

int ReadOnlyProjectSettings::GetMinContentWidth() const
{
	return fSettings.GetMinContentWidth();
}
int ReadOnlyProjectSettings::GetMaxContentWidth() const
{
	return fSettings.GetMaxContentWidth();
}

int ReadOnlyProjectSettings::GetMinContentHeight() const
{
	return fSettings.GetMinContentHeight();
}
int ReadOnlyProjectSettings::GetMaxContentHeight() const
{
	return fSettings.GetMaxContentHeight();
}

}	// namespace Rtt
