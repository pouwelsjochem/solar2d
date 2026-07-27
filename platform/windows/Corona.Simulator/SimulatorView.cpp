//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <shlwapi.h>
#include <gdiplus.h>
#include <stdlib.h>
#include <math.h>

#include "Core\Rtt_Allocator.h"
#include "Interop\Input\Key.h"
#include "Interop\MDeviceSimulatorServices.h"
#include "Interop\SimulatorRuntimeEnvironment.h"
#include "Rtt_MPlatform.h"
#include "Rtt_MPlatformDevice.h"
#include "Rtt_Event.h"
#include "Rtt_PlatformSimulator.h"
#include "Rtt_Runtime.h"
#include "Simulator.h"
#include "MainFrm.h"
#include "SimulatorDoc.h"
#include "SimulatorView.h"
#include "AboutDlg.h"
#include "WinString.h"
#include "SimulatorMessages.h"
#include "MessageDlg.h"   // Alert
#include "CoronaInterface.h"

// ----------------------------------------------------------------------------

// Microsoft Visual C++ macro which allows us to easily do bitwise operations on the given enum like in C.
DEFINE_ENUM_FLAG_OPERATORS(Rtt::Runtime::LaunchOptions)

#define ENABLE_DEBUG_PRINT	0

#if ENABLE_DEBUG_PRINT
	#define DEBUG_PRINT( ... ) Rtt_LogException( __VA_ARGS__ );
#else
	#define DEBUG_PRINT( ... )
#endif

// ----------------------------------------------------------------------------

// The #includes must go above this line because this macro will override the "new" operator
// which will cause compiler errors with header files belonging to other libraries.
#ifdef _DEBUG
#	define new DEBUG_NEW
#endif

#pragma endregion

#pragma region Message Mappings
IMPLEMENT_DYNCREATE(CSimulatorView, CView)

BEGIN_MESSAGE_MAP(CSimulatorView, CView)
	ON_WM_CREATE()
    ON_WM_CLOSE()
	ON_WM_DESTROY()
	ON_WM_ERASEBKGND()
	ON_WM_SETFOCUS()
	ON_WM_TIMER()
	ON_COMMAND(ID_APP_ABOUT, &CSimulatorView::OnAppAbout)
	ON_COMMAND(ID_HELP, &CSimulatorView::OnHelp)
	ON_COMMAND(ID_VIEW_SUSPEND, &CSimulatorView::OnViewSuspend)
	ON_COMMAND(ID_VIEW_NAVIGATE_BACK, &CSimulatorView::OnViewNavigateBack)
	ON_COMMAND(ID_FILE_MRU_FILE1, &CSimulatorView::OnFileMRU1)
	ON_COMMAND(ID_FILE_OPEN, &CSimulatorView::OnFileOpen)
	ON_COMMAND(ID_FILE_OPENINEDITOR, &CSimulatorView::OnFileOpenInEditor)
	ON_COMMAND(ID_FILE_RELAUNCH, &CSimulatorView::OnFileRelaunch)
	ON_COMMAND(ID_FILE_SHOW_PROJECT_FILES, &CSimulatorView::OnShowProjectFiles)
	ON_COMMAND(ID_FILE_SHOWPROJECTSANDBOX, &CSimulatorView::OnShowProjectSandbox)
	ON_COMMAND(ID_FILE_CLEARPROJECTSANDBOX, &CSimulatorView::OnClearProjectSandbox)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SUSPEND, &CSimulatorView::OnUpdateViewSuspend)
	ON_COMMAND_RANGE(ID_VIEWAS_BEGIN, ID_VIEWAS_END, &CSimulatorView::OnWindowViewAs)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VIEWAS_BEGIN, ID_VIEWAS_END, &CSimulatorView::OnUpdateWindowViewAs )
	ON_UPDATE_COMMAND_UI(ID_VIEW_VIEWAS, &CSimulatorView::OnUpdateWindowViewAs )
	ON_UPDATE_COMMAND_UI(ID_VIEW_NAVIGATE_BACK, &CSimulatorView::OnUpdateViewNavigateBack)
	ON_UPDATE_COMMAND_UI(ID_FILE_RELAUNCH, &CSimulatorView::OnUpdateFileRelaunch)
	ON_UPDATE_COMMAND_UI(ID_FILE_OPENINEDITOR, &CSimulatorView::OnUpdateFileOpenInEditor)
	ON_UPDATE_COMMAND_UI(ID_FILE_SHOW_PROJECT_FILES, &CSimulatorView::OnUpdateShowProjectFiles)
	ON_UPDATE_COMMAND_UI(ID_FILE_SHOWPROJECTSANDBOX, &CSimulatorView::OnUpdateShowProjectSandbox)
	ON_MESSAGE(WMU_NATIVEALERT, &CSimulatorView::OnNativeAlert)
	ON_MESSAGE(WMU_APPLY_SIMULATOR_CONFIGURATION, &CSimulatorView::OnApplySimulatorConfiguration)
	ON_MESSAGE(WMU_SEND_SIMULATOR_INPUT, &CSimulatorView::OnSendSimulatorInput)
END_MESSAGE_MAP()

BEGIN_MESSAGE_MAP(CSimulatorView::CCoronaControlContainer, CStatic)
	ON_WM_CREATE()
	ON_WM_CTLCOLOR_REFLECT()
	ON_WM_SIZE()
END_MESSAGE_MAP()

#pragma endregion


#pragma region Constructor/Destructor
/// Creates a new Corona Simulator CView.
CSimulatorView::CSimulatorView()
:	mScopedComInitializer(Interop::ScopedComInitializer::ApartmentType::kSingleThreaded),
	mMessageDlgPointer(nullptr),
	mDeviceConfig(*Rtt_AllocatorCreate()),
	mRuntimeLoadedEventHandler(this, &CSimulatorView::OnRuntimeLoaded)
{
	CSimulatorApp *applicationPointer = (CSimulatorApp*)AfxGetApp();

	mRuntimeEnvironmentPointer = nullptr;
	mDeviceName = applicationPointer->GetDeviceName();
	m_nSkinId = Rtt::TargetDevice::kUnknownSkin;
	mRelaunchCount = 0;
	mIsCustomDevice = false;
	mIsDeviceConfigurationTemporary = false;
	mIsRelaunchPending = false;
	mBackgroundTimerId = 0;
}

/// Destructor. Destroys owned objects.
CSimulatorView::~CSimulatorView()
{
	CStringA relaunchCountStr;
	relaunchCountStr.Format("%d", mRelaunchCount);

	if (mBackgroundTimerId)
	{
		KillTimer(mBackgroundTimerId);
		mBackgroundTimerId = 0;
	}
	if (mRuntimeEnvironmentPointer)
	{
		Interop::SimulatorRuntimeEnvironment::Destroy(mRuntimeEnvironmentPointer);
		mRuntimeEnvironmentPointer = nullptr;
	}
	if (mMessageDlgPointer)
	{
		if (mMessageDlgPointer->GetSafeHwnd())
		{
			mMessageDlgPointer->SendMessage(WM_COMMAND, IDCANCEL, 0);
		}
		delete mMessageDlgPointer;
		mMessageDlgPointer = nullptr;
	}
}

#pragma endregion


#pragma region Window Event Handlers
/// Called after a new file was selected by the CSimulatorDoc.
void CSimulatorView::OnInitialUpdate()
{
}

/// Called after the CSimulatorDoc has been updated with a different file or if the file was closed.
/// Starts simulating the selected file
/// @param pSender Pointer to the view that requested an update. NULL if update came from the document.
/// @param lHint Contains information about the modification made to the document.
/// @param pHint Pointer to an object storing information about the modification.
///              NULL if not information has been provided.
void CSimulatorView::OnUpdate(CView* pSender, LPARAM lHint, CObject *pHint)
{
	// Start simulation with the new or updated file.
	// If the last file was closed, then this will display the home screen.
	LoadSkinResources();
	if (mDeviceName == _T("Custom"))
	{
		Rtt::MSimulatorHost::Configuration configuration;
		configuration.deviceSelection = Rtt::MSimulatorHost::Configuration::kCustomDevice;
		((CSimulatorApp*)AfxGetApp())->GetCustomDeviceSettings(
			configuration.width, configuration.height,
			configuration.safeAreaInsets.top, configuration.safeAreaInsets.left,
			configuration.safeAreaInsets.bottom, configuration.safeAreaInsets.right);
		InitializeCustomSimulation(configuration, true);
		return;
	}

	Rtt::TargetDevice::Skin skinId = Rtt::TargetDevice::FindSkinForLabel(CStringA(mDeviceName));
	if (skinId == Rtt::TargetDevice::kUnknownSkin)
	{
		skinId = Rtt::TargetDevice::fDefaultSkinID;
	}
	InitializeSimulation(skinId);
}

// OnDestroy - clean up
void CSimulatorView::OnDestroy()
{
	StopSimulation();
	CView::OnDestroy();
}

// PreCreateWindow - remove border, set default size
BOOL CSimulatorView::PreCreateWindow(CREATESTRUCT& cs)
{
    cs.style &= ~WS_BORDER;  // view window doesn't need a border
	cs.cx = 320; // Note that this doesn't do much, see Simulator.cpp for where the main frame size gets set.
	cs.cy = 480;
	return CView::PreCreateWindow(cs);
}

