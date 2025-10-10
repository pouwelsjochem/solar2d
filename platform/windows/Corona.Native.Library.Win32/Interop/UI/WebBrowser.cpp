//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "WebBrowser.h"
#include "Core\Rtt_Assert.h"
#include <algorithm>
#include <iomanip>
#include <cwctype>
#include <sstream>
#include <wrl.h>

namespace Interop { namespace UI {

namespace
{
	const wchar_t kWebViewHostWindowClass[] = L"CoronaWebViewHost";

	std::wstring TrimWhitespace(const std::wstring& value)
	{
		size_t start = 0;
		while (start < value.size() && iswspace(value[start]))
		{
			++start;
		}
		if (start == value.size())
		{
			return std::wstring();
		}
		size_t end = value.size();
		while (end > start && iswspace(value[end - 1]))
		{
			--end;
		}
		return value.substr(start, end - start);
	}

	std::wstring CleanPath(const std::wstring& value)
	{
		std::wstring trimmed = TrimWhitespace(value);
		if (trimmed.size() >= 2 && trimmed.front() == L'"' && trimmed.back() == L'"')
		{
			trimmed = trimmed.substr(1, trimmed.size() - 2);
			trimmed = TrimWhitespace(trimmed);
		}
		return trimmed;
	}

	std::wstring FetchEnvironmentPath(const wchar_t* name)
	{
		DWORD required = ::GetEnvironmentVariableW(name, nullptr, 0);
		if (required == 0)
		{
			return std::wstring();
		}
		std::wstring buffer(required, L'\0');
		DWORD written = ::GetEnvironmentVariableW(name, buffer.data(), required);
		if (written == 0)
		{
			return std::wstring();
		}
		buffer.resize(written);
		return CleanPath(buffer);
	}

