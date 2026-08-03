//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "StartupDiagnostics.h"
#include "CoronaWin32.h"
#include "CoronaVersion.h"
#include <algorithm>
#include <DbgHelp.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <wchar.h>


namespace
{
	volatile LONG sIsHandlingCrash = 0;

	std::wstring GetEnvironmentVariableValue(const wchar_t *name)
	{
		std::wstring value;
		DWORD characterCount = ::GetEnvironmentVariableW(name, nullptr, 0);
		if (characterCount > 1)
		{
			std::vector<wchar_t> buffer(characterCount);
			characterCount = ::GetEnvironmentVariableW(name, buffer.data(), (DWORD)buffer.size());
			if ((characterCount > 0) && (characterCount < buffer.size()))
			{
				value.assign(buffer.data(), characterCount);
			}
		}
		return value;
	}

	std::wstring GetExecutableFileNameWithoutExtension()
	{
		std::wstring fileName(L"Corona App");
		std::vector<wchar_t> buffer(32768);
		DWORD characterCount = ::GetModuleFileNameW(nullptr, buffer.data(), (DWORD)buffer.size());
		if ((characterCount > 0) && (characterCount < buffer.size()))
		{
			fileName.assign(buffer.data(), characterCount);
			auto index = fileName.find_last_of(L"\\/");
			if (index != std::wstring::npos)
			{
				fileName.erase(0, index + 1);
			}
			index = fileName.find_last_of(L'.');
			if (index != std::wstring::npos)
			{
				fileName.erase(index);
			}
		}
		return fileName;
	}

	std::wstring SanitizePathComponent(const wchar_t *text, const wchar_t *fallbackText)
	{
		std::wstring value(text ? text : L"");
		for (auto& character : value)
		{
			if ((character < 32) || (wcschr(L"<>:\"/\\|?*", character) != nullptr))
			{
				character = L'_';
			}
		}
		while (!value.empty() && ((value.back() == L' ') || (value.back() == L'.')))
		{
			value.pop_back();
		}
		if (value.empty() || (value == L".") || (value == L".."))
		{
			value = fallbackText ? fallbackText : L"Solar2D";
		}
		if (value.length() > 100)
		{
			value.erase(100);
		}
		return value;
	}

	std::string Utf8From(const wchar_t *text)
	{
		std::string result;
		if (!text || (text[0] == L'\0'))
		{
			return result;
		}

		int byteCount = ::WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
		if (byteCount > 1)
		{
			std::vector<char> buffer(byteCount);
			byteCount = ::WideCharToMultiByte(
				CP_UTF8, 0, text, -1, buffer.data(), (int)buffer.size(), nullptr, nullptr);
			if (byteCount > 1)
			{
				result.assign(buffer.data(), byteCount - 1);
			}
		}
		return result;
	}

	bool IsFileTimeOlder(const WIN32_FIND_DATAW& first, const WIN32_FIND_DATAW& second)
	{
		return ::CompareFileTime(&first.ftLastWriteTime, &second.ftLastWriteTime) < 0;
	}

	bool IsFileTimeNewer(const WIN32_FIND_DATAW& first, const WIN32_FIND_DATAW& second)
	{
		return ::CompareFileTime(&first.ftLastWriteTime, &second.ftLastWriteTime) > 0;
	}
}


StartupDiagnostics* StartupDiagnostics::sActiveInstancePointer = nullptr;

StartupDiagnostics::StartupDiagnostics()
:	fDbgHelpModuleHandle(nullptr),
	fLogFileHandle(INVALID_HANDLE_VALUE),
	fLogFileSizeInBytes(0),
	fIsLogSizeLimitReached(false),
	fIsStarted(false),
	fPreviousUnhandledExceptionFilter(nullptr)
{
	::InitializeCriticalSection(&fCriticalSection);
}

StartupDiagnostics::~StartupDiagnostics()
{
	if (fIsStarted)
	{
		::CoronaWin32SetLogCallback(nullptr, nullptr);
		::CoronaWin32SetDiagnosticLogPath(nullptr);
	}
	if (sActiveInstancePointer == this)
	{
		sActiveInstancePointer = nullptr;
	}
	if (fLogFileHandle != INVALID_HANDLE_VALUE)
	{
		::FlushFileBuffers(fLogFileHandle);
		::CloseHandle(fLogFileHandle);
		fLogFileHandle = INVALID_HANDLE_VALUE;
	}
	if (fDbgHelpModuleHandle)
	{
		::FreeLibrary(fDbgHelpModuleHandle);
		fDbgHelpModuleHandle = nullptr;
	}
	::DeleteCriticalSection(&fCriticalSection);
}

