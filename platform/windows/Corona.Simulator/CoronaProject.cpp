//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Simulator.h"
#include "SimulatorView.h"

#include <ctype.h>  // _istalpha()
#include <wincrypt.h>  // CryptProtectData(), CryptUnprotectData()

#include "WinString.h"

#include "Core/Rtt_Build.h"
#include "Core/Rtt_String.h"
#include "Rtt_PlatformAppPackager.h"
#include "CoronaProject.h"

///////////////////////////////////////////////////////////////////////////////
// CCoronaProject
///////////////////////////////////////////////////////////////////////////////

// Constructor with a project path, call Init to read registry
CCoronaProject::CCoronaProject( CString sPath )
:	CCoronaProject()
{
	m_sPath = sPath;
    Init( sPath );
}

// Destructor - no cleanup needed
CCoronaProject::~CCoronaProject(void)
{
}

// Init - use path as registry section, read in stored values if any
void
CCoronaProject::Init( CString sPath )
{
	// Fetch a pointer to the Corona Simulator's main application instance.
	auto appPointer = AfxGetApp();

	// Store the path of the main.lua file which "should" include the file name.
    m_sPath = sPath;

	// Extract the project's path without the file name.
	CString sDirectory = RemoveMainLua( m_sPath );

    // Use lower-case version of path as section
    CString sSection = GetDir();
    sSection.MakeLower();
	sSection.Replace(_T("\\\\"), _T(""));  // Fixes issue with network shares
    RegistryGet( sSection );

    // If no stored name, get project name from last directory name in path
    if( m_sName == REGISTRY_NAME_DEFAULT )
	{
		 int i = sDirectory.ReverseFind( _T('\\') );
		 if( i == -1 )
			 m_sName = sDirectory;
		 else
			 m_sName = sDirectory.Right( sDirectory.GetLength() - i - 1);
	}

	// If an EXE file name is not assigned, then generate one via the application name.
	if (m_sExeFileName.IsEmpty())
	{
		WinString appName;
		appName.SetTCHAR(m_sName);
		if (Rtt_StringEndsWithNoCase(appName.GetUTF8(), ".exe") == false)
		{
			appName.Append(".exe");
		}
		Rtt::String utf8EscapedExeFileName;
		Rtt::PlatformAppPackager::EscapeFileName(appName.GetUTF8(), utf8EscapedExeFileName, false);
		appName.SetUTF8(utf8EscapedExeFileName.GetString());
		m_sExeFileName = appName.GetTCHAR();
	}

	// If the destinatin path is empty, then choose a default path.
	if (m_sSaveDir.IsEmpty())
	{
		// First, attempt to fetch the last build path the used by another project.
		m_sSaveDir = appPointer->GetProfileString(REGISTRY_SECTION, REGISTRY_BUILD_DIR);

		// If we still don't have a path, then this is the first time the user is doing a build.
		// Choose a nice directory under the system's Documents directory by default.
		if (m_sSaveDir.IsEmpty())
		{
			::SHGetFolderPath(nullptr, CSIDL_MYDOCUMENTS, nullptr, 0, m_sSaveDir.GetBuffer(MAX_PATH));
			m_sSaveDir.ReleaseBuffer();
			if (m_sSaveDir.IsEmpty() == false)
			{
				TCHAR lastCharacter = m_sSaveDir.GetAt(m_sSaveDir.GetLength() - 1);
				if ((lastCharacter != _T('\\')) && (lastCharacter != _T('/')))
				{
					m_sSaveDir.Append(_T("\\"));
				}
				m_sSaveDir.Append(_T("Corona Built Apps"));
			}
		}
	}

	// If this is a new project, then attempt to choose a default company name.
	// Note: If the project's existing company name registry entry is blank, then respect that setting.
	if (m_sCompanyName.IsEmpty() && !RegistryEntryExists(sSection, REGISTRY_COMPANY_NAME))
	{
		if (RegistryEntryExists(REGISTRY_SECTION, REGISTRY_COMPANY_NAME))
		{
			// Use the last built app's company name.
			m_sCompanyName = appPointer->GetProfileString(REGISTRY_SECTION, REGISTRY_COMPANY_NAME);
		}
		else
		{
			// Attempt to fetch the company name registered to the Windows system.
			// Note: This will only happen if an app was never built before or the simulator's preferences were deleted.
			const TCHAR kRegistryKeyName[] = _T("Software\\Microsoft\\Windows NT\\CurrentVersion");
			const TCHAR kRegistryEntryName[] = _T("RegisteredOrganization");
			HKEY registryKeyHandle = nullptr;
			::RegOpenKeyEx(HKEY_LOCAL_MACHINE, kRegistryKeyName, 0, KEY_READ | KEY_WOW64_64KEY, &registryKeyHandle);
			if (registryKeyHandle)
			{
				DWORD valueType = REG_NONE;
				DWORD valueByteLength = 0;
				::RegQueryValueEx(registryKeyHandle, kRegistryEntryName, nullptr, &valueType, nullptr, &valueByteLength);
				if ((REG_SZ == valueType) && (valueByteLength > 0))
				{
					::RegQueryValueEx(
							registryKeyHandle, kRegistryEntryName, nullptr, &valueType,
							(LPBYTE)m_sCompanyName.GetBuffer(128), &valueByteLength);
					m_sCompanyName.ReleaseBuffer();
					m_sCompanyName.Trim();
				}
				::RegCloseKey(registryKeyHandle);
			}
		}
	}
}

