//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <gdiplus.h>
#include <shellapi.h>
#include <string>
#include <vector>

#include "Simulator.h"
#include "MainFrm.h"

#include "CoronaProject.h"
#include "SimulatorDocTemplate.h"
#include "SimulatorView.h"

#include "resource.h"

#include "Core/Rtt_Build.h"
#include "Rtt_SimulatorControl.h"


#ifdef _DEBUG
#	define new DEBUG_NEW
#endif


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Single Instance Control
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Name of the semaphore used to control the number of instances running
LPCTSTR SINGLE_INSTANCE_OBJECT_NAME = _T("com.coronalabs.windows.single_instance.v1");
 
// This data is shared by all instances, so that when we
// detect that we have another instance running, we can use
// this to send messages to the window of that first instance:
#pragma data_seg (".SingleInstance")
LONG  SimFirstInstanceHwnd = 0;
// If you want to add anything here the crucial thing to remember is that 
// any shared data items must be initialized to some value
#pragma data_seg ()

// Tell the linker about our shared data segment
#pragma comment(linker, "/section:.SingleInstance,RWS")


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Static Member Variables
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// <summary>The one and only CSimulatorApp object.</summary>
CSimulatorApp theApp;

// CSimulatorApp initialization
// Copied from appcore.cpp
AFX_STATIC_DATA const TCHAR _afxFileSection[] = _T("Recent File List");
AFX_STATIC_DATA const TCHAR _afxFileEntry[] = _T("File%d");

class CSimulatorCommandLineInfo : public CCommandLineInfo
{
	public:
		CSimulatorCommandLineInfo()
		:	IsAgentModeEnabled(false),
			IsValid(true),
			fPendingValue(kNoPendingValue)
		{
		}

		virtual void ParseParam(const TCHAR* value, BOOL isFlag, BOOL isLast) override
		{
			if (fPendingValue != kNoPendingValue)
			{
				if (isFlag)
				{
					IsValid = false;
				}
				else if (fPendingValue == kAgentModeValue)
				{
					CString normalizedValue(value);
					normalizedValue.MakeLower();
					if (normalizedValue == _T("yes") || normalizedValue == _T("true") || normalizedValue == _T("1"))
					{
						IsAgentModeEnabled = true;
					}
					else if (normalizedValue == _T("no") || normalizedValue == _T("false") || normalizedValue == _T("0"))
					{
						IsAgentModeEnabled = false;
					}
					else
					{
						IsValid = false;
					}
				}
				else if (fPendingValue == kSimulatorControlDirectoryValue)
				{
					SimulatorControlDirectory = value;
				}
				else
				{
					m_nShellCommand = CCommandLineInfo::FileOpen;
					m_strFileName = value;
					IsProjectExplicit = true;
				}
				fPendingValue = kNoPendingValue;
				return;
			}

			if (isFlag)
			{
				CString normalizedValue(value);
				normalizedValue.MakeLower();
				if (normalizedValue == _T("agent-mode"))
				{
					fPendingValue = kAgentModeValue;
					if (isLast)
					{
						IsValid = false;
					}
					return;
				}
				if (normalizedValue == _T("project"))
				{
					fPendingValue = kProjectValue;
					if (isLast)
					{
						IsValid = false;
					}
					return;
				}
				if (normalizedValue == _T("simulator-control-dir"))
				{
					fPendingValue = kSimulatorControlDirectoryValue;
					if (isLast)
					{
						IsValid = false;
					}
					return;
				}
				if (normalizedValue == _T("singleton") || normalizedValue == _T("debug") ||
					normalizedValue == _T("allowluaexit"))
				{
					return;
				}
			}
			else
			{
				IsProjectExplicit = true;
			}
			CCommandLineInfo::ParseParam(value, isFlag, isLast);
		}

		bool IsAgentModeEnabled;
		bool IsProjectExplicit = false;
		bool IsValid;
		CString SimulatorControlDirectory;

	private:
		enum PendingValue
		{
			kNoPendingValue,
			kAgentModeValue,
			kProjectValue,
			kSimulatorControlDirectoryValue
		};
		PendingValue fPendingValue;
};

static std::string Utf8FromSimulatorControlWideString(const wchar_t* value)
{
	if (!value || !value[0])
	{
		return std::string();
	}
	int length = ::WideCharToMultiByte(
		CP_UTF8, 0, value, -1, nullptr, 0, nullptr, nullptr);
	if (length <= 1)
	{
		return std::string();
	}
	std::vector<char> buffer(length);
	::WideCharToMultiByte(
		CP_UTF8, 0, value, -1, buffer.data(), length, nullptr, nullptr);
	return std::string(buffer.data());
}

