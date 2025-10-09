//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Core\Rtt_Macros.h"
#include "Control.h"
#include "HandleMessageEventArgs.h"
#include "WebBrowserNavigatedEventArgs.h"
#include "WebBrowserNavigatingEventArgs.h"
#include "WebBrowserNavigationFailedEventArgs.h"
#include "Interop\Event.h"
#include <WebView2.h>
#include <functional>
#include <memory>
#include <string>
#include <wrl/client.h>

namespace Interop { namespace UI {

/// <summary>Microsoft Edge WebView2 backed web browser control.</summary>
class WebBrowser : public Control, public std::enable_shared_from_this<WebBrowser>
{
	Rtt_CLASS_NO_COPIES(WebBrowser)

	public:
		#pragma region Public Event Types
		typedef Event<WebBrowser&, WebBrowserNavigatingEventArgs&> NavigatingEvent;
		typedef Event<WebBrowser&, const WebBrowserNavigatedEventArgs&> NavigatedEvent;
		typedef Event<WebBrowser&, const WebBrowserNavigationFailedEventArgs&> NavigationFailedEvent;
		#pragma endregion

		#pragma region CreationSettings Structure
		struct CreationSettings
		{
			HWND ParentWindowHandle;
			RECT Bounds;
			const wchar_t* IEOverrideRegistryPath;
		};
		#pragma endregion

	#pragma region Constructors/Destructors
		static std::shared_ptr<WebBrowser> Create(const CreationSettings& settings);
		virtual ~WebBrowser();
		#pragma endregion

		#pragma region Public Methods
		NavigatingEvent::HandlerManager& GetNavigatingEventHandlers();
		NavigatedEvent::HandlerManager& GetNavigatedEventHandlers();
		NavigationFailedEvent::HandlerManager& GetNavigationFailedEventHandlers();

		const wchar_t* GetIEOverrideRegistryPath() const;

		bool CanNavigateBack();
		bool CanNavigateForward();

		void NavigateBack();
		void NavigateForward();
		void NavigateTo(const wchar_t* url);
		void NavigateToWithHeader(const wchar_t* url, const wchar_t* header);
		void Reload();
		void StopLoading();
		#pragma endregion

	private:
		WebBrowser(const CreationSettings& settings);

		#pragma region Private Types
		struct PendingRequest
		{
			std::wstring Url;
			std::wstring Headers;
		};
		#pragma endregion

		#pragma region Private Methods
		void InitializeWebViewAsync();
		void OnControllerCreated(Microsoft::WRL::ComPtr<ICoreWebView2Controller> controller);
		void UpdateControllerBounds();
		void ApplyPendingRequest();
		void RaiseNavigationFailed(const std::wstring& url, HRESULT result, const std::wstring& message);
		void OnReceivedMessage(UIComponent& sender, HandleMessageEventArgs& arguments);

		void HandleNavigationStarting(ICoreWebView2NavigationStartingEventArgs* args);
		void HandleNavigationCompleted(ICoreWebView2NavigationCompletedEventArgs* args);
		std::wstring CopyUriFromArgs(std::function<HRESULT(wchar_t**)> getter) const;
		std::wstring DescribeWebErrorStatus(COREWEBVIEW2_WEB_ERROR_STATUS status) const;
		#pragma endregion

		#pragma region Private Member Variables
		NavigatingEvent fNavigatingEvent;
		NavigatedEvent fNavigatedEvent;
		NavigationFailedEvent fNavigationFailedEvent;

		Microsoft::WRL::ComPtr<ICoreWebView2Environment> fEnvironment;
		Microsoft::WRL::ComPtr<ICoreWebView2Controller> fController;
		Microsoft::WRL::ComPtr<ICoreWebView2> fWebView;

		EventRegistrationToken fNavigationStartingToken;
		EventRegistrationToken fNavigationCompletedToken;

		PendingRequest fPendingRequest;
		bool fIsWebViewReady;
		std::wstring fIEOverrideRegistryPath;

		UIComponent::ReceivedMessageEvent::MethodHandler<WebBrowser> fReceivedMessageEventHandler;
		#pragma endregion
};

} }	// namespace Interop::UI