// OnCreate - init OpenGL
int CSimulatorView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CView::OnCreate(lpCreateStruct) == -1)
	{
		return -1;
	}

	// Add the Corona container control to the view.
	// This container control will always be sized to fit the screen within the device skin.
	RECT bounds;
	bounds.top = 0;
	bounds.left = 0;
	bounds.bottom = lpCreateStruct->cy;
	bounds.right = lpCreateStruct->cx;
	mCoronaContainerControl.Create(nullptr, WS_CHILD | WS_VISIBLE, bounds, this);
	mCoronaContainerControl.GetCoronaControl().ShowWindow(SW_HIDE);
	
	// Get system skin directory (we have to do this manually because the just-in-time
	// way that the platform is loaded in tandem with the first application means that the information 
	// is not available in the platform when we need it)
	Rtt_ASSERT(AfxGetApp() != NULL);
	mSystemSkinsDir = ((CSimulatorApp *)AfxGetApp())->GetResourceDir();
	mSystemSkinsDir += _T("\\Skins\\");
	LoadSkinResources();

	// Now that we've loaded the skins, get a valid skin id
	if (mDeviceName.IsEmpty())
	{
		m_nSkinId = Rtt::TargetDevice::fDefaultSkinID;
	}
	else
	{
		m_nSkinId = Rtt::TargetDevice::SkinForLabel(CStringA(mDeviceName));
	}

	return 0;
}

void CSimulatorView::OnSetFocus(CWnd* pOldWnd)
{
	CWnd& coronaControl = mCoronaContainerControl.GetCoronaControl();
	if (coronaControl.IsWindowVisible())
	{
		coronaControl.SetFocus();
	}
}

// Forward view close requests to the main window.
void CSimulatorView::OnClose()
{
	StopSimulation();
    AfxGetMainWnd()->SendMessage( WM_CLOSE );
}

void CSimulatorView::OnDraw(CDC* pDC)
{
	// Fetch the region to draw in.
	CRect rect;
	GetClientRect(&rect);

	// Draw a solid background.
	COLORREF backgroundColor = RGB(0, 0, 0);
	CBrush brush(backgroundColor);
	CBrush* pOldBrush = pDC->SelectObject(&brush);
	CPen pen;
	pen.CreatePen(PS_SOLID, 3, backgroundColor);
	CPen* pOldPen = pDC->SelectObject(&pen);
	pDC->Rectangle(rect);
	pDC->SelectObject(pOldBrush);
	pDC->SelectObject(pOldPen);
}

// OnEraseBkgnd - do nothing, to minimize flickering
BOOL CSimulatorView::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}

#pragma endregion


#pragma region Menu Event Handlers
/// Displays the About Box window.
void CSimulatorView::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

/// Displays help documentation on Corona Labs' website via the system's default web browser.
void CSimulatorView::OnHelp()
{
	try
	{
		const wchar_t kUrl[] = L"https://docs.coronalabs.com/guide";
		::ShellExecuteW(nullptr, L"open", kUrl, nullptr, nullptr, SW_SHOWNORMAL);
	}
	catch (...) { }
}

// OnViewSuspend - handle Suspend/Resume menu item
void CSimulatorView::OnViewSuspend()
{
	// Suspend/resume the Corona runtime.
	// Always show the "Suspended" overlay, unless the shift key was held down.
	bool showSuspendedScreen = !((::GetKeyState(VK_LSHIFT) | ::GetKeyState(VK_RSHIFT)) & 0x80);
	SuspendResumeSimulationWithOverlay(showSuspendedScreen, true);
}

// OnUpdateViewSuspend - set to Suspend or Resume as appropriate
void CSimulatorView::OnUpdateViewSuspend(CCmdUI *pCmdUI)
{
	bool isSuspended = IsSimulationSuspended();
	int id = isSuspended ? IDS_HARDWARE_RESUME : IDS_HARDWARE_SUSPEND;
	CString sCaption;
	sCaption.LoadString(id);
	pCmdUI->SetText( sCaption );
}

void CSimulatorView::OnViewNavigateBack()
{
	if (!mRuntimeEnvironmentPointer || !mRuntimeEnvironmentPointer->GetDeviceSimulatorServices())
	{
		return;
	}

	// Do not continue if we're currently suspended.
	if (IsSimulationSuspended())
	{
		return;
	}

	// Send "back" key down/up messages to the Corona control.
	// Corona will automatically terminate the runtime if the Lua key listener does not handle them.
	auto windowPointer = &mCoronaContainerControl.GetCoronaControl();
	windowPointer->PostMessage(WM_KEYDOWN, VK_BROWSER_BACK, 0L);
	windowPointer->PostMessage(WM_KEYUP, VK_BROWSER_BACK, 0x40000000L);
}

/// Called when the top most file in the "Most Recent Files" list is clicked on in the menu.
/// The application won't update the document when the top most file is selected so we
/// we have to update it manually here.
void CSimulatorView::OnFileMRU1()
{
	// Fetch the application's MRU list.
	// Warning: The returned pointer can be NULL if no files were ever loaded.
	CRecentFileList *recentFileListPointer = ((CSimulatorApp*)AfxGetApp())->GetRecentFileList();
	if (NULL == recentFileListPointer)
	{
		return;
	}
	
	// Compare the selected file name with what is currently opened.
	const CString& fileName = (*recentFileListPointer)[0];
	if ((GetDocument()->GetPath() == fileName))
	{
		// The selected file is already open. Relaunch it in the simulator.
		++mRelaunchCount;
		RestartSimulation();
	}
}

// OnFileOpen - overloaded to check OpenGL and reopen file if already open
void CSimulatorView::OnFileOpen()
{
	CString filename;

	// Do not continue if the machine does not meet the simulator's minimum OpenGL requirements.
	if (ValidateOpenGL() == false)
	{
		return;
	}

	// Open the file if the user didn't "Cancel" out of the dialog.
	if (AfxGetApp()->DoPromptFileName(filename, AFX_IDS_OPENFILE,
                               OFN_HIDEREADONLY | OFN_FILEMUSTEXIST,
                               TRUE, NULL))
	{
		  // if the file is already open, relaunch
		  if ((GetDocument()->GetPath() == filename))
		  {
			  ++mRelaunchCount;
			  RestartSimulation();
		  }
	}
}

// OnFileOpenInEditor - give project name to shell, if associated with an editor
// Note TODO item below
void CSimulatorView::OnFileOpenInEditor()
{
	const int MAX_PATH_LENGTH = 512;
	CString applicationFileName;
	CString fileAssociation;
	DWORD fileAssociationLength = MAX_PATH_LENGTH;
	HRESULT result;
	int length;
	int index;
	bool hasValidFileAssociation = false;

	try
	{
		// Fetch this application's file name without the path.
		length = ::GetModuleFileName(NULL, applicationFileName.GetBuffer(MAX_PATH_LENGTH), MAX_PATH_LENGTH);
		applicationFileName.ReleaseBuffer(length);
		if (length > 0)
		{
			index = applicationFileName.ReverseFind(_T('\\'));
			if (index > 0)
			{
				applicationFileName.Delete(0, index + 1);
			}
		}

		// Fetch the file association in Windows for all Lua files, if assigned.
		// ie: This is the default executable used to open a Lua file when double clicked on.
		result = ::AssocQueryString(
						ASSOCF_INIT_DEFAULTTOSTAR, ASSOCSTR_EXECUTABLE, _T(".lua"),
						NULL, fileAssociation.GetBuffer(MAX_PATH_LENGTH), &fileAssociationLength);
		fileAssociation.ReleaseBuffer();
		CString fullAssociationPath(fileAssociation);
		if (S_OK == result)
		{
			index = fileAssociation.ReverseFind(_T('\\'));
			if (index > 0)
			{
				fileAssociation.Delete(0, index + 1);
			}
		}

		// Check if we have a valid file association in Windows to edit the "main.lua" file.
		// Note: The default association on Windows XP or lower is an empty string.
		if (fileAssociation.GetLength() > 0)
		{
			// A file association has been assigned.
			// Make sure it is associated with an application that can open the file.
			// Note: The default association on Windows Vista or higher is "shell32.dll".
			CString extension = fileAssociation.Right(4);
			if ((extension.CompareNoCase(_T(".exe")) == 0) ||
			    (extension.CompareNoCase(_T(".com")) == 0) ||
			    (extension.CompareNoCase(_T(".bat")) == 0))
			{
				// Also, don't allow the applications below since they won't edit the file.
				if ((fileAssociation.CompareNoCase(applicationFileName) != 0) &&
				    (fileAssociation.CompareNoCase(_T("Lightroom.exe")) != 0))     // Adobe Lightroom
				{
					// The application associated with the file is valid.
					hasValidFileAssociation = true;
				}
			}
		}

		// Open the Lua file for editing using the default application assigned in Windows.
		// If Windows doesn't have a valid file association, then open it with Notepad.
		if (hasValidFileAssociation)
		{
			if (fileAssociation == _T("sublime_text.exe") || fileAssociation == _T("Code.exe"))
			{
				CString fullPath(GetDocument()->GetPath());
				index = fullPath.ReverseFind(_T('\\'));
				if (index > 0)
				{
					CString dirPath(fullPath);
					dirPath.Delete(index, dirPath.GetLength() - index);
					fullPath.Insert(0, _T('"'));
					fullPath.Append(_T("\" --add \""));
					fullPath.Append(dirPath);
					fullPath.Append(_T("\""));
				}
				::ShellExecute(nullptr, nullptr, fullAssociationPath, fullPath, nullptr, SW_SHOWNORMAL);
			}
			else
			{
				::ShellExecute(nullptr, _T("open"), GetDocument()->GetPath(), nullptr, nullptr, SW_SHOWNORMAL);
			}
			WinString appName;
			appName.SetTCHAR(fileAssociation);
		}
		else
		{
			::ShellExecute(nullptr, nullptr, _T("notepad.exe"), GetDocument()->GetPath(), nullptr, SW_SHOWNORMAL);
		}
	}
	catch (...)
	{
	}
}