// Save - use path as registry section, save values
void
CCoronaProject::Save()
{
    // Use all lowercase version of path as registry section
    CString sPath = GetDir();
    sPath.MakeLower();
	sPath.Replace(_T("\\\\"), _T(""));  // Fixes issue with network shares

	// Save settings to the project's registry key.
    RegistryPut( sPath );

	// Save last used build path and company name to the application's global "Preferences" registry key.
	// To be used as defaults for new projects in the future.
	AfxGetApp()->WriteProfileString(REGISTRY_SECTION, REGISTRY_BUILD_DIR, m_sSaveDir);
	AfxGetApp()->WriteProfileString(REGISTRY_SECTION, REGISTRY_COMPANY_NAME, m_sCompanyName);
}

// RegistryGet - read each saved value from the given section
// Passwords are managed separately because they're encrypted
void
CCoronaProject::RegistryGet( CString sSection )
{
	CString stringBuffer;
    CWinApp *pApp = AfxGetApp();

    m_sName = pApp->GetProfileString( sSection, REGISTRY_NAME, REGISTRY_NAME_DEFAULT );
    stringBuffer = pApp->GetProfileString( sSection, REGISTRY_ANDROID_VERSION_CODE, REGISTRY_ANDROID_VERSION_CODE_DEFAULT );
    m_sWin32VersionString = pApp->GetProfileString( sSection, REGISTRY_WIN32_VERSION_STRING, REGISTRY_VERSION_STRING_DEFAULT );
    m_sSaveDir = pApp->GetProfileString( sSection, REGISTRY_SAVEDIR, REGISTRY_SAVEDIR_DEFAULT );
	m_sCopyright = pApp->GetProfileString( sSection, REGISTRY_COPYRIGHT );
	m_sCompanyName = pApp->GetProfileString( sSection, REGISTRY_COMPANY_NAME );
	m_sAppDescription = pApp->GetProfileString( sSection, REGISTRY_APP_DESCRIPTION );
	m_sExeFileName = pApp->GetProfileString( sSection, REGISTRY_EXE_FILE_NAME );
}

// RegistryPut - save each value to the given section
void
CCoronaProject::RegistryPut( CString sSection )
{
	CString stringBuffer;
    CWinApp *pApp = AfxGetApp();

    pApp->WriteProfileString( sSection, REGISTRY_NAME, m_sName );
    pApp->WriteProfileString( sSection, REGISTRY_WIN32_VERSION_STRING, m_sWin32VersionString );
    pApp->WriteProfileString( sSection, REGISTRY_SAVEDIR, m_sSaveDir );
    pApp->WriteProfileString( sSection, REGISTRY_COPYRIGHT, m_sCopyright );
    pApp->WriteProfileString( sSection, REGISTRY_COMPANY_NAME, m_sCompanyName );
    pApp->WriteProfileString( sSection, REGISTRY_APP_DESCRIPTION, m_sAppDescription );
    pApp->WriteProfileString( sSection, REGISTRY_EXE_FILE_NAME, m_sExeFileName );
}

bool
CCoronaProject::RegistryEntryExists( CString sSection, CString sEntryName )
{
	bool wasEntryFound = false;
	HKEY registryKeyHandle = nullptr;
	::RegOpenKeyEx(AfxGetApp()->GetAppRegistryKey(), sSection, 0, KEY_READ, &registryKeyHandle);
	if (registryKeyHandle)
	{
		auto result = ::RegQueryValueEx(registryKeyHandle, sEntryName, nullptr, nullptr, nullptr, nullptr);
		if (ERROR_SUCCESS == result)
		{
			wasEntryFound = true;
		}
		::RegCloseKey(registryKeyHandle);
	}
	return wasEntryFound;
}

// RemoveMainLua - Returns string with \\main.lua removed, if present.
// Does not modify incoming string.
CString
CCoronaProject::RemoveMainLua( CString sPath )
{
    CString sFilename = _T("\\main.lua");

   // remove main.lua from path
    if( sPath.Right( sFilename.GetLength() ) == sFilename )
        sPath = sPath.Left( sPath.GetLength() - sFilename.GetLength() );

    return sPath;
}

// GetDir - return directory of project minus main.lua
CString
CCoronaProject::GetDir()
{
    return RemoveMainLua( m_sPath );
}