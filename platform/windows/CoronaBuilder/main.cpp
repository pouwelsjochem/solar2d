//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

// #pragma comment(lib, "Lua.Library.Win32\\lua.lib")

#include "stdafx.h"

#include "Core/Rtt_Build.h"

#include "Rtt_CoronaBuilder.h"
#include "Rtt_WinConsolePlatform.h"
#include "Rtt_WinPlatformServices.h"

#include "Rtt_String.h"

int main( int argc, const char *argv[] )
{
	using namespace Rtt;

	int result = 0;                                                                                                                                                                                                                                                                                      
	CString resourceDir = _wgetenv(_T("CORONA_PATH"));

	if (resourceDir.GetLength() == 0)
	{
		TCHAR builderPath[MAX_PATH];
		GetModuleFileName(NULL, builderPath, MAX_PATH);
		TCHAR* end = StrRChr(builderPath, NULL, '\\');
		if (!end) return 42;
		end++;
		StrCpy(end, _T("7za.exe"));
		if (INVALID_FILE_ATTRIBUTES != GetFileAttributes(builderPath))
		{
			end[0] = 0;
			_wputenv_s(_T("CORONA_PATH"), builderPath);
		} 
		else
		{
			StrCpy(end, _T("..\\..\\..\\..\\7za.exe"));
			if (INVALID_FILE_ATTRIBUTES != GetFileAttributes(builderPath))
			{
				end[12] = 0;
				_wputenv_s(_T("CORONA_PATH"), builderPath);
			}
			else
			{
				fprintf(stderr, "CoronaBuilder: cannot find a setting for CORONA_PATH.  Install Corona first\n");
				return 1;
			}
		}
	}

	resourceDir.TrimRight(_T("\\"));
	resourceDir.Append(_T("\\Resources"));

	WinConsolePlatform *platform = new WinConsolePlatform;
	Rtt::WinPlatformServices *services = new Rtt::WinPlatformServices(*platform);
	CoronaBuilder builder( *platform, *services );
	result = builder.Main( argc, argv );

	delete services;
	delete platform;

	return result;
}

// ----------------------------------------------------------------------------
