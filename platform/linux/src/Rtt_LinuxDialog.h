//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Rtt_LinuxContainer.h"

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imfilebrowser.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <map>

#define BUTTON_WIDTH 100

namespace Rtt
{
	//
	// base class
	//

	struct Window : public ref_counted
	{
		Window(const std::string& title, int w, int h, Uint32 flags = 0);
		virtual ~Window();

		virtual void Draw() = 0;
		void ProcessEvent(const SDL_Event& evt);
		SDL_Window* GetWindow() const { return fWindow; }
		void GetWindowSize(int* w, int* h);
		static void MoveToCenter();
		static void FocusHere();
		static void SetStyle();

	protected:

		void begin();
		void end();

	private:

		SDL_Window* fWindow;
		SDL_GLContext fGLcontext;
		ImGuiContext* fImCtx;

		// for state saving 
		SDL_Window* window;
		SDL_GLContext glcontext;
		ImGuiContext* imctx;
	};

	struct DlgAbout : public Window
	{
		DlgAbout(const std::string& title, int w, int h);
		virtual ~DlgAbout();
		void Draw() override;

		GLuint tex_id;
		int width;
		int height;
	};

	struct DlgOpen : public Window
	{
		DlgOpen(const std::string& title, int w, int h, const std::string& startFolder);
		virtual ~DlgOpen();

		void Draw() override;

		ImGui::FileBrowser fileDialog;
	};

	struct DlgMenu : public ref_counted
	{
		DlgMenu(const std::string& appName);

		void Draw();
		int GetHeight() const { return fMenuSize.y;	}

	private:

		bool fIsMainMenu;
		ImVec2 fMenuSize;
	};

	struct DlgPreferences : public Window
	{
		DlgPreferences(const std::string& title, int w, int h);
		virtual ~DlgPreferences();

		void Draw() override;

	private:

		bool fOpenlastProject;
		int fStyleIndex;
		bool fDebugBuildProcess;
	};

	struct Skins;
	struct DlgViewAs : public Window
	{
		DlgViewAs(const std::string& title, int w, int h, Skins* skins);
		virtual ~DlgViewAs();

		void Draw() override;

	private:

		void DrawView(const std::string& name);
		void Clear();

		int fViewIndex;
		Skins* fSkins;
		char** fItems;
		int fItemsLen;
		int fItemCurrent;
		std::string fTabCurrent;
	};

	class LuaResource;
	struct DlgAlert : public Window
	{
		DlgAlert(const char* title, const char* msg, const char** buttonLabels, int numButtons, LuaResource* resource);
		virtual ~DlgAlert();

		void Draw() override;

	private:

		void onClick(int nButton);

		std::string fMsg;
		std::vector<std::string> fButtons;
		LuaResource* fCallback;
	};

	struct DlgRuntimeError : public Window
	{
		DlgRuntimeError(const char* title, int w, int h, const char* errorType, const char* message, const char* stacktrace);
		virtual ~DlgRuntimeError();

		void Draw() override;

	private:

		std::string fErrorType;
		std::string fMessage;
		std::string fStackTrace;
	};

}
