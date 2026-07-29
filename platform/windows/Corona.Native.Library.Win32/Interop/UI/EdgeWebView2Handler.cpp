//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "EdgeWebView2Handler.h"
#include <WebView2.h>
#include <sstream>
#include <string>
#include <wrl/client.h>
#include <wrl/event.h>


namespace Interop { namespace UI {

class EdgeWebView2Handler::Implementation
:	public std::enable_shared_from_this<EdgeWebView2Handler::Implementation>
{
	public:
		Implementation(HWND parentWindowHandle, const wchar_t* userDataFolderPath, Delegate* delegatePointer)
		:	fParentWindowHandle(parentWindowHandle),
			fDelegatePointer(delegatePointer),
			fNavigationStartingToken{},
			fNavigationCompletedToken{},
			fIsNavigationStartingHandlerRegistered(false),
			fIsNavigationCompletedHandlerRegistered(false),
			fIsDetached(false),
			fIsReady(false),
			fHasPendingUrl(false),
			fHasReportedInitializationFailure(false)
		{
			if (userDataFolderPath)
			{
				fUserDataFolderPath = userDataFolderPath;
			}
		}

		~Implementation()
		{
			DetachFromControl();
		}

		static bool IsRuntimeAvailable()
		{
			LPWSTR versionStringPointer = nullptr;
			HRESULT result = ::GetAvailableCoreWebView2BrowserVersionString(nullptr, &versionStringPointer);
			bool isAvailable =
					SUCCEEDED(result) && versionStringPointer && (versionStringPointer[0] != L'\0');
			if (versionStringPointer)
			{
				::CoTaskMemFree(versionStringPointer);
			}
			return isAvailable;
		}

		bool BeginAsyncCreation()
		{
			auto selfPointer = shared_from_this();
			auto handlerPointer =
					Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
							[selfPointer](HRESULT result, ICoreWebView2Environment* environmentPointer) -> HRESULT
							{
								return selfPointer->OnEnvironmentCreated(result, environmentPointer);
							});
			if (!handlerPointer)
			{
				return false;
			}

			const wchar_t* userDataFolderPath =
					fUserDataFolderPath.empty() ? nullptr : fUserDataFolderPath.c_str();
			HRESULT result = ::CreateCoreWebView2EnvironmentWithOptions(
					nullptr, userDataFolderPath, nullptr, handlerPointer.Get());
			return SUCCEEDED(result);
		}

		void DetachFromControl()
		{
			if (fIsDetached)
			{
				return;
			}
			fIsDetached = true;
			fDelegatePointer = nullptr;

			if (fWebViewPointer)
			{
				if (fIsNavigationStartingHandlerRegistered)
				{
					fWebViewPointer->remove_NavigationStarting(fNavigationStartingToken);
				}
				if (fIsNavigationCompletedHandlerRegistered)
				{
					fWebViewPointer->remove_NavigationCompleted(fNavigationCompletedToken);
				}
				fWebViewPointer.Reset();
			}

			if (fControllerPointer)
			{
				fControllerPointer->Close();
				fControllerPointer.Reset();
			}
			fEnvironmentPointer.Reset();
			fParentWindowHandle = nullptr;
			fIsReady = false;
			fHasPendingUrl = false;
			fPendingUrl.clear();
			fPendingHeaders.clear();
		}

		bool CanNavigateBack() const
		{
			BOOL canNavigateBack = FALSE;
			return !fIsDetached && fWebViewPointer &&
					SUCCEEDED(fWebViewPointer->get_CanGoBack(&canNavigateBack)) && canNavigateBack;
		}

		bool CanNavigateForward() const
		{
			BOOL canNavigateForward = FALSE;
			return !fIsDetached && fWebViewPointer &&
					SUCCEEDED(fWebViewPointer->get_CanGoForward(&canNavigateForward)) && canNavigateForward;
		}

		void NavigateBack()
		{
			if (fIsDetached || !fWebViewPointer)
			{
				return;
			}

			BOOL canNavigateBack = FALSE;
			if (SUCCEEDED(fWebViewPointer->get_CanGoBack(&canNavigateBack)) && canNavigateBack)
			{
				fWebViewPointer->GoBack();
			}
		}

		void NavigateForward()
		{
			if (fIsDetached || !fWebViewPointer)
			{
				return;
			}

			BOOL canNavigateForward = FALSE;
			if (SUCCEEDED(fWebViewPointer->get_CanGoForward(&canNavigateForward)) && canNavigateForward)
			{
				fWebViewPointer->GoForward();
			}
		}