// OnUpdateFileOpenInEditor - enable menu item when project is open
void CSimulatorView::OnUpdateFileOpenInEditor(CCmdUI *pCmdUI)
{
   pCmdUI->Enable( mRuntimeEnvironmentPointer && ! GetDocument()->GetPath().IsEmpty() );
}

/// Restart simulation of the last selected project.
void CSimulatorView::OnFileRelaunch()
{
	if (GetDocument()->GetPath().GetLength() > 0)
	{
		// Restart the currently running project.
		RestartSimulation();
	}
	else
	{
		// A project is not currently open.
		// Attempt to open and run the last ran project in the "recent file list".
		CRecentFileList *recentFileListPointer = ((CSimulatorApp*)AfxGetApp())->GetRecentFileList();
		CString lastFilePathName = (*recentFileListPointer)[0];
		if (lastFilePathName.GetString() > 0)
		{
			GetDocument()->GetDocTemplate()->OpenDocumentFile(lastFilePathName);
		}
	}
}

/// Enables or disables the "Relaunch" menu item.
void CSimulatorView::OnUpdateFileRelaunch(CCmdUI *pCmdUI)
{
	CRecentFileList *recentFileListPointer = ((CSimulatorApp*)AfxGetApp())->GetRecentFileList();
	bool hasRecentFile = (recentFileListPointer && ((*recentFileListPointer)[0].GetLength() > 0));
	bool isProjectOpen = (GetDocument()->GetPath().GetLength() > 0);
	pCmdUI->Enable((isProjectOpen || hasRecentFile) ? TRUE : FALSE);
}

/// Display the currently running Corona project's root folder in Windows Explorer.
void CSimulatorView::OnShowProjectFiles()
{
	// Fetch current project. Will be NULL no project is selected.
	auto projectPointer = GetDocument()->GetProject();
	if (!projectPointer)
	{
		return;
	}

	// Fetch the project's absolute path without the file name. Make sure directory exists.
	CString projectPath = projectPointer->GetDir();
	if (CSimulatorApp::CheckDirExists(projectPath) == false)
	{
		return;
	}
	
	// Display the project folder in the default file viewer. Typically "Windows Explorer".
	try { ::ShellExecute(nullptr, _T("open"), projectPath, nullptr, nullptr, SW_SHOWNORMAL); }
	catch (...) { }	            
}

/// Enables or disables the "Show Project Files" menu item.
void CSimulatorView::OnUpdateShowProjectFiles(CCmdUI *pCmdUI)
{
	BOOL enabled = (GetDocument()->GetPath().GetLength() > 0) ? TRUE : FALSE;
	pCmdUI->Enable(enabled);
}

// OnShowProjectSandbox - Open this app's sandbox in Windows Explorer
void CSimulatorView::OnShowProjectSandbox()
{
	// Do not continue if not simulating a project.
	if (!mRuntimeEnvironmentPointer || !mRuntimeEnvironmentPointer->GetDeviceSimulatorServices())
	{
		return;
	}

	CStringW utf16DirectoryPath = mRuntimeEnvironmentPointer->GetUtf16PathFor(Rtt::MPlatform::kDocumentsDir);
	utf16DirectoryPath.Append(L"\\..\\.");
	if (CSimulatorApp::CheckDirExists(utf16DirectoryPath))
	{
		::ShellExecuteW(nullptr, L"open", utf16DirectoryPath, nullptr, nullptr, SW_SHOWNORMAL);
	}
}

// OnClearProjectSandbox - Open this app's sandbox in Windows Explorer
void CSimulatorView::OnClearProjectSandbox()
{
	// Do not continue if not simulating a project.
	if (!mRuntimeEnvironmentPointer || !mRuntimeEnvironmentPointer->GetDeviceSimulatorServices())
	{
		return;
	}

	const TCHAR *confPrefKey = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\DontShowMeThisDialogAgain");
	TCHAR sandboxPath[MAX_PATH];
	CStringW utf16DirectoryPath = mRuntimeEnvironmentPointer->GetUtf16PathFor(Rtt::MPlatform::kDocumentsDir);

	// TODO: handle paths longer than MAX_PATH gracefully

	// We have the Documents directory but we want its parent
	utf16DirectoryPath.Append(L"\\..\\.");

	// Resolve the relative part of the sandbox path
	PathCanonicalize(sandboxPath, utf16DirectoryPath.GetString());

	// The trailing part of the sandbox path works as a key for SHMessageBoxCheck()
	CString confPrefValue = CString("CPS-");

	confPrefValue.Append(PathFindFileName(sandboxPath));

	// If shift is held down, forget any remembered value for the "don't show dialog again" checkbox
	bool forgetRememberedSetting = (((::GetKeyState(VK_LSHIFT) | ::GetKeyState(VK_RSHIFT)) & 0x80) != 0);

	if (forgetRememberedSetting)
	{
		HKEY hKey = NULL;
		DWORD regResult = RegOpenKeyEx(HKEY_CURRENT_USER,
			confPrefKey,
			0L,
			KEY_SET_VALUE,
			&hKey);

		if (regResult == ERROR_SUCCESS)
		{
			RegDeleteValue(hKey, confPrefValue);
			RegCloseKey(hKey);
		}
	}

	WinString clearPrompt;

	clearPrompt.Format(_T("Are you sure you want to delete the contents of the sandbox for '%s'?\n\nThis will also clear any app preferences and restart the project"), GetDocument()->GetTitle());

	// This confirmation dialog includes a "don't show dialog again" checkbox
	int yesNo = SHMessageBoxCheck(this->GetSafeHwnd(),
		clearPrompt.GetUTF16(),
		TEXT("Clear Project Sandbox"),
		MB_YESNO,
		IDYES,
		confPrefValue);

	if (yesNo == IDYES)
	{
		StopSimulation();

		// The fine API has a crunchy way of handling multiple paths and it must be
		// terminated with a double NUL
		sandboxPath[_tcslen(sandboxPath)+1] = 0;

		// Send the app's sandbox to the recycle bin.  As luck would have it, the prefs database 
		// for the app is in the same location so we have nothing more to do
		SHFILEOPSTRUCT shFileOp = {
			NULL,
			FO_DELETE,
			sandboxPath,
			NULL,
			FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_ALLOWUNDO,
			FALSE,
			NULL,
			NULL };

		SHFileOperation(&shFileOp);

		Rtt_Log("Project sandbox and preferences cleared");

		StartSimulation();
	}
	else
	{
		// they choose not to proceed, forget that answer if they checked the box
		HKEY hKey = NULL;
		DWORD regResult = RegOpenKeyEx(HKEY_CURRENT_USER,
			confPrefKey,
			0L,
			KEY_SET_VALUE,
			&hKey);

		if (regResult == ERROR_SUCCESS)
		{
			RegDeleteValue(hKey, confPrefValue);
			RegCloseKey(hKey);
		}
	}
}

// OnUpdateShowProjectSandbox - enable menu item when project is open
void CSimulatorView::OnUpdateShowProjectSandbox(CCmdUI *pCmdUI)
{
	bool isSimulatingProject = (mRuntimeEnvironmentPointer && mRuntimeEnvironmentPointer->GetDeviceSimulatorServices());
	pCmdUI->Enable(isSimulatingProject ? TRUE : FALSE);
}

/// Called when a device skin has been selected from the "View As" menu.
/// Displays the selected device skin within the simulator window.
/// @param nID The unique ID of the menu item that was clicked on.
void CSimulatorView::OnWindowViewAs( UINT nID )
{
	Rtt::TargetDevice::Skin skinId = Rtt::TargetDevice::kUnknownSkin;
	skinId = SkinIDFromMenuID(nID);
	CString skinName(Rtt::TargetDevice::LabelForSkin(skinId));

	// Display the selected device skin
	InitializeSimulation(skinId);
}

