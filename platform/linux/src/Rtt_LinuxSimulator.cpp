//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Rtt_LinuxSimulator.h"
#include "Rtt_LinuxUtils.h"
#include "Rtt_LinuxSimulatorView.h"
#include "Rtt_LinuxDialogBuild.h"
#include "Rtt_FileSystem.h"
#include "Rtt_LuaContext.h"
#include "Rtt_LinuxConsoleApp.h"
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/un.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <netinet/ip.h> 

#if defined(Rtt_SIMULATOR)
#include "resource/simulator.xpm"
#endif

using namespace std;

// global
Rtt::SolarSimulator* solarSimulator = NULL;

void LinuxLog(const char* buf, int len)
{
	if (app)
		app->Log(buf, len);
}

namespace Rtt
{
	//
	// SolarSimulator
	//

	SolarSimulator::SolarSimulator(const string& resourceDir)
		: SolarApp(resourceDir)
	{
		app = this;
		solarSimulator = this;		// save

		const char* homeDir = GetHomePath();
		string buildPath(homeDir);
		string projectCreationPath(homeDir);

		// create default directories if missing

		buildPath.append("/Documents/Solar2D Built Apps");
		projectCreationPath.append("/Documents/Solar2D Projects");
		if (!Rtt_IsDirectory(buildPath.c_str()))
		{
			Rtt_MakeDirectory(buildPath.c_str());
		}
		if (!Rtt_IsDirectory(projectCreationPath.c_str()))
		{
			Rtt_MakeDirectory(projectCreationPath.c_str());
		}
	}

	SolarSimulator::~SolarSimulator()
	{
	}

	bool SolarSimulator::Init()
	{
		if (InitSDL())
		{
			StartConsole();

			const string& lastProjectDirectory = fConfig["lastProjectDirectory"].to_string();
			return LoadApp(!lastProjectDirectory.empty() ? lastProjectDirectory : fResourceDir);
		}
		return false;
	}

	bool SolarSimulator::LoadApp(const string& ppath)
	{
#ifdef Rtt_SIMULATOR
		//		SetIcon(simulator_xpm);
#endif

		fConfig.Save();
		string path(ppath);	// keep alive

		fContext = new SolarAppContext(fWindow);
		if (fContext->LoadApp(path))
		{
			Window::SetStyle();
			CreateMenu();
			fSkins.Load(fContext->GetRuntime()->VMContext().L());

			bool showErrors = fConfig["showRuntimeErrors"].to_bool();
			GetRuntime()->SetProperty(Rtt::Runtime::kShowRuntimeErrorsSet, true);
			GetRuntime()->SetProperty(Rtt::Runtime::kShowRuntimeErrors, showErrors);

			string title;

			UpdateRecentDocs(GetAppName(), path + "/main.lua");
			fConfig["lastProjectDirectory"] = fContext->GetAppPath();

			WatchFolder(GetAppPath());
			LinuxSimulatorView::OnLinuxPluginGet(GetAppPath().c_str(), GetAppName().c_str(), fContext->GetPlatform());

			Rtt_Log("Loading project from: %s\n", fContext->GetAppPath().c_str());
			Rtt_Log("Project sandbox folder: %s\n", GetSandboxPath(GetAppName()).c_str());
			return true;
		}

		Rtt_LogException("Failed to load app %s\n", path.c_str());
		return false;
}

