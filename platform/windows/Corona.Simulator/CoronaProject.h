//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Rtt_TargetDevice.h"


// CCoronaProjec

// Registry entry names.
#define REGISTRY_NAME _T("Name")
#define REGISTRY_WIN32_VERSION_STRING _T("Win32VersionString")
#define REGISTRY_PACKAGE _T("Package")
#define REGISTRY_SAVEDIR _T("SaveDir")
#define REGISTRY_BUILD_DIR _T("BuildDir")
#define REGISTRY_COPYRIGHT _T("Copyright")
#define REGISTRY_COMPANY_NAME _T("CompanyName")
#define REGISTRY_APP_DESCRIPTION _T("AppDescription")
#define REGISTRY_EXE_FILE_NAME _T("ExeFileName")

// Registry defaults for each individual project.
#define REGISTRY_NAME_DEFAULT _T("")
#define REGISTRY_VERSION_STRING_DEFAULT _T("1.0.0")
#define REGISTRY_PACKAGE_DEFAULT _T("")
#define REGISTRY_SAVEDIR_DEFAULT _T("")

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

	CString GetWin32VersionString() { return m_sWin32VersionString; }
	void SetWin32VersionString(const CString& value) { m_sWin32VersionString = value; }

	CString GetSaveDir() { return m_sSaveDir; }
	void SetSaveDir( CString sPath ) { m_sSaveDir = sPath; }

	CString GetCopyright() { return m_sCopyright; }
	void SetCopyright(const CString& value) { m_sCopyright = value; }

	CString GetCompanyName() { return m_sCompanyName; }
	void SetCompanyName(const CString& value) { m_sCompanyName = value; }

	CString GetAppDescription() { return m_sAppDescription; }
	void SetAppDescription(const CString& value) { m_sAppDescription = value; }

	CString GetExeFileName() { return m_sExeFileName; }
	void SetExeFileName(const CString& value) { m_sExeFileName = value; }

protected:
    void RegistryGet( CString sSection );
    void RegistryPut( CString sSection );
	bool RegistryEntryExists( CString sSection, CString sEntryName );

protected:
    CString m_sPath;  // full path, incl. main.lua
    CString m_sName;  // default dir name
	CString m_sWin32VersionString;
    CString m_sSaveDir;
    CString m_sTargetOS;
	CString m_sCopyright;
	CString m_sCompanyName;
	CString m_sAppDescription;
	CString m_sExeFileName;
};

