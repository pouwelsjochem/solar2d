//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "CoronaProject.h"
#include "Simulator.h"

///////////////////////////////////////////////////////////////////////////////
// CCoronaProject
///////////////////////////////////////////////////////////////////////////////
CCoronaProject::CCoronaProject()
{
}

// Constructor with a project path, call Init to read registry
CCoronaProject::CCoronaProject( CString sPath )
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
	// Store the path of the main.lua file which "should" include the file name.
    m_sPath = sPath;

	// Extract the project's path without the file name.
	CString sDirectory = RemoveMainLua( m_sPath );

    // Use lower-case version of path as section
    CString sSection = GetDir();
    sSection.MakeLower();
	sSection.Replace(_T("\\\\"), _T(""));  // Fixes issue with network shares
	if (!((CSimulatorApp*)AfxGetApp())->IsAgentModeEnabled())
	{
		RegistryGet(sSection);
	}

    // If no stored name, get project name from last directory name in path
    if( m_sName == REGISTRY_NAME_DEFAULT )
	{
		 int i = sDirectory.ReverseFind( _T('\\') );
		 if( i == -1 )
			 m_sName = sDirectory;
		 else
			 m_sName = sDirectory.Right( sDirectory.GetLength() - i - 1);
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
}

// RegistryGet - read each saved value from the given section
// Passwords are managed separately because they're encrypted
void
CCoronaProject::RegistryGet( CString sSection )
{
    CWinApp *pApp = AfxGetApp();

    m_sName = pApp->GetProfileString( sSection, REGISTRY_NAME, REGISTRY_NAME_DEFAULT );
}

// RegistryPut - save each value to the given section
void
CCoronaProject::RegistryPut( CString sSection )
{
    CWinApp *pApp = AfxGetApp();

    pApp->WriteProfileString( sSection, REGISTRY_NAME, m_sName );
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
