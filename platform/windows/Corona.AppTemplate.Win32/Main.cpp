//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CoronaWin32.h"
#include "resource.h"
#include "StartupDiagnostics.h"
#include <CommCtrl.h>
#include <Windows.h>


/// <summary>The Win32 handle to the main application window.</summary>
static HWND sMainWindowHandle = nullptr;

/// <summary>The Win32 handle to a child control that Corona will render to.</summary>
static HWND sRenderSurfaceWindowHandle = nullptr;

enum ApplicationExitCode
{
	kExitCodeSuccess = 0,
	kExitCodeMainWindowCreationFailed = 10,
	kExitCodeRenderSurfaceCreationFailed = 11,
	kExitCodeRuntimeStartFailed = 12,
	kExitCodeMessageLoopFailed = 13,
};

/// <summary>Exit code to be returned to the process which launched this application.</summary>
static int sExitCode = kExitCodeSuccess;


/// <summary>
///  A "WndProc" C callback that gets invoked when a Windows message has been received by the main application window.
/// </summary>
/// <param name="windowHandle">Handle to the window or control that is receiving the message.</param>
/// <param name="messageId">Unique integer ID of the message such as WM_CLOSE, WM_SiZE, etc.</param>
/// <param name="wParam">Additional information assigned to the message.</para>
/// <param name="lParam">Additional information assigned to the message.</para>
/// <returns>Returns a value to the source of the Windows message.</returns>
LRESULT CALLBACK OnProcessWindowMessage(HWND windowHandle, UINT messageId, WPARAM wParam, LPARAM lParam)
{
	switch (messageId)
	{
		case WM_SETFOCUS:
		{
			// The main window has just received the focus. This can happen after moving/resizing the window.
			// Change focus back to the child render surface control so that it'll continue to receive key events.
			::SetFocus(sRenderSurfaceWindowHandle);
			break;
		}
		case WM_SIZE:
		{
			// The main window has been resized.
			// Resize the child render surface control to completely fill the new client bounds.
			RECT bounds{};
			::GetClientRect(windowHandle, &bounds);
			::SetWindowPos(sRenderSurfaceWindowHandle, nullptr, 0, 0, (bounds.right - bounds.left), (bounds.bottom - bounds.top), SWP_NOZORDER);
			break;
		}
		case WM_ERASEBKGND:
		{
			// Don't allow Windows to repainting the window's client area. This achieves the following:
			// 1) Prevents a black flickering effect on every repaint of the window and its child control.
			// 2) Small performance boost because we don't need to paint in the window's client area since the
			//    the child control completely cover the background.
			return 1;
		}
		case WM_DESTROY:
		{
			// The main window has been closed.
			// Post a quit message so that we can exit out of the main message loop below and quit the app.
			::PostQuitMessage(sExitCode);
			sMainWindowHandle = nullptr;
			return 0;
		}
	}
	return ::DefWindowProc(windowHandle, messageId, wParam, lParam);
}

/// <summary>
///  A "WndProc" C callback that gets invoked when a Windows message has been received by the child render surface control.
/// </summary>
/// <param name="windowHandle">Handle to the window or control that is receiving the message.</param>
/// <param name="messageId">Unique integer ID of the message such as WM_CLOSE, WM_SiZE, etc.</param>
/// <param name="wParam">Additional information assigned to the message.</para>
/// <param name="lParam">Additional information assigned to the message.</para>
/// <returns>Returns a value to the source of the Windows message.</returns>
LRESULT CALLBACK OnProcessControlMessage(HWND windowHandle, UINT messageId, WPARAM wParam, LPARAM lParam)
{
	return ::DefWindowProc(windowHandle, messageId, wParam, lParam);
}

