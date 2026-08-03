//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

#include <stddef.h>
#include <Windows.h>
#include <string>


/// <summary>
///  Persists Win32 startup and engine log output and writes best-effort native crash dumps.
/// </summary>
class StartupDiagnostics
{
	public:
		StartupDiagnostics();
		virtual ~StartupDiagnostics();

		bool Start();
		void Log(const char *format, ...);
		void LogLastError(const char *stageName, DWORD errorCode);
		void ShowStartupError(
			HWND parentWindowHandle, const wchar_t *errorCode, const wchar_t *message, DWORD systemErrorCode);

		const wchar_t* GetLogFilePath() const;

	private:
		static const unsigned long long kMaxLogFileSizeInBytes = 5ULL * 1024ULL * 1024ULL;
		static const int kMaxLogFileCount = 5;
		static const int kMaxCrashDumpCount = 5;

		static StartupDiagnostics *sActiveInstancePointer;

		static void OnCoronaLog(const char *message, size_t length, void *contextPointer);
		static LONG WINAPI OnUnhandledException(EXCEPTION_POINTERS *exceptionPointers);

		bool CreateDiagnosticDirectories();
		bool OpenLogFile();
		void RemoveOldLogFiles();
		void RemoveOldCrashDumps();
		void LogSystemInformation();
		void WriteMessage(const char *message, size_t length);
		void WriteEmergencyMessage(const char *message);
		bool WriteCrashDump(EXCEPTION_POINTERS *exceptionPointers, wchar_t *dumpFilePath, size_t dumpFilePathLength);

		StartupDiagnostics(const StartupDiagnostics& diagnostics) {}
		void operator=(const StartupDiagnostics& diagnostics) {}

		CRITICAL_SECTION fCriticalSection;
		HMODULE fDbgHelpModuleHandle;
		HANDLE fLogFileHandle;
		unsigned long long fLogFileSizeInBytes;
		bool fIsLogSizeLimitReached;
		bool fIsStarted;
		LPTOP_LEVEL_EXCEPTION_FILTER fPreviousUnhandledExceptionFilter;
		std::wstring fDiagnosticDirectoryPath;
		std::wstring fLogDirectoryPath;
		std::wstring fCrashDirectoryPath;
		std::wstring fLogFilePath;
};
