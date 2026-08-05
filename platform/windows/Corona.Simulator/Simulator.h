//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
// Simulator.h : header file for CSimulatorApp, CRecentDirList, CSimDocManager, CLuaFileDlg
// main header for Simulator application
//////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"       // main symbols
#include "afxcmn.h"

// CSimulatorApp:
// See Simulator.cpp for the implementation of this class
//

// Define all registry items here
#define REGISTRY_SECTION _T("Preferences")
#define REGISTRY_WORKINGDIR _T("WorkingDir")
#define REGISTRY_DEVICE _T("Device")
#define REGISTRY_CUSTOM_DEVICE_WIDTH _T("CustomDeviceWidth")
#define REGISTRY_CUSTOM_DEVICE_HEIGHT _T("CustomDeviceHeight")
#define REGISTRY_CUSTOM_DEVICE_SAFE_AREA_TOP _T("CustomDeviceSafeAreaTop")
#define REGISTRY_CUSTOM_DEVICE_SAFE_AREA_LEFT _T("CustomDeviceSafeAreaLeft")
#define REGISTRY_CUSTOM_DEVICE_SAFE_AREA_BOTTOM _T("CustomDeviceSafeAreaBottom")
#define REGISTRY_CUSTOM_DEVICE_SAFE_AREA_RIGHT _T("CustomDeviceSafeAreaRight")
#define REGISTRY_XPOS _T("XPos")
#define REGISTRY_YPOS _T("YPos")
#define REGISTRY_LAST_RUN_SUCCEEDED _T("lastRunSucceeded")

// Define all registry item defaults here
#define REGISTRY_CUSTOM_DEVICE_WIDTH_DEFAULT 800
#define REGISTRY_CUSTOM_DEVICE_HEIGHT_DEFAULT 600
#define REGISTRY_CUSTOM_DEVICE_SAFE_AREA_DEFAULT 0
#define CUSTOM_DEVICE_MAXIMUM_DIMENSION 16384
#define REGISTRY_XPOS_DEFAULT 0
#define REGISTRY_YPOS_DEFAULT 0

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSimulatorApp
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CSimulatorApp : public CWinApp
{
public:
	CSimulatorApp();

// Overrides
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	virtual void AddToRecentFileList(LPCTSTR path);

public:
    // Hold values to be written to/from Registry.  CSimulatorView has the real versions.
	CString GetDeviceName() { return m_sDeviceName; }
	void PutDeviceName( CString sDevice )  { m_sDeviceName = sDevice; }
	CRecentFileList* GetRecentFileList() { return m_pRecentFileList; }
	void PutWP(const WINDOWPLACEMENT& newval);
	void GetCustomDeviceSettings(
		int& width, int& height, int& safeAreaTop, int& safeAreaLeft,
		int& safeAreaBottom, int& safeAreaRight);
	void PutCustomDeviceSettings(
		int width, int height, int safeAreaTop, int safeAreaLeft,
		int safeAreaBottom, int safeAreaRight);

	bool IsAgentModeEnabled() { return m_isAgentModeEnabled; }
	bool IsLuaExitAllowed() { return m_isLuaExitAllowed; }
	void SetExitCode(int value) { m_exitCode = value; m_hasExplicitExitCode = true; }
    CString GetWorkingDir();
    void SetWorkingDir( CString sDir );
	CString GetApplicationDir();
	CString GetResourceDir();
	static bool CheckDirExists(LPCTSTR dirName);

protected:
	ULONG_PTR m_gdiplusToken;
	bool m_isGdiPlusInitialized;
    CString m_sDeviceName;
	WINDOWPLACEMENT m_WP;
	bool m_isAgentModeEnabled;
	bool m_isLuaExitAllowed;
	int m_exitCode;
	bool m_hasExplicitExitCode;
	CString m_sApplicationDir;
    CString m_sResourceDir;
};

extern CSimulatorApp theApp;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 
// CRecentDirList - Displays directory rather than "main.lua" in File menu recently used list
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CRecentDirList : public CRecentFileList {
public:
        CRecentDirList(UINT nStart, LPCTSTR lpszSection,
                LPCTSTR lpszEntryFormat, int nSize,
                int nMaxDispLen = AFX_ABBREV_FILENAME_LEN)
                : CRecentFileList(nStart, lpszSection,lpszEntryFormat, nSize,
                nMaxDispLen) {}
        BOOL GetDisplayName( CString &strName, int nIndex, LPCTSTR lpszCurDir, int nCurDir, BOOL bAtLeastName) const;
};  // class CRecentDirList

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSimDocManager - Allows setting initial directory for Open File dialog
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CDocManager is an undocumented MFC class
class CSimDocManager : public CDocManager
{
    public:
       CSimDocManager();
       CSimDocManager( CSimDocManager &);
 
       BOOL DoPromptFileName(CString& fileName, UINT nIDSTitle, DWORD lFlags, BOOL bOpenFileDialog, CDocTemplate* pTemplate);

	   void SetInitialDir( CString sDir ) { m_sInitialDir = sDir; }
	   CString GetInitialDir() { return m_sInitialDir; }     

    protected:
       CString m_sInitialDir;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CLuaFileDialog - Open File Dialog which selects only main.lua
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CLuaFileDialog : public CFileDialog {
public:
	CLuaFileDialog(BOOL bOpenFileDialog = TRUE, // TRUE for FileOpen, FALSE for FileSaveAs
		LPCTSTR lpszDefExt = CLuaFileDialog::szCustomDefExt,
		LPCTSTR lpszFileName = CLuaFileDialog::szCustomDefFileName,
		DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		LPCTSTR lpszFilter = CLuaFileDialog::szCustomDefFilter,
		CWnd* pParentWnd = NULL, 
	    DWORD dwSize = 0,
		BOOL bVistaStyle = TRUE );

static CString szCustomDefFilter;
static CString szCustomDefExt;
static CString szCustomDefFileName;

//	virtual void OnFolderChange( );
	virtual BOOL OnFileNameOK( );

};
