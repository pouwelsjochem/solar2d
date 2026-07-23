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

///////////////////////////////////////////////////////////////////////////////
// CCoronaProject
///////////////////////////////////////////////////////////////////////////////
CCoronaProject::CCoronaProject()
{
}

// Destructor - no cleanup needed
CCoronaProject::~CCoronaProject(void)
{
}

// Init - store the project path.
void
CCoronaProject::Init( CString sPath )
{
	// Store the path of the main.lua file which "should" include the file name.
    m_sPath = sPath;
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