static bool TryRunSimulatorControlClient(int& exitCode)
{
	int argumentCount = 0;
	LPWSTR* wideArguments =
		::CommandLineToArgvW(::GetCommandLineW(), &argumentCount);
	if (!wideArguments)
	{
		return false;
	}

	std::vector<std::string> arguments;
	for (int index = 0; index < argumentCount; index++)
	{
		arguments.push_back(
			Utf8FromSimulatorControlWideString(wideArguments[index]));
	}
	::LocalFree(wideArguments);

	std::vector<const char*> argumentPointers;
	for (std::vector<std::string>::const_iterator iterator = arguments.begin();
		iterator != arguments.end(); iterator++)
	{
		argumentPointers.push_back(iterator->c_str());
	}
	return 0 != Rtt_RunSimulatorControlClient(
		(int)argumentPointers.size(), argumentPointers.data(), &exitCode);
}

static bool IsAgentModeRequested()
{
	int argumentCount = 0;
	LPWSTR* arguments = ::CommandLineToArgvW(::GetCommandLineW(), &argumentCount);
	bool result = false;
	for (int index = 1; arguments && index < argumentCount; index++)
	{
		CString argument(arguments[index]);
		argument.MakeLower();
		argument.Replace(TCHAR('/'), TCHAR('-'));
		if (argument == _T("-agent-mode"))
		{
			result = true;
			if (index + 1 < argumentCount)
			{
				CString value(arguments[++index]);
				value.MakeLower();
				if (value == _T("no") || value == _T("false") || value == _T("0"))
				{
					result = false;
				}
			}
			break;
		}
	}
	if (arguments)
	{
		::LocalFree(arguments);
	}
	return result;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSimulatorApp
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CSimulatorApp::CSimulatorApp()
{
	// Place all significant initialization in InitInstance
	m_isGdiPlusInitialized = false;
	m_isDebugModeEnabled = false;
	m_isAgentModeEnabled = false;
	m_isLuaExitAllowed = false;
	m_exitCode = 0;
	m_hasExplicitExitCode = false;
}

// InitInstance - initialize the application
BOOL CSimulatorApp::InitInstance()
{
	const bool wasAgentModeRequested = IsAgentModeRequested();
	m_isAgentModeEnabled = wasAgentModeRequested;

	// Don't buffer stdout and stderr. Agent mode relies on the launching terminal for logs.
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	int simulatorControlExitCode = 0;
	if (TryRunSimulatorControlClient(simulatorControlExitCode))
	{
		m_isAgentModeEnabled = true;
		m_exitCode = simulatorControlExitCode;
		m_hasExplicitExitCode = true;
		return FALSE;
	}

	// Load the simulator version of the Corona library, which is only used by plugins to link against by name.
	// This is a thin proxy DLL which forwards Solar2D's public APIs to this EXE's statically linked Solar2D APIs.
	// This ensures that plugins link with the simulator's library and not the non-simulator version of the library.
	CString coronaLibraryPath = GetApplicationDir() + _T("\\Resources\\CoronaLabs.Corona.Native.dll");
	if (!Rtt_VERIFY(::LoadLibrary(coronaLibraryPath)))
	{
		CString message =
			_T("Failed to load the Solar2D Simulator's library.\r\n")
			_T("This might mean that your Solar2D installation is corrupted.\r\n")
			_T("You may be able to fix this by re-installing the Solar2D.");
		if (m_isAgentModeEnabled)
		{
			fwprintf(stderr, L"%ls\n", (LPCWSTR)message);
			m_exitCode = 1;
		}
		else
		{
			AfxMessageBox(message, MB_OK | MB_ICONEXCLAMATION);
		}
		return FALSE;
	}

	CWinApp::InitInstance();

	// Hacks to make life easier
	if (_wgetenv(L"CORONA_PATH") == NULL) {
		TCHAR coronaDir[MAX_PATH];
		GetModuleFileName(NULL, coronaDir, MAX_PATH);
		TCHAR* end = StrRChr(coronaDir, NULL, '\\');
		if (end) 
		{
			end[1] = 0;
			_wputenv_s(L"CORONA_PATH", coronaDir);
		}

	}

	SetRegistryKey(_T("Ansca Corona"));
	m_pszProfileName = _tcsdup(_T("Corona Simulator"));

	CSimulatorCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);
	m_isAgentModeEnabled = cmdInfo.IsAgentModeEnabled;
	if (!cmdInfo.IsValid)
	{
		m_isAgentModeEnabled = wasAgentModeRequested;
		m_exitCode = 1;
		fprintf(
			stderr,
			"ERROR: -agent-mode, -project, and -simulator-control-dir "
			"require valid values.\n");
		return FALSE;
	}
	std::string simulatorControlDirectory = Utf8FromSimulatorControlWideString(
		(LPCWSTR)cmdInfo.SimulatorControlDirectory);
	Rtt::SimulatorControl::SetDirectory(
		simulatorControlDirectory.c_str());

	// See if we ran successfully without a crash last time (mostly
	// used to detect crappy video drivers that crash us)
	int lastRunSucceeded = m_isAgentModeEnabled ? 1 :
		GetProfileInt(REGISTRY_SECTION, REGISTRY_LAST_RUN_SUCCEEDED, 1);

	if (!lastRunSucceeded)
	{
		CString message =
			_T("Solar2D Simulator crashed last time it was run\n\n")
			_T("This can happen because Windows or the video driver need to be updated.  ")
			_T("If it crashes again, make sure the software for your video card is up to date (you ")
			_T("may need to visit the manufacturer's web site to check this) and ensure that all ")
			_T("available Windows updates have been installed.\n\n")
			_T("If the problem persists, contact support@solar2d.com including as much detail as possible.");

		SHMessageBoxCheck(NULL,
			message,
			TEXT("Solar2D Simulator"),
			MB_OK | MB_ICONEXCLAMATION,
			IDOK,
			L"CoronaShowCrashWarning");
	}
	else if (!m_isAgentModeEnabled)
	{
#ifndef Rtt_DEBUG
		// Set the telltale to 0 so we'll know if we don't reset it in ExitInstance()
		// (but don't do it for debug builds which are probably running in the debugger)
		WriteProfileInt(REGISTRY_SECTION, REGISTRY_LAST_RUN_SUCCEEDED, 0);
#endif
	}

	// Parse for command line arguments.
	// Supports arguments starting with '-' and '/'. Arguments are not case sensitive.
	CString commandLine = m_lpCmdLine;
	commandLine.MakeLower();
	commandLine.Replace(TCHAR('/'), TCHAR('-'));

	// Have we been asked to run just a single instance at a time?
	if (!m_isAgentModeEnabled && commandLine.Find(_T("-singleton")) >= 0)
	{
		// Check whether we have already an instance running
		// (even if we get ERROR_ALREADY_EXISTS we increment the reference count so the semaphore
		// will exist so long as there is a running Simulator)
		HANDLE hSingleInstanceSemaphore = ::CreateSemaphore(NULL, 0, 1, SINGLE_INSTANCE_OBJECT_NAME);
		DWORD err = ::GetLastError();

		if (err == ERROR_ALREADY_EXISTS)
		{
			// The semaphore exists already, must have been created by a previous instance, tell it to quit 
			HWND windowHandle = (HWND)SimFirstInstanceHwnd;
			ASSERT(windowHandle != 0);
			if (::IsWindow(windowHandle))
			{
				// Fetch the other single instance app's process handle.
				HANDLE processHandle = nullptr;
				DWORD processId = 0;
				::GetWindowThreadProcessId(windowHandle, &processId);
				if (processId)
				{
					processHandle = ::OpenProcess(SYNCHRONIZE, FALSE, processId);
				}

				// Tell the other single instance app to quit, making this app instance the single instance.
				if (::IsWindowEnabled(windowHandle))
				{
					// The app isn't displaying a modal dialog.
					// This means we can close it gracefully with a close message.
					::PostMessage(windowHandle, WM_CLOSE, 0, 0);
				}
				else
				{
					// *** The app is currently displaying a modal dialog. ***

					// Send a quit message to exit the app. (Not a clean way to exit an app.)
					// Note: Child dialogs and Doc/View will not receive close messages because of this.
					::PostMessage(windowHandle, WM_QUIT, 0, 0);
				}

				// Wait for the other app to exit before continuing.
				// This way it's last saved registry settings will be available to this app.
				if (processHandle)
				{
					::WaitForSingleObject(processHandle, 5000);
					::CloseHandle(processHandle);
				}
			}
		}
	}

	// Initialize GDIplus early to avoid crashes when double-clicking on a .lua file
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	m_isGdiPlusInitialized =
		Gdiplus::Ok == Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);

	// Handle the rest of the command line arguments.
	if (commandLine.Find(_T("-debug")) >= 0)
	{
		m_isDebugModeEnabled = true;
	}
	if (commandLine.Find(_T("-allowluaexit")) >= 0)
	{
		m_isLuaExitAllowed = true;
	}
	// Stop MFC from flashing the simulator window on startup.
	m_nCmdShow = SW_HIDE;

	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);
	AfxInitRichEdit();
	AfxEnableControlContainer();

	int maxRecentFileCount = 10;
	if (!m_isAgentModeEnabled)
	{
		LoadStdProfileSettings(maxRecentFileCount);  // Load standard INI file options (including MRU)
	}

    // Delete the m_pRecentFileList created in the LoadStdProfileSettings.
	// We want to show directory name, not filename (which is always main.lua)
    delete m_pRecentFileList;

    // The nSize argument of the constructor is set to four because the  
    // LoadStdProfileSettings takes a default of four.  If you specify a  
    // different value for the nMaxMRU argument you need to change the
    // nSize argument for the constructor call.
    m_pRecentFileList = new CRecentDirList(0, _afxFileSection, _afxFileEntry, maxRecentFileCount);
	if (!m_isAgentModeEnabled)
	{
		m_pRecentFileList->ReadList();
	}

    // Override default CDocManager class to manage initial directory for open file dialog
    // Initial directory set below
    delete m_pDocManager;
    m_pDocManager = new CSimDocManager();

	// Register the simulator's document template.
	// This is used to manage an open Solar2D project with an MFC SDI document/view interface.
	// Note: This custom doc template allows the simulator to open Solar2D projects by directory or "main.lua".
	auto pDocTemplate = new CSimulatorDocTemplate();
	if (!pDocTemplate)
	{
		m_exitCode = m_isAgentModeEnabled ? 1 : m_exitCode;
		return FALSE;
	}
	AddDocTemplate(pDocTemplate);

    // Do this before any of the ways app can exit (including not authorized)
	printf("\nSolar2D Simulator %s (%s %s)\n\n", Rtt_STRING_BUILD, __DATE__, __TIME__);

	// Load user preferences from registry
    // Initialize member variables used to write out preferences
	SetWorkingDir(m_isAgentModeEnabled ? GetResourceDir() :
		GetProfileString(REGISTRY_SECTION, REGISTRY_WORKINGDIR, GetResourceDir()));
	m_sDeviceName = m_isAgentModeEnabled ? _T("") :
		GetProfileString(REGISTRY_SECTION, REGISTRY_DEVICE, _T(""));

	// Fake out command line since we want to automatically open the last open project.
	// This is necessary because ProcessShellCommand() initiates a "New File"
	// operation if there is no filename on the command line and this is very
	// hard to unravel and inject the remembered filename into.
	if (!m_isAgentModeEnabled && !m_isDebugModeEnabled &&
		(cmdInfo.m_nShellCommand == CCommandLineInfo::FileNew))
	{
		CRecentFileList *recentFileListPointer = GetRecentFileList();
		if (recentFileListPointer && (recentFileListPointer->GetSize() > 0))
		{
			auto lastRanFilePath = (*recentFileListPointer)[0];
			if (!lastRanFilePath.IsEmpty() && ::PathFileExists(lastRanFilePath))
			{
				// We have a remembered last project so pretend we were asked to open it on the command line.
				cmdInfo.m_nShellCommand = CCommandLineInfo::FileOpen;
				cmdInfo.m_strFileName = lastRanFilePath;
			}
		}
	}

	// If a Solar2D project directory was provided at the command line, then append a "main.lua" file to the path.
	if (!cmdInfo.m_strFileName.IsEmpty() && ::PathIsDirectory(cmdInfo.m_strFileName))
	{
		TCHAR mainLuaFilePath[2048];
		mainLuaFilePath[0] = _T('\0');
		::PathCombine(mainLuaFilePath, cmdInfo.m_strFileName, _T("main.lua"));
		if (::PathFileExists(mainLuaFilePath))
		{
			cmdInfo.m_strFileName = mainLuaFilePath;
		}
		else if (m_isAgentModeEnabled)
		{
			cmdInfo.m_strFileName.Empty();
		}
	}
	if (m_isAgentModeEnabled &&
		(!cmdInfo.IsProjectExplicit || cmdInfo.m_strFileName.IsEmpty() ||
		 !::PathFileExists(cmdInfo.m_strFileName) ||
		 _tcsicmp(::PathFindFileName(cmdInfo.m_strFileName), _T("main.lua")) != 0))
	{
		fprintf(stderr, "ERROR: Agent mode requires an existing Solar2D project via -project <path>.\n");
		m_exitCode = 1;
		return FALSE;
	}

	// Dispatch commands specified on the command line.  Will return FALSE if
	// app was launched with /RegServer, /Register, /Unregserver or /Unregister.
	// This will cause a new "document" to be opened and most initialization to take place
	if (!ProcessShellCommand(cmdInfo))
	{
		m_exitCode = m_isAgentModeEnabled ? 1 : m_exitCode;
		return FALSE;
	}

    // If Resources dir doesn't exist, exit.
	CString sDir = GetResourceDir();
    if (!CheckDirExists(sDir))
	{
        CString msg;
        msg.Format( IDS_DIR_s_NOTFOUND_INSTALL, sDir );
		if (m_isAgentModeEnabled)
		{
			fwprintf(stderr, L"%ls\n", (LPCWSTR)msg);
			m_exitCode = 1;
		}
		else
		{
			::AfxMessageBox(msg);
		}
        return FALSE;
	}

	// Get the current window size (it's been calculated from the current app)
	CRect cwr;
	m_pMainWnd->GetWindowRect(&cwr);
	m_WP.rcNormalPosition.left = m_isAgentModeEnabled ? REGISTRY_XPOS_DEFAULT :
		GetProfileInt(REGISTRY_SECTION, REGISTRY_XPOS, REGISTRY_XPOS_DEFAULT);
	m_WP.rcNormalPosition.top = m_isAgentModeEnabled ? REGISTRY_YPOS_DEFAULT :
		GetProfileInt(REGISTRY_SECTION, REGISTRY_YPOS, REGISTRY_YPOS_DEFAULT);
	// Don't use any remembered size as it might not be for the same app
	m_WP.rcNormalPosition.right = m_WP.rcNormalPosition.left + (cwr.right - cwr.left); 
	m_WP.rcNormalPosition.bottom = m_WP.rcNormalPosition.top + (cwr.bottom - cwr.top);
	m_WP.length = sizeof(WINDOWPLACEMENT);
    const WINDOWPLACEMENT * wp = &m_WP;
	// Places the window on the screen even if the information in WINDOWPLACEMENT puts the window off screen
	if (!m_isAgentModeEnabled)
	{
		SetWindowPlacement(*m_pMainWnd, wp);
	}

	// Remember the main window's HWND in the shared memory area so other instances
	// can find it and tell us to do things
	SimFirstInstanceHwnd = (LONG) (m_pMainWnd->GetSafeHwnd());

	// Set main window size based on device
    CMainFrame *pMainFrm = (CMainFrame *)m_pMainWnd;
	CSimulatorView *pView = (CSimulatorView *)pMainFrm->GetActiveView();
	if (m_isAgentModeEnabled)
	{
		m_pMainWnd->SetMenu(nullptr);
	}

	// The one and only window has been initialized, so show and update it
	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();
	// call DragAcceptFiles only if there's a suffix
	//  In an SDI app, this should occur after ProcessShellCommand
	// Enable drag/drop open
	m_pMainWnd->DragAcceptFiles();

	// This application has been initialized and is authorized to continue.
	// Returning TRUE allows this application to continue running.
	return TRUE;
}