// OnUpdateWindowViewAs - check mark for the currently displayed skin
// (gets called when "Window" menu is clicked as well as when "View As" menu is shown)
void CSimulatorView::OnUpdateWindowViewAs( CCmdUI *pCmdUI )
{
	int skinID = 0;

	CMainFrame *pFrameWindowPointer = (CMainFrame*)GetParentFrame();
	if (pFrameWindowPointer != NULL)
	{
		CMenu *pMainMenu = pFrameWindowPointer->GetMenu();
		CMenu *pViewMenu = pMainMenu->GetSubMenu(2);  // position of "View" submenu in main menu
		CMenu *pViewAsMenu = pViewMenu->GetSubMenu(0);  // position of "View As" submenu in "View" menu

		// Create the skins menu if we haven't already
		if (pViewAsMenu == NULL || pViewAsMenu->GetMenuItemCount() == 1)
		{
			CMenu *pParentMenu = pViewAsMenu;

#if _DEBUG
			Rtt_TRACE(( "CSimulatorView::OnUpdateWindowViewAs: populating View As menu\n" ));
#endif

			// Remove the placeholder which is needed to trigger the population of the otherwise
			// empty menu (because it's a popup, "View As" can't have an ID and thus can't trigger)
			pViewAsMenu->RemoveMenu(0, MF_BYPOSITION);

			const char *skinName = NULL;
			const char *skinCategory = NULL;
			const char *lastSkinCategory = NULL;
			long skinCount = 0;
			long itemCount = 0;
			long viewAsItemCount = 0;
			while ((skinName = Rtt::TargetDevice::NameForSkin(skinCount)) != NULL)
			{
				CString itemTitle;
				int skinWidth = Rtt::TargetDevice::WidthForSkin(skinCount);
				int skinHeight = Rtt::TargetDevice::HeightForSkin(skinCount);
				skinCategory = Rtt::TargetDevice::CategoryForSkin(skinCount);

				itemTitle.Format(_T("%S\t%dx%d"), skinName, skinWidth, skinHeight);

				pParentMenu->InsertMenu(itemCount, MF_BYPOSITION, ID_VIEWAS_BEGIN + skinCount, itemTitle);

				// If the device type changes, insert a separator in the menu
				if (lastSkinCategory != NULL && strcmp(skinCategory, lastSkinCategory) != 0)
				{
					pParentMenu->InsertMenu(itemCount, MF_BYPOSITION|MFT_SEPARATOR, 0, _T("-"));
					++itemCount;
				}

				lastSkinCategory = skinCategory;

				++skinCount;
				++itemCount;
			}

			// Separator
			pViewAsMenu->InsertMenu(viewAsItemCount, MF_BYPOSITION|MFT_SEPARATOR, 0, _T("-"));
			++viewAsItemCount;
		}
	}

	pCmdUI->SetCheck( SkinIDFromMenuID( pCmdUI->m_nID ) == m_nSkinId );
}

void CSimulatorView::OnUpdateViewNavigateBack(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(!IsSimulationSuspended() ? TRUE : FALSE);
}

#pragma endregion

void CSimulatorView::GetFilePaths(LPCTSTR pattern, CStringArray& filepaths)
{
	// Object to enumerate files
	CFileFind finder;

	// Init the file finding job
	BOOL working = finder.FindFile( pattern );

	// For each file that is found:
	while ( working )
	{
		// Update finder status with new file
		working = finder.FindNextFile();

		// Skip '.' and '..'
		if ( finder.IsDots() )
		{
			continue;
		}

		// Skip sub-directories
		if ( finder.IsDirectory() )
		{
			continue;
		}
		// Add file path to container
		filepaths.Add( finder.GetFilePath() );
	}
	// Cleanup file finder
	finder.Close();
}


bool CSimulatorView::LoadSkinResources()
{
	CString msg;
	CStringArray filePaths;
	CString systemSkinFilesGlob;

	systemSkinFilesGlob = mSystemSkinsDir + _T("*.lua");
	
	GetFilePaths(systemSkinFilesGlob, filePaths);
	CString projectPath = CCoronaProject::RemoveMainLua(GetDocument()->GetPath());
	if (!projectPath.IsEmpty())
	{
		CString projectDeviceFilesGlob = projectPath + _T("\\simulator\\devices\\*.lua");
		GetFilePaths(projectDeviceFilesGlob, filePaths);
	}

	// Put the skins into a data structure we can share with core code
	char **skinPaths;

	skinPaths = (char **) calloc(sizeof(char *), filePaths.GetSize());

	if (skinPaths == NULL && filePaths.GetSize() > 0)
	{
		printf("CSimulatorView::OnUpdateWindowViewAs: Problem processing skin files.  Please restart\n");
	}
	else
	{
		for ( int i = 0; i < filePaths.GetSize(); i++ )
		{
			skinPaths[i] = (char *) _strdup(CStringA(filePaths[i]));
		}

		// Tell the core about the skins
		Rtt::TargetDevice::Initialize(skinPaths, filePaths.GetSize());

		for (int i = 0; i < filePaths.GetSize(); i++)
		{
			free(skinPaths[i]);
		}
		free(skinPaths);
	}

	return TRUE;
}

#pragma region Corona Event Handlers
void CSimulatorView::OnRuntimeLoaded(Interop::RuntimeEnvironment& sender, const Interop::EventArgs& arguments)
{
	// Store a pointer to the newly created/loaded Corona runtime.
	mRuntimeEnvironmentPointer = (Interop::SimulatorRuntimeEnvironment*)&sender;

	// If simulating a project, then log its paths.
	if (mRuntimeEnvironmentPointer->GetDeviceSimulatorServices())
	{
		Rtt_TRACE_SIM((
				"Loading project from:   %s\r\n",
				mRuntimeEnvironmentPointer->GetUtf8PathFor(Rtt::MPlatform::kResourceDir)));
		Rtt_TRACE_SIM((
				"Project sandbox folder: %s\r\n",
				mRuntimeEnvironmentPointer->GetUtf8PathFor(Rtt::MPlatform::kDocumentsDir)));
	}

	// Update the window for the current skin.
	UpdateSimulatorSkin();
}

// OnNativeAlert - message from lua code
// Use CMessageDlg, configured as indicated by WMU_ALERT_PARAMS in lParam
// wParam = 0 for cancel, 1 for show.  Only one window can be active at a time.
// lParam = NULL (only OK button) or ptr to array of strings for buttons
LRESULT CSimulatorView::OnNativeAlert(WPARAM wParam, LPARAM lParam)
{
    int result = 0;

	WMU_ALERT_PARAMS *pWAP = (WMU_ALERT_PARAMS *)lParam;
	if (nullptr == pWAP)
	{
		return -1;
	}

    if (0 == wParam)  // Cancel alert -- usually canceled directly
	{
		if (0 != pWAP->hwnd)
		{
			::SendMessage((HWND)pWAP->hwnd, WM_COMMAND, IDCANCEL, NULL);
		}
	}
	else // 0 != wParam, show alert
	{
        // Clean up previous memory
        if (mMessageDlgPointer)
		{
			if (mMessageDlgPointer->GetSafeHwnd())
			{
				mMessageDlgPointer->SendMessage(WM_COMMAND, IDCANCEL, 0);
			}
			delete mMessageDlgPointer;
			mMessageDlgPointer = nullptr;
		}

        // Create a new message box.
		mMessageDlgPointer = new CMessageDlg(this);

		// Set up parameters of message window
		WinString string;
		string.SetUTF8( pWAP->sTitle );
		mMessageDlgPointer->SetTitle(string.GetTCHAR());
		string.SetUTF8( pWAP->sMsg );
		mMessageDlgPointer->SetText(string.GetTCHAR());

		// Custom labels for up to 3 buttons
		if ( pWAP->nButtonLabels > 0 )
		{
			string.SetUTF8( pWAP->psButtonLabels[0] );
			mMessageDlgPointer->SetDefaultText(string.GetTCHAR());
		}
		if ( pWAP->nButtonLabels > 1 )
		{
			string.SetUTF8( pWAP->psButtonLabels[1] );
			mMessageDlgPointer->SetAltText(string.GetTCHAR());
		}
		if ( pWAP->nButtonLabels > 2 )
		{
			string.SetUTF8( pWAP->psButtonLabels[2] );
			mMessageDlgPointer->SetButton3Text(string.GetTCHAR());
		}

		// When a button is pressed pLuaResource is used to call back into lua code
		mMessageDlgPointer->SetNativeAlertInfo(pWAP->pLuaResource);

		// Create the message window (non-zero means success)
		if (mMessageDlgPointer->Create(CMessageDlg::IDD) == 0)
		{
			result = -1;
		}
		mMessageDlgPointer->ShowWindow(SW_SHOW);

		// Return hwnd for future cancel message
		pWAP->hwnd = mMessageDlgPointer->GetSafeHwnd();
	}

	return result;
}

#pragma endregion


#pragma region Public Functions
/// Starts simulating the project file currently held by the CSimulatorDoc.
void CSimulatorView::StartSimulation()
{
	// Stop the last running simulation.
	if (mRuntimeEnvironmentPointer)
	{
		StopSimulation();
	}

	// Run the currently selected Corona project set in this view's document.
	RunCoronaProject();
}

/// Restarts simulation of the project file currently held by the CSimulatorDoc.
void CSimulatorView::RestartSimulation()
{
	// If the simulator is not currently running, then do a "Start" instead.
	if (nullptr == mRuntimeEnvironmentPointer)
	{
		StartSimulation();
		return;
	}

	// Record the "Restart" to the usage feedback to be posted to Corona Labs' server later.
	if (GetDocument()->GetPath().GetLength() > 0)
	{
	    ++mRelaunchCount;
	}

	// Run the currently selected Corona project set in this view's document.
	RunCoronaProject();
}

