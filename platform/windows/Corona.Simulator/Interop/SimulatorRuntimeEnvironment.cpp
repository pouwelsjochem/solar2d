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
#include "Simulator.h"
#include "SimulatorView.h"
#include "WinGlobalProperties.h"
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
		if (false == settings.DeviceConfigPointer->configLoaded)
		{
			return CreationResult::FailedWith(L"Failed to load device configuration for the simulator.");
		}
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
	if (systemResourceDirectoryPath.empty() && settings.IsRuntimeCreationEnabled )
	{
		auto appProperties = GetWinProperties();
		if (appProperties && appProperties->GetResourcesDir())
		{
			WinString stringConverter;
			stringConverter.SetUTF8(appProperties->GetResourcesDir());
			systemResourceDirectoryPath = stringConverter.GetUTF16();
		}
	}
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

#pragma endregion


#pragma region DeviceSimulatorServices Class
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

	// Terminate the runtime by invoking the "File\Close Project" menu item in the Corona Simulator window.
	// Note: This assumes that only one Corona project can be simulated at a time.
	::PostMessage(viewPointer->GetSafeHwnd(), WM_COMMAND, ID_FILE_CLOSE, 0);
}

#pragma endregion

}	// namespace Interop
