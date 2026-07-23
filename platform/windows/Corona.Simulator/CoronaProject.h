//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

// CCoronaProjec

///////////////////////////////////////////////////////////////////////////////
// CCoronaProject
///////////////////////////////////////////////////////////////////////////////
class CCoronaProject :
	public CObject
{
public:
	CCoronaProject();
  	virtual ~CCoronaProject();

	static CString RemoveMainLua( CString sPath );  // for RecentlyUsed menu items

public:
    void Init( CString sPath );

	CString GetPath() { return m_sPath; }
	CString GetDir();  // computed from path, no "SetDir()"

protected:
    CString m_sPath;  // full path, incl. main.lua
};