/// Gets this application's absolute path without the file name.
CString CSimulatorApp::GetApplicationDir()
{
	// Fetch the app's absolute path without the file name, if not done so already.
	if (m_sApplicationDir.IsEmpty())
	{
		const int MAX_PATH_LENGTH = 1024;
		CString applicationPath;
		int result;
		int index;
		result = ::GetModuleFileName(nullptr, applicationPath.GetBuffer(MAX_PATH_LENGTH), MAX_PATH_LENGTH);
		applicationPath.ReleaseBuffer((int)result);
		if (result > 0)
		{
			// Remove the file name from the path string.
			index = applicationPath.ReverseFind(_T('\\'));
			if (index > 0)
			{
				m_sApplicationDir = applicationPath.Left(index);
			}
		}
	}

	// Return this application's absolute path.
	return m_sApplicationDir;
}

// GetResourceDir - Return directory for skin image files, and other needed files.
// Path is AppPath\Resources
CString CSimulatorApp::GetResourceDir()
{
    if (m_sResourceDir.IsEmpty())
	{
		m_sResourceDir = GetApplicationDir();
		m_sResourceDir += _T("\\Resources");
	}
    return m_sResourceDir;
}

// CheckDirExists - return true if dirName is directory and exists
// Make sure paths don't have trailing backslashes
bool CSimulatorApp::CheckDirExists(LPCTSTR dirName)
{ 
	WIN32_FIND_DATA  data; 
	HANDLE handle = FindFirstFile(dirName,&data); 
	if (handle != INVALID_HANDLE_VALUE)
	{
		FindClose(handle);
		return (0 != (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY));
	}
	return false;
}

