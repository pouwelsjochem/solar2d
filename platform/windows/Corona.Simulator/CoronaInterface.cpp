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