void CSimulatorView::SuspendResumeSimulationWithOverlay(bool showOverlay, bool sendSystemEvents)
{
	// Do not continue if we're not currently running a project.
	if (!mRuntimeEnvironmentPointer)
	{
		return;
	}
	auto runtimePointer = mRuntimeEnvironmentPointer->GetRuntime();
	if (!runtimePointer)
	{
		return;
	}

	// Suspend/resume the Corona runtime.
	if (runtimePointer->IsSuspended())
	{
		runtimePointer->Resume(sendSystemEvents);
	}
	else
	{
		runtimePointer->Suspend(sendSystemEvents);
	}
	if (runtimePointer->IsSuspended())
	{
		if (showOverlay)
		{
			mCoronaContainerControl.SetWindowTextW(L"Suspended");
			mCoronaContainerControl.GetCoronaControl().ShowWindow(SW_HIDE);
		}
		EnableWindow(FALSE);
	}
	else
	{
		EnableWindow(TRUE);
		if (mCoronaContainerControl.GetCoronaControl().IsWindowVisible() == FALSE)
		{
			mCoronaContainerControl.SetWindowTextW(L"");
			mCoronaContainerControl.GetCoronaControl().ShowWindow(SW_SHOW);
			mCoronaContainerControl.GetCoronaControl().SetFocus();
		}
	}
}

/// Stops the current simulation and blanks out the screen.
void CSimulatorView::StopSimulation()
{
	if (mBackgroundTimerId)
	{
		KillTimer(mBackgroundTimerId);
		mBackgroundTimerId = 0;
	}

	// Do not continue if already stopped.
	if (!mRuntimeEnvironmentPointer)
	{
		return;
	}

	// Terminate the Corona runtime.
	Interop::SimulatorRuntimeEnvironment::Destroy(mRuntimeEnvironmentPointer);
	mRuntimeEnvironmentPointer = nullptr;

	RECT bounds;

	mCoronaContainerControl.GetWindowRect(&bounds);

	mCoronaContainerControl.DestroyWindow();
	mCoronaContainerControl.Create(nullptr, WS_CHILD | WS_VISIBLE, bounds, this);

	// Hide the Corona control and show its black container without any text.
	mCoronaContainerControl.SetWindowTextW(L"");
	mCoronaContainerControl.GetCoronaControl().ShowWindow(SW_HIDE);

	// Clear the simulator screen.
	UpdateSimulatorSkin();
}

bool CSimulatorView::IsSimulationSuspended() const
{
	if (mRuntimeEnvironmentPointer)
	{
		auto runtimePointer = mRuntimeEnvironmentPointer->GetRuntime();
		if (runtimePointer)
		{
			return runtimePointer->IsSuspended();
		}
	}
	return false;
}

Rtt::MSimulatorHost::ConfigureResult CSimulatorView::ConfigureAndRelaunch(
	const Rtt::MSimulatorHost::Configuration& configuration, bool onlyIfNeeded)
{
	if (configuration.hasRoundedCorners && configuration.roundedCorners)
	{
		return Rtt::MSimulatorHost::kConfigureFailed;
	}
	if (configuration.deviceSelection == Rtt::MSimulatorHost::Configuration::kCustomDevice &&
		(configuration.width <= 0 || configuration.height <= 0 ||
		 configuration.width > CUSTOM_DEVICE_MAXIMUM_DIMENSION ||
		 configuration.height > CUSTOM_DEVICE_MAXIMUM_DIMENSION ||
		 configuration.safeAreaInsets.top < 0 || configuration.safeAreaInsets.left < 0 ||
		 configuration.safeAreaInsets.bottom < 0 || configuration.safeAreaInsets.right < 0 ||
		 configuration.safeAreaInsets.top > configuration.height ||
		 configuration.safeAreaInsets.bottom > configuration.height - configuration.safeAreaInsets.top ||
		 configuration.safeAreaInsets.left > configuration.width ||
		 configuration.safeAreaInsets.right > configuration.width - configuration.safeAreaInsets.left))
	{
		return Rtt::MSimulatorHost::kConfigureFailed;
	}

	bool configurationChanged = false;
	if (configuration.deviceSelection == Rtt::MSimulatorHost::Configuration::kNamedDevice)
	{
		Rtt::TargetDevice::Skin skin = Rtt::TargetDevice::FindSkinForLabel(configuration.deviceId.c_str());
		if (skin == Rtt::TargetDevice::kUnknownSkin && configuration.deviceId.compare(0, 8, "project:") != 0)
		{
			std::string projectIdentifier("project:");
			projectIdentifier.append(configuration.deviceId);
			skin = Rtt::TargetDevice::FindSkinForLabel(projectIdentifier.c_str());
		}
		if (skin == Rtt::TargetDevice::kUnknownSkin)
		{
			return Rtt::MSimulatorHost::kConfigureFailed;
		}
		configurationChanged = mIsCustomDevice || skin != m_nSkinId;
	}
	else if (configuration.deviceSelection == Rtt::MSimulatorHost::Configuration::kCustomDevice)
	{
		configurationChanged =
			!mIsCustomDevice ||
			(int)mDeviceConfig.deviceWidth != configuration.width ||
			(int)mDeviceConfig.deviceHeight != configuration.height ||
			(int)mDeviceConfig.safeAreaInsetTop != configuration.safeAreaInsets.top ||
			(int)mDeviceConfig.safeAreaInsetLeft != configuration.safeAreaInsets.left ||
			(int)mDeviceConfig.safeAreaInsetBottom != configuration.safeAreaInsets.bottom ||
			(int)mDeviceConfig.safeAreaInsetRight != configuration.safeAreaInsets.right;
	}

	if (onlyIfNeeded && !configurationChanged && !mIsRelaunchPending)
	{
		ApplyConfigurationPersistence(configuration);
		return Rtt::MSimulatorHost::kConfigureAlreadyActive;
	}

	mPendingConfiguration = configuration;
	if (!mIsRelaunchPending)
	{
		mIsRelaunchPending = true;
		if (!PostMessage(WMU_APPLY_SIMULATOR_CONFIGURATION, 0, 0))
		{
			mIsRelaunchPending = false;
			return Rtt::MSimulatorHost::kConfigureFailed;
		}
	}
	return Rtt::MSimulatorHost::kConfigureApplied;
}

bool CSimulatorView::SetSimulatorFullscreen(bool fullscreen)
{
	CMainFrame* framePointer = (CMainFrame*)GetParentFrame();
	return framePointer && framePointer->SetSimulatorFullscreen(fullscreen);
}

bool CSimulatorView::SendSimulatorInput(const Rtt::MSimulatorHost::Input& input)
{
	if (!mRuntimeEnvironmentPointer || !mRuntimeEnvironmentPointer->GetRuntime() || IsSimulationSuspended())
	{
		return false;
	}
	mPendingSimulatorInputs.push_back(input);
	if (!PostMessage(WMU_SEND_SIMULATOR_INPUT, 0, 0))
	{
		mPendingSimulatorInputs.pop_back();
		return false;
	}
	return true;
}