// GetWorkingDir - return the directory to use for File Open dialogs
// Used to save value to registry on exit
CString CSimulatorApp::GetWorkingDir() 
{ 
	return ((CSimDocManager *)m_pDocManager)->GetInitialDir();
}

// SetWorkingDir - set the directory to use for File Open dialogs
// Remembered in registry on app exit/init, and set when project is opened
void CSimulatorApp::SetWorkingDir( CString sDir ) 
{ 
	((CSimDocManager *)m_pDocManager)->SetInitialDir( sDir );
}

// ExitInstance - save position, etc. to registry
int CSimulatorApp::ExitInstance()
{
	if (m_isAgentModeEnabled)
	{
		if (m_isGdiPlusInitialized)
		{
			Gdiplus::GdiplusShutdown(m_gdiplusToken);
			m_isGdiPlusInitialized = false;
		}
		return m_exitCode;
	}

	// Check if we're exiting before we initialized.
	if (m_pDocManager == nullptr)
	{
		return CWinApp::ExitInstance();
	}

    // Save user preferences to registry
    if (!GetDeviceName().IsEmpty())
	{
		WriteProfileString( REGISTRY_SECTION, REGISTRY_DEVICE, GetDeviceName());
	}
    if (!GetWorkingDir().IsEmpty())
	{
		WriteProfileString( REGISTRY_SECTION, REGISTRY_WORKINGDIR, GetWorkingDir() );
	}

    // Write window position (saved in CMainFrame::OnClose)
	WriteProfileInt(REGISTRY_SECTION, REGISTRY_XPOS, m_WP.rcNormalPosition.left);
    WriteProfileInt( REGISTRY_SECTION, REGISTRY_YPOS, m_WP.rcNormalPosition.top);

    // Uninitialize GDIplus
	if (m_isGdiPlusInitialized)
	{
		Gdiplus::GdiplusShutdown(m_gdiplusToken);
		m_isGdiPlusInitialized = false;
	}

	// If we got this far, we ran successfully without a crash (mostly
	// used to detect crappy video drivers that crash us)
	WriteProfileInt(REGISTRY_SECTION, REGISTRY_LAST_RUN_SUCCEEDED, 1);

	// Exit this application.
	int defaultExitCode = CWinApp::ExitInstance();
	return m_hasExplicitExitCode ? m_exitCode : defaultExitCode;
}