		void NavigateTo(const wchar_t* url, const wchar_t* headers)
		{
			if (fIsDetached || !url || (url[0] == L'\0'))
			{
				return;
			}

			if (!fIsReady || !fWebViewPointer)
			{
				fPendingUrl = url;
				fPendingHeaders = headers ? headers : L"";
				fHasPendingUrl = true;
				return;
			}

			HRESULT result = S_OK;
			if (headers && (headers[0] != L'\0'))
			{
				// Apply headers to the initial document request only. Intercepting all web resources would risk
				// forwarding an authorization header to third-party pages loaded by the sign-in flow.
				Microsoft::WRL::ComPtr<ICoreWebView2Environment2> environment2Pointer;
				Microsoft::WRL::ComPtr<ICoreWebView2_2> webView2Pointer;
				Microsoft::WRL::ComPtr<ICoreWebView2WebResourceRequest> requestPointer;

				result = fEnvironmentPointer.As(&environment2Pointer);
				if (SUCCEEDED(result))
				{
					result = fWebViewPointer.As(&webView2Pointer);
				}
				if (SUCCEEDED(result))
				{
					result = environment2Pointer->CreateWebResourceRequest(
							url, L"GET", nullptr, headers, requestPointer.GetAddressOf());
				}
				if (SUCCEEDED(result))
				{
					result = webView2Pointer->NavigateWithWebResourceRequest(requestPointer.Get());
				}
			}
			else
			{
				result = fWebViewPointer->Navigate(url);
			}

			if (FAILED(result))
			{
				std::wstringstream messageStream;
				messageStream << L"WebView2 failed to start navigation (HRESULT 0x";
				messageStream << std::hex << static_cast<unsigned long>(result) << L").";
				RaiseNavigationFailed(url, static_cast<int>(result), messageStream.str().c_str());
			}
		}

		void Reload()
		{
			if (!fIsDetached && fWebViewPointer)
			{
				fWebViewPointer->Reload();
			}
		}

		void StopLoading()
		{
			if (!fIsDetached && fWebViewPointer)
			{
				fWebViewPointer->Stop();
			}
		}

		void UpdateBounds()
		{
			if (fIsDetached || !fControllerPointer || !fParentWindowHandle)
			{
				return;
			}

			RECT bounds{};
			if (::GetClientRect(fParentWindowHandle, &bounds))
			{
				fControllerPointer->put_Bounds(bounds);
			}
		}

	private:
		HRESULT OnEnvironmentCreated(HRESULT result, ICoreWebView2Environment* environmentPointer)
		{
			if (fIsDetached)
			{
				return S_OK;
			}
			if (FAILED(result) || !environmentPointer)
			{
				FailInitialization(FAILED(result) ? result : E_FAIL);
				return S_OK;
			}

			fEnvironmentPointer = environmentPointer;
			if (!fParentWindowHandle || !::IsWindow(fParentWindowHandle))
			{
				FailInitialization(E_HANDLE);
				return S_OK;
			}

			auto selfPointer = shared_from_this();
			auto handlerPointer =
					Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
							[selfPointer](HRESULT result, ICoreWebView2Controller* controllerPointer) -> HRESULT
							{
								return selfPointer->OnControllerCreated(result, controllerPointer);
							});
			if (!handlerPointer)
			{
				FailInitialization(E_OUTOFMEMORY);
				return S_OK;
			}

			result = fEnvironmentPointer->CreateCoreWebView2Controller(
					fParentWindowHandle, handlerPointer.Get());
			if (FAILED(result))
			{
				FailInitialization(result);
			}
			return S_OK;
		}

		HRESULT OnControllerCreated(HRESULT result, ICoreWebView2Controller* controllerPointer)
		{
			if (fIsDetached)
			{
				return S_OK;
			}
			if (FAILED(result) || !controllerPointer)
			{
				FailInitialization(FAILED(result) ? result : E_FAIL);
				return S_OK;
			}

			fControllerPointer = controllerPointer;
			result = fControllerPointer->get_CoreWebView2(fWebViewPointer.GetAddressOf());
			if (FAILED(result) || !fWebViewPointer)
			{
				FailInitialization(FAILED(result) ? result : E_FAIL);
				return S_OK;
			}

			Microsoft::WRL::ComPtr<ICoreWebView2Settings> settingsPointer;
			if (SUCCEEDED(fWebViewPointer->get_Settings(settingsPointer.GetAddressOf())) && settingsPointer)
			{
				settingsPointer->put_IsScriptEnabled(TRUE);
				settingsPointer->put_AreDefaultScriptDialogsEnabled(TRUE);
				settingsPointer->put_AreDefaultContextMenusEnabled(TRUE);
				settingsPointer->put_AreDevToolsEnabled(FALSE);
				settingsPointer->put_IsStatusBarEnabled(FALSE);
				settingsPointer->put_IsZoomControlEnabled(TRUE);
			}

			result = RegisterEventHandlers();
			if (FAILED(result))
			{
				FailInitialization(result);
				return S_OK;
			}

			UpdateBounds();
			result = fControllerPointer->put_IsVisible(TRUE);
			if (FAILED(result))
			{
				FailInitialization(result);
				return S_OK;
			}

			fIsReady = true;
			DispatchPendingNavigation();
			return S_OK;
		}