bool CSimulatorView::DispatchSimulatorInput(const Rtt::MSimulatorHost::Input& input)
{
	if (!mRuntimeEnvironmentPointer || !mRuntimeEnvironmentPointer->GetRuntime() || IsSimulationSuspended())
	{
		return false;
	}
	Rtt::Runtime* runtimePointer = mRuntimeEnvironmentPointer->GetRuntime();

	if (input.type == Rtt::MSimulatorHost::Input::kBackInput)
	{
		OnViewNavigateBack();
		return true;
	}
	if (input.type == Rtt::MSimulatorHost::Input::kKeyInput)
	{
		const Interop::Input::Key* keyPointer = Interop::Input::Key::FromCoronaName(input.keyName.c_str());
		int nativeKeyCode = input.hasNativeKeyCode ? input.nativeKeyCode :
			(keyPointer ? keyPointer->GetNativeCodeValue() : 0);
		if (!nativeKeyCode)
		{
			return false;
		}
		const char* qwertyKeyName = input.hasQwertyKeyName ? input.qwertyKeyName.c_str() : nullptr;
		auto dispatchKey = [&](Rtt::KeyEvent::Phase phase)
		{
			Rtt::KeyEvent event(
				nullptr, phase, input.keyName.c_str(), nativeKeyCode,
				input.isShiftDown, input.isAltDown, input.isCtrlDown, input.isCommandDown,
				qwertyKeyName);
			runtimePointer->DispatchEvent(event);
		};
		if (input.phase == Rtt::MSimulatorHost::Input::kDownPhase ||
			input.phase == Rtt::MSimulatorHost::Input::kPressedPhase)
		{
			dispatchKey(Rtt::KeyEvent::kDown);
		}
		if (input.phase == Rtt::MSimulatorHost::Input::kUpPhase ||
			input.phase == Rtt::MSimulatorHost::Input::kPressedPhase)
		{
			dispatchKey(Rtt::KeyEvent::kUp);
		}
		return input.phase == Rtt::MSimulatorHost::Input::kDownPhase ||
			input.phase == Rtt::MSimulatorHost::Input::kUpPhase ||
			input.phase == Rtt::MSimulatorHost::Input::kPressedPhase;
	}
	if (input.type == Rtt::MSimulatorHost::Input::kTextInput)
	{
		HWND renderSurfaceHandle = mCoronaContainerControl.GetCoronaControl().GetSafeHwnd();
		HWND focusedWindowHandle = ::GetFocus();
		if (focusedWindowHandle == renderSurfaceHandle)
		{
			Rtt::CharacterEvent event(nullptr, input.text.c_str());
			runtimePointer->DispatchEvent(event);
			return true;
		}
		if (!focusedWindowHandle || !::IsChild(renderSurfaceHandle, focusedWindowHandle))
		{
			return false;
		}

		WinString text(input.text.c_str());
		for (const wchar_t* character = text.GetUTF16(); *character; character++)
		{
			if (!::PostMessageW(focusedWindowHandle, WM_CHAR, *character, 0))
			{
				return false;
			}
		}
		return true;
	}
	if (input.type == Rtt::MSimulatorHost::Input::kTouchInput)
	{
		Rtt::TouchEvent::Phase phase;
		switch (input.phase)
		{
			case Rtt::MSimulatorHost::Input::kBeganPhase: phase = Rtt::TouchEvent::kBegan; break;
			case Rtt::MSimulatorHost::Input::kMovedPhase: phase = Rtt::TouchEvent::kMoved; break;
			case Rtt::MSimulatorHost::Input::kEndedPhase: phase = Rtt::TouchEvent::kEnded; break;
			case Rtt::MSimulatorHost::Input::kCancelledPhase: phase = Rtt::TouchEvent::kCancelled; break;
			default: return false;
		}
		Rtt::TouchEvent event(
			Rtt_FloatToReal((float)input.x), Rtt_FloatToReal((float)input.y),
			Rtt_FloatToReal((float)input.xStart), Rtt_FloatToReal((float)input.yStart), phase);
		event.SetId((const void*)1);
		if (runtimePointer->Platform().GetDevice().DoesNotify(Rtt::MPlatformDevice::kMultitouchEvent))
		{
			runtimePointer->DispatchEvent(Rtt::MultitouchEvent(&event, 1));
		}
		else
		{
			runtimePointer->DispatchEvent(event);
		}
		return true;
	}
	if (input.type == Rtt::MSimulatorHost::Input::kMouseInput)
	{
		Rtt::MouseEvent::MouseEventType eventType;
		switch (input.phase)
		{
			case Rtt::MSimulatorHost::Input::kDownPhase: eventType = Rtt::MouseEvent::kDown; break;
			case Rtt::MSimulatorHost::Input::kUpPhase: eventType = Rtt::MouseEvent::kUp; break;
			case Rtt::MSimulatorHost::Input::kDragPhase: eventType = Rtt::MouseEvent::kDrag; break;
			case Rtt::MSimulatorHost::Input::kMovePhase: eventType = Rtt::MouseEvent::kMove; break;
			case Rtt::MSimulatorHost::Input::kExitPhase: eventType = Rtt::MouseEvent::kExit; break;
			case Rtt::MSimulatorHost::Input::kScrollPhase: eventType = Rtt::MouseEvent::kScroll; break;
			default: return false;
		}
		Rtt::MouseEvent event(
			eventType,
			Rtt_FloatToReal((float)input.x), Rtt_FloatToReal((float)input.y),
			Rtt_FloatToReal((float)input.scrollX), Rtt_FloatToReal((float)input.scrollY),
			input.clickCount,
			input.isPrimaryButtonDown, input.isSecondaryButtonDown, input.isMiddleButtonDown,
			input.isShiftDown, input.isAltDown, input.isCtrlDown, input.isCommandDown);
		runtimePointer->DispatchEvent(event);
		return true;
	}
	return false;
}

LRESULT CSimulatorView::OnSendSimulatorInput(WPARAM wParam, LPARAM lParam)
{
	if (mPendingSimulatorInputs.empty())
	{
		return 0;
	}
	Rtt::MSimulatorHost::Input input = mPendingSimulatorInputs.front();
	mPendingSimulatorInputs.pop_front();
	return DispatchSimulatorInput(input) ? 1 : 0;
}

bool CSimulatorView::SimulateEvent(const Rtt::MSimulatorHost::Event& event)
{
	if (!mRuntimeEnvironmentPointer || !mRuntimeEnvironmentPointer->GetRuntime() || IsSimulationSuspended())
	{
		return false;
	}
	Rtt::Runtime* runtimePointer = mRuntimeEnvironmentPointer->GetRuntime();
	switch (event.type)
	{
		case Rtt::MSimulatorHost::Event::kMemoryWarningEvent:
		{
			Rtt::MemoryWarningEvent memoryWarningEvent;
			runtimePointer->DispatchEvent(memoryWarningEvent);
			return true;
		}
		case Rtt::MSimulatorHost::Event::kBackgroundEvent:
		{
			if (!IsSimulationSuspended())
			{
				SuspendResumeSimulationWithOverlay(false, true);
			}
			if (event.duration <= 0.0)
			{
				SuspendResumeSimulationWithOverlay(false, true);
			}
			else
			{
				if (mBackgroundTimerId)
				{
					KillTimer(mBackgroundTimerId);
				}
				UINT duration = event.duration < (double)MAXUINT ? (UINT)event.duration : MAXUINT;
				mBackgroundTimerId = SetTimer(1, duration, nullptr);
				if (!mBackgroundTimerId)
				{
					SuspendResumeSimulationWithOverlay(false, true);
					return false;
				}
			}
			return true;
		}
		case Rtt::MSimulatorHost::Event::kAccelerometerEvent:
		{
			double gravity[] = { event.xGravity, event.yGravity, event.zGravity };
			double instant[] = { event.xInstant, event.yInstant, event.zInstant };
			double raw[] = { event.xRaw, event.yRaw, event.zRaw };
			Rtt::AccelerometerEvent accelerometerEvent(
				gravity, instant, raw, event.isShake, event.deltaTime);
			runtimePointer->DispatchEvent(accelerometerEvent);
			return true;
		}
		case Rtt::MSimulatorHost::Event::kGyroscopeEvent:
		{
			Rtt::GyroscopeEvent gyroscopeEvent(
				event.xRotation, event.yRotation, event.zRotation, event.deltaTime);
			runtimePointer->DispatchEvent(gyroscopeEvent);
			return true;
		}
	}
	return false;
}

LRESULT CSimulatorView::OnApplySimulatorConfiguration(WPARAM wParam, LPARAM lParam)
{
	Rtt::MSimulatorHost::Configuration configuration = mPendingConfiguration;
	mIsRelaunchPending = false;

	if (configuration.deviceSelection == Rtt::MSimulatorHost::Configuration::kNamedDevice)
	{
		Rtt::TargetDevice::Skin skin = Rtt::TargetDevice::FindSkinForLabel(configuration.deviceId.c_str());
		if (skin == Rtt::TargetDevice::kUnknownSkin)
		{
			std::string projectIdentifier("project:");
			projectIdentifier.append(configuration.deviceId);
			skin = Rtt::TargetDevice::FindSkinForLabel(projectIdentifier.c_str());
		}
		if (skin != Rtt::TargetDevice::kUnknownSkin)
		{
			InitializeSimulation(skin, !configuration.temporary);
		}
	}
	else if (configuration.deviceSelection == Rtt::MSimulatorHost::Configuration::kCustomDevice)
	{
		InitializeCustomSimulation(configuration, !configuration.temporary);
	}
	else
	{
		ApplyConfigurationPersistence(configuration);
		RestartSimulation();
	}
	return 0;
}

void CSimulatorView::OnTimer(UINT_PTR timerId)
{
	if (timerId == mBackgroundTimerId)
	{
		KillTimer(timerId);
		mBackgroundTimerId = 0;
		if (IsSimulationSuspended())
		{
			SuspendResumeSimulationWithOverlay(false, true);
		}
	}
	CView::OnTimer(timerId);
}

// InitializeSimulation - select new skin and update
bool CSimulatorView::InitializeSimulation(Rtt::TargetDevice::Skin skinId, bool persist)
{
	CSimulatorApp* applicationPointer = (CSimulatorApp*)AfxGetApp();
	persist = persist && !applicationPointer->IsAgentModeEnabled();
	mDeviceName = Rtt::TargetDevice::LabelForSkin(skinId);
	mIsCustomDevice = false;
	mIsDeviceConfigurationTemporary = !persist;
	if (persist)
	{
		applicationPointer->PutDeviceName(mDeviceName);
	}

	bool skinLoaded = InitSkin(skinId);
	UpdateSimulatorSkin();

	// Draw the device skin onscreen.
	if (skinLoaded)
	{
		RestartSimulation();
	}

	return skinLoaded;
}