void CSimulatorApp::AddToRecentFileList(LPCTSTR path)
{
	if (!m_isAgentModeEnabled)
	{
		CWinApp::AddToRecentFileList(path);
	}
}

// PutWP - save window placement so it can be written to registry
void CSimulatorApp::PutWP(const WINDOWPLACEMENT& newval)
{	
	m_WP = newval;
	m_WP.length = sizeof(m_WP);
}

void CSimulatorApp::GetCustomDeviceSettings(
	int& width, int& height, int& safeAreaTop, int& safeAreaLeft,
	int& safeAreaBottom, int& safeAreaRight)
{
	width = GetProfileInt(
		REGISTRY_SECTION, REGISTRY_CUSTOM_DEVICE_WIDTH, REGISTRY_CUSTOM_DEVICE_WIDTH_DEFAULT);
	height = GetProfileInt(
		REGISTRY_SECTION, REGISTRY_CUSTOM_DEVICE_HEIGHT, REGISTRY_CUSTOM_DEVICE_HEIGHT_DEFAULT);
	safeAreaTop = GetProfileInt(
		REGISTRY_SECTION, REGISTRY_CUSTOM_DEVICE_SAFE_AREA_TOP, REGISTRY_CUSTOM_DEVICE_SAFE_AREA_DEFAULT);
	safeAreaLeft = GetProfileInt(
		REGISTRY_SECTION, REGISTRY_CUSTOM_DEVICE_SAFE_AREA_LEFT, REGISTRY_CUSTOM_DEVICE_SAFE_AREA_DEFAULT);
	safeAreaBottom = GetProfileInt(
		REGISTRY_SECTION, REGISTRY_CUSTOM_DEVICE_SAFE_AREA_BOTTOM, REGISTRY_CUSTOM_DEVICE_SAFE_AREA_DEFAULT);
	safeAreaRight = GetProfileInt(
		REGISTRY_SECTION, REGISTRY_CUSTOM_DEVICE_SAFE_AREA_RIGHT, REGISTRY_CUSTOM_DEVICE_SAFE_AREA_DEFAULT);

	if (width <= 0 || width > CUSTOM_DEVICE_MAXIMUM_DIMENSION)
	{
		width = REGISTRY_CUSTOM_DEVICE_WIDTH_DEFAULT;
	}
	if (height <= 0 || height > CUSTOM_DEVICE_MAXIMUM_DIMENSION)
	{
		height = REGISTRY_CUSTOM_DEVICE_HEIGHT_DEFAULT;
	}
	if (safeAreaTop < 0 || safeAreaLeft < 0 || safeAreaBottom < 0 || safeAreaRight < 0 ||
		safeAreaTop > CUSTOM_DEVICE_MAXIMUM_DIMENSION ||
		safeAreaLeft > CUSTOM_DEVICE_MAXIMUM_DIMENSION ||
		safeAreaBottom > CUSTOM_DEVICE_MAXIMUM_DIMENSION ||
		safeAreaRight > CUSTOM_DEVICE_MAXIMUM_DIMENSION ||
		safeAreaTop + safeAreaBottom > height || safeAreaLeft + safeAreaRight > width)
	{
		safeAreaTop = REGISTRY_CUSTOM_DEVICE_SAFE_AREA_DEFAULT;
		safeAreaLeft = REGISTRY_CUSTOM_DEVICE_SAFE_AREA_DEFAULT;
		safeAreaBottom = REGISTRY_CUSTOM_DEVICE_SAFE_AREA_DEFAULT;
		safeAreaRight = REGISTRY_CUSTOM_DEVICE_SAFE_AREA_DEFAULT;
	}
}