		HRESULT RegisterEventHandlers()
		{
			auto selfPointer = shared_from_this();
			auto navigationStartingHandlerPointer =
					Microsoft::WRL::Callback<ICoreWebView2NavigationStartingEventHandler>(
							[selfPointer](
									ICoreWebView2* senderPointer,
									ICoreWebView2NavigationStartingEventArgs* argumentsPointer) -> HRESULT
							{
								if (selfPointer->fIsDetached || !senderPointer || !argumentsPointer)
								{
									return S_OK;
								}

								LPWSTR urlPointer = nullptr;
								HRESULT result = argumentsPointer->get_Uri(&urlPointer);
								if (FAILED(result) || !urlPointer)
								{
									return S_OK;
								}
								std::wstring url(urlPointer);
								::CoTaskMemFree(urlPointer);

								bool wasCanceled = false;
								auto delegatePointer = selfPointer->fDelegatePointer;
								if (delegatePointer)
								{
									wasCanceled = delegatePointer->OnEdgeWebView2Navigating(url.c_str());
								}
								if (!selfPointer->fIsDetached && wasCanceled)
								{
									argumentsPointer->put_Cancel(TRUE);
								}
								return S_OK;
							});
			if (!navigationStartingHandlerPointer)
			{
				return E_OUTOFMEMORY;
			}

			HRESULT result = fWebViewPointer->add_NavigationStarting(
					navigationStartingHandlerPointer.Get(), &fNavigationStartingToken);
			if (FAILED(result))
			{
				return result;
			}
			fIsNavigationStartingHandlerRegistered = true;

			auto navigationCompletedHandlerPointer =
					Microsoft::WRL::Callback<ICoreWebView2NavigationCompletedEventHandler>(
							[selfPointer](
									ICoreWebView2* senderPointer,
									ICoreWebView2NavigationCompletedEventArgs* argumentsPointer) -> HRESULT
							{
								if (selfPointer->fIsDetached || !senderPointer || !argumentsPointer)
								{
									return S_OK;
								}

								LPWSTR urlPointer = nullptr;
								senderPointer->get_Source(&urlPointer);
								std::wstring url = urlPointer ? urlPointer : L"";
								if (urlPointer)
								{
									::CoTaskMemFree(urlPointer);
								}

								BOOL wasSuccessful = FALSE;
								argumentsPointer->get_IsSuccess(&wasSuccessful);
								auto delegatePointer = selfPointer->fDelegatePointer;
								if (wasSuccessful)
								{
									if (delegatePointer)
									{
										delegatePointer->OnEdgeWebView2Navigated(url.c_str());
									}
								}
								else
								{
									COREWEBVIEW2_WEB_ERROR_STATUS webErrorStatus =
											COREWEBVIEW2_WEB_ERROR_STATUS_UNKNOWN;
									argumentsPointer->get_WebErrorStatus(&webErrorStatus);
									std::wstringstream messageStream;
									messageStream << L"WebView2 navigation failed (web error status ";
									messageStream << static_cast<int>(webErrorStatus) << L").";
									if (delegatePointer)
									{
										delegatePointer->OnEdgeWebView2NavigationFailed(
												url.c_str(), static_cast<int>(webErrorStatus),
												messageStream.str().c_str());
									}
								}
								return S_OK;
							});
			if (!navigationCompletedHandlerPointer)
			{
				return E_OUTOFMEMORY;
			}

			result = fWebViewPointer->add_NavigationCompleted(
					navigationCompletedHandlerPointer.Get(), &fNavigationCompletedToken);
			if (FAILED(result))
			{
				return result;
			}
			fIsNavigationCompletedHandlerRegistered = true;
			return S_OK;
		}

		void DispatchPendingNavigation()
		{
			if (!fHasPendingUrl || !fIsReady || !fWebViewPointer)
			{
				return;
			}

			std::wstring url(fPendingUrl);
			std::wstring headers(fPendingHeaders);
			fHasPendingUrl = false;
			fPendingUrl.clear();
			fPendingHeaders.clear();
			NavigateTo(url.c_str(), headers.empty() ? nullptr : headers.c_str());
		}

