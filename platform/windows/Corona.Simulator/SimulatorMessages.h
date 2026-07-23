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


// Messages processed by CSimulatorView.
#define WMU_NATIVEALERT (WM_APP + 101)
#define WMU_APPLY_SIMULATOR_CONFIGURATION (WM_APP + 102)

// Parameters passed to WMU_NATIVEALERT. Strings are UTF-8.
typedef struct wmu_alert_params
{
	const char *sTitle;
	const char *sMsg;
	int nButtonLabels;
	const char **psButtonLabels;
	void *pLuaResource;
	HWND hwnd;
} WMU_ALERT_PARAMS;