void CSimulatorApp::PutCustomDeviceSettings(
	int width, int height, int safeAreaTop, int safeAreaLeft,
	int safeAreaBottom, int safeAreaRight)
{
	if (width <= 0 || height <= 0 ||
		width > CUSTOM_DEVICE_MAXIMUM_DIMENSION || height > CUSTOM_DEVICE_MAXIMUM_DIMENSION ||
		safeAreaTop < 0 || safeAreaLeft < 0 || safeAreaBottom < 0 || safeAreaRight < 0 ||
		safeAreaTop > CUSTOM_DEVICE_MAXIMUM_DIMENSION ||
		safeAreaLeft > CUSTOM_DEVICE_MAXIMUM_DIMENSION ||
		safeAreaBottom > CUSTOM_DEVICE_MAXIMUM_DIMENSION ||
		safeAreaRight > CUSTOM_DEVICE_MAXIMUM_DIMENSION ||
		safeAreaTop + safeAreaBottom > height || safeAreaLeft + safeAreaRight > width)
	{
		return;
	}

	WriteProfileInt(REGISTRY_SECTION, REGISTRY_CUSTOM_DEVICE_WIDTH, width);
	WriteProfileInt(REGISTRY_SECTION, REGISTRY_CUSTOM_DEVICE_HEIGHT, height);
	WriteProfileInt(REGISTRY_SECTION, REGISTRY_CUSTOM_DEVICE_SAFE_AREA_TOP, safeAreaTop);
	WriteProfileInt(REGISTRY_SECTION, REGISTRY_CUSTOM_DEVICE_SAFE_AREA_LEFT, safeAreaLeft);
	WriteProfileInt(REGISTRY_SECTION, REGISTRY_CUSTOM_DEVICE_SAFE_AREA_BOTTOM, safeAreaBottom);
	WriteProfileInt(REGISTRY_SECTION, REGISTRY_CUSTOM_DEVICE_SAFE_AREA_RIGHT, safeAreaRight);
}