bool StartupDiagnostics::Start()
{
	if (fIsStarted)
	{
		return true;
	}
	if (!CreateDiagnosticDirectories() || !OpenLogFile())
	{
		return false;
	}

	wchar_t dbgHelpPath[MAX_PATH];
	UINT dbgHelpPathLength = ::GetSystemDirectoryW(dbgHelpPath, _countof(dbgHelpPath));
	DWORD dbgHelpErrorCode = dbgHelpPathLength > 0 ? ERROR_INSUFFICIENT_BUFFER : ::GetLastError();
	if ((dbgHelpPathLength > 0) && (dbgHelpPathLength < (_countof(dbgHelpPath) - 12)))
	{
		wcscat_s(dbgHelpPath, _countof(dbgHelpPath), L"\\DbgHelp.dll");
		fDbgHelpModuleHandle = ::LoadLibraryW(dbgHelpPath);
		dbgHelpErrorCode = fDbgHelpModuleHandle ? ERROR_SUCCESS : ::GetLastError();
	}
	fIsStarted = true;
	sActiveInstancePointer = this;
	fPreviousUnhandledExceptionFilter = ::SetUnhandledExceptionFilter(&StartupDiagnostics::OnUnhandledException);
	::CoronaWin32SetDiagnosticLogPath(fLogFilePath.c_str());
	::CoronaWin32SetLogCallback(&StartupDiagnostics::OnCoronaLog, this);

	Log("stage=diagnostics-started pid=%lu", ::GetCurrentProcessId());
	if (!fDbgHelpModuleHandle)
	{
		LogLastError("dbghelp-load-failed", dbgHelpErrorCode);
	}
	LogSystemInformation();
	return true;
}

void StartupDiagnostics::Log(const char *format, ...)
{
	if (!format)
	{
		return;
	}

	char message[8192];
	va_list arguments;
	va_start(arguments, format);
	int characterCount = _vsnprintf_s(message, sizeof(message), _TRUNCATE, format, arguments);
	va_end(arguments);
	if (characterCount < 0)
	{
		characterCount = (int)strlen(message);
	}
	WriteMessage(message, characterCount);
}

void StartupDiagnostics::LogLastError(const char *stageName, DWORD errorCode)
{
	LPWSTR systemMessage = nullptr;
	::FormatMessageW(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&systemMessage, 0, nullptr);
	std::string utf8SystemMessage = Utf8From(systemMessage);
	if (systemMessage)
	{
		::LocalFree(systemMessage);
	}
	while (!utf8SystemMessage.empty() &&
	       ((utf8SystemMessage.back() == '\r') || (utf8SystemMessage.back() == '\n')))
	{
		utf8SystemMessage.pop_back();
	}
	Log("stage=%s windowsError=%lu message=%s",
		stageName ? stageName : "unknown", errorCode,
		utf8SystemMessage.empty() ? "unknown" : utf8SystemMessage.c_str());
}

void StartupDiagnostics::ShowStartupError(
	HWND parentWindowHandle, const wchar_t *errorCode, const wchar_t *message, DWORD systemErrorCode)
{
	std::wstring fullMessage(message ? message : L"The application failed to start.");
	if (systemErrorCode != ERROR_SUCCESS)
	{
		LPWSTR systemMessage = nullptr;
		::FormatMessageW(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr, systemErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPWSTR)&systemMessage, 0, nullptr);
		if (systemMessage)
		{
			fullMessage.append(L"\r\n\r\nWindows reported:\r\n");
			fullMessage.append(systemMessage);
			::LocalFree(systemMessage);
		}
	}
	fullMessage.append(L"\r\n\r\nError code: ");
	fullMessage.append(errorCode ? errorCode : L"S2D-WIN-STARTUP");
	if (!fLogFilePath.empty())
	{
		fullMessage.append(L"\r\nDiagnostic log:\r\n");
		fullMessage.append(fLogFilePath);
	}

	std::string utf8Message = Utf8From(fullMessage.c_str());
	Log("startup-error code=%s message=%s",
		Utf8From(errorCode).c_str(), utf8Message.c_str());
	::MessageBoxW(parentWindowHandle, fullMessage.c_str(), L"Startup Error", MB_OK | MB_ICONERROR);
}

