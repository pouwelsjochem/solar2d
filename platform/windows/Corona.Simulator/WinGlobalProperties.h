//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

#include <Windows.h>


// Corona defined messages for activity indicator and alert windows
// Processed by CSimulatorView
#define WMU_NATIVEALERT (WM_APP + 101)

// Struct passed as lParam to WMU_ALERT
// Note: params are char *, expected in UTF8 format
typedef struct wmu_alert_params
{
	const char *sTitle;
	const char *sMsg;
	int nButtonLabels;
	const char **psButtonLabels;
	void *pLuaResource;
	HWND hwnd;
} WMU_ALERT_PARAMS;


namespace Interop
{
	class SimulatorRuntimeEnvironment;
}
namespace Rtt
{
	class WinPlatform;
	class WinPlatformServices;
};


/// <summary>
///  <para>This class mirrors some members of CSimulatorApp for convenience of access.</para>
///  <para>Instead of AfxGetApp()->GetRegistryKey(), use GetWinProperties()->GetRegistryKey().</para>
///  <para>Note that char * strings are expected to be in UTF8 format. Use WinString to convert.</para>
///  <para>It also holds pointers to the four objects needed for authorization.</para>
/// </summary>
class WinGlobalProperties
{
	private:
		WinGlobalProperties();

	public:
		virtual ~WinGlobalProperties();

		const char *GetRegistryKey() { return m_sRegistryKey; }
		void SetRegistryKey(const char *sKey);
		const char *GetRegistryProfile() { return m_sRegistryProfile; }
		void SetRegistryProfile(const char *sProfile);
		const char *GetResourcesDir() { return m_sResourcesDir; }
		void SetResourcesDir(const char *sDir);
		Rtt::WinPlatform *GetPlatform();
		Rtt::WinPlatformServices *GetServices() { return m_pServices; }

		static WinGlobalProperties* GetInstance();

	private:

		char *m_sRegistryKey;
		char *m_sRegistryProfile;
		char *m_sResourcesDir;
		Interop::SimulatorRuntimeEnvironment *m_pEnvironment;
		Rtt::WinPlatformServices *m_pServices;
};

WinGlobalProperties *GetWinProperties();