/////////////////////////////////////////////////////////////////////////////////////////
// CRecentDirList
/////////////////////////////////////////////////////////////////////////////////////////
/* This class is a subclass of CRecentFileList which overrides the
* GetDisplayName() member function to display the directory name instead of 
* the filename, which is always main.lua.
*/
BOOL CRecentDirList::GetDisplayName( CString &strName, int nIndex, LPCTSTR lpszCurDir, int nCurDir, BOOL bAtLeastName = 1) const
{
    // Change entry at nIndex to remove "main.lua"
    CString sOrigPath = m_arrNames[nIndex];
	CString sDirPath = CCoronaProject::RemoveMainLua( sOrigPath );
    m_arrNames[nIndex] = sDirPath;

    BOOL bRetval = CRecentFileList::GetDisplayName( strName, nIndex, lpszCurDir, nCurDir, bAtLeastName );

    // Restore entry
    m_arrNames[nIndex] = sOrigPath;

	return bRetval;
}



////////////////////////////////////////////////////////////////////////////////////////////
// CSimDocManager - our own version of CDocManager
////////////////////////////////////////////////////////////////////////////////////////////
// Overloaded to save working directory for File Open dialog
CSimDocManager::CSimDocManager()
{
}

CSimDocManager::CSimDocManager( CSimDocManager &mgr ) 
{
	m_sInitialDir = mgr.m_sInitialDir;
}

// defined below
void AFXAPI _AfxAppendFilterSuffix(
		CString& filter, OPENFILENAME& ofn, CDocTemplate* pTemplate, CString* pstrDefaultExt);

