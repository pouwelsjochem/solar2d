//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "SimulatorRuntimeEnvironment.h"
#include "Core\Rtt_Build.h"
#include "Core\Rtt_Allocator.h"
#include "Display\Rtt_Display.h"
#include "Interop\ApplicationServices.h"
#include "Resource.h"
#include "Rtt_Lua.h"
#include "Rtt_LuaContext.h"
#include "Rtt_PlatformPlayer.h"
#include "Rtt_Runtime.h"
#include "Rtt_WinPlatform.h"
#include "MainFrm.h"
#include "Simulator.h"
#include "SimulatorMessages.h"
#include "SimulatorView.h"
#include <exception>


namespace Interop {

#pragma region Constructors/Destructors
SimulatorRuntimeEnvironment::SimulatorRuntimeEnvironment(const SimulatorRuntimeEnvironment::CreationSettings& settings)
:	RuntimeEnvironment(settings),
	fLoadedEventHandler(this, &SimulatorRuntimeEnvironment::OnRuntimeLoaded),
	fTerminatingEventHandler(this, &SimulatorRuntimeEnvironment::OnRuntimeTerminating),
	fDeviceSimulatorServicesPointer(nullptr)
{
	// Create a device services object if given a device configuration to simulate.
	if (settings.DeviceConfigPointer)
	{
		fDeviceSimulatorServicesPointer = new DeviceSimulatorServices(this, settings.DeviceConfigPointer);
	}

	// Add event handlers.
	this->GetLoadedEventHandlers().Add(&fLoadedEventHandler);
	this->GetTerminatingEventHandlers().Add(&fTerminatingEventHandler);
}

SimulatorRuntimeEnvironment::~SimulatorRuntimeEnvironment()
{
	// Remove event handlers.
	this->GetLoadedEventHandlers().Remove(&fLoadedEventHandler);
	this->GetTerminatingEventHandlers().Remove(&fTerminatingEventHandler);

	// Delete our device simulator interface.
	if (fDeviceSimulatorServicesPointer)
	{
		delete fDeviceSimulatorServicesPointer;
		fDeviceSimulatorServicesPointer = nullptr;
	}
}

#pragma endregion


#pragma region Public Methods
MDeviceSimulatorServices* SimulatorRuntimeEnvironment::GetDeviceSimulatorServices() const
{
	return fDeviceSimulatorServicesPointer;
}

#pragma endregion


#pragma region Public Static Functions
SimulatorRuntimeEnvironment::CreationResult SimulatorRuntimeEnvironment::CreateUsing(
	const SimulatorRuntimeEnvironment::CreationSettings& settings)
{
	// Do not continue if given an invalid device configuration to simulate, if provided.
	if (settings.DeviceConfigPointer)
	{
		if ((settings.DeviceConfigPointer->deviceWidth <= 0) || (settings.DeviceConfigPointer->deviceHeight <= 0))
		{
			return CreationResult::FailedWith(L"Device configuration to simulate has an invalid screen width and height.");
		}
	}

	/// <summary>Returns the given string if not null. Returns an empty string if given null.</summary>
	/// <param name="text">The string to be returned if not null.</param>
#	define ReturnEmptyWStringIfNull(text) text ? text : L""

	// Fetch the Lua "system.SystemResourceDirectory" path equivalent.
	std::wstring systemResourceDirectoryPath(ReturnEmptyWStringIfNull(settings.SystemResourceDirectoryPath));
	if (systemResourceDirectoryPath.empty())
	{
		systemResourceDirectoryPath = ApplicationServices::GetDirectoryPath();
		systemResourceDirectoryPath += L"\\Resources";
	}

	// Fetch the Lua "system.ResourceDirectory" path equivalent.
	std::wstring resourceDirectoryPath(ReturnEmptyWStringIfNull(settings.ResourceDirectoryPath));
	auto index = resourceDirectoryPath.find_last_not_of(L"\\/");
	if (index == std::wstring::npos)
	{
		resourceDirectoryPath.clear();
	}
	else if ((index + 1) < resourceDirectoryPath.length())
	{
		resourceDirectoryPath.erase(index + 1);
	}
	if (resourceDirectoryPath.empty())
	{
		// If a resource directory was not provided, then use the application's directory by default.
		resourceDirectoryPath = ApplicationServices::GetDirectoryPath();
	}

	// Fetch a path to Corona's plugins directory.
	std::wstring pluginsDirectoryPath(ReturnEmptyWStringIfNull(settings.PluginsDirectoryPath));
	if (pluginsDirectoryPath.empty())
	{
		RuntimeEnvironment::CopySimulatorPluginDirectoryPathTo(pluginsDirectoryPath);
	}

	// Create a sandbox directory path for the given Corona project folder.
	std::wstring sandboxDirectoryPath;
	RuntimeEnvironment::GenerateSimulatorSandboxPath(resourceDirectoryPath.c_str(), sandboxDirectoryPath);

	// Fetch the Lua "system.DocumentsDirectory" path equivalent.
	std::wstring documentsDirectoryPath(ReturnEmptyWStringIfNull(settings.DocumentsDirectoryPath));
	if (documentsDirectoryPath.empty())
	{
		documentsDirectoryPath = sandboxDirectoryPath;
		documentsDirectoryPath.append(L"\\Documents");
	}

	// Fetch the Lua "system.ApplicationSupportDirectory" path equivalent.
	std::wstring applicationSupportDirectoryPath(ReturnEmptyWStringIfNull(settings.ApplicationSupportDirectoryPath));
	if (applicationSupportDirectoryPath.empty())
	{
		applicationSupportDirectoryPath = sandboxDirectoryPath;
		applicationSupportDirectoryPath.append(L"\\ApplicationSupport");
	}

	// Fetch the Lua "system.TemporaryDirectory" path equivalent.
	std::wstring temporaryDirectoryPath(ReturnEmptyWStringIfNull(settings.TemporaryDirectoryPath));
	if (temporaryDirectoryPath.empty())
	{
		temporaryDirectoryPath = sandboxDirectoryPath;
		temporaryDirectoryPath.append(L"\\TemporaryFiles");
	}

	// Fetch the Lua "system.CachesDirectory" path equivalent.
	std::wstring cachesDirectoryPath(ReturnEmptyWStringIfNull(settings.CachesDirectoryPath));
	if (cachesDirectoryPath.empty())
	{
		cachesDirectoryPath = sandboxDirectoryPath;
		cachesDirectoryPath.append(L"\\CachedFiles");
	}

	// Fetch the Lua "system.SystemCachesDirectory" path equivalent.
	std::wstring systemCachesDirectoryPath(ReturnEmptyWStringIfNull(settings.SystemCachesDirectoryPath));
	if (systemCachesDirectoryPath.empty())
	{
		systemCachesDirectoryPath = sandboxDirectoryPath;
		systemCachesDirectoryPath.append(L"\\.system");
	}

	// Create a copy of the given settings and update it with the simulator's default settings above.
	CreationSettings updatedSettings(settings);
	updatedSettings.ResourceDirectoryPath = resourceDirectoryPath.c_str();
	updatedSettings.DocumentsDirectoryPath = documentsDirectoryPath.c_str();
	updatedSettings.ApplicationSupportDirectoryPath = applicationSupportDirectoryPath.c_str();
	updatedSettings.TemporaryDirectoryPath = temporaryDirectoryPath.c_str();
	updatedSettings.CachesDirectoryPath = cachesDirectoryPath.c_str();
	updatedSettings.SystemCachesDirectoryPath = systemCachesDirectoryPath.c_str();
	updatedSettings.SystemResourceDirectoryPath = systemResourceDirectoryPath.c_str();
	updatedSettings.PluginsDirectoryPath = pluginsDirectoryPath.c_str();

	// Attempt to create the Corona runtime environment.
	SimulatorRuntimeEnvironment* environmentPointer = nullptr;
	try
	{
		environmentPointer = new SimulatorRuntimeEnvironment(updatedSettings);
	}
	catch (const std::exception& ex)
	{
		return CreationResult::FailedWith(ex.what());
	}
	catch (...) { }
	if (!environmentPointer)
	{
		// The constructor threw an exception.
		return CreationResult::FailedWith(L"Failed to create the Corona runtime environment.");
	}

	// Load and run a Corona project, if enabled.
	if (settings.IsRuntimeCreationEnabled)
	{
		auto result = environmentPointer->RunUsing(settings);
		if (result.HasFailed())
		{
			return CreationResult::FailedWith(result.GetMessage());
		}
	}

	// Return the newly created Corona runtime environment.
	return CreationResult::SucceededWith(environmentPointer);
}

void SimulatorRuntimeEnvironment::Destroy(SimulatorRuntimeEnvironment* environmentPointer)
{
	if (environmentPointer)
	{
		delete environmentPointer;
	}
}

#pragma endregion


#pragma region Private Methods
void SimulatorRuntimeEnvironment::OnRuntimeLoaded(RuntimeEnvironment& sender, const EventArgs& arguments)
{
	// Fetch all plugin in the "build.settings" file to be downloaded by the simulator's "shell.lua" script.
	sender.GetRuntime()->FindDownloadablePlugins("win32-sim");

	// Validate the Corona project's "build.settings" and "config.lua" files.
	Rtt::PlatformSimulator::ValidateSettings(sender.GetRuntime()->Platform());
}
void SimulatorRuntimeEnvironment::OnRuntimeTerminating(RuntimeEnvironment& sender, const EventArgs& arguments)
{

}

#pragma endregion


#pragma region DeviceSimulatorServices Class
static CSimulatorView* FetchSimulatorView()
{
	CFrameWnd* mainWindowPointer = dynamic_cast<CFrameWnd*>(::AfxGetMainWnd());
	return mainWindowPointer ? dynamic_cast<CSimulatorView*>(mainWindowPointer->GetActiveView()) : nullptr;
}

static UINT FetchWindowDpi(HWND windowHandle)
{
	// GetDpiForWindow is only exported by newer Windows versions, so avoid a static loader dependency.
	typedef UINT(WINAPI* GetDpiForWindowCallback)(HWND);
	GetDpiForWindowCallback getDpiForWindow = nullptr;
	HMODULE user32Module = ::GetModuleHandleW(L"user32.dll");
	if (user32Module)
	{
		getDpiForWindow = reinterpret_cast<GetDpiForWindowCallback>(
			::GetProcAddress(user32Module, "GetDpiForWindow"));
	}

	UINT dpi = getDpiForWindow ? getDpiForWindow(windowHandle) : 0;
	if (!dpi)
	{
		HDC deviceContext = ::GetDC(windowHandle);
		if (deviceContext)
		{
			dpi = ::GetDeviceCaps(deviceContext, LOGPIXELSX);
			::ReleaseDC(windowHandle, deviceContext);
		}
	}
	return dpi ? dpi : 96;
}

static void ReadSimulatorDevice(CSimulatorView& view, Rtt::MSimulatorHost::Device& result)
{
	result = Rtt::MSimulatorHost::Device();
	const Rtt::PlatformSimulator::Config& config = view.GetDeviceConfig();
	if (view.IsCustomDevice())
	{
		result.id = "custom";
		result.name = "Custom";
		result.category = "Custom";
		result.isCustom = true;
	}
	else
	{
		Rtt::TargetDevice::Skin skin = view.GetDeviceSkin();
		const char* identifier = Rtt::TargetDevice::LabelForSkin(skin);
		const char* name = Rtt::TargetDevice::NameForSkin(skin);
		const char* category = Rtt::TargetDevice::CategoryForSkin(skin);
		result.id = identifier ? identifier : "";
		result.name = name ? name : "";
		result.category = category ? category : "";
		result.isProject = result.id.compare(0, 8, "project:") == 0;
	}
	result.width = (int)config.deviceWidth;
	result.height = (int)config.deviceHeight;
	result.safeAreaInsets.top = (int)config.safeAreaInsetTop;
	result.safeAreaInsets.left = (int)config.safeAreaInsetLeft;
	result.safeAreaInsets.bottom = (int)config.safeAreaInsetBottom;
	result.safeAreaInsets.right = (int)config.safeAreaInsetRight;
	result.hasRoundedCorners = true;
	result.roundedCorners = false;
	result.isCurrent = true;
}

SimulatorRuntimeEnvironment::DeviceSimulatorServices::DeviceSimulatorServices(
	SimulatorRuntimeEnvironment* environmentPointer,
	const Rtt::PlatformSimulator::Config* deviceConfigPointer)
:	fEnvironmentPointer(environmentPointer),
	fDeviceConfigPointer(deviceConfigPointer)
{
	if (!fEnvironmentPointer || !fDeviceConfigPointer)
	{
		throw std::invalid_argument(nullptr);
	}
}

const Rtt::PlatformSimulator::Config* SimulatorRuntimeEnvironment::DeviceSimulatorServices::GetDeviceConfig() const
{
	return fDeviceConfigPointer;
}

bool SimulatorRuntimeEnvironment::DeviceSimulatorServices::IsAgentModeEnabled() const
{
	auto applicationPointer = ((CSimulatorApp*)::AfxGetApp());
	return applicationPointer && applicationPointer->IsAgentModeEnabled();
}

bool SimulatorRuntimeEnvironment::DeviceSimulatorServices::IsLuaExitAllowed() const
{
	auto applicationPointer = ((CSimulatorApp*)::AfxGetApp());
	if (applicationPointer)
	{
		return applicationPointer->IsLuaExitAllowed();
	}
	return false;
}

void* SimulatorRuntimeEnvironment::DeviceSimulatorServices::ShowNativeAlert(
	const char *title, const char *message, const char **buttonLabels, int buttonCount, Rtt::LuaResource *resource)
{
	// Fetch the Corona Simulator view.
	CSimulatorView *viewPointer = nullptr;
	CFrameWnd *mainWindowPointer = dynamic_cast<CFrameWnd*>(::AfxGetMainWnd());
	if (mainWindowPointer)
	{
		viewPointer = dynamic_cast<CSimulatorView*>(mainWindowPointer->GetActiveView());
	}
	if (nullptr == viewPointer)
	{
		return nullptr;
	}

	// Send a user-defined message to the CSimulatorView, which will show the native alert message box.
	WMU_ALERT_PARAMS WAP{};
	WAP.sTitle = title;
	WAP.sMsg = message;
	WAP.nButtonLabels = buttonCount;
	WAP.psButtonLabels = buttonLabels;
	WAP.pLuaResource = resource;
	WAP.hwnd = nullptr;
	::SendMessage(viewPointer->GetSafeHwnd(), WMU_NATIVEALERT, 1, (LPARAM)&WAP);

	// Return the message box's window handle.
	return WAP.hwnd;
}

void SimulatorRuntimeEnvironment::DeviceSimulatorServices::CancelNativeAlert(void* alertReference)
{
	if (alertReference)
	{
		::SendMessage((HWND)alertReference, WM_COMMAND, IDCANCEL, 0);
	}
}

void SimulatorRuntimeEnvironment::DeviceSimulatorServices::RequestRestart()
{
	// Fetch the Corona Simulator view.
	CSimulatorView *viewPointer = nullptr;
	CFrameWnd *mainWindowPointer = dynamic_cast<CFrameWnd*>(::AfxGetMainWnd());
	if (mainWindowPointer)
	{
		viewPointer = dynamic_cast<CSimulatorView*>(mainWindowPointer->GetActiveView());
	}
	if (nullptr == viewPointer)
	{
		return;
	}

	// Restart the runtime's Corona project by invoking the "File\Relaunch" menu item in the Corona Simulator window.
	// Note: This assumes that only one Corona project can be simulated at a time.
	::PostMessage(viewPointer->GetSafeHwnd(), WM_COMMAND, ID_FILE_RELAUNCH, 0);
}

void SimulatorRuntimeEnvironment::DeviceSimulatorServices::RequestTerminate()
{
	// Close the Corona Simulator window.
	CFrameWnd *mainWindowPointer = dynamic_cast<CFrameWnd*>(::AfxGetMainWnd());
	if (!mainWindowPointer)
	{
		return;
	}

	::PostMessage(mainWindowPointer->GetSafeHwnd(), WM_CLOSE, 0, 0);
}

bool SimulatorRuntimeEnvironment::DeviceSimulatorServices::GetCurrentDevice(
	Rtt::MSimulatorHost::Device& result) const
{
	CSimulatorView* viewPointer = FetchSimulatorView();
	if (!viewPointer)
	{
		return false;
	}
	ReadSimulatorDevice(*viewPointer, result);
	return true;
}

bool SimulatorRuntimeEnvironment::DeviceSimulatorServices::GetState(
	Rtt::MSimulatorHost::State& result) const
{
	CSimulatorView* viewPointer = FetchSimulatorView();
	CMainFrame* framePointer = dynamic_cast<CMainFrame*>(::AfxGetMainWnd());
	if (!viewPointer || !framePointer || !GetCurrentDevice(result.device))
	{
		return false;
	}

	result.isSuspended = viewPointer->IsSimulationSuspended();
	result.safeAreaGuidesVisible = false;
	result.isRelaunchPending = viewPointer->IsRelaunchPending();
	result.relaunchCount = viewPointer->GetRelaunchCount();

	CRect windowBounds;
	framePointer->GetWindowRect(windowBounds);
	UINT windowDpi = FetchWindowDpi(framePointer->GetSafeHwnd());
	result.window.backingScale = (double)windowDpi / 96.0;
	result.window.x = windowBounds.left / result.window.backingScale;
	result.window.y = windowBounds.top / result.window.backingScale;
	result.window.width = windowBounds.Width() / result.window.backingScale;
	result.window.height = windowBounds.Height() / result.window.backingScale;
	result.window.isFullscreen = framePointer->IsSimulatorFullscreen();
	return true;
}

bool SimulatorRuntimeEnvironment::DeviceSimulatorServices::GetDevices(
	std::vector<Rtt::MSimulatorHost::Device>& result) const
{
	CSimulatorView* viewPointer = FetchSimulatorView();
	if (!viewPointer)
	{
		return false;
	}

	result.clear();
	result.reserve(Rtt::TargetDevice::fSkinCount + 1);
	for (int index = 0; index < Rtt::TargetDevice::fSkinCount; index++)
	{
		Rtt::MSimulatorHost::Device device;
		const char* identifier = Rtt::TargetDevice::LabelForSkin(index);
		const char* name = Rtt::TargetDevice::NameForSkin(index);
		const char* category = Rtt::TargetDevice::CategoryForSkin(index);
		device.id = identifier ? identifier : "";
		device.name = name ? name : "";
		device.category = category ? category : "";
		device.width = Rtt::TargetDevice::WidthForSkin(index);
		device.height = Rtt::TargetDevice::HeightForSkin(index);
		device.isProject = device.id.compare(0, 8, "project:") == 0;
		device.hasRoundedCorners = true;
		device.roundedCorners = false;
		device.isCurrent = !viewPointer->IsCustomDevice() && viewPointer->GetDeviceSkin() == index;
		device.safeAreaInsets.top = Rtt::TargetDevice::SafeAreaInsetTopForSkin(index);
		device.safeAreaInsets.left = Rtt::TargetDevice::SafeAreaInsetLeftForSkin(index);
		device.safeAreaInsets.bottom = Rtt::TargetDevice::SafeAreaInsetBottomForSkin(index);
		device.safeAreaInsets.right = Rtt::TargetDevice::SafeAreaInsetRightForSkin(index);
		result.push_back(device);
	}

	Rtt::MSimulatorHost::Device customDevice;
	customDevice.id = "custom";
	customDevice.name = "Custom";
	customDevice.category = "Custom";
	customDevice.isCustom = true;
	customDevice.hasRoundedCorners = true;
	customDevice.roundedCorners = false;
	customDevice.isCurrent = viewPointer->IsCustomDevice();
	if (customDevice.isCurrent)
	{
		const Rtt::PlatformSimulator::Config& config = viewPointer->GetDeviceConfig();
		customDevice.width = (int)config.deviceWidth;
		customDevice.height = (int)config.deviceHeight;
		customDevice.safeAreaInsets.top = (int)config.safeAreaInsetTop;
		customDevice.safeAreaInsets.left = (int)config.safeAreaInsetLeft;
		customDevice.safeAreaInsets.bottom = (int)config.safeAreaInsetBottom;
		customDevice.safeAreaInsets.right = (int)config.safeAreaInsetRight;
	}
	else
	{
		CSimulatorApp* applicationPointer = (CSimulatorApp*)::AfxGetApp();
		applicationPointer->GetCustomDeviceSettings(
			customDevice.width, customDevice.height,
			customDevice.safeAreaInsets.top, customDevice.safeAreaInsets.left,
			customDevice.safeAreaInsets.bottom, customDevice.safeAreaInsets.right);
	}
	result.push_back(customDevice);
	return true;
}

Rtt::MSimulatorHost::ConfigureResult
SimulatorRuntimeEnvironment::DeviceSimulatorServices::ConfigureAndRelaunch(
	const Rtt::MSimulatorHost::Configuration& configuration, bool onlyIfNeeded) const
{
	CSimulatorView* viewPointer = FetchSimulatorView();
	return viewPointer ? viewPointer->ConfigureAndRelaunch(configuration, onlyIfNeeded) :
		Rtt::MSimulatorHost::kConfigureFailed;
}

bool SimulatorRuntimeEnvironment::DeviceSimulatorServices::Relaunch() const
{
	CSimulatorView* viewPointer = FetchSimulatorView();
	if (!viewPointer)
	{
		return false;
	}
	return !!::PostMessage(viewPointer->GetSafeHwnd(), WM_COMMAND, ID_FILE_RELAUNCH, 0);
}

bool SimulatorRuntimeEnvironment::DeviceSimulatorServices::SetSafeAreaGuidesVisible(bool visible) const
{
	return !visible;
}

bool SimulatorRuntimeEnvironment::DeviceSimulatorServices::SetFullscreen(bool fullscreen) const
{
	CSimulatorView* viewPointer = FetchSimulatorView();
	return viewPointer && viewPointer->SetSimulatorFullscreen(fullscreen);
}

bool SimulatorRuntimeEnvironment::DeviceSimulatorServices::SendInput(
	const Rtt::MSimulatorHost::Input& input) const
{
	CSimulatorView* viewPointer = FetchSimulatorView();
	return viewPointer && viewPointer->SendSimulatorInput(input);
}

bool SimulatorRuntimeEnvironment::DeviceSimulatorServices::Simulate(
	const Rtt::MSimulatorHost::Event& event) const
{
	CSimulatorView* viewPointer = FetchSimulatorView();
	return viewPointer && viewPointer->SimulateEvent(event);
}

bool SimulatorRuntimeEnvironment::DeviceSimulatorServices::Quit(int exitCode) const
{
	CSimulatorApp* applicationPointer = (CSimulatorApp*)::AfxGetApp();
	CFrameWnd* mainWindowPointer = dynamic_cast<CFrameWnd*>(::AfxGetMainWnd());
	if (!applicationPointer || !mainWindowPointer)
	{
		return false;
	}
	applicationPointer->SetExitCode(exitCode);
	return !!::PostMessage(mainWindowPointer->GetSafeHwnd(), WM_CLOSE, 0, 0);
}

#pragma endregion

}	// namespace Interop