	void SolarSimulator::SolarEvent(const SDL_Event& e)
	{
		//Rtt_Log("SolarEvent %d\n", e.type);
		switch (e.type)
		{

		case sdl::OnPreferencesChanged:
		{
			bool showErrors = fConfig["showRuntimeErrors"].to_bool();
			GetRuntime()->SetProperty(Rtt::Runtime::kShowRuntimeErrorsSet, true);
			GetRuntime()->SetProperty(Rtt::Runtime::kShowRuntimeErrors, showErrors);
			break;
		}
		case sdl::OnRuntimeError:
		{
			char** data = (char**)e.user.data1;
			fDlg = new DlgRuntimeError("Runtime Error", 500, 200, data[0], data[1], data[2]);
			free(data[0]);
			free(data[1]);
			free(data[2]);
			delete[] data;
			break;
		}

		case sdl::OnOpenProject:
		{
			if (e.user.data1)
			{
				string path = (const char*)e.user.data1;
				free(e.user.data1);
				OnOpen(path);
			}
			else
			{
				// open file dialog
				fDlg = new DlgOpen("Open Project", 640, 480, fConfig["lastProjectDirectory"].to_string());
			}
			break;
		}

		case sdl::OnRelaunch:
			OnRelaunch();
			break;

		case sdl::OnOpenInEditor:
		{
			string path = fContext->GetAppPath();
			path.append("/main.lua");
			OpenURL(path);
			break;
		}

		case sdl::OnCloseProject:
		{
			fDlg = NULL;
			break;
		}

		case sdl::OnOpenDocumentation:
			OpenURL("https://docs.coronalabs.com/api/index.html");
			break;

		case sdl::OnOpenSampleProjects:
		{
			string samplesPath = GetStartupPath(NULL);
			samplesPath.append("/Resources/SampleCode");
			if (!Rtt_IsDirectory(samplesPath.c_str()))
			{
				Rtt_LogException("%s\n not found", samplesPath.c_str());
				break;
			}

			// open file dialog
			fDlg = new DlgOpen("Select Sample Project", 480, 320, samplesPath);
			break;
		}
		case sdl::OnAbout:
			fDlg = new DlgAbout("About Solar2D Simulator", 480, 240);
			break;

		case sdl::OnFileBrowserSelected:
		{
			string path = (const char*)e.user.data1;
			free(e.user.data1);
			OnOpen(path);

			fDlg = NULL;
			break;
		}

		case sdl::OnShowProjectFiles:
			OpenURL(fContext->GetAppPath());
			break;

		case sdl::OnShowProjectSandbox:
		{
			OpenURL(GetSandboxPath(GetAppName()));
			break;
		}

		case sdl::OnClearProjectSandbox:
		{
			string dir = GetSandboxPath(GetAppName());
			Rtt_DeleteDirectory(dir.c_str());

			OnRelaunch();
			break;
		}
		case sdl::OnRelaunchLastProject:
		{
			const string& lastProjectDirectory = fConfig["lastProjectDirectory"].to_string();
			if (lastProjectDirectory.size() > 0)
			{
				string path(lastProjectDirectory);
				path.append("/main.lua");
				OnOpen(path);
			}
			break;
		}

		case sdl::OnOpenPreferences:
			fDlg = new DlgPreferences("Solar2D Simulator Preferences", 400, 380);
			break;

		case sdl::onCloseDialog:
			fDlg = NULL;
			break;

		case sdl::OnFileSystemEvent:
		{
			string path = (const char*)e.user.data1;
			free(e.user.data1);
			break;
		}

		case sdl::OnBuildLinux:
			fDlg = new DlgLinuxBuild("Linux Build Setup", 640, 260);
			break;

		case sdl::OnBuildAndroid:
			fDlg = new DlgAndroidBuild("Android Build Setup", 650, 350);
			break;

		case sdl::OnStyleColorsLight:
			ImGui::StyleColorsLight();
			break;
		case sdl::OnStyleColorsClassic:
			ImGui::StyleColorsClassic();
			break;
		case sdl::OnStyleColorsDark:
			ImGui::StyleColorsDark();
			break;

		case sdl::OnSetFocusConsole:
			if (fConsole)
			{
				SDL_RaiseWindow(fConsole->GetWindow());
			}
			break;

		case sdl::OnViewAs:
			fDlg = new DlgViewAs("View As", 500, 450, &fSkins);
			break;

		case sdl::OnChangeView:
		{
			const SkinProperties* skin = (const SkinProperties*)e.user.data1;
			OnViewAsChanged(skin);
			break;
		}

		default:
			break;
		}
	}

	void SolarSimulator::WatchFolder(const string& path)
	{
		fWatcher = NULL;
		fWatcher = new FileWatcher();
		fWatcher->Start(path);
	}

	void SolarSimulator::OnRelaunch()
	{
		LoadApp(GetAppPath());
	}

	void SolarSimulator::OnViewAsChanged(const SkinProperties* skin)
	{
		SDL_DisplayMode screen;
		if (SDL_GetCurrentDisplayMode(0, &screen) == 0)
		{
			string title(GetAppName());
			title.append(" - ").append(skin->skinTitle);
			fContext->SetTitle(title);

			int w = skin->deviceWidth;
			int h = skin->deviceHeight;
			fContext->SetSize(w, h);
		}
	}

	void SolarSimulator::OnOpen(const string& fullPath)
	{
		// sanity check
		if (fullPath.size() < 10)
			return;

		string path = fullPath.substr(0, fullPath.size() - 9); // without main.lua
		LoadApp(path);
	}

	void SolarSimulator::StartConsole()
	{
		fConsole = new ConsoleWindow("Solar2D Simulator Console", 640, 480, &fLogData);
	}

	void SolarSimulator::CreateMenu()
	{
		fMenu = new DlgMenu(fContext->GetAppName());
	}


}