// Override undocumented class CDocManager in order to set initial directory for Open dialog
// This function copied from MFC source docmgr.cpp
BOOL CSimDocManager::DoPromptFileName(CString& fileName, UINT nIDSTitle, DWORD lFlags, BOOL bOpenFileDialog, CDocTemplate* pTemplate)
{
    // Call derived class which only selects main.lua
	CLuaFileDialog dlgFile(bOpenFileDialog, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, NULL, NULL, 0);

    // Here's the big change
    // Set dialog's initial directory
    dlgFile.m_ofn.lpstrInitialDir = GetInitialDir();

	// Load the dialog title string.
	CString title;
	title.LoadString(nIDSTitle);

	// Set up the file filter string.
	CString strFilter;
	CString stringBuffer;
	stringBuffer.LoadString(IDS_SIMULATOR_FILES);
	strFilter += stringBuffer;
	strFilter += TCHAR('\0');
	strFilter += _T("main.lua");
	strFilter += TCHAR('\0');
	stringBuffer.LoadString(AFX_IDS_ALLFILTER);
	strFilter += stringBuffer;
	strFilter += (TCHAR)'\0';
	strFilter += _T("*.*");
	strFilter += (TCHAR)'\0';
	strFilter += (TCHAR)'\0';

	// Set up the dialog.
	dlgFile.m_ofn.Flags |= lFlags;
	dlgFile.m_ofn.nMaxCustFilter++;
	dlgFile.m_ofn.lpstrFilter = strFilter;
	dlgFile.m_ofn.lpstrTitle = title;
	dlgFile.m_ofn.lpstrFile = fileName.GetBuffer(_MAX_PATH);

	// Show the "Open File" dialog.
	INT_PTR nResult = dlgFile.DoModal();
	fileName.ReleaseBuffer();
	return (nResult == IDOK);
}

// Copied from docmgr.cpp because DoPromptFilename needs it.
/*
AFX_STATIC void AFXAPI _AfxAppendFilterSuffix(
	CString& filter, OPENFILENAME& ofn, CDocTemplate* pTemplate, CString* pstrDefaultExt)
{
	ENSURE_VALID(pTemplate);
	ASSERT_KINDOF(CDocTemplate, pTemplate);

	CString strFilterExt, strFilterName;
	if (pTemplate->GetDocString(strFilterExt, CDocTemplate::filterExt) &&
		!strFilterExt.IsEmpty() &&
		pTemplate->GetDocString(strFilterName, CDocTemplate::filterName) &&
		!strFilterName.IsEmpty())
	{
		if (pstrDefaultExt != NULL)
			pstrDefaultExt->Empty();

		// add to filter
		filter += strFilterName;
		ASSERT(!filter.IsEmpty());  // must have a file type name
		filter += (TCHAR)'\0';  // next string please

		int iStart = 0;
		do
		{
			CString strExtension = strFilterExt.Tokenize( _T( ";" ), iStart );

			if (iStart != -1)
			{
				// a file based document template - add to filter list
				int index = strExtension.Find(TCHAR('.'));
				if (index >= 0)
				{
					if ((pstrDefaultExt != NULL) && pstrDefaultExt->IsEmpty())
					{
						// set the default extension
						*pstrDefaultExt = strExtension.Mid( index + 1 );  // skip the '.'
						ofn.lpstrDefExt = const_cast< LPTSTR >((LPCTSTR)(*pstrDefaultExt));
						ofn.nFilterIndex = ofn.nMaxCustFilter + 1;  // 1 based number
					}
					if (0 == index)
					{
						filter += (TCHAR)'*';
					}
					filter += strExtension;
					filter += (TCHAR)';';  // Always append a ';'.  The last ';' will get replaced with a '\0' later.
				}
			}
		} while (iStart != -1);

		filter.SetAt( filter.GetLength()-1, '\0' );;  // Replace the last ';' with a '\0'
		ofn.nMaxCustFilter++;
	}
}
*/
/////////////////////////////////////////////////////////////////////////////////////
// CLuaFileDialog dialog - only allow selection of main.lua
/////////////////////////////////////////////////////////////////////////////////////
CString CLuaFileDialog::szCustomDefFilter(_T("All Files (*.*)|*.*|Lua Files (*.lua)|*.lua||"));
CString CLuaFileDialog::szCustomDefExt(_T("lua"));
CString CLuaFileDialog::szCustomDefFileName(_T("main"));

CLuaFileDialog::CLuaFileDialog(
	BOOL bOpenFileDialog, // TRUE for FileOpen, FALSE for FileSaveAs
	LPCTSTR lpszDefExt,
	LPCTSTR lpszFileName,
	DWORD dwFlags,
	LPCTSTR lpszFilter,
	CWnd* pParentWnd, 
	DWORD dwSize,
	BOOL bVistaStyle)
:	CFileDialog(bOpenFileDialog, lpszDefExt, lpszFileName, dwFlags, lpszFilter, pParentWnd, dwSize, bVistaStyle)
{
}

// OnFileNameOK - override member of CFileDialog
// Returns 0 if filename allowed, 1 otherwise
// We only allow main.lua
BOOL CLuaFileDialog::OnFileNameOK()
{
	CString sPath = GetPathName();
    CString sFilename = _T("\\main.lua");
	if (sPath.Right(sFilename.GetLength()) == sFilename)
	{
		return 0;
	}

	::AfxMessageBox( IDS_ONLYMAINLUA );
	return 1;
}