/// <summary>The application's main function that gets called on startup.</summary>
/// <param name="instanceHandle">Handle assigned to this application instance by the operating system.</param>
/// <param name="previousInstanceHandle">This parameter is always null.</param>
/// <param name="commandLineString">
///  <para>String providing all of the command line arguments passed to the application.</para>
///  <para>This does not include the application name/path as the first argument.</para>
/// </param>
/// <param name="showWindowState">
///  <para>The show state requested by shell such as SW_MAXIMIZE, SW_MINIMZE, etc.</para>
///  <para>This usually comes from the application shortcut's "Run" field.</para>
/// </param>
/// <returns>
///  <para>Returns the WM_QUIT message's wParam value when the application terminates gracefully.</para>
///  <para>Returns a stable non-zero error code if startup or the message loop fails.</para>
/// </returns>
int APIENTRY wWinMain(
	_In_ HINSTANCE instanceHandle, _In_opt_ HINSTANCE previousInstanceHandle,
	_In_ LPWSTR commandLineString, _In_ int showWindowState)
{
	StartupDiagnostics diagnostics;
	diagnostics.Start();
	diagnostics.Log("stage=process-entry");

	// Enable themed controls and support for version 6 (or newer) of the "ComCtl32.dll" library.
	::InitCommonControls();
	diagnostics.Log("stage=common-controls-initialized");

#ifdef _DEBUG
	// Enable memory leak tracking.
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	// Ignore compiler warnings for the following unused function arguments.
	UNREFERENCED_PARAMETER(previousInstanceHandle);

	// Load the application icon to be used by the main window.
	HICON largeAppIconHandle = nullptr;
	HICON smallAppIconHandle = nullptr;
	{
		// First, attempt to load the appropriate icon sizes matching the system's current DPI scale factor.
		// Note: The below API is only supported on Windows Vista and newer OS versions.
		typedef HRESULT(WINAPI *LoadIconMetricCallback)(HINSTANCE, PCWSTR, int, HICON*);
		HMODULE moduleHandle = ::LoadLibraryW(L"ComCtl32");
		LoadIconMetricCallback loadIconMetricCallback = nullptr;
		if (moduleHandle)
		{
			loadIconMetricCallback = (LoadIconMetricCallback)::GetProcAddress(moduleHandle, "LoadIconMetric");
		}
		if (loadIconMetricCallback)
		{
			loadIconMetricCallback(instanceHandle, MAKEINTRESOURCEW(IDI_MAIN_APP), LIM_LARGE, &largeAppIconHandle);
			loadIconMetricCallback(instanceHandle, MAKEINTRESOURCEW(IDI_MAIN_APP), LIM_SMALL, &smallAppIconHandle);
		}

		// If we've failed to load icons above, then fallback to loading them via older Win32 APIs.
		// Note: This is always the case for Windows XP.
		if (!largeAppIconHandle)
		{
			largeAppIconHandle = ::LoadIconW(instanceHandle, MAKEINTRESOURCEW(IDI_MAIN_APP));
		}
		if (!smallAppIconHandle)
		{
			int iconWidth = ::GetSystemMetrics(SM_CXSMICON);
			int iconHeight = ::GetSystemMetrics(SM_CYSMICON);
			if (iconWidth <= 0)
			{
				iconWidth = 16;
			}
			if (iconHeight <= 0)
			{
				iconHeight = 16;
			}
			smallAppIconHandle = (HICON)::LoadImageW(
					instanceHandle, MAKEINTRESOURCEW(IDI_MAIN_APP), IMAGE_ICON, iconWidth, iconHeight, LR_DEFAULTCOLOR);
			if (!smallAppIconHandle)
			{
				smallAppIconHandle = largeAppIconHandle;
			}
		}
	}

	// Create and configure the main application window.
	// Note: This window is invisible/hidden until ShowWindow() has been called on it.
	//       We don't want to show the window until we let Corona do size it according to the "build.settings" file.
	const wchar_t kDefaultWindowClassName[] = L"CoronaLabs.Corona.App.Window";
	const wchar_t kDefaultWindowTitleText[] = L"";
	{
		WNDCLASSEXW windowClassSettings{};
		windowClassSettings.cbSize = sizeof(windowClassSettings);
		windowClassSettings.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
		windowClassSettings.lpfnWndProc = ::OnProcessWindowMessage;
		windowClassSettings.cbClsExtra = 0;
		windowClassSettings.cbWndExtra = 0;
		windowClassSettings.hInstance = instanceHandle;
		windowClassSettings.hIcon = largeAppIconHandle;
		windowClassSettings.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
		windowClassSettings.hbrBackground = (HBRUSH)::GetStockObject(BLACK_BRUSH);
		windowClassSettings.lpszClassName = kDefaultWindowClassName;
		windowClassSettings.hIconSm = smallAppIconHandle;
		auto atom = ::RegisterClassExW(&windowClassSettings);
		sMainWindowHandle = ::CreateWindowW(
				windowClassSettings.lpszClassName, kDefaultWindowTitleText, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0,
				CW_USEDEFAULT, 0, nullptr, nullptr, instanceHandle, nullptr);
	}
	if (!sMainWindowHandle)
	{
		auto errorCode = ::GetLastError();
		diagnostics.LogLastError("main-window-create-failed", errorCode);
		diagnostics.ShowStartupError(
			nullptr, L"S2D-WIN-WINDOW-CREATE",
			L"The application could not create its main window.", errorCode);
		return kExitCodeMainWindowCreationFailed;
	}
	diagnostics.Log("stage=main-window-created");

	// If this application is being requested to display as maximized or minimized on launch, then handle it now.
	// Note: This show state usually comes from the application shortcut's "Run" field.
	switch (showWindowState)
	{
		case SW_MINIMIZE:
		case SW_SHOWMINIMIZED:
		case SW_SHOWMINNOACTIVE:
		case SW_FORCEMINIMIZE:
			// We were requested to launch as minimized.
			// This causes the client area to have a zero width/height at launch, which causes launch errors in Corona.
			// Work-around this by not honoring the minimized mode and launching in normal mode instead.
			showWindowState = SW_RESTORE;
			break;
		case SW_MAXIMIZE:
			// We were requested to launch as maximized.
			// We should honor this launch window mode over the "build.settings" file's "defaultMode" window setting.
			::ShowWindow(sMainWindowHandle, showWindowState);
			::UpdateWindow(sMainWindowHandle);
			break;
	}

	// Create and configure a child control for Corona to render to.
	// This child control will be automatically sized to fit the main window's client area via WM_SIZE messages above.
	// Note: Rendering to a child control forces Windows to render OpenGL in non-exclusive fullscreen mode when
	//       displaying the window as a maximized borderless window, which allows us to display native Win32 UI.
	HWND testSurfaceHandle = NULL;
	{
		RECT clientBounds{};
		::GetClientRect(sMainWindowHandle, &clientBounds);
		DWORD controlStyles = WS_TABSTOP | WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
		WNDCLASSEXW windowClassSettings{};
		windowClassSettings.cbSize = sizeof(windowClassSettings);
		windowClassSettings.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
		windowClassSettings.lpfnWndProc = ::OnProcessControlMessage;
		windowClassSettings.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
		windowClassSettings.hInstance = instanceHandle;
		windowClassSettings.hbrBackground = (HBRUSH)::GetStockObject(BLACK_BRUSH);
		windowClassSettings.lpszClassName = L"CoronaLabs.Corona.RenderSurface";
		auto atom = ::RegisterClassExW(&windowClassSettings);
		testSurfaceHandle = ::CreateWindowW(
				windowClassSettings.lpszClassName, nullptr, controlStyles, 0, 0,
				clientBounds.right - clientBounds.left, clientBounds.bottom - clientBounds.top,
				sMainWindowHandle, nullptr, instanceHandle, nullptr);
		if (!testSurfaceHandle)
		{
			diagnostics.LogLastError("graphics-test-surface-create-failed", ::GetLastError());
		}
		ShowWindow(testSurfaceHandle, SW_HIDE);
		sRenderSurfaceWindowHandle = ::CreateWindowW(
				windowClassSettings.lpszClassName, nullptr, controlStyles, 0, 0,
				clientBounds.right - clientBounds.left, clientBounds.bottom - clientBounds.top,
				sMainWindowHandle, nullptr, instanceHandle, nullptr);
	}
	if (!sRenderSurfaceWindowHandle)
	{
		auto errorCode = ::GetLastError();
		diagnostics.LogLastError("render-surface-create-failed", errorCode);
		diagnostics.ShowStartupError(
			sMainWindowHandle, L"S2D-WIN-RENDER-SURFACE-CREATE",
			L"The application could not create its rendering surface.", errorCode);
		return kExitCodeRenderSurfaceCreationFailed;
	}
	::SetFocus(sRenderSurfaceWindowHandle);
	diagnostics.Log("stage=render-surface-created");

	// Create and configure the Corona runtime.
	bool hasCoronaRuntimeStarted = false;
	Corona::Win32::Runtime coronaRuntime;
	Corona::Win32::LaunchSettings& settings = coronaRuntime.GetLaunchSettings();
	settings.SetMainWindowHandle(sMainWindowHandle);
	settings.SetRenderSurfaceHandle(sRenderSurfaceWindowHandle);
	settings.SetTestSurfaceHandle(testSurfaceHandle);

	// Fetch the command line arguments (if any) and copy them to Corona's launch settings.
	bool hasCommandLineArguments = false;
	if (commandLineString && (commandLineString[0] != L'\0'))
	{
		hasCommandLineArguments = true;
		int argumentCount = 0;
		auto argumentArray = ::CommandLineToArgvW(commandLineString, &argumentCount);
		if (argumentArray)
		{
			for (int argumentIndex = 0; argumentIndex < argumentCount; argumentIndex++)
			{
				// Fetch the next command line argument.
				auto argumentStringPointer = argumentArray[argumentIndex];
				if (!argumentStringPointer)
				{
					continue;
				}

				// Add the next command line argument to Corona's launch settings.
				settings.AddLaunchArgument(argumentStringPointer);
			}
			::LocalFree(argumentArray);
		}
	}

	// Setting the resource directory to null makes Corona use the EXE directory by default.
	settings.SetResourceDirectory(nullptr);

	// Start the Corona runtime.
	diagnostics.Log("stage=runtime-starting launch-argument-count=%d", settings.GetLaunchArgumentCount());
	hasCoronaRuntimeStarted = coronaRuntime.Run();

	// Update the main application window.
	if (!hasCoronaRuntimeStarted)
	{
		sExitCode = kExitCodeRuntimeStartFailed;
		diagnostics.Log("stage=runtime-start-failed exit-code=%d", sExitCode);
		// We've failed to start the Corona runtime. Close the main application window.
		// Note: The Corona runtime would have already displayed an error message to the user at this point.
		::PostMessageW(sMainWindowHandle, WM_CLOSE, 0, 0);
	}
	else
	{
		diagnostics.Log("stage=runtime-started");
		// We've successfully started the Corona runtime.
		// If the main application window hasn't been made visible yet, then do so now.
		if (::IsWindowVisible(sMainWindowHandle) == FALSE)
		{
			::ShowWindow(sMainWindowHandle, SW_SHOW);
		}
		diagnostics.Log("stage=main-window-visible");
	}

	// Run the window's main message loop.
	// This loop will run until a WM_QUIT message has been received, causing GetMessage() to return false.
	MSG message{};
	BOOL getMessageResult = 0;
	diagnostics.Log("stage=message-loop-started");
	while ((getMessageResult = ::GetMessage(&message, nullptr, 0, 0)) > 0)
	{
		::TranslateMessage(&message);
		::DispatchMessage(&message);
	}
	if (getMessageResult < 0)
	{
		sExitCode = kExitCodeMessageLoopFailed;
		diagnostics.LogLastError("message-loop-failed", ::GetLastError());
	}

	// The application is terminating.
	// Return the exit code provided by the WM_QUIT message.
	diagnostics.Log("stage=process-exit exit-code=%d", sExitCode);
	return sExitCode;
}