const wchar_t* StartupDiagnostics::GetLogFilePath() const
{
	return fLogFilePath.c_str();
}

void StartupDiagnostics::OnCoronaLog(const char *message, size_t length, void *contextPointer)
{
	auto diagnosticsPointer = (StartupDiagnostics*)contextPointer;
	if (diagnosticsPointer)
	{
		diagnosticsPointer->WriteMessage(message, length);
	}
}

LONG WINAPI StartupDiagnostics::OnUnhandledException(EXCEPTION_POINTERS *exceptionPointers)
{
	if (::InterlockedCompareExchange(&sIsHandlingCrash, 1, 0) != 0)
	{
		return EXCEPTION_CONTINUE_SEARCH;
	}
	auto diagnosticsPointer = sActiveInstancePointer;
	LPTOP_LEVEL_EXCEPTION_FILTER previousFilter = diagnosticsPointer
		? diagnosticsPointer->fPreviousUnhandledExceptionFilter : nullptr;
	if (diagnosticsPointer)
	{
		wchar_t dumpFilePath[MAX_PATH];
		dumpFilePath[0] = L'\0';
		DWORD exceptionCode = exceptionPointers && exceptionPointers->ExceptionRecord
			? exceptionPointers->ExceptionRecord->ExceptionCode : 0;
		char message[128];
		_snprintf_s(message, sizeof(message), _TRUNCATE,
			"unhandled-exception code=0x%08lX; writing crash dump", exceptionCode);
		diagnosticsPointer->WriteEmergencyMessage(message);
		if (diagnosticsPointer->WriteCrashDump(exceptionPointers, dumpFilePath, _countof(dumpFilePath)))
		{
			char utf8DumpFilePath[MAX_PATH * 4];
			int byteCount = ::WideCharToMultiByte(
				CP_UTF8, 0, dumpFilePath, -1, utf8DumpFilePath, (int)_countof(utf8DumpFilePath), nullptr, nullptr);
			if (byteCount > 1)
			{
				_snprintf_s(message, sizeof(message), _TRUNCATE, "crash-dump-written path=");
				diagnosticsPointer->WriteEmergencyMessage(message);
				diagnosticsPointer->WriteEmergencyMessage(utf8DumpFilePath);
			}
		}
		else
		{
			diagnosticsPointer->WriteEmergencyMessage("crash-dump-failed");
		}
	}
	if (previousFilter && (previousFilter != &StartupDiagnostics::OnUnhandledException))
	{
		return previousFilter(exceptionPointers);
	}
	return EXCEPTION_CONTINUE_SEARCH;
}

bool StartupDiagnostics::CreateDiagnosticDirectories()
{
	std::wstring localAppDataPath = GetEnvironmentVariableValue(L"LOCALAPPDATA");
	if (localAppDataPath.empty())
	{
		localAppDataPath = GetEnvironmentVariableValue(L"TEMP");
	}
	if (localAppDataPath.empty())
	{
		return false;
	}

	std::wstring executableName = GetExecutableFileNameWithoutExtension();
	std::wstring companyName = SanitizePathComponent(
		::CoronaWin32ApplicationGetCompanyName(), L"Solar2D");
	std::wstring productName = SanitizePathComponent(
		::CoronaWin32ApplicationGetProductName(), executableName.c_str());
	fDiagnosticDirectoryPath = localAppDataPath;
	fDiagnosticDirectoryPath.append(L"\\");
	fDiagnosticDirectoryPath.append(companyName);
	::CreateDirectoryW(fDiagnosticDirectoryPath.c_str(), nullptr);
	fDiagnosticDirectoryPath.append(L"\\");
	fDiagnosticDirectoryPath.append(productName);
	if (!::CreateDirectoryW(fDiagnosticDirectoryPath.c_str(), nullptr) &&
	    (::GetLastError() != ERROR_ALREADY_EXISTS))
	{
		return false;
	}

	fLogDirectoryPath = fDiagnosticDirectoryPath + L"\\Logs";
	fCrashDirectoryPath = fDiagnosticDirectoryPath + L"\\Crashes";
	if (!::CreateDirectoryW(fLogDirectoryPath.c_str(), nullptr) &&
	    (::GetLastError() != ERROR_ALREADY_EXISTS))
	{
		return false;
	}
	if (!::CreateDirectoryW(fCrashDirectoryPath.c_str(), nullptr) &&
	    (::GetLastError() != ERROR_ALREADY_EXISTS))
	{
		return false;
	}
	return true;
}

