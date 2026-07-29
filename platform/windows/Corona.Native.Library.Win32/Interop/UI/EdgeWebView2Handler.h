//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

#include <memory>
#include <Windows.h>


namespace Interop { namespace UI {

/// <summary>Wraps a Microsoft Edge WebView2 controller without exposing WebView2 SDK types.</summary>
class EdgeWebView2Handler
{
	public:
		/// <summary>Receives WebView2 lifecycle and navigation events.</summary>
		class Delegate
		{
			public:
				virtual bool OnEdgeWebView2Navigating(const wchar_t* url) = 0;
				virtual void OnEdgeWebView2Navigated(const wchar_t* url) = 0;
				virtual void OnEdgeWebView2NavigationFailed(
						const wchar_t* url, int errorCode, const wchar_t* errorMessage) = 0;
				virtual void OnEdgeWebView2InitializationFailed(
						HRESULT result, const wchar_t* pendingUrl, const wchar_t* pendingHeaders) = 0;

			protected:
				virtual ~Delegate() {}
		};

		/// <summary>
		///  Creates and asynchronously attaches WebView2 to the given parent window.
		///  Returns null when the WebView2 Runtime is unavailable or creation cannot be started.
		/// </summary>
		static EdgeWebView2Handler* CreateAndAttachTo(
				HWND parentWindowHandle, const wchar_t* userDataFolderPath, Delegate* delegatePointer);

		~EdgeWebView2Handler();

		void DetachFromControl();
		bool CanNavigateBack() const;
		bool CanNavigateForward() const;
		void NavigateBack();
		void NavigateForward();
		void NavigateTo(const wchar_t* url, const wchar_t* headers = nullptr);
		void Reload();
		void StopLoading();
		void UpdateBounds();

	private:
		class Implementation;

		explicit EdgeWebView2Handler(const std::shared_ptr<Implementation>& implementationPointer);
		EdgeWebView2Handler(const EdgeWebView2Handler&) = delete;
		EdgeWebView2Handler& operator=(const EdgeWebView2Handler&) = delete;

		std::shared_ptr<Implementation> fImplementationPointer;
};

} }	// namespace Interop::UI