	std::wstring AppendPath(const std::wstring& base, const wchar_t* component)
	{
		if (!component || (component[0] == L'\0'))
		{
			return base;
		}
		std::wstring result = base;
		if (!result.empty() && result.back() != L'\' && result.back() != L'/')
		{
			result.append(1, L'\');
		}
		result.append(component);
		return result;
	}

	bool DirectoryExists(const std::wstring& path)
	{
		if (path.empty())
		{
			return false;
		}
		DWORD attributes = ::GetFileAttributesW(path.c_str());
		return (attributes != INVALID_FILE_ATTRIBUTES) && ((attributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
	}

	bool FileExists(const std::wstring& path)
	{
		if (path.empty())
		{
			return false;
		}
		DWORD attributes = ::GetFileAttributesW(path.c_str());
		return (attributes != INVALID_FILE_ATTRIBUTES) && ((attributes & FILE_ATTRIBUTE_DIRECTORY) == 0);
	}

	bool IsValidRuntimeDirectory(const std::wstring& path)
	{
		if (!DirectoryExists(path))
		{
			return false;
		}
		if (FileExists(AppendPath(path, L"msedgewebview2.exe")))
		{
			return true;
		}
		if (FileExists(AppendPath(path, L"WebView2Loader.dll")))
		{
			return true;
		}
		return false;
	}

	std::wstring FindRuntimeInDirectory(const std::wstring& root)
	{
		if (!DirectoryExists(root))
		{
			return std::wstring();
		}
		std::wstring pattern = AppendPath(root, L"*");
		WIN32_FIND_DATAW findData{};
		HANDLE handle = ::FindFirstFileW(pattern.c_str(), &findData);
		if (handle == INVALID_HANDLE_VALUE)
		{
			return std::wstring();
		}
		std::wstring result;
		do
		{
			if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
			{
				continue;
			}
			if (findData.cFileName[0] == L'.')
			{
				if (findData.cFileName[1] == L'\0' || (findData.cFileName[1] == L'.' && findData.cFileName[2] == L'\0'))
				{
					continue;
				}
			}
			std::wstring candidate = AppendPath(root, findData.cFileName);
			if (IsValidRuntimeDirectory(candidate))
			{
				result = candidate;
				break;
			}
		} while (::FindNextFileW(handle, &findData));
		::FindClose(handle);
		return result;
	}

	std::wstring ResolveCandidatePath(const std::wstring& candidate)
	{
		if (candidate.empty())
		{
			return std::wstring();
		}
		if (IsValidRuntimeDirectory(candidate))
		{
			return candidate;
		}
		return FindRuntimeInDirectory(candidate);
	}

	std::wstring GetModuleDirectory()
	{
		DWORD size = MAX_PATH;
		for (;;)
		{
			std::wstring buffer(size, L'\0');
			DWORD length = ::GetModuleFileNameW(nullptr, buffer.data(), size);
			if (length == 0)
			{
				return std::wstring();
			}
			if (length < size - 1)
			{
				buffer.resize(length);
				size_t separator = buffer.find_last_of(L"\/");
				if (separator != std::wstring::npos)
				{
					buffer.resize(separator);
				}
				return buffer;
			}
			size *= 2;
		}
	}

	std::wstring ResolveRuntimeFolder()
	{
		if (auto resolved = ResolveCandidatePath(FetchEnvironmentPath(L"CORONA_WEBVIEW2_RUNTIME_DIR")); !resolved.empty())
		{
			return resolved;
		}
		if (auto resolved = ResolveCandidatePath(FetchEnvironmentPath(L"WEBVIEW2_BROWSER_EXECUTABLE_FOLDER")); !resolved.empty())
		{
			return resolved;
		}
		auto moduleDir = GetModuleDirectory();
		if (!moduleDir.empty())
		{
			if (auto resolved = ResolveCandidatePath(moduleDir); !resolved.empty())
			{
				return resolved;
			}
			if (auto resolved = ResolveCandidatePath(AppendPath(moduleDir, L"WebView2Runtime")); !resolved.empty())
			{
				return resolved;
			}
		}
		return std::wstring();
	}

	ATOM EnsureHostWindowClassRegistered()
	{
		static ATOM sClassAtom = 0;
		if (sClassAtom)
		{
			return sClassAtom;
		}

		WNDCLASSEXW windowClass{};
		windowClass.cbSize = sizeof(windowClass);
		windowClass.style = CS_HREDRAW | CS_VREDRAW;
		windowClass.lpfnWndProc = ::DefWindowProcW;
		windowClass.hInstance = ::GetModuleHandleW(nullptr);
		windowClass.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
		windowClass.lpszClassName = kWebViewHostWindowClass;

		sClassAtom = ::RegisterClassExW(&windowClass);
		if (!sClassAtom)
		{
			if (::GetLastError() == ERROR_CLASS_ALREADY_EXISTS)
			{
				WNDCLASSEXW existingClass{};
				existingClass.cbSize = sizeof(existingClass);
				if (::GetClassInfoExW(::GetModuleHandleW(nullptr), kWebViewHostWindowClass, &existingClass))
				{
					sClassAtom = existingClass.atom;
				}
			}
		}
		return sClassAtom;
	}
}


bool WebBrowser::IsRuntimeAvailable()
{
	PWSTR versionInfo = nullptr;
	if (SUCCEEDED(GetAvailableCoreWebView2BrowserVersionString(nullptr, &versionInfo)))
	{
		if (versionInfo)
		{
			::CoTaskMemFree(versionInfo);
		}
		return true;
	}
	if (versionInfo)
	{
		::CoTaskMemFree(versionInfo);
		versionInfo = nullptr;
	}

	std::wstring fallback = ResolveRuntimeFolder();
	if (!fallback.empty())
	{
		if (SUCCEEDED(GetAvailableCoreWebView2BrowserVersionString(fallback.c_str(), &versionInfo)))
		{
			if (versionInfo)
			{
				::CoTaskMemFree(versionInfo);
			}
			return true;
		}
	}
	if (versionInfo)
	{
		::CoTaskMemFree(versionInfo);
	}
	return false;
}

std::shared_ptr<WebBrowser> WebBrowser::Create(const WebBrowser::CreationSettings& settings)
{
	auto webBrowser = std::shared_ptr<WebBrowser>(new WebBrowser(settings));
	if (!webBrowser->GetWindowHandle())
	{
		return nullptr;
	}
	webBrowser->InitializeWebViewAsync();
	return webBrowser;
}

#pragma region Constructors/Destructors
WebBrowser::WebBrowser(const WebBrowser::CreationSettings& settings)
:	Control(),
	fNavigationStartingToken{},
	fNavigationCompletedToken{},
	fIsWebViewReady(false),
	fBrowserExecutableFolder(ResolveRuntimeFolder()),
	fReceivedMessageEventHandler(this, &WebBrowser::OnReceivedMessage)
{
	if (settings.IEOverrideRegistryPath && settings.IEOverrideRegistryPath[0] != L'\0')
	{
		fIEOverrideRegistryPath = settings.IEOverrideRegistryPath;
	}

	EnsureHostWindowClassRegistered();

	DWORD styles = WS_TABSTOP | WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
	int width = std::max<LONG>(1, settings.Bounds.right - settings.Bounds.left);
	int height = std::max<LONG>(1, settings.Bounds.bottom - settings.Bounds.top);
	HWND windowHandle = ::CreateWindowExW(
			WS_EX_CONTROLPARENT | WS_EX_NOPARENTNOTIFY,
			kWebViewHostWindowClass,
			L"",
			styles,
			settings.Bounds.left,
			settings.Bounds.top,
			width,
			height,
			settings.ParentWindowHandle,
			nullptr,
			::GetModuleHandleW(nullptr),
			nullptr);

	OnSetWindowHandle(windowHandle);
	if (windowHandle)
	{
		::BringWindowToTop(windowHandle);
	}

	GetReceivedMessageEventHandlers().Add(&fReceivedMessageEventHandler);
}

WebBrowser::~WebBrowser()
{
	GetReceivedMessageEventHandlers().Remove(&fReceivedMessageEventHandler);

	if (fWebView && fNavigationStartingToken.value)
	{
		fWebView->remove_NavigationStarting(fNavigationStartingToken);
	}
	if (fWebView && fNavigationCompletedToken.value)
	{
		fWebView->remove_NavigationCompleted(fNavigationCompletedToken);
	}

	if (fController)
	{
		fController->Close();
	}

	fWebView.Reset();
	fController.Reset();
	fEnvironment.Reset();

	auto windowHandle = GetWindowHandle();
	if (windowHandle)
	{
		OnSetWindowHandle(nullptr);
		::DestroyWindow(windowHandle);
	}
}

#pragma endregion

#pragma region Public Methods
WebBrowser::NavigatingEvent::HandlerManager& WebBrowser::GetNavigatingEventHandlers()
{
	return fNavigatingEvent.GetHandlerManager();
}

WebBrowser::NavigatedEvent::HandlerManager& WebBrowser::GetNavigatedEventHandlers()
{
	return fNavigatedEvent.GetHandlerManager();
}

WebBrowser::NavigationFailedEvent::HandlerManager& WebBrowser::GetNavigationFailedEventHandlers()
{
	return fNavigationFailedEvent.GetHandlerManager();
}

const wchar_t* WebBrowser::GetIEOverrideRegistryPath() const
{
	return fIEOverrideRegistryPath.empty() ? nullptr : fIEOverrideRegistryPath.c_str();
}

bool WebBrowser::CanNavigateBack()
{
	BOOL canNavigate = FALSE;
	if (fWebView && SUCCEEDED(fWebView->get_CanGoBack(&canNavigate)))
	{
		return canNavigate ? true : false;
	}
	return false;
}

bool WebBrowser::CanNavigateForward()
{
	BOOL canNavigate = FALSE;
	if (fWebView && SUCCEEDED(fWebView->get_CanGoForward(&canNavigate)))
	{
		return canNavigate ? true : false;
	}
	return false;
}

void WebBrowser::NavigateBack()
{
	if (fWebView)
	{
		fWebView->GoBack();
	}
}

void WebBrowser::NavigateForward()
{
	if (fWebView)
	{
		fWebView->GoForward();
	}
}

void WebBrowser::NavigateTo(const wchar_t* url)
{
	if (!url || (url[0] == L'\0'))
	{
		return;
	}

	if (fIsWebViewReady && fWebView)
	{
		fWebView->Navigate(url);
	}
	else
	{
		fPendingRequest.Url.assign(url);
		fPendingRequest.Headers.clear();
	}
}

void WebBrowser::NavigateToWithHeader(const wchar_t* url, const wchar_t* header)
{
	if (!url || (url[0] == L'\0'))
	{
		return;
	}

	if (!header || (header[0] == L'\0'))
	{
		NavigateTo(url);
		return;
	}

	if (fIsWebViewReady && fWebView && fEnvironment)
	{
		Microsoft::WRL::ComPtr<ICoreWebView2WebResourceRequest> request;
		HRESULT result = fEnvironment->CreateWebResourceRequest(url, L"GET", nullptr, header, &request);
		if (SUCCEEDED(result) && request)
		{
			fWebView->NavigateWithWebResourceRequest(request.Get());
			return;
		}

		// Fallback to a simple navigation if the request could not be constructed.
		fWebView->Navigate(url);
	}
	else
	{
		fPendingRequest.Url.assign(url);
		fPendingRequest.Headers.assign(header);
	}
}

void WebBrowser::Reload()
{
	if (fWebView)
	{
		fWebView->Reload();
	}
}

void WebBrowser::StopLoading()
{
	if (fWebView)
	{
		fWebView->Stop();
	}
}

#pragma endregion

#pragma region Private Methods
void WebBrowser::InitializeWebViewAsync()
{
	auto windowHandle = GetWindowHandle();
	if (!windowHandle)
	{
		RaiseNavigationFailed(L"", E_FAIL, L"Failed to create host window for WebView2.");
		return;
	}

	auto weakSelf = std::weak_ptr<WebBrowser>(shared_from_this());

	const wchar_t* browserExecutableFolder = fBrowserExecutableFolder.empty() ? nullptr : fBrowserExecutableFolder.c_str();
	HRESULT result = CreateCoreWebView2EnvironmentWithOptions(
			browserExecutableFolder,
			nullptr,
			nullptr,
			Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
				[weakSelf, windowHandle](HRESULT hr, ICoreWebView2Environment* environment) -> HRESULT
				{
					auto self = weakSelf.lock();
					if (!self)
					{
						return S_OK;
					}

					if (FAILED(hr) || !environment)
					{
						self->RaiseNavigationFailed(L"", hr, L"Unable to create WebView2 environment.");
						return S_OK;
					}

					self->fEnvironment = environment;
					return environment->CreateCoreWebView2Controller(
							windowHandle,
							Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
								[weakSelf](HRESULT createHr, ICoreWebView2Controller* controller) -> HRESULT
								{
									auto innerSelf = weakSelf.lock();
									if (!innerSelf)
									{
										return S_OK;
									}

									if (FAILED(createHr) || !controller)
									{
										innerSelf->RaiseNavigationFailed(L"", createHr, L"Unable to create WebView2 controller.");
										return S_OK;
									}

									innerSelf->OnControllerCreated(controller);
									return S_OK;
								}).Get());
			}).Get());

	if (FAILED(result))
	{
		RaiseNavigationFailed(L"", result, L"CreateCoreWebView2EnvironmentWithOptions() failed.");
	}
}

void WebBrowser::OnControllerCreated(Microsoft::WRL::ComPtr<ICoreWebView2Controller> controller)
{
	fController = controller;
	if (!fController)
	{
		RaiseNavigationFailed(L"", E_FAIL, L"Failed to acquire WebView2 controller.");
		return;
	}

	Microsoft::WRL::ComPtr<ICoreWebView2> coreWebView;
	if (FAILED(fController->get_CoreWebView2(&coreWebView)) || !coreWebView)
	{
		RaiseNavigationFailed(L"", E_FAIL, L"Failed to acquire WebView2 core instance.");
		return;
	}

	fWebView = coreWebView;
	fIsWebViewReady = true;

	UpdateControllerBounds();
	fController->put_IsVisible(IsVisible());

	fWebView->add_NavigationStarting(
		Microsoft::WRL::Callback<ICoreWebView2NavigationStartingEventHandler>(
			[this](ICoreWebView2*, ICoreWebView2NavigationStartingEventArgs* args) -> HRESULT
			{
				HandleNavigationStarting(args);
				return S_OK;
			}).Get(),
		&fNavigationStartingToken);

	fWebView->add_NavigationCompleted(
		Microsoft::WRL::Callback<ICoreWebView2NavigationCompletedEventHandler>(
			[this](ICoreWebView2*, ICoreWebView2NavigationCompletedEventArgs* args) -> HRESULT
			{
				HandleNavigationCompleted(args);
				return S_OK;
			}).Get(),
		&fNavigationCompletedToken);

	ApplyPendingRequest();
}

void WebBrowser::UpdateControllerBounds()
{
	if (!fController)
	{
		return;
	}

	RECT clientBounds = GetClientBounds();
	RECT bounds{};
	bounds.left = 0;
	bounds.top = 0;
	bounds.right = std::max<LONG>(1, clientBounds.right - clientBounds.left);
	bounds.bottom = std::max<LONG>(1, clientBounds.bottom - clientBounds.top);
	fController->put_Bounds(bounds);
}

void WebBrowser::ApplyPendingRequest()
{
	if (!fIsWebViewReady)
	{
		return;
	}

	if (fPendingRequest.Url.empty())
	{
		return;
	}

	auto url = fPendingRequest.Url;
	auto headers = fPendingRequest.Headers;
	fPendingRequest.Url.clear();
	fPendingRequest.Headers.clear();

	if (!headers.empty())
	{
		NavigateToWithHeader(url.c_str(), headers.c_str());
	}
	else
	{
		NavigateTo(url.c_str());
	}
}

void WebBrowser::RaiseNavigationFailed(const std::wstring& url, HRESULT result, const std::wstring& message)
{
	std::wstringstream stream;
	stream << message;
	if (FAILED(result))
	{
		stream << L" (HRESULT: 0x" << std::hex << std::uppercase << static_cast<unsigned long>(result) << L")";
	}

	std::wstring combined = stream.str();
	WebBrowserNavigationFailedEventArgs eventArgs(url.c_str(), static_cast<int>(result), combined.c_str());
	fNavigationFailedEvent.Raise(*this, eventArgs);
}

void WebBrowser::OnReceivedMessage(UIComponent&, HandleMessageEventArgs& arguments)
{
	switch (arguments.GetMessageId())
	{
		case WM_SIZE:
		case WM_WINDOWPOSCHANGED:
		{
			UpdateControllerBounds();
			break;
		}
		case WM_SHOWWINDOW:
		{
			if (fController)
			{
				BOOL isVisible = (arguments.GetWParam() != 0);
				fController->put_IsVisible(isVisible ? TRUE : FALSE);
			}
			break;
		}
		default:
			break;
	}
}

void WebBrowser::HandleNavigationStarting(ICoreWebView2NavigationStartingEventArgs* args)
{
	if (!args)
	{
		return;
	}

	std::wstring uri = CopyUriFromArgs([args](wchar_t** value) -> HRESULT { return args->get_Uri(value); });
	WebBrowserNavigatingEventArgs eventArgs(uri.c_str());
	fNavigatingEvent.Raise(*this, eventArgs);
	if (eventArgs.WasCanceled())
	{
		args->put_Cancel(TRUE);
	}
}

void WebBrowser::HandleNavigationCompleted(ICoreWebView2NavigationCompletedEventArgs* args)
{
	if (!args)
	{
		return;
	}

	BOOL isSuccess = FALSE;
	if (FAILED(args->get_IsSuccess(&isSuccess)))
	{
		isSuccess = FALSE;
	}

	COREWEBVIEW2_WEB_ERROR_STATUS errorStatus = COREWEBVIEW2_WEB_ERROR_STATUS_UNKNOWN;
	args->get_WebErrorStatus(&errorStatus);

	std::wstring uri = CopyUriFromArgs([args](wchar_t** value) -> HRESULT { return args->get_Uri(value); });

	if (isSuccess)
	{
		WebBrowserNavigatedEventArgs eventArgs(uri.c_str());
		fNavigatedEvent.Raise(*this, eventArgs);
	}
	else
	{
		std::wstring description = DescribeWebErrorStatus(errorStatus);
		if (description.empty())
		{
			description = L"Navigation failed.";
		}
		WebBrowserNavigationFailedEventArgs eventArgs(
				uri.c_str(), static_cast<int>(errorStatus), description.c_str());
		fNavigationFailedEvent.Raise(*this, eventArgs);
	}
}

std::wstring WebBrowser::CopyUriFromArgs(std::function<HRESULT(wchar_t**)> getter) const
{
	if (!getter)
	{
		return std::wstring();
	}

	wchar_t* rawUri = nullptr;
	HRESULT result = getter(&rawUri);
	std::wstring uri;
	if (SUCCEEDED(result) && rawUri)
	{
		uri.assign(rawUri);
	}
	if (rawUri)
	{
		::CoTaskMemFree(rawUri);
	}
	return uri;
}

std::wstring WebBrowser::DescribeWebErrorStatus(COREWEBVIEW2_WEB_ERROR_STATUS status) const
{
	switch (status)
	{
		case COREWEBVIEW2_WEB_ERROR_STATUS_UNKNOWN:
			return L"Unknown navigation error.";
		case COREWEBVIEW2_WEB_ERROR_STATUS_CERTIFICATE_COMMON_NAME_IS_INCORRECT:
			return L"Certificate common name is incorrect.";
		case COREWEBVIEW2_WEB_ERROR_STATUS_CERTIFICATE_EXPIRED:
			return L"Certificate has expired.";
		case COREWEBVIEW2_WEB_ERROR_STATUS_CLIENT_CERTIFICATE_CONTAINS_ERRORS:
			return L"Client certificate contains errors.";
		case COREWEBVIEW2_WEB_ERROR_STATUS_CERTIFICATE_REVOKED:
			return L"Certificate was revoked.";
		case COREWEBVIEW2_WEB_ERROR_STATUS_CERTIFICATE_IS_INVALID:
			return L"Certificate is invalid.";
		case COREWEBVIEW2_WEB_ERROR_STATUS_DISCONNECTED:
			return L"The connection was disconnected.";
		case COREWEBVIEW2_WEB_ERROR_STATUS_CANNOT_CONNECT:
			return L"Cannot connect to destination.";
		case COREWEBVIEW2_WEB_ERROR_STATUS_NOT_FOUND:
			return L"The requested resource was not found.";
		case COREWEBVIEW2_WEB_ERROR_STATUS_CONNECTION_ABORTED:
			return L"The connection was aborted.";
		case COREWEBVIEW2_WEB_ERROR_STATUS_CONNECTION_RESET:
			return L"The connection was reset.";
		case COREWEBVIEW2_WEB_ERROR_STATUS_HOST_NAME_NOT_RESOLVED:
			return L"Host name could not be resolved.";
		case COREWEBVIEW2_WEB_ERROR_STATUS_OPERATION_CANCELED:
			return L"The navigation was canceled.";
		case COREWEBVIEW2_WEB_ERROR_STATUS_REDIRECT_FAILED:
			return L"Navigation redirect failed.";
	}

	std::wstringstream stream;
	stream << L"Navigation failed with status " << static_cast<int>(status) << L'.';
	return stream.str();
}

#pragma endregion

} }