bool CSimulatorView::InitializeCustomSimulation(
	const Rtt::MSimulatorHost::Configuration& configuration, bool persist)
{
	CSimulatorApp* applicationPointer = (CSimulatorApp*)AfxGetApp();
	persist = persist && !applicationPointer->IsAgentModeEnabled();

	bool skinLoaded = InitSkin(Rtt::TargetDevice::fDefaultSkinID);
	mIsCustomDevice = true;
	mIsDeviceConfigurationTemporary = !persist;
	m_nSkinId = Rtt::TargetDevice::kUnknownSkin;
	mDeviceName = _T("Custom");
	mDeviceConfig.deviceName.Set("Custom");
	mDeviceConfig.deviceWidth = (float)configuration.width;
	mDeviceConfig.deviceHeight = (float)configuration.height;
	mDeviceConfig.safeAreaInsetTop = (float)configuration.safeAreaInsets.top;
	mDeviceConfig.safeAreaInsetLeft = (float)configuration.safeAreaInsets.left;
	mDeviceConfig.safeAreaInsetBottom = (float)configuration.safeAreaInsets.bottom;
	mDeviceConfig.safeAreaInsetRight = (float)configuration.safeAreaInsets.right;

	if (persist)
	{
		applicationPointer->PutDeviceName(mDeviceName);
		applicationPointer->PutCustomDeviceSettings(
			configuration.width, configuration.height,
			configuration.safeAreaInsets.top, configuration.safeAreaInsets.left,
			configuration.safeAreaInsets.bottom, configuration.safeAreaInsets.right);
	}

	UpdateSimulatorSkin();
	if (skinLoaded)
	{
		RestartSimulation();
	}
	return skinLoaded;
}

void CSimulatorView::ApplyConfigurationPersistence(
	const Rtt::MSimulatorHost::Configuration& configuration)
{
	CSimulatorApp* applicationPointer = (CSimulatorApp*)AfxGetApp();
	bool persist = !configuration.temporary && !applicationPointer->IsAgentModeEnabled();
	mIsDeviceConfigurationTemporary = !persist;
	if (!persist)
	{
		return;
	}

	CString deviceName = mDeviceName;
	bool isCustomDevice = mIsCustomDevice;
	int width = (int)mDeviceConfig.deviceWidth;
	int height = (int)mDeviceConfig.deviceHeight;
	int safeAreaTop = (int)mDeviceConfig.safeAreaInsetTop;
	int safeAreaLeft = (int)mDeviceConfig.safeAreaInsetLeft;
	int safeAreaBottom = (int)mDeviceConfig.safeAreaInsetBottom;
	int safeAreaRight = (int)mDeviceConfig.safeAreaInsetRight;

	if (configuration.deviceSelection == Rtt::MSimulatorHost::Configuration::kNamedDevice)
	{
		Rtt::TargetDevice::Skin skin = Rtt::TargetDevice::FindSkinForLabel(configuration.deviceId.c_str());
		if (skin == Rtt::TargetDevice::kUnknownSkin)
		{
			std::string projectIdentifier("project:");
			projectIdentifier.append(configuration.deviceId);
			skin = Rtt::TargetDevice::FindSkinForLabel(projectIdentifier.c_str());
		}
		deviceName = Rtt::TargetDevice::LabelForSkin(skin);
		isCustomDevice = false;
	}
	else if (configuration.deviceSelection == Rtt::MSimulatorHost::Configuration::kCustomDevice)
	{
		deviceName = _T("Custom");
		isCustomDevice = true;
		width = configuration.width;
		height = configuration.height;
		safeAreaTop = configuration.safeAreaInsets.top;
		safeAreaLeft = configuration.safeAreaInsets.left;
		safeAreaBottom = configuration.safeAreaInsets.bottom;
		safeAreaRight = configuration.safeAreaInsets.right;
	}

	applicationPointer->PutDeviceName(deviceName);
	if (isCustomDevice)
	{
		applicationPointer->PutCustomDeviceSettings(
			width, height, safeAreaTop, safeAreaLeft, safeAreaBottom, safeAreaRight);
	}
}

// UpdateSimulatorSkin - Update main window size & skin based on display type
void CSimulatorView::UpdateSimulatorSkin()
{
	CSimulatorApp *applicationPointer = (CSimulatorApp*)AfxGetApp();
	
	// Determine what the size of the client area of the window needs to be.
    UINT clientWidth = mDeviceConfig.deviceWidth;
    UINT clientHeight = mDeviceConfig.deviceHeight;
	
	// Validate width and height.
	if ((clientWidth <= 0) || (clientHeight <= 0))
	{
        return;
	}
	
	// Fetch the size of the window's client area, which the surface we render to.
	CMainFrame *pMainWnd = (CMainFrame*)GetParentFrame();
	if (!pMainWnd)
	{
		return;
	}
	CRect clientBounds;
	pMainWnd->GetClientRect(clientBounds);
	
	// Do not update window size if it is currently minimized.
	if (pMainWnd->IsIconic() || (clientBounds.Width() <= 0) || (clientBounds.Height() <= 0))
	{
		return;
	}
	
	// Set the client window size that will render the device skin and the Corona contents.
	if (!pMainWnd->IsSimulatorFullscreen())
	{
		clientBounds.top = 0;
		clientBounds.left = 0;
		clientBounds.right = clientWidth;
		clientBounds.bottom = clientHeight;
		pMainWnd->SizeToClient(clientBounds);
	}
	
	// Calculate the bounds of the Corona control.
	CRect coronaBounds;
	coronaBounds.CopyRect(&clientBounds);
	mCoronaContainerControl.MoveWindow(coronaBounds, FALSE);

    // Set size, position, and visibility of view window
	this->MoveWindow(clientBounds, TRUE);
	this->ShowWindow(SW_SHOW);
}

#ifdef _DEBUG
void CSimulatorView::AssertValid() const
{
	CView::AssertValid();
}

void CSimulatorView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CSimulatorDoc* CSimulatorView::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CSimulatorDoc)));
	return (CSimulatorDoc*)m_pDocument;
}
#endif //_DEBUG

#pragma endregion


#pragma region Private Functions
/// Open the document's currently selectd Corona project and starts simulation.
/// Shows the home screen if no project was selected and only if enabled in application preferences.
void CSimulatorView::RunCoronaProject()
{
	if (ValidateOpenGL() == false)
	{
		return;
 	}

    // Fetch the document's currently selected "main.lua" file.
    CString filePath = GetDocument()->GetPath();
    RunCoronaProject(CCoronaProject::RemoveMainLua(filePath));
}

/// Opens the given Corona project and starts simulation.
/// @param filePath The path to the directory containing the Corona project's "main.lua" file.
///                 Set this to an empty string to stop simulation and show a blank screen.
void CSimulatorView::RunCoronaProject(CString& projectPath)
{
	// Fetch the application object.
	CSimulatorApp *applicationPointer = ((CSimulatorApp*)AfxGetApp());
	if (!applicationPointer)
	{
		return;
	}

	CString sParentDir = projectPath;
	sParentDir = sParentDir.Left( sParentDir.ReverseFind(_T('\\')) );
	applicationPointer->SetWorkingDir( sParentDir );
	
	// If we're opening the "home screen" project, then show the home menu.
	// Otherwise, show the device simulator menu.
	auto frameWindowPointer = (CMainFrame*)GetParentFrame();
	if (frameWindowPointer && !applicationPointer->IsAgentModeEnabled())
	{
		// Only replace the menu if it needs changing. If we're already showing the right menu, do nothing.
		UINT nextMenuId = IDR_SIMULATOR_MENU;
		auto lastMenuPointer = frameWindowPointer->GetMenu();
		MENUINFO menuInfo{};
		menuInfo.cbSize = sizeof(menuInfo);
		menuInfo.fMask = MIM_MENUDATA;
		lastMenuPointer->GetMenuInfo(&menuInfo);
		if (menuInfo.dwMenuData != nextMenuId)
		{
			// Load a new menu from our resource table.
			CMenu newMenu;
			newMenu.LoadMenu(IDR_SIMULATOR_MENU);

			// Assign the loaded menu's resource ID to its info struct.
			// This is an optimization. We don't want to reload the menu everytime we start/stop a Corona project.
			memset(&menuInfo, 0, sizeof(menuInfo));
			menuInfo.cbSize = sizeof(menuInfo);
			menuInfo.fMask = MIM_MENUDATA;
			if (newMenu.GetMenuInfo(&menuInfo))
			{
				menuInfo.dwMenuData = nextMenuId;
				newMenu.SetMenuInfo(&menuInfo);
			}

			// Replace the window's menu.
			frameWindowPointer->SetMenu(&newMenu);
			frameWindowPointer->m_hMenuDefault = newMenu.GetSafeHmenu();
			lastMenuPointer->DestroyMenu();
			newMenu.Detach();
		}
	}
	
	// Do not load the project if the machine does not meet the minimum OpenGL requirements.
	if ((projectPath.GetLength() > 0) && (ValidateOpenGL() == false))
	{
		projectPath.Empty();
	}
	Rtt_LogEnable();
	
	// Terminate the last Corona runtime.
	Interop::SimulatorRuntimeEnvironment::Destroy(mRuntimeEnvironmentPointer);
	mRuntimeEnvironmentPointer = nullptr;

	// Load and run the application.
	if (projectPath.GetLength() > 0)
	{
		RECT bounds;

		mCoronaContainerControl.GetWindowRect(&bounds);

		mCoronaContainerControl.DestroyWindow();
		mCoronaContainerControl.Create(nullptr, WS_CHILD | WS_VISIBLE, bounds, this);

		// Show the Corona control before creating the runtime. The Corona runtime will render to this control.
		mCoronaContainerControl.GetCoronaControl().ShowWindow(SW_SHOW);
		mCoronaContainerControl.GetCoronaControl().SetFocus();

		// Set up the Corona runtime launch settings.
		Interop::SimulatorRuntimeEnvironment::CreationSettings settings;
		settings.ResourceDirectoryPath = projectPath;
		settings.MainWindowHandle = nullptr;			// <- Do not let the runtime take control of the main window.
		settings.RenderSurfaceHandle = mCoronaContainerControl.GetCoronaControl().GetSafeHwnd();
		settings.LoadedEventHandlerPointer = &mRuntimeLoadedEventHandler;
		// Provide the configuration of the device we will be simulating.
		settings.DeviceConfigPointer = &mDeviceConfig;

		// Set up the runtime for debug mode, if enabled.
		if (((CSimulatorApp*)::AfxGetApp())->IsDebugModeEnabled())
		{
			settings.LaunchOptions |= Rtt::Runtime::kConnectToDebugger;
		}

		// Create and startup the Corona runtime.
		// Note: This class' OnRuntimeLoaded() method will be called before this function returns if successfully loaded.
		auto result = Interop::SimulatorRuntimeEnvironment::CreateUsing(settings);

		// Display an error message if we've failed to load the Corona project.
		if (result.HasFailed() || !mRuntimeEnvironmentPointer)
		{
			CStringW title;
			title.LoadString(IDS_WARNING);
			auto errorMessage = result.GetMessageW();
			if (!errorMessage || (L'\0' == errorMessage[0]))
			{
				errorMessage = L"Failed to load Corona project.";
			}
			if (applicationPointer->IsAgentModeEnabled())
			{
				fwprintf(stderr, L"ERROR: %ls\n", errorMessage);
				applicationPointer->SetExitCode(1);
				::PostMessage(AfxGetMainWnd()->GetSafeHwnd(), WM_CLOSE, 0, 0);
			}
			else
			{
				::MessageBoxW(GetSafeHwnd(), errorMessage, (LPCWSTR)title, MB_OK | MB_ICONWARNING);
			}
		}
	}
	
	// Update the size of the window to match the project's configured skin, if specified.
	EnableWindow(TRUE);	// in case we were suspended
	if (!mRuntimeEnvironmentPointer)
	{
		UpdateSimulatorSkin();
		mCoronaContainerControl.SetWindowTextW(L"");
		mCoronaContainerControl.GetCoronaControl().ShowWindow(SW_HIDE);
	}

}