bool StartupDiagnostics::OpenLogFile()
{
	RemoveOldCrashDumps();

	SYSTEMTIME systemTime{};
	::GetSystemTime(&systemTime);
	wchar_t fileName[128];
	int characterCount = _snwprintf_s(
		fileName, _countof(fileName), _TRUNCATE,
		L"launch-%04u-%02u-%02uT%02u-%02u-%02u-%03uZ-%lu.log",
		systemTime.wYear, systemTime.wMonth, systemTime.wDay, systemTime.wHour,
		systemTime.wMinute, systemTime.wSecond, systemTime.wMilliseconds, ::GetCurrentProcessId());
	if (characterCount <= 0)
	{
		return false;
	}

	fLogFilePath = fLogDirectoryPath + L"\\" + fileName;
	fLogFileHandle = ::CreateFileW(
		fLogFilePath.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE,
		nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (fLogFileHandle == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	RemoveOldLogFiles();
	return true;
}

void StartupDiagnostics::RemoveOldLogFiles()
{
	std::vector<WIN32_FIND_DATAW> files;
	std::wstring searchPattern = fLogDirectoryPath + L"\\launch-*.log";
	WIN32_FIND_DATAW findData{};
	HANDLE findHandle = ::FindFirstFileW(searchPattern.c_str(), &findData);
	if (findHandle != INVALID_HANDLE_VALUE)
	{
		do
		{
			if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
			{
				files.push_back(findData);
			}
		} while (::FindNextFileW(findHandle, &findData));
		::FindClose(findHandle);
	}
	if (files.size() <= kMaxLogFileCount)
	{
		return;
	}

	std::sort(files.begin(), files.end(), IsFileTimeNewer);
	for (size_t index = kMaxLogFileCount; index < files.size(); index++)
	{
		std::wstring filePath = fLogDirectoryPath + L"\\" + files[index].cFileName;
		if (_wcsicmp(filePath.c_str(), fLogFilePath.c_str()) != 0)
		{
			// Deletion fails with a sharing violation if another app instance still has this log open.
			::DeleteFileW(filePath.c_str());
		}
	}
}

void StartupDiagnostics::RemoveOldCrashDumps()
{
	std::vector<WIN32_FIND_DATAW> files;
	std::wstring searchPattern = fCrashDirectoryPath + L"\\crash-*.dmp";
	WIN32_FIND_DATAW findData{};
	HANDLE findHandle = ::FindFirstFileW(searchPattern.c_str(), &findData);
	if (findHandle != INVALID_HANDLE_VALUE)
	{
		do
		{
			if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
			{
				files.push_back(findData);
			}
		} while (::FindNextFileW(findHandle, &findData));
		::FindClose(findHandle);
	}
	if (files.size() < kMaxCrashDumpCount)
	{
		return;
	}

	std::sort(files.begin(), files.end(), IsFileTimeOlder);
	size_t deleteCount = files.size() - kMaxCrashDumpCount + 1;
	for (size_t index = 0; index < deleteCount; index++)
	{
		std::wstring filePath = fCrashDirectoryPath + L"\\" + files[index].cFileName;
		::DeleteFileW(filePath.c_str());
	}
}

void StartupDiagnostics::LogSystemInformation()
{
	Log("company=%s", Utf8From(::CoronaWin32ApplicationGetCompanyName()).c_str());
	Log("product=%s", Utf8From(::CoronaWin32ApplicationGetProductName()).c_str());
	Log("version=%s", Utf8From(::CoronaWin32ApplicationGetFileVersionString()).c_str());
	Log("solar2d-build=%s", ::CoronaVersionBuildString());
	Log("diagnostic-directory=%s", Utf8From(fDiagnosticDirectoryPath.c_str()).c_str());

	std::vector<wchar_t> pathBuffer(32768);
	std::wstring executablePath;
	DWORD characterCount = ::GetModuleFileNameW(nullptr, pathBuffer.data(), (DWORD)pathBuffer.size());
	if ((characterCount > 0) && (characterCount < pathBuffer.size()))
	{
		executablePath.assign(pathBuffer.data(), characterCount);
		Log("executable=%s", Utf8From(pathBuffer.data()).c_str());
	}
	characterCount = ::GetCurrentDirectoryW((DWORD)pathBuffer.size(), pathBuffer.data());
	if ((characterCount > 0) && (characterCount < pathBuffer.size()))
	{
		Log("working-directory=%s", Utf8From(pathBuffer.data()).c_str());
	}
	if (!executablePath.empty())
	{
		auto separatorIndex = executablePath.find_last_of(L"\\/");
		if (separatorIndex != std::wstring::npos)
		{
			std::wstring executableDirectory = executablePath.substr(0, separatorIndex);
			const wchar_t *relativeResourcePaths[] =
			{
				L"resource.car", L"main.lua", L"Resources\\resource.car", L"Resources\\main.lua"
			};
			for (auto relativePath : relativeResourcePaths)
			{
				std::wstring resourcePath = executableDirectory + L"\\" + relativePath;
				DWORD attributes = ::GetFileAttributesW(resourcePath.c_str());
				Log("startup-resource path=%s present=%s",
					Utf8From(relativePath).c_str(),
					(attributes != INVALID_FILE_ATTRIBUTES) && !(attributes & FILE_ATTRIBUTE_DIRECTORY)
						? "yes" : "no");
			}
		}
	}

	OSVERSIONINFOW versionInfo{};
	versionInfo.dwOSVersionInfoSize = sizeof(versionInfo);
	HMODULE ntdllModuleHandle = ::GetModuleHandleW(L"ntdll.dll");
	if (ntdllModuleHandle)
	{
		typedef LONG (WINAPI *RtlGetVersionCallback)(OSVERSIONINFOW*);
		auto callback = (RtlGetVersionCallback)::GetProcAddress(ntdllModuleHandle, "RtlGetVersion");
		if (callback && (callback(&versionInfo) == 0))
		{
			Log("windows-version=%lu.%lu.%lu service-pack=%s",
				versionInfo.dwMajorVersion, versionInfo.dwMinorVersion, versionInfo.dwBuildNumber,
				Utf8From(versionInfo.szCSDVersion).c_str());
		}
	}

	SYSTEM_INFO systemInfo{};
	::GetNativeSystemInfo(&systemInfo);
	const char *architectureName = "unknown";
	switch (systemInfo.wProcessorArchitecture)
	{
		case PROCESSOR_ARCHITECTURE_AMD64:
			architectureName = "x64";
			break;
		case PROCESSOR_ARCHITECTURE_ARM64:
			architectureName = "arm64";
			break;
		case PROCESSOR_ARCHITECTURE_INTEL:
			architectureName = "x86";
			break;
	}
	Log("native-architecture=%s process-architecture=x86 processors=%lu",
		architectureName, systemInfo.dwNumberOfProcessors);

	for (DWORD index = 0; index < 8; index++)
	{
		DISPLAY_DEVICEW displayDevice{};
		displayDevice.cb = sizeof(displayDevice);
		if (!::EnumDisplayDevicesW(nullptr, index, &displayDevice, 0))
		{
			break;
		}
		if (displayDevice.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER)
		{
			continue;
		}
		Log("display-adapter[%lu]=%s device-id=%s state=0x%08lX",
			index, Utf8From(displayDevice.DeviceString).c_str(),
			Utf8From(displayDevice.DeviceID).c_str(), displayDevice.StateFlags);
	}
}

void StartupDiagnostics::WriteMessage(const char *message, size_t length)
{
	if ((fLogFileHandle == INVALID_HANDLE_VALUE) || !message)
	{
		return;
	}
	::EnterCriticalSection(&fCriticalSection);
	if (fIsLogSizeLimitReached)
	{
		::LeaveCriticalSection(&fCriticalSection);
		return;
	}

	SYSTEMTIME systemTime{};
	::GetSystemTime(&systemTime);
	char prefix[64];
	int prefixLength = _snprintf_s(
		prefix, sizeof(prefix), _TRUNCATE, "[%04u-%02u-%02uT%02u:%02u:%02u.%03uZ] [thread=%lu] ",
		systemTime.wYear, systemTime.wMonth, systemTime.wDay, systemTime.wHour,
		systemTime.wMinute, systemTime.wSecond, systemTime.wMilliseconds, ::GetCurrentThreadId());
	bool needsNewLine = (length <= 0) || ((message[length - 1] != '\n') && (message[length - 1] != '\r'));
	unsigned long long nextSize = fLogFileSizeInBytes + prefixLength + length + (needsNewLine ? 2 : 0);
	if (nextSize > kMaxLogFileSizeInBytes)
	{
		const char limitMessage[] = "Log size limit reached. Further messages are omitted.\r\n";
		DWORD bytesWritten = 0;
		::WriteFile(fLogFileHandle, prefix, prefixLength, &bytesWritten, nullptr);
		::WriteFile(fLogFileHandle, limitMessage, sizeof(limitMessage) - 1, &bytesWritten, nullptr);
		fIsLogSizeLimitReached = true;
		::LeaveCriticalSection(&fCriticalSection);
		return;
	}

	DWORD bytesWritten = 0;
	::WriteFile(fLogFileHandle, prefix, prefixLength, &bytesWritten, nullptr);
	::WriteFile(fLogFileHandle, message, (DWORD)length, &bytesWritten, nullptr);
	if (needsNewLine)
	{
		::WriteFile(fLogFileHandle, "\r\n", 2, &bytesWritten, nullptr);
	}
	fLogFileSizeInBytes = nextSize;
	::LeaveCriticalSection(&fCriticalSection);
}

void StartupDiagnostics::WriteEmergencyMessage(const char *message)
{
	if ((fLogFileHandle == INVALID_HANDLE_VALUE) || !message || !::TryEnterCriticalSection(&fCriticalSection))
	{
		return;
	}
	DWORD bytesWritten = 0;
	::WriteFile(fLogFileHandle, "[crash-handler] ", 16, &bytesWritten, nullptr);
	::WriteFile(fLogFileHandle, message, (DWORD)strlen(message), &bytesWritten, nullptr);
	::WriteFile(fLogFileHandle, "\r\n", 2, &bytesWritten, nullptr);
	::FlushFileBuffers(fLogFileHandle);
	::LeaveCriticalSection(&fCriticalSection);
}

bool StartupDiagnostics::WriteCrashDump(
	EXCEPTION_POINTERS *exceptionPointers, wchar_t *dumpFilePath, size_t dumpFilePathLength)
{
	if (!dumpFilePath || (dumpFilePathLength <= 0) || fCrashDirectoryPath.empty())
	{
		return false;
	}

	SYSTEMTIME systemTime{};
	::GetSystemTime(&systemTime);
	int characterCount = _snwprintf_s(
		dumpFilePath, dumpFilePathLength, _TRUNCATE,
		L"%s\\crash-%04u%02u%02u-%02u%02u%02u-%lu.dmp",
		fCrashDirectoryPath.c_str(), systemTime.wYear, systemTime.wMonth, systemTime.wDay,
		systemTime.wHour, systemTime.wMinute, systemTime.wSecond, ::GetCurrentProcessId());
	if (characterCount <= 0)
	{
		return false;
	}

	HANDLE dumpFileHandle = ::CreateFileW(
		dumpFilePath, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (dumpFileHandle == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	if (!fDbgHelpModuleHandle)
	{
		::CloseHandle(dumpFileHandle);
		::DeleteFileW(dumpFilePath);
		return false;
	}
	typedef BOOL (WINAPI *MiniDumpWriteDumpCallback)(
		HANDLE, DWORD, HANDLE, MINIDUMP_TYPE, PMINIDUMP_EXCEPTION_INFORMATION,
		PMINIDUMP_USER_STREAM_INFORMATION, PMINIDUMP_CALLBACK_INFORMATION);
	auto callback = (MiniDumpWriteDumpCallback)::GetProcAddress(fDbgHelpModuleHandle, "MiniDumpWriteDump");
	bool wasWritten = false;
	if (callback)
	{
		MINIDUMP_EXCEPTION_INFORMATION exceptionInformation{};
		exceptionInformation.ThreadId = ::GetCurrentThreadId();
		exceptionInformation.ExceptionPointers = exceptionPointers;
		exceptionInformation.ClientPointers = FALSE;
		MINIDUMP_TYPE dumpType = (MINIDUMP_TYPE)(
			MiniDumpWithDataSegs | MiniDumpWithHandleData | MiniDumpWithIndirectlyReferencedMemory |
			MiniDumpWithThreadInfo | MiniDumpWithUnloadedModules);
		wasWritten = callback(
			::GetCurrentProcess(), ::GetCurrentProcessId(), dumpFileHandle, dumpType,
			exceptionPointers ? &exceptionInformation : nullptr, nullptr, nullptr) ? true : false;
	}
	::FlushFileBuffers(dumpFileHandle);
	::CloseHandle(dumpFileHandle);
	if (!wasWritten)
	{
		::DeleteFileW(dumpFilePath);
	}
	return wasWritten;
}
