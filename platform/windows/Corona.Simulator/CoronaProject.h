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

// Registry entry names.
#define REGISTRY_NAME _T("Name")

// Registry defaults for each individual project.
#define REGISTRY_NAME_DEFAULT _T("")

///////////////////////////////////////////////////////////////////////////////
// CCoronaProject
///////////////////////////////////////////////////////////////////////////////
class CCoronaProject :
	public CObject
{
public:
	CCoronaProject();
	CCoronaProject( CString sPath );
  	virtual ~CCoronaProject();

	static CString RemoveMainLua( CString sPath );  // for RecentlyUsed menu items

public:
    void Init( CString sPath );
    void Save();

	CString GetPath() { return m_sPath; }
	CString GetDir();  // computed from path, no "SetDir()"

	CString GetName()  { return m_sName; }
	void SetName( CString sName ) { m_sName = sName; }

protected:
    void RegistryGet( CString sSection );
    void RegistryPut( CString sSection );

protected:
    CString m_sPath;  // full path, incl. main.lua
    CString m_sName;  // default dir name
};