// SkinDeviceNameFromID - translate skin type from resource id
Rtt::TargetDevice::Skin CSimulatorView::SkinIDFromMenuID( UINT nMenuID )
{
	Rtt::TargetDevice::Skin skinID = (Rtt::TargetDevice::Skin) ( nMenuID - ID_VIEWAS_BEGIN );

	return skinID;
}

// InitSkin - Load device bitmap and rotate as needed
// Only called from InitializeSimulation()
bool CSimulatorView::InitSkin( Rtt::TargetDevice::Skin skinId )
{
	WinString skinFile;
	skinFile.SetUTF8( Rtt::TargetDevice::LuaObjectFileFromSkin( skinId ) );

	m_nSkinId = skinId;

	// Get the skins directory which is wherever the Lua file for the skin is
	wchar_t skinPathBuf[MAX_PATH];

	_tcsncpy_s(skinPathBuf, skinFile.GetTCHAR(), MAX_PATH);

	PathRemoveFileSpec(skinPathBuf);

	// Load the skin's configuration in Lua.
	Rtt::PlatformSimulator::LoadConfig(skinFile.GetUTF8(), mDeviceConfig);
	
	// Device skin was loaded successfully. It will be drawn within UpdateSimulatorSkin().
    return true;
}

// ValidateOpenGL - 
// Checks if the current OpenGL context meets Corona's minimum requirements.
// This function should be called after calling EnableOpenGL() and before simulating an app.
// Displays an error message if the minimum requirements have not been met.
// Returns true if the minimum OpenGL requirements for simulation have been met and
// it is okay to proceed to simulate an app.
bool CSimulatorView::ValidateOpenGL()
{
	// Verify that the control we want to render to meets Corona's minimum requirements.
	HWND windowHandle = mCoronaContainerControl.GetCoronaControl().GetSafeHwnd();
	auto result = Interop::RuntimeEnvironment::ValidateRenderSurface(windowHandle);

	// Fetch the renderer's version string.
	WinString rendererVersionString;
	rendererVersionString.SetUTF8(result.RendererVersion.GetString());
	if (rendererVersionString.IsEmpty())
	{
		rendererVersionString.SetUTF16(L"OpenGL Driver Version: Unknown");
	}
	if (((CSimulatorApp*)AfxGetApp())->IsAgentModeEnabled())
	{
		if (!result.CanRender)
		{
			fwprintf(stderr, L"ERROR: Solar2D cannot render with %ls.\n", rendererVersionString.GetTCHAR());
			((CSimulatorApp*)AfxGetApp())->SetExitCode(1);
			::PostMessage(AfxGetMainWnd()->GetSafeHwnd(), WM_CLOSE, 0, 0);
			return false;
		}
		if (!result.SupportsAllShaders)
		{
			fwprintf(stderr, L"WARNING: Some shaders are unsupported by %ls.\n", rendererVersionString.GetTCHAR());
		}
		if (!mCoronaContainerControl.IsWindowVisible())
		{
			mCoronaContainerControl.ShowWindow(SW_SHOW);
		}
		return true;
	}

	// Display a warning message if we can render, but not all graphics features will work.
	if (result.CanRender && (false == result.SupportsAllShaders))
	{
		CMessageDlg messageDlg;
		CString sMessage;
		CString sRequiredVersion;

		sRequiredVersion.Format(
				_T("%d.%d"), result.MinVersionSupported.GetMajorNumber(), result.MinVersionSupported.GetMinorNumber());
		sMessage.Format(IDS_OPENGL_VERSION_WARNING, sRequiredVersion, rendererVersionString.GetTCHAR());
		messageDlg.SetTitle(IDS_WARNING);
		messageDlg.SetText(sMessage);
		messageDlg.SetDefaultText(IDS_OK);
		messageDlg.SetIconStyle(MB_ICONEXCLAMATION);
		if (messageDlg.DoModal() != ID_MSG_BUTTON2)
		{
			// User has chosen not to continue. Fail the validation test.
			mCoronaContainerControl.ShowWindow(SW_HIDE);
			return false;
		}
	}

	// Display a major warning only once if we cannot render reliable at all.
	// This will most likely happen if the OpenGL driver is too old.
	static bool sDisableCheckForGL21 = false;
	if (!sDisableCheckForGL21 && !result.CanRender)
	{
		CMessageDlg messageDlg;
		CString sMessage;
		CString sRequiredVersion;

		sRequiredVersion.Format(
				_T("%d.%d"), result.MinVersionSupported.GetMajorNumber(), result.MinVersionSupported.GetMinorNumber());
		sMessage.Format(IDS_OPENGL21_VERSION_WARNING, sRequiredVersion, rendererVersionString.GetTCHAR());
		messageDlg.SetTitle(IDS_WARNING);
		messageDlg.SetText(sMessage);
		messageDlg.SetDefaultText(IDS_CANCEL);
		messageDlg.SetAltText(IDS_IGNORE);
		messageDlg.SetIconStyle(MB_ICONEXCLAMATION);
		if (messageDlg.DoModal() != ID_MSG_BUTTON2)
		{
			// User has chosen not to continue. Fail the validation test.
			mCoronaContainerControl.ShowWindow(SW_HIDE);
			return false;
		}
		else
		{
			// User chose to ignore the warning.
			// In this case, never warn the user about this issue again so as not to annoy him/her.
			sDisableCheckForGL21 = true;
		}
	}

	// The system successfully meets Corona's minimum graphics requirements
	// ...or there is a graphics issue and the user has chosen to continue at his/her own risk.
	if (!mCoronaContainerControl.IsWindowVisible())
	{
		mCoronaContainerControl.ShowWindow(SW_SHOW);
	}
	return true;
}

#pragma endregion


#pragma region CCoronaControlContainer Class
int CSimulatorView::CCoronaControlContainer::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	// Call the base class' method first.
	int result = CStatic::OnCreate(lpCreateStruct);
	if (-1 == result)
	{
		return -1;
	}

	// This center aligns this control's "Suspended" text horizontally and vertically.
	ModifyStyle(WS_TABSTOP, SS_CENTER | SS_CENTERIMAGE);

	// Create the Corona control.
	RECT bounds;
	bounds.top = 0;
	bounds.left = 0;
	bounds.bottom = lpCreateStruct->cy;
	bounds.right = lpCreateStruct->cx;
	mCoronaControl.Create(
			nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, bounds, this, 1);
	mCoronaControl.SetFocus();
	return 0;
}

HBRUSH CSimulatorView::CCoronaControlContainer::CtlColor(CDC* pDC, UINT nCtlColor)
{
	pDC->SetTextColor(RGB(255, 255, 255));
	pDC->SetBkMode(OPAQUE);
	pDC->SetBkColor(RGB(0, 0, 0));
	return (HBRUSH)GetStockObject(BLACK_BRUSH);
}

void CSimulatorView::CCoronaControlContainer::OnSize(UINT nType, int cx, int cy)
{
	// Resize the Corona control to match the new size of this container control
	mCoronaControl.SetWindowPos(nullptr, 0, 0, cx, cy, SWP_NOZORDER | SWP_NOMOVE);
}

CWnd& CSimulatorView::CCoronaControlContainer::GetCoronaControl()
{
	return mCoronaControl;
}

#pragma endregion
