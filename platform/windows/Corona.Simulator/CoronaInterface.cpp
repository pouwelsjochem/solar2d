//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

// All functions start with "app" by convention - not sure why, it was that way when I
// took it over.
// These are functions that are compiled in with Corona Simulator.exe
// CoronaDll\corona.cpp, .h contain functions exported from that DLL.

#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
#include "Core\Rtt_Build.h"
#include "Interop\SimulatorRuntimeEnvironment.h"
#include "Rtt_WinPlatformServices.h"
#include "Rtt_WinPlatform.h"
#include "Rtt_PlatformAppPackager.h"
#include "Rtt_NxSAppPackager.h"
#include "Rtt_PlatformPlayer.h"
#include "Rtt_LuaContext.h"
#include "Rtt_LuaLibNative.h"
#include "Rtt_LuaFile.h"
#include "Rtt_LuaResource.h"
#include "WinString.h"
#include "WinGlobalProperties.h"
#include "CoronaInterface.h"
#include "Simulator.h"
#include "MessageDlg.h"
#include "Rtt_Runtime.h"
#include "Rtt_PlatformSurface.h"

static UINT GetStatusMessageResourceIdFor(int resultCode)
{
	UINT resourceId;

	switch (resultCode)
	{
		case 0:
			resourceId = IDS_BUILD_SUCCEEDED;
			break;
		default:
			resourceId = IDS_BUILD_FAILED;
			break;
	}
	return resourceId;
}

void appNxSBuild(
	Interop::SimulatorRuntimeEnvironment* pSim,
	const char* srcDir,
	const char* nmetaPath,
	const char* applicationName, const char* versionName,
	const char* dstDir,
	const Rtt::TargetDevice::Platform targetPlatform,
	const char* targetos, bool isDistribution, int versionCode)
{
	Rtt::WinPlatformServices* pServices = GetWinProperties()->GetServices();


	// Translate targetOS in BuildAndroidDlg.h into enum from Rtt_PlatformAppPackager.h
	int targetVersion = Rtt::TargetDevice::kNxS;

	// Create the app packager.
	Rtt::NxSAppPackager packager(*pServices);

	// Read the application's "build.settings" file.
	bool wasSuccessful = packager.ReadBuildSettings(srcDir);
	if (!wasSuccessful)
	{
		CString message;
		message.LoadString(IDS_BUILD_SETTINGS_FILE_ERROR);
		return;
	}

	// Check if a custom build ID has been assigned.
	// This is typically assigned to daily build versions of Corona.
	const char* customBuildId = packager.GetCustomBuildId();
	if (!Rtt_StringIsEmpty(customBuildId))
	{
		Rtt_Log("\nUsing custom Build Id %s\n", customBuildId);
	}

	// these are currently unused
	const char* bundleId = "bundleId"; //TODO
	const char* sdkRoot = "";

	// Package build settings parameters.
	Rtt::NxSAppPackagerParams params(
		applicationName, versionName, NULL, NULL,
		srcDir, dstDir, nmetaPath, NULL,
		targetPlatform, targetVersion,
		Rtt::TargetDevice::kNxS, customBuildId,
		NULL, bundleId, isDistribution, NULL);

	// Select build template
	const char kBuildSettings[] = "build.settings";
	Rtt::String buildSettingsPath;
	pSim->GetPlatform()->PathForFile(kBuildSettings, Rtt::MPlatform::kResourceDir, Rtt::MPlatform::kTestFileExists, buildSettingsPath);
	params.SetBuildSettingsPath(buildSettingsPath.GetString());

	// Get Windows temp directory
	TCHAR TempPath[MAX_PATH];
	GetTempPath(MAX_PATH, TempPath);  // ends in '\\'
	TCHAR* sCompanyName = _T("Corona Labs");
	_tcsncpy_s(TempPath + _tcslen(TempPath), (MAX_PATH - _tcslen(TempPath)), sCompanyName, _tcslen(sCompanyName));
	TempPath[_tcslen(TempPath)] = '\0';  // ensure null-termination
	WinString strTempDir;
	strTempDir.SetTCHAR(TempPath);

	// Have the server build the app. (Warning! This is a long blocking call.)
	int code = packager.Build(&params, strTempDir.GetUTF8());

	// Return the result of the build.
	CString statusMessage;
	if (0 == code)
	{
		statusMessage.LoadString(GetStatusMessageResourceIdFor(code));
	}
	else
	{
		WinString serverMsg;

		serverMsg.SetUTF8(params.GetBuildMessage() ? params.GetBuildMessage() : "Error while building app");

		serverMsg.Append("\r\nMore information may be available in the Simulator console");
		statusMessage.Format(IDS_BUILD_ERROR_FORMAT_MESSAGE, code, serverMsg.GetTCHAR());

		CStringA logMesg;
		logMesg.Format("[%ld] %s", code, params.GetBuildMessage());
	}
	return;
}

void appEndNativeAlert(void *resource, int nButtonIndex, bool bCanceled)
{
	// Validate.
	if (!resource)
	{
		return;
	}

	// Invoke the Lua listener.
	Rtt::LuaResource *pLuaResource = (Rtt::LuaResource *)resource;
	Rtt::LuaLibNative::AlertComplete(*pLuaResource, nButtonIndex, bCanceled);

	// Delete the Lua resource.
	Rtt_DELETE(pLuaResource);
}