		void FailInitialization(HRESULT result)
		{
			if (fIsDetached || fHasReportedInitializationFailure)
			{
				return;
			}
			fHasReportedInitializationFailure = true;

			auto delegatePointer = fDelegatePointer;
			if (delegatePointer)
			{
				delegatePointer->OnEdgeWebView2InitializationFailed(
						result,
						fHasPendingUrl ? fPendingUrl.c_str() : nullptr,
						fHasPendingUrl && !fPendingHeaders.empty() ? fPendingHeaders.c_str() : nullptr);
			}
			else
			{
				DetachFromControl();
			}
		}

		void RaiseNavigationFailed(const wchar_t* url, int errorCode, const wchar_t* errorMessage)
		{
			auto delegatePointer = fDelegatePointer;
			if (!fIsDetached && delegatePointer)
			{
				delegatePointer->OnEdgeWebView2NavigationFailed(
						url ? url : L"", errorCode, errorMessage ? errorMessage : L"");
			}
		}

		HWND fParentWindowHandle;
		Delegate* fDelegatePointer;
		std::wstring fUserDataFolderPath;
		Microsoft::WRL::ComPtr<ICoreWebView2Environment> fEnvironmentPointer;
		Microsoft::WRL::ComPtr<ICoreWebView2Controller> fControllerPointer;
		Microsoft::WRL::ComPtr<ICoreWebView2> fWebViewPointer;
		EventRegistrationToken fNavigationStartingToken;
		EventRegistrationToken fNavigationCompletedToken;
		bool fIsNavigationStartingHandlerRegistered;
		bool fIsNavigationCompletedHandlerRegistered;
		bool fIsDetached;
		bool fIsReady;
		bool fHasPendingUrl;
		bool fHasReportedInitializationFailure;
		std::wstring fPendingUrl;
		std::wstring fPendingHeaders;
};

EdgeWebView2Handler* EdgeWebView2Handler::CreateAndAttachTo(
		HWND parentWindowHandle, const wchar_t* userDataFolderPath, Delegate* delegatePointer)
{
	if (!parentWindowHandle || !delegatePointer || !Implementation::IsRuntimeAvailable())
	{
		return nullptr;
	}

	auto implementationPointer = std::make_shared<Implementation>(
			parentWindowHandle, userDataFolderPath, delegatePointer);
	if (!implementationPointer->BeginAsyncCreation())
	{
		implementationPointer->DetachFromControl();
		return nullptr;
	}
	return new EdgeWebView2Handler(implementationPointer);
}

EdgeWebView2Handler::EdgeWebView2Handler(
		const std::shared_ptr<EdgeWebView2Handler::Implementation>& implementationPointer)
:	fImplementationPointer(implementationPointer)
{
}

EdgeWebView2Handler::~EdgeWebView2Handler()
{
	DetachFromControl();
}

void EdgeWebView2Handler::DetachFromControl()
{
	if (fImplementationPointer)
	{
		fImplementationPointer->DetachFromControl();
		fImplementationPointer.reset();
	}
}

bool EdgeWebView2Handler::CanNavigateBack() const
{
	return fImplementationPointer ? fImplementationPointer->CanNavigateBack() : false;
}

bool EdgeWebView2Handler::CanNavigateForward() const
{
	return fImplementationPointer ? fImplementationPointer->CanNavigateForward() : false;
}

void EdgeWebView2Handler::NavigateBack()
{
	if (fImplementationPointer)
	{
		fImplementationPointer->NavigateBack();
	}
}

void EdgeWebView2Handler::NavigateForward()
{
	if (fImplementationPointer)
	{
		fImplementationPointer->NavigateForward();
	}
}

void EdgeWebView2Handler::NavigateTo(const wchar_t* url, const wchar_t* headers)
{
	auto implementationPointer = fImplementationPointer;
	if (implementationPointer)
	{
		implementationPointer->NavigateTo(url, headers);
	}
}

void EdgeWebView2Handler::Reload()
{
	if (fImplementationPointer)
	{
		fImplementationPointer->Reload();
	}
}

void EdgeWebView2Handler::StopLoading()
{
	if (fImplementationPointer)
	{
		fImplementationPointer->StopLoading();
	}
}

void EdgeWebView2Handler::UpdateBounds()
{
	if (fImplementationPointer)
	{
		fImplementationPointer->UpdateBounds();
	}
}

} }	// namespace Interop::UI
