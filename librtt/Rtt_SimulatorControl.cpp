//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Core/Rtt_Build.h"
#include "Core/Rtt_Assert.h"
#include "Core/Rtt_Data.h"

#include "Rtt_SimulatorControl.h"

#ifdef Rtt_AUTHORING_SIMULATOR

#include "Display/Rtt_BitmapPaint.h"
#include "Display/Rtt_Display.h"
#include "Display/Rtt_PlatformBitmap.h"
#include "Rtt_LuaContext.h"
#include "Rtt_MPlatform.h"
#include "Rtt_MSimulatorHost.h"
#include "Rtt_Runtime.h"

#include <ctype.h>
#include <deque>
#include <errno.h>
#include <fcntl.h>
#include <float.h>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits.h>
#include <map>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/stat.h>
#include <time.h>
#include <utility>

#ifdef Rtt_WIN_ENV
	#include <io.h>
	#include <process.h>
	#include <windows.h>
#else
	#include <dirent.h>
	#include <pthread.h>
	#include <signal.h>
	#include <sys/time.h>
	#include <unistd.h>
#endif

// ----------------------------------------------------------------------------

namespace Rtt
{

// ----------------------------------------------------------------------------

static int
AbsoluteLuaIndex( lua_State *L, int index )
{
	return index > 0 || index <= LUA_REGISTRYINDEX ? index : lua_gettop( L ) + index + 1;
}

static bool
IsFiniteNumber( lua_Number value )
{
	return value == value && value <= DBL_MAX && value >= -DBL_MAX;
}

static const size_t kSimulatorControlMaximumRequestSize = 1024 * 1024;
static const size_t kSimulatorControlMaximumExecFileSize = 16 * 1024 * 1024;
static const size_t kSimulatorControlMaximumStringSize = 4096;
static const size_t kSimulatorControlMaximumResponseSize = 256 * 1024;
static const size_t kSimulatorControlMaximumDiagnosticMessageSize = 16 * 1024;
static const size_t kSimulatorControlMaximumDiagnosticStackTraceSize = 128 * 1024;
static const size_t kSimulatorControlMaximumLogBytes = 128 * 1024;
static const size_t kSimulatorControlMaximumLogEntries = 500;
static const int kSimulatorControlMaximumEntries = 100;
static char kSimulatorControlRegistryKey;

class SimulatorControlLogMutex
{
	public:
		SimulatorControlLogMutex()
		{
#ifdef Rtt_WIN_ENV
			InitializeCriticalSection( & fMutex );
#else
			pthread_mutex_init( & fMutex, NULL );
#endif
		}

		~SimulatorControlLogMutex()
		{
#ifdef Rtt_WIN_ENV
			DeleteCriticalSection( & fMutex );
#else
			pthread_mutex_destroy( & fMutex );
#endif
		}

		void Lock()
		{
#ifdef Rtt_WIN_ENV
			EnterCriticalSection( & fMutex );
#else
			pthread_mutex_lock( & fMutex );
#endif
		}

		void Unlock()
		{
#ifdef Rtt_WIN_ENV
			LeaveCriticalSection( & fMutex );
#else
			pthread_mutex_unlock( & fMutex );
#endif
		}

	private:
#ifdef Rtt_WIN_ENV
		CRITICAL_SECTION fMutex;
#else
		pthread_mutex_t fMutex;
#endif
};

class SimulatorControlLogGuard
{
	public:
		SimulatorControlLogGuard( SimulatorControlLogMutex& mutex )
		:	fMutex( mutex )
		{
			fMutex.Lock();
		}

		~SimulatorControlLogGuard()
		{
			fMutex.Unlock();
		}

	private:
		SimulatorControlLogMutex& fMutex;
};

struct SimulatorControlLogEntry
{
	unsigned long sequence;
	std::string message;
};

struct SimulatorControlLogState
{
	SimulatorControlLogState()
	:	sequence( 0 ),
		byteCount( 0 ),
		droppedEntries( false )
	{
	}

	unsigned long sequence;
	size_t byteCount;
	bool droppedEntries;
	std::deque< SimulatorControlLogEntry > entries;
};

static SimulatorControlLogMutex&
GetSimulatorControlLogMutex()
{
	static SimulatorControlLogMutex *mutex =
		new SimulatorControlLogMutex();
	return *mutex;
}

static SimulatorControlLogState&
GetSimulatorControlLogState()
{
	static SimulatorControlLogState *state =
		new SimulatorControlLogState();
	return *state;
}

static void
ResetSimulatorControlLogs()
{
	SimulatorControlLogGuard guard( GetSimulatorControlLogMutex() );
	SimulatorControlLogState& state = GetSimulatorControlLogState();
	state.sequence = 0;
	state.byteCount = 0;
	state.droppedEntries = false;
	state.entries.clear();
}

static void
RecordSimulatorControlLog(
	const char *message, size_t length, void *context )
{
	(void)context;
	if ( ! message || 0 == length )
	{
		return;
	}
	while ( length > 0 &&
		( '\n' == message[length - 1] || '\r' == message[length - 1] ) )
	{
		length--;
	}
	if ( 0 == length )
	{
		return;
	}

	SimulatorControlLogGuard guard( GetSimulatorControlLogMutex() );
	SimulatorControlLogState& state = GetSimulatorControlLogState();
	SimulatorControlLogEntry entry;
	entry.sequence = ++state.sequence;
	entry.message.assign( message, length );
	state.byteCount += entry.message.length();
	state.entries.push_back( entry );

	while ( state.entries.size() > kSimulatorControlMaximumLogEntries ||
		state.byteCount > kSimulatorControlMaximumLogBytes )
	{
		state.byteCount -= state.entries.front().message.length();
		state.entries.pop_front();
		state.droppedEntries = true;
	}
}

static std::string&
GetSimulatorControlDirectory()
{
	static std::string directory;
	return directory;
}

static std::string&
GetSimulatorControlOwnerPath()
{
	static std::string path;
	return path;
}

struct SimulatorControlRuntimeState
{
	SimulatorControlRuntimeState()
	:	generation( 0 ),
		diagnosticSequence( 0 ),
		diagnosticFrame( 0 ),
		hasDiagnostic( false ),
		diagnosticTruncated( false ),
		diagnosticsWritten( false ),
		sessionWritten( false )
	{
	}

	unsigned long generation;
	unsigned long diagnosticSequence;
	unsigned long diagnosticFrame;
	std::string directory;
	std::string sessionId;
	std::string diagnosticType;
	std::string diagnosticMessage;
	std::string diagnosticStackTrace;
	bool hasDiagnostic;
	bool diagnosticTruncated;
	bool diagnosticsWritten;
	bool sessionWritten;
};

struct SimulatorControlSession
{
	SimulatorControlSession()
	:	generation( 0 ),
		processId( 0 )
	{
	}

	unsigned long generation;
	unsigned long processId;
	std::string sessionId;
};

typedef std::map< const Runtime*, SimulatorControlRuntimeState >
	SimulatorControlRuntimeStateMap;

static SimulatorControlRuntimeStateMap&
GetSimulatorControlRuntimeStates()
{
	static SimulatorControlRuntimeStateMap states;
	return states;
}

static unsigned long
NextSimulatorControlGeneration()
{
	static unsigned long generation = 0;
	return ++generation;
}

static long
GetSimulatorControlProcessId()
{
#ifdef Rtt_WIN_ENV
	return (long)_getpid();
#else
	return (long)getpid();
#endif
}

static std::string
SimulatorControlPath( const std::string& directory, const char *filename )
{
	if ( directory.empty() )
	{
		return std::string();
	}

	char lastCharacter = directory[directory.length() - 1];
	if ( '/' == lastCharacter || '\\' == lastCharacter )
	{
		return directory + filename;
	}

#ifdef Rtt_WIN_ENV
	return directory + "\\" + filename;
#else
	return directory + "/" + filename;
#endif
}

#ifdef Rtt_WIN_ENV

static bool
ConvertSimulatorControlUtf8ToWide(
	const std::string& value, std::wstring& result )
{
	if ( value.empty() )
	{
		result.clear();
		return true;
	}
	int length = MultiByteToWideChar(
		CP_UTF8, MB_ERR_INVALID_CHARS,
		value.data(), (int)value.length(), NULL, 0 );
	if ( length <= 0 )
	{
		return false;
	}
	result.resize( (size_t)length );
	return length == MultiByteToWideChar(
		CP_UTF8, MB_ERR_INVALID_CHARS,
		value.data(), (int)value.length(), & result[0], length );
}

static bool
ConvertSimulatorControlWideToUtf8(
	const std::wstring& value, std::string& result )
{
	if ( value.empty() )
	{
		result.clear();
		return true;
	}
	int length = WideCharToMultiByte(
		CP_UTF8, WC_ERR_INVALID_CHARS,
		value.data(), (int)value.length(), NULL, 0, NULL, NULL );
	if ( length <= 0 )
	{
		return false;
	}
	result.resize( (size_t)length );
	return length == WideCharToMultiByte(
		CP_UTF8, WC_ERR_INVALID_CHARS,
		value.data(), (int)value.length(), & result[0], length, NULL, NULL );
}

static void
SetSimulatorControlErrnoFromWindowsError( DWORD error )
{
	switch ( error )
	{
		case ERROR_FILE_EXISTS:
		case ERROR_ALREADY_EXISTS:
			errno = EEXIST;
			break;
		case ERROR_FILE_NOT_FOUND:
		case ERROR_PATH_NOT_FOUND:
			errno = ENOENT;
			break;
		case ERROR_ACCESS_DENIED:
			errno = EACCES;
			break;
		default:
			errno = EIO;
			break;
	}
}

#endif

static int
DeleteSimulatorControlFile( const std::string& path )
{
#ifdef Rtt_WIN_ENV
	std::wstring widePath;
	if ( ! ConvertSimulatorControlUtf8ToWide( path, widePath ) )
	{
		errno = EINVAL;
		return -1;
	}
	if ( DeleteFileW( widePath.c_str() ) )
	{
		return 0;
	}
	SetSimulatorControlErrnoFromWindowsError( GetLastError() );
	return -1;
#else
	return remove( path.c_str() );
#endif
}

static int
MoveSimulatorControlFile(
	const std::string& sourcePath, const std::string& destinationPath,
	bool replaceExisting )
{
#ifdef Rtt_WIN_ENV
	std::wstring wideSourcePath;
	std::wstring wideDestinationPath;
	if ( ! ConvertSimulatorControlUtf8ToWide(
			sourcePath, wideSourcePath ) ||
		! ConvertSimulatorControlUtf8ToWide(
			destinationPath, wideDestinationPath ) )
	{
		errno = EINVAL;
		return -1;
	}
	DWORD flags = replaceExisting ? MOVEFILE_REPLACE_EXISTING : 0;
	if ( MoveFileExW(
		wideSourcePath.c_str(), wideDestinationPath.c_str(), flags ) )
	{
		return 0;
	}
	SetSimulatorControlErrnoFromWindowsError( GetLastError() );
	return -1;
#else
	if ( ! replaceExisting )
	{
		struct stat information;
		if ( 0 == stat( destinationPath.c_str(), & information ) )
		{
			errno = EEXIST;
			return -1;
		}
	}
	return rename( sourcePath.c_str(), destinationPath.c_str() );
#endif
}

static bool
ReadSimulatorControlFile(
	const std::string& path, std::string& result,
	size_t maximumSize = kSimulatorControlMaximumRequestSize )
{
#ifdef Rtt_WIN_ENV
	std::wstring widePath;
	if ( ! ConvertSimulatorControlUtf8ToWide( path, widePath ) )
	{
		return false;
	}
	HANDLE fileHandle = CreateFileW(
		widePath.c_str(), GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( INVALID_HANDLE_VALUE == fileHandle )
	{
		return false;
	}

	LARGE_INTEGER length;
	if ( ! GetFileSizeEx( fileHandle, & length ) ||
		length.QuadPart < 0 ||
		(unsigned long long)length.QuadPart >
			maximumSize )
	{
		CloseHandle( fileHandle );
		return false;
	}

	result.resize( (size_t)length.QuadPart );
	DWORD readLength = 0;
	bool succeeded = result.empty() ||
		( ReadFile(
			fileHandle, & result[0], (DWORD)result.length(),
			& readLength, NULL ) &&
			readLength == result.length() );
	CloseHandle( fileHandle );
	if ( ! succeeded )
	{
		result.clear();
	}
	return succeeded;
#else
	std::ifstream stream( path.c_str(), std::ios::in | std::ios::binary );
	if ( ! stream )
	{
		return false;
	}

	stream.seekg( 0, std::ios::end );
	std::streamoff length = stream.tellg();
	if ( length < 0 || (size_t)length > maximumSize )
	{
		return false;
	}
	stream.seekg( 0, std::ios::beg );

	result.assign( (std::istreambuf_iterator< char >( stream )), std::istreambuf_iterator< char >() );
	return stream.good() || stream.eof();
#endif
}

static bool
WriteSimulatorControlFileAtomically( const std::string& path, const std::string& contents )
{
	std::string temporaryPath = path + ".tmp";
#ifdef Rtt_WIN_ENV
	std::wstring wideTemporaryPath;
	std::wstring widePath;
	if ( ! ConvertSimulatorControlUtf8ToWide(
			temporaryPath, wideTemporaryPath ) ||
		! ConvertSimulatorControlUtf8ToWide( path, widePath ) )
	{
		return false;
	}
	HANDLE fileHandle = CreateFileW(
		wideTemporaryPath.c_str(), GENERIC_WRITE, 0, NULL,
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( INVALID_HANDLE_VALUE == fileHandle )
	{
		return false;
	}
	DWORD writtenLength = 0;
	bool succeeded = contents.empty() ||
		( WriteFile(
			fileHandle, contents.data(), (DWORD)contents.length(),
			& writtenLength, NULL ) &&
			writtenLength == contents.length() );
	CloseHandle( fileHandle );
	if ( ! succeeded ||
		! MoveFileExW(
			wideTemporaryPath.c_str(), widePath.c_str(),
			MOVEFILE_REPLACE_EXISTING ) )
	{
		DeleteFileW( wideTemporaryPath.c_str() );
		return false;
	}
	return true;
#else
	{
		std::ofstream stream( temporaryPath.c_str(), std::ios::out | std::ios::binary | std::ios::trunc );
		if ( ! stream )
		{
			return false;
		}
		stream.write( contents.data(), (std::streamsize)contents.length() );
			if ( ! stream )
			{
				stream.close();
				DeleteSimulatorControlFile( temporaryPath );
				return false;
			}
		}

	if ( 0 != MoveSimulatorControlFile(
		temporaryPath, path, true ) )
	{
		DeleteSimulatorControlFile( temporaryPath );
		return false;
	}
	return true;
#endif
}

static unsigned long long
GetSimulatorControlMilliseconds()
{
#ifdef Rtt_WIN_ENV
	return (unsigned long long)GetTickCount64();
#else
	struct timeval value;
	gettimeofday( & value, NULL );
	return ( (unsigned long long)value.tv_sec * 1000ULL ) +
		( (unsigned long long)value.tv_usec / 1000ULL );
#endif
}

static void
SleepSimulatorControlMilliseconds( unsigned long milliseconds )
{
#ifdef Rtt_WIN_ENV
	Sleep( (DWORD)milliseconds );
#else
	usleep( (useconds_t)( milliseconds * 1000UL ) );
#endif
}

static bool
IsSimulatorControlProcessAlive( unsigned long processId )
{
	if ( 0 == processId )
	{
		return false;
	}
	if ( processId == (unsigned long)GetSimulatorControlProcessId() )
	{
		return true;
	}

#ifdef Rtt_WIN_ENV
	HANDLE processHandle = OpenProcess(
		SYNCHRONIZE, FALSE, (DWORD)processId );
	if ( processHandle )
	{
		DWORD waitResult = WaitForSingleObject( processHandle, 0 );
		CloseHandle( processHandle );
		return WAIT_TIMEOUT == waitResult;
	}

	// If Windows refuses access, assume the process is alive. It is safer to
	// leave a lock or session in place than to steal one from a live process.
	return ERROR_ACCESS_DENIED == GetLastError();
#else
	if ( 0 == kill( (pid_t)processId, 0 ) )
	{
		return true;
	}
	return EPERM == errno;
#endif
}

static std::string
CreateSimulatorControlSessionId( unsigned long generation )
{
	static unsigned long counter = 0;
	char value[128];
	snprintf(
		value, sizeof( value ), "%08lx-%08lx-%016llx-%08lx",
		(unsigned long)GetSimulatorControlProcessId(),
		(unsigned long)time( NULL ),
		GetSimulatorControlMilliseconds(),
		generation + ++counter );
	return std::string( value );
}

static SimulatorControlRuntimeState*
GetOrCreateSimulatorControlRuntimeState( Runtime& runtime )
{
	const std::string& directory = GetSimulatorControlDirectory();
	if ( directory.empty() )
	{
		return NULL;
	}

	SimulatorControlRuntimeStateMap& states =
		GetSimulatorControlRuntimeStates();
	SimulatorControlRuntimeStateMap::iterator iterator =
		states.find( & runtime );
	if ( states.end() == iterator )
	{
		SimulatorControlRuntimeState state;
		state.directory = directory;
		state.generation = NextSimulatorControlGeneration();
		state.sessionId =
			CreateSimulatorControlSessionId( state.generation );
		iterator = states.insert(
			std::make_pair( (const Runtime*)& runtime, state ) ).first;
	}
	return & iterator->second;
}

static void
AssignSimulatorControlDiagnosticText(
	std::string& result, const char *value,
	size_t maximumLength, bool& truncated )
{
	const char *safeValue = value ? value : "";
	size_t length = strlen( safeValue );
	if ( length > maximumLength )
	{
		length = maximumLength;
		truncated = true;
	}
	result.assign( safeValue, length );
}

static void
RecordSimulatorControlDiagnostic(
	Runtime& runtime, SimulatorControlRuntimeState& state,
	const char *errorType, const char *message, const char *stackTrace )
{
	state.diagnosticSequence++;
	state.diagnosticFrame = (unsigned long)runtime.GetFrame();
	state.diagnosticTruncated = false;
	AssignSimulatorControlDiagnosticText(
		state.diagnosticType, errorType ? errorType : "Runtime error",
		256, state.diagnosticTruncated );
	AssignSimulatorControlDiagnosticText(
		state.diagnosticMessage, message,
		kSimulatorControlMaximumDiagnosticMessageSize,
		state.diagnosticTruncated );
	AssignSimulatorControlDiagnosticText(
		state.diagnosticStackTrace, stackTrace,
		kSimulatorControlMaximumDiagnosticStackTraceSize,
		state.diagnosticTruncated );
	state.hasDiagnostic = true;
	state.diagnosticsWritten = false;
}

static void
RecordSimulatorControlExecutionDiagnostic(
	Runtime& runtime, SimulatorControlRuntimeState& state,
	const std::string& error )
{
	size_t stackTraceOffset = error.find( "\nstack traceback:" );
	if ( std::string::npos == stackTraceOffset )
	{
		RecordSimulatorControlDiagnostic(
			runtime, state, "Simulator control error",
			error.c_str(), "" );
		return;
	}

	std::string message = error.substr( 0, stackTraceOffset );
	std::string stackTrace = error.substr( stackTraceOffset );
	RecordSimulatorControlDiagnostic(
		runtime, state, "Simulator control error",
		message.c_str(), stackTrace.c_str() );
}

static bool
SimulatorControlFileExists( const std::string& path )
{
#ifdef Rtt_WIN_ENV
	std::wstring widePath;
	if ( ! ConvertSimulatorControlUtf8ToWide( path, widePath ) )
	{
		return false;
	}
	return INVALID_FILE_ATTRIBUTES !=
		GetFileAttributesW( widePath.c_str() );
#else
	struct stat information;
	return 0 == stat( path.c_str(), & information );
#endif
}

static bool
SimulatorControlDirectoryExists( const std::string& path )
{
#ifdef Rtt_WIN_ENV
	std::wstring widePath;
	if ( ! ConvertSimulatorControlUtf8ToWide( path, widePath ) )
	{
		return false;
	}
	DWORD attributes = GetFileAttributesW( widePath.c_str() );
	return INVALID_FILE_ATTRIBUTES != attributes &&
		0 != ( attributes & FILE_ATTRIBUTE_DIRECTORY );
#else
	struct stat information;
	return 0 == stat( path.c_str(), & information ) &&
		S_ISDIR( information.st_mode );
#endif
}

static bool
IsSimulatorControlFileStale( const std::string& path, unsigned long seconds )
{
#ifdef Rtt_WIN_ENV
	std::wstring widePath;
	if ( ! ConvertSimulatorControlUtf8ToWide( path, widePath ) )
	{
		return false;
	}
	struct _stat64 information;
	if ( 0 != _wstat64( widePath.c_str(), & information ) )
#else
	struct stat information;
	if ( 0 != stat( path.c_str(), & information ) )
#endif
	{
		return false;
	}
	return time( NULL ) - information.st_mtime > (time_t)seconds;
}

static bool
CreateSimulatorControlExclusivePidFile( const std::string& path )
{
#ifdef Rtt_WIN_ENV
	std::wstring widePath;
	if ( ! ConvertSimulatorControlUtf8ToWide( path, widePath ) )
	{
		errno = EINVAL;
		return false;
	}
	HANDLE fileHandle = CreateFileW(
		widePath.c_str(), GENERIC_WRITE, 0, NULL,
		CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( INVALID_HANDLE_VALUE == fileHandle )
	{
		SetSimulatorControlErrnoFromWindowsError( GetLastError() );
		return false;
	}
#else
	int descriptor = open( path.c_str(), O_CREAT | O_EXCL | O_WRONLY, 0600 );
	if ( descriptor < 0 )
	{
		return false;
	}
#endif

	char contents[64];
	int length = snprintf(
		contents, sizeof( contents ), "%ld\n%lu\n",
		GetSimulatorControlProcessId(), (unsigned long)time( NULL ) );
#ifdef Rtt_WIN_ENV
	DWORD writtenLength = 0;
	BOOL didWrite = WriteFile(
		fileHandle, contents, (DWORD)length, & writtenLength, NULL );
	bool succeeded = didWrite && writtenLength == (DWORD)length;
	DWORD writeError = didWrite ? ERROR_WRITE_FAULT : GetLastError();
	CloseHandle( fileHandle );
	if ( ! succeeded )
	{
		SetSimulatorControlErrnoFromWindowsError( writeError );
	}
	int written = succeeded ? length : -1;
#else
	ssize_t written = write( descriptor, contents, (size_t)length );
	close( descriptor );
#endif
	if ( written != length )
	{
		int writeError = errno ? errno : EIO;
		DeleteSimulatorControlFile( path );
		errno = writeError;
		return false;
	}
	return true;
}

static bool
ReadSimulatorControlFileOwner(
	const std::string& path, unsigned long& processId )
{
	std::string contents;
	if ( ! ReadSimulatorControlFile( path, contents ) )
	{
		return false;
	}

	char *end = NULL;
	errno = 0;
	processId = strtoul( contents.c_str(), & end, 10 );
	return 0 != processId && ERANGE != errno && end &&
		end != contents.c_str() && ( '\n' == end[0] || '\r' == end[0] );
}

static void
ReleaseSimulatorControlDirectory()
{
	std::string& ownerPath = GetSimulatorControlOwnerPath();
	unsigned long ownerProcessId = 0;
	if ( ! ownerPath.empty() &&
		ReadSimulatorControlFileOwner( ownerPath, ownerProcessId ) &&
		ownerProcessId == (unsigned long)GetSimulatorControlProcessId() )
	{
		DeleteSimulatorControlFile( ownerPath );
	}
	ownerPath.clear();
}

static bool
MoveSimulatorControlFileAside( const std::string& path )
{
	static unsigned long counter = 0;
	char suffix[96];
	snprintf(
		suffix, sizeof( suffix ), ".stale.%ld.%llu.%lu",
		GetSimulatorControlProcessId(),
		GetSimulatorControlMilliseconds(), ++counter );
	std::string stalePath = path + suffix;
	if ( 0 != MoveSimulatorControlFile(
		path, stalePath, false ) )
	{
		return false;
	}
	DeleteSimulatorControlFile( stalePath );
	return true;
}

static bool
AcquireSimulatorControlLock(
	const std::string& directory, unsigned long timeoutMilliseconds,
	std::string& lockPath, std::string& error )
{
	lockPath = SimulatorControlPath( directory, ".simulator-control.lock" );
	unsigned long long deadline =
		GetSimulatorControlMilliseconds() + timeoutMilliseconds;
	while ( GetSimulatorControlMilliseconds() < deadline )
	{
		if ( CreateSimulatorControlExclusivePidFile( lockPath ) )
		{
			return true;
		}
		if ( EEXIST != errno )
		{
			error = "unable to create the Simulator control lock";
			return false;
		}

		unsigned long ownerProcessId = 0;
		if ( ReadSimulatorControlFileOwner( lockPath, ownerProcessId ) )
		{
			if ( ! IsSimulatorControlProcessAlive( ownerProcessId ) )
			{
				if ( MoveSimulatorControlFileAside( lockPath ) )
				{
					continue;
				}
			}
		}
		else if ( IsSimulatorControlFileStale( lockPath, 30 ) )
		{
			// Preserve compatibility with malformed or pre-owner lock files,
			// but never steal a valid lock from a process that is still alive.
			if ( MoveSimulatorControlFileAside( lockPath ) )
			{
				continue;
			}
		}
		SleepSimulatorControlMilliseconds( 20 );
	}
	error = "another Simulator control request held the lock for too long";
	return false;
}

static bool
MakeSimulatorControlAbsolutePath(
	const std::string& path, std::string& result )
{
#ifdef Rtt_WIN_ENV
	std::wstring widePath;
	if ( ! ConvertSimulatorControlUtf8ToWide( path, widePath ) )
	{
		return false;
	}
	DWORD requiredLength =
		GetFullPathNameW( widePath.c_str(), 0, NULL, NULL );
	if ( 0 == requiredLength )
	{
		return false;
	}
	std::wstring absolutePath( requiredLength, L'\0' );
	DWORD writtenLength = GetFullPathNameW(
		widePath.c_str(), requiredLength, & absolutePath[0], NULL );
	if ( 0 == writtenLength || writtenLength >= requiredLength )
	{
		return false;
	}
	absolutePath.resize( writtenLength );
	return ConvertSimulatorControlWideToUtf8( absolutePath, result );
#else
	char *absolutePath = realpath( path.c_str(), NULL );
	if ( ! absolutePath )
	{
		return false;
	}
	result.assign( absolutePath );
	free( absolutePath );
	return true;
#endif
}

static bool
MakeSimulatorControlAbsoluteOutputPath(
	const std::string& path, std::string& result )
{
	if ( path.empty() )
	{
		return false;
	}
#ifdef Rtt_WIN_ENV
	return MakeSimulatorControlAbsolutePath( path, result );
#else
	if ( '/' == path[0] )
	{
		result = path;
		return true;
	}
	char workingDirectory[PATH_MAX];
	if ( ! getcwd( workingDirectory, sizeof( workingDirectory ) ) )
	{
		return false;
	}
	result.assign( workingDirectory );
	result.push_back( '/' );
	result.append( path );
	return true;
#endif
}

static void
AppendSimulatorControlJsonString( std::string& result, const char *value, size_t length )
{
	static const char kHexadecimalDigits[] = "0123456789abcdef";
	result.push_back( '"' );
	for ( size_t index = 0; index < length; index++ )
	{
		unsigned char character = (unsigned char)value[index];
		switch ( character )
		{
			case '"': result.append( "\\\"" ); break;
			case '\\': result.append( "\\\\" ); break;
			case '\b': result.append( "\\b" ); break;
			case '\f': result.append( "\\f" ); break;
			case '\n': result.append( "\\n" ); break;
			case '\r': result.append( "\\r" ); break;
			case '\t': result.append( "\\t" ); break;
			default:
				if ( character < 0x20 )
				{
					result.append( "\\u00" );
					result.push_back( kHexadecimalDigits[( character >> 4 ) & 0x0f] );
					result.push_back( kHexadecimalDigits[character & 0x0f] );
				}
				else
				{
					result.push_back( (char)character );
				}
				break;
		}
	}
	result.push_back( '"' );
}

static void
AppendSimulatorControlJsonString( std::string& result, const std::string& value )
{
	AppendSimulatorControlJsonString( result, value.data(), value.length() );
}

static std::string
SimulatorControlErrorResponse( const std::string& message )
{
	std::string result( "{\"ok\":false,\"error\":{\"message\":" );
	AppendSimulatorControlJsonString( result, message );
	result.append( "}}" );
	return result;
}

static std::string
SimulatorControlSuccessResponse( const std::string& jsonResult )
{
	return std::string( "{\"ok\":true,\"result\":" ) + jsonResult + "}";
}

static std::string
BuildSimulatorControlDiagnostics(
	const SimulatorControlRuntimeState& state )
{
	std::string result( "{\"sessionId\":" );
	AppendSimulatorControlJsonString( result, state.sessionId );
	char identity[128];
	snprintf(
		identity, sizeof( identity ),
		",\"generation\":%lu,\"latestRuntimeError\":",
		state.generation );
	result.append( identity );
	if ( ! state.hasDiagnostic )
	{
		result.append( "null}" );
		return result;
	}

	char metadata[128];
	snprintf(
		metadata, sizeof( metadata ),
		"{\"sequence\":%lu,\"frame\":%lu,\"type\":",
		state.diagnosticSequence, state.diagnosticFrame );
	result.append( metadata );
	AppendSimulatorControlJsonString( result, state.diagnosticType );
	result.append( ",\"message\":" );
	AppendSimulatorControlJsonString( result, state.diagnosticMessage );
	result.append( ",\"stackTrace\":" );
	AppendSimulatorControlJsonString( result, state.diagnosticStackTrace );
	result.append(
		state.diagnosticTruncated ?
			",\"truncated\":true}}" : ",\"truncated\":false}}" );
	return result;
}

static std::string
BuildSimulatorControlLogs(
	const SimulatorControlRuntimeState& runtimeState,
	unsigned long sinceSequence )
{
	std::deque< SimulatorControlLogEntry > entries;
	unsigned long latestSequence = 0;
	bool droppedEntries = false;
	{
		SimulatorControlLogGuard guard( GetSimulatorControlLogMutex() );
		SimulatorControlLogState& logState = GetSimulatorControlLogState();
		entries = logState.entries;
		latestSequence = logState.sequence;
		droppedEntries = logState.droppedEntries;
	}

	unsigned long oldestSequence =
		entries.empty() ? 0 : entries.front().sequence;
	bool truncated = droppedEntries && oldestSequence > 0 &&
		sinceSequence < oldestSequence - 1;
	std::string result( "{\"sessionId\":" );
	AppendSimulatorControlJsonString( result, runtimeState.sessionId );
	char information[192];
	snprintf(
		information, sizeof( information ),
		",\"generation\":%lu,\"oldestSequence\":%lu,"
		"\"latestSequence\":%lu,\"truncated\":%s,\"entries\":[",
		runtimeState.generation, oldestSequence, latestSequence,
		truncated ? "true" : "false" );
	result.append( information );

	bool first = true;
	bool hasMore = false;
	for ( std::deque< SimulatorControlLogEntry >::const_iterator iterator =
			entries.begin(); entries.end() != iterator; iterator++ )
	{
		if ( iterator->sequence <= sinceSequence )
		{
			continue;
		}

		std::string serialized;
		char sequence[64];
		snprintf(
			sequence, sizeof( sequence ),
			"{\"sequence\":%lu,\"message\":", iterator->sequence );
		serialized.append( sequence );
		AppendSimulatorControlJsonString( serialized, iterator->message );
		serialized.push_back( '}' );
		if ( result.length() + serialized.length() + 40 >
			kSimulatorControlMaximumResponseSize )
		{
			hasMore = true;
			break;
		}
		if ( ! first )
		{
			result.push_back( ',' );
		}
		result.append( serialized );
		first = false;
	}
	result.append( "],\"hasMore\":" );
	result.append( hasMore ? "true}" : "false}" );
	return result;
}

static bool
CaptureSimulatorControlScreenshot(
	Runtime& runtime, const std::string& path,
	std::string& result, std::string& error )
{
	BitmapPaint *paint =
		runtime.GetDisplay().CaptureSave( NULL, false, NULL );
	if ( ! paint )
	{
		error = "the Simulator could not capture the current screen";
		return false;
	}

	PlatformBitmap *bitmap = paint->GetBitmap();
	if ( ! bitmap )
	{
		Rtt_DELETE( paint );
		error = "the Simulator screenshot did not contain a bitmap";
		return false;
	}
	int width = bitmap->Width();
	int height = bitmap->Height();
	Data< const char > pngBytes( runtime.GetAllocator() );
	runtime.Platform().SaveBitmap( bitmap, pngBytes );
	Rtt_DELETE( paint );
	if ( ! pngBytes.GetData() || pngBytes.GetLength() <= 0 )
	{
		error = "the Simulator could not encode the screenshot as PNG";
		return false;
	}

	std::string contents(
		pngBytes.GetData(), (size_t)pngBytes.GetLength() );
	if ( ! WriteSimulatorControlFileAtomically( path, contents ) )
	{
		error = "the Simulator could not write the screenshot";
		return false;
	}

	result.assign( "{\"path\":" );
	AppendSimulatorControlJsonString( result, path );
	char information[128];
	snprintf(
		information, sizeof( information ),
		",\"width\":%d,\"height\":%d,\"bytes\":%ld}",
		width, height, (long)pngBytes.GetLength() );
	result.append( information );
	return true;
}

static bool
WriteSimulatorControlSession(
	SimulatorControlRuntimeState& state )
{
	if ( state.sessionWritten )
	{
		return true;
	}

	char session[512];
	snprintf(
		session, sizeof( session ),
		"{\"protocol\":2,\"transport\":\"filesystem\",\"sessionId\":\"%s\","
		"\"generation\":%lu,\"pid\":%ld,\"requestFile\":\"request\"}",
		state.sessionId.c_str(), state.generation,
		GetSimulatorControlProcessId() );
	state.sessionWritten = WriteSimulatorControlFileAtomically(
		SimulatorControlPath( state.directory, "session.json" ), session );
	return state.sessionWritten;
}

static bool
WriteSimulatorControlDiagnostics(
	SimulatorControlRuntimeState& state )
{
	state.diagnosticsWritten = WriteSimulatorControlFileAtomically(
		SimulatorControlPath( state.directory, "diagnostics.json" ),
		SimulatorControlSuccessResponse(
			BuildSimulatorControlDiagnostics( state ) ) );
	return state.diagnosticsWritten;
}

static void
PushSimulatorControlRegistry( lua_State *L )
{
	lua_pushlightuserdata( L, & kSimulatorControlRegistryKey );
	lua_rawget( L, LUA_REGISTRYINDEX );
	if ( lua_istable( L, -1 ) )
	{
		return;
	}
	lua_pop( L, 1 );

	lua_newtable( L );
	int registryIndex = lua_gettop( L );

	lua_newtable( L );
	lua_newtable( L );
	lua_pushliteral( L, "v" );
	lua_setfield( L, -2, "__mode" );
	lua_setmetatable( L, -2 );
	lua_setfield( L, registryIndex, "handles" );

	lua_newtable( L );
	lua_newtable( L );
	lua_pushliteral( L, "k" );
	lua_setfield( L, -2, "__mode" );
	lua_setmetatable( L, -2 );
	lua_setfield( L, registryIndex, "reverse" );

	lua_pushinteger( L, 1 );
	lua_setfield( L, registryIndex, "nextHandle" );

	lua_pushlightuserdata( L, & kSimulatorControlRegistryKey );
	lua_pushvalue( L, registryIndex );
	lua_rawset( L, LUA_REGISTRYINDEX );
}

static int
RegisterSimulatorControlHandle( lua_State *L, int valueIndex )
{
	valueIndex = AbsoluteLuaIndex( L, valueIndex );
	PushSimulatorControlRegistry( L );
	int registryIndex = lua_gettop( L );

	lua_getfield( L, registryIndex, "reverse" );
	int reverseIndex = lua_gettop( L );
	lua_pushvalue( L, valueIndex );
	lua_rawget( L, reverseIndex );
	if ( lua_type( L, -1 ) == LUA_TNUMBER )
	{
		int result = (int)lua_tointeger( L, -1 );
		lua_pop( L, 3 );
		return result;
	}
	lua_pop( L, 1 );

	lua_getfield( L, registryIndex, "nextHandle" );
	int result = (int)lua_tointeger( L, -1 );
	lua_pop( L, 1 );
	lua_pushinteger( L, result + 1 );
	lua_setfield( L, registryIndex, "nextHandle" );

	lua_pushvalue( L, valueIndex );
	lua_pushinteger( L, result );
	lua_rawset( L, reverseIndex );

	lua_getfield( L, registryIndex, "handles" );
	lua_pushvalue( L, valueIndex );
	lua_rawseti( L, -2, result );
	lua_pop( L, 3 );
	return result;
}

static bool
PushSimulatorControlHandle( lua_State *L, int handle )
{
	PushSimulatorControlRegistry( L );
	lua_getfield( L, -1, "handles" );
	lua_rawgeti( L, -1, handle );
	lua_remove( L, -2 );
	lua_remove( L, -2 );
	if ( lua_isnil( L, -1 ) )
	{
		lua_pop( L, 1 );
		return false;
	}
	return true;
}

static int
CountSimulatorControlTableEntries( lua_State *L, int tableIndex )
{
	tableIndex = AbsoluteLuaIndex( L, tableIndex );
	int count = 0;
	lua_pushnil( L );
	while ( lua_next( L, tableIndex ) )
	{
		count++;
		lua_pop( L, 1 );
	}
	return count;
}

static void
AppendSimulatorControlNumber( std::string& result, lua_Number value )
{
	char buffer[64];
	snprintf( buffer, sizeof( buffer ), "%.17g", (double)value );
	result.append( buffer );
}

static void
AppendSimulatorControlValueSummary( lua_State *L, int valueIndex, std::string& result )
{
	valueIndex = AbsoluteLuaIndex( L, valueIndex );
	switch ( lua_type( L, valueIndex ) )
	{
		case LUA_TNIL:
			result.append( "{\"type\":\"nil\",\"value\":null}" );
			break;
		case LUA_TBOOLEAN:
			result.append( lua_toboolean( L, valueIndex ) ?
				"{\"type\":\"boolean\",\"value\":true}" :
				"{\"type\":\"boolean\",\"value\":false}" );
			break;
		case LUA_TNUMBER:
		{
			lua_Number value = lua_tonumber( L, valueIndex );
			result.append( "{\"type\":\"number\",\"value\":" );
			if ( IsFiniteNumber( value ) )
			{
				AppendSimulatorControlNumber( result, value );
			}
			else
			{
				result.append( "null,\"description\":" );
				const char *description = value != value ? "nan" : ( value > 0 ? "infinity" : "-infinity" );
				AppendSimulatorControlJsonString( result, description, strlen( description ) );
			}
			result.push_back( '}' );
			break;
		}
		case LUA_TSTRING:
		{
			size_t length = 0;
			const char *value = lua_tolstring( L, valueIndex, & length );
			size_t serializedLength =
				length > kSimulatorControlMaximumStringSize ? kSimulatorControlMaximumStringSize : length;
			result.append( "{\"type\":\"string\",\"value\":" );
			AppendSimulatorControlJsonString( result, value, serializedLength );
			result.append( ",\"length\":" );
			char buffer[32];
			snprintf( buffer, sizeof( buffer ), "%lu", (unsigned long)length );
			result.append( buffer );
			if ( serializedLength < length )
			{
				result.append( ",\"truncated\":true" );
			}
			result.push_back( '}' );
			break;
		}
		case LUA_TTABLE:
		{
			int handle = RegisterSimulatorControlHandle( L, valueIndex );
			int count = CountSimulatorControlTableEntries( L, valueIndex );
			char buffer[96];
			snprintf(
				buffer, sizeof( buffer ),
				"{\"type\":\"table\",\"handle\":%d,\"count\":%d}",
				handle, count );
			result.append( buffer );
			break;
		}
		case LUA_TFUNCTION:
		{
			result.append( "{\"type\":\"function\"" );
			lua_pushvalue( L, valueIndex );
			lua_Debug information;
			memset( & information, 0, sizeof( information ) );
			if ( lua_getinfo( L, ">S", & information ) )
			{
				if ( information.short_src[0] )
				{
					result.append( ",\"source\":" );
					AppendSimulatorControlJsonString(
						result, information.short_src, strlen( information.short_src ) );
				}
				if ( information.linedefined > 0 )
				{
					char buffer[48];
					snprintf( buffer, sizeof( buffer ), ",\"line\":%d", information.linedefined );
					result.append( buffer );
				}
				if ( information.what )
				{
					result.append( ",\"kind\":" );
					AppendSimulatorControlJsonString(
						result, information.what, strlen( information.what ) );
				}
			}
			result.push_back( '}' );
			break;
		}
		case LUA_TUSERDATA:
		case LUA_TLIGHTUSERDATA:
			result.append( lua_type( L, valueIndex ) == LUA_TUSERDATA ?
				"{\"type\":\"userdata\"" : "{\"type\":\"lightuserdata\"" );
			if ( lua_getmetatable( L, valueIndex ) )
			{
				lua_pushliteral( L, "__name" );
				lua_rawget( L, -2 );
				if ( lua_type( L, -1 ) == LUA_TSTRING )
				{
					size_t length = 0;
					const char *name = lua_tolstring( L, -1, & length );
					result.append( ",\"name\":" );
					AppendSimulatorControlJsonString( result, name, length );
				}
				lua_pop( L, 2 );
			}
			result.push_back( '}' );
			break;
		case LUA_TTHREAD:
			result.append( "{\"type\":\"thread\"}" );
			break;
		default:
			result.append( "{\"type\":\"unknown\"}" );
			break;
	}
}

static void
SkipSimulatorControlPathWhitespace( const std::string& path, size_t& offset )
{
	while ( offset < path.length() && isspace( (unsigned char)path[offset] ) )
	{
		offset++;
	}
}

static bool
ParseSimulatorControlIdentifier(
	const std::string& path, size_t& offset, std::string& result )
{
	size_t start = offset;
	if ( offset >= path.length() ||
		( '_' != path[offset] && ! isalpha( (unsigned char)path[offset] ) ) )
	{
		return false;
	}
	offset++;
	while ( offset < path.length() &&
		( '_' == path[offset] || isalnum( (unsigned char)path[offset] ) ) )
	{
		offset++;
	}
	result.assign( path, start, offset - start );
	return true;
}

static bool
ParseSimulatorControlQuotedString(
	const std::string& path, size_t& offset, std::string& result, std::string& error )
{
	if ( offset >= path.length() || ( '\'' != path[offset] && '"' != path[offset] ) )
	{
		return false;
	}

	char quote = path[offset++];
	result.clear();
	while ( offset < path.length() )
	{
		char character = path[offset++];
		if ( quote == character )
		{
			return true;
		}
		if ( '\\' == character )
		{
			if ( offset >= path.length() )
			{
				error = "inspect path ends inside an escape sequence";
				return false;
			}
			character = path[offset++];
			switch ( character )
			{
				case 'n': result.push_back( '\n' ); break;
				case 'r': result.push_back( '\r' ); break;
				case 't': result.push_back( '\t' ); break;
				case '\\': result.push_back( '\\' ); break;
				case '\'': result.push_back( '\'' ); break;
				case '"': result.push_back( '"' ); break;
				default:
					error = "inspect path contains an unsupported escape sequence";
					return false;
			}
		}
		else
		{
			result.push_back( character );
		}
	}

	error = "inspect path contains an unterminated string";
	return false;
}

static bool
PushSimulatorControlInspectedValue(
	lua_State *L, const std::string& path, std::string& error )
{
	size_t offset = 0;
	SkipSimulatorControlPathWhitespace( path, offset );
	if ( offset >= path.length() )
	{
		error = "inspect expects a global path or handle";
		return false;
	}

	if ( '@' == path[offset] )
	{
		offset++;
		size_t start = offset;
		while ( offset < path.length() && isdigit( (unsigned char)path[offset] ) )
		{
			offset++;
		}
		if ( start == offset )
		{
			error = "inspect handle must contain a number after '@'";
			return false;
		}
		int handle = atoi( path.substr( start, offset - start ).c_str() );
		if ( handle <= 0 || ! PushSimulatorControlHandle( L, handle ) )
		{
			error = "inspect handle has expired or does not exist";
			return false;
		}
	}
	else
	{
		std::string identifier;
		if ( ! ParseSimulatorControlIdentifier( path, offset, identifier ) )
		{
			error = "inspect path must start with a global identifier or handle";
			return false;
		}
		lua_pushvalue( L, LUA_GLOBALSINDEX );
		lua_pushlstring( L, identifier.data(), identifier.length() );
		lua_rawget( L, -2 );
		lua_remove( L, -2 );
	}

	for ( ;; )
	{
		SkipSimulatorControlPathWhitespace( path, offset );
		if ( offset >= path.length() )
		{
			return true;
		}

		if ( ! lua_istable( L, -1 ) )
		{
			lua_pop( L, 1 );
			error = "inspect path attempts to index a non-table value";
			return false;
		}

		if ( '.' == path[offset] )
		{
			offset++;
			std::string identifier;
			if ( ! ParseSimulatorControlIdentifier( path, offset, identifier ) )
			{
				lua_pop( L, 1 );
				error = "inspect path expects a field name after '.'";
				return false;
			}
			lua_pushlstring( L, identifier.data(), identifier.length() );
			lua_rawget( L, -2 );
			lua_remove( L, -2 );
		}
		else if ( '[' == path[offset] )
		{
			offset++;
			SkipSimulatorControlPathWhitespace( path, offset );
			if ( offset >= path.length() )
			{
				lua_pop( L, 1 );
				error = "inspect path contains an unterminated '['";
				return false;
			}

			if ( '\'' == path[offset] || '"' == path[offset] )
			{
				std::string key;
				if ( ! ParseSimulatorControlQuotedString( path, offset, key, error ) )
				{
					lua_pop( L, 1 );
					return false;
				}
				lua_pushlstring( L, key.data(), key.length() );
			}
			else
			{
				bool negative = false;
				if ( '-' == path[offset] )
				{
					negative = true;
					offset++;
				}
				size_t start = offset;
				while ( offset < path.length() && isdigit( (unsigned char)path[offset] ) )
				{
					offset++;
				}
				if ( start == offset )
				{
					lua_pop( L, 1 );
					error = "inspect brackets support only integer or quoted-string keys";
					return false;
				}
				lua_Integer value = (lua_Integer)atol( path.substr( start, offset - start ).c_str() );
				lua_pushinteger( L, negative ? -value : value );
			}

			SkipSimulatorControlPathWhitespace( path, offset );
			if ( offset >= path.length() || ']' != path[offset] )
			{
				lua_pop( L, 2 );
				error = "inspect path expects a closing ']'";
				return false;
			}
			offset++;
			lua_rawget( L, -2 );
			lua_remove( L, -2 );
		}
		else
		{
			lua_pop( L, 1 );
			error = "inspect path supports only '.', integer keys, and quoted-string keys";
			return false;
		}
	}
}

static std::string
BuildSimulatorControlInspection( lua_State *L, int valueIndex, int cursor )
{
	valueIndex = AbsoluteLuaIndex( L, valueIndex );
	if ( ! lua_istable( L, valueIndex ) )
	{
		std::string result;
		AppendSimulatorControlValueSummary( L, valueIndex, result );
		return result;
	}

	int handle = RegisterSimulatorControlHandle( L, valueIndex );
	std::string entries;
	int count = 0;
	int emitted = 0;
	lua_pushnil( L );
	while ( lua_next( L, valueIndex ) )
	{
		if ( count >= cursor && emitted < kSimulatorControlMaximumEntries &&
			entries.length() < kSimulatorControlMaximumResponseSize )
		{
			if ( emitted > 0 )
			{
				entries.push_back( ',' );
			}
			entries.append( "{\"key\":" );
			AppendSimulatorControlValueSummary( L, -2, entries );
			entries.append( ",\"value\":" );
			AppendSimulatorControlValueSummary( L, -1, entries );
			entries.push_back( '}' );
			emitted++;
		}
		count++;
		lua_pop( L, 1 );
	}

	char prefix[128];
	snprintf(
		prefix, sizeof( prefix ),
		"{\"type\":\"table\",\"handle\":%d,\"count\":%d,\"cursor\":%d,\"entries\":[",
		handle, count, cursor );
	std::string result( prefix );
	result.append( entries );
	result.push_back( ']' );
	if ( cursor + emitted < count )
	{
		char suffix[96];
		snprintf(
			suffix, sizeof( suffix ),
			",\"truncated\":true,\"nextCursor\":%d",
			cursor + emitted );
		result.append( suffix );
	}
	else
	{
		result.append( ",\"truncated\":false" );
	}
	result.push_back( '}' );
	return result;
}

static int
SimulatorControlTraceback( lua_State *L )
{
	const char *message = lua_tostring( L, 1 );
	if ( ! message )
	{
		message = "(error object is not a string)";
	}

	lua_getfield( L, LUA_GLOBALSINDEX, "debug" );
	if ( ! lua_istable( L, -1 ) )
	{
		lua_pop( L, 1 );
		lua_pushstring( L, message );
		return 1;
	}
	lua_getfield( L, -1, "traceback" );
	if ( ! lua_isfunction( L, -1 ) )
	{
		lua_pop( L, 2 );
		lua_pushstring( L, message );
		return 1;
	}
	lua_pushstring( L, message );
	lua_pushinteger( L, 2 );
	lua_call( L, 2, 1 );
	return 1;
}

static bool
ExecuteSimulatorControlChunk(
	lua_State *L, const std::string& payload, const char *chunkName,
	bool isExpression, bool isFile, std::string& jsonResult, std::string& error )
{
	int originalTop = lua_gettop( L );
	int status = 0;
	if ( isFile )
	{
		if ( payload.find( '\0' ) != std::string::npos )
		{
			error = "exec-file path cannot contain a null byte";
			return false;
		}
		std::string code;
		if ( ! ReadSimulatorControlFile(
			payload, code, kSimulatorControlMaximumExecFileSize ) )
		{
			error =
				"unable to read exec-file or file exceeded sixteen megabytes";
			return false;
		}
		std::string fileChunkName = "@" + payload;
		status = luaL_loadbuffer(
			L, code.data(), code.length(), fileChunkName.c_str() );
	}
	else
	{
		std::string code = isExpression ? std::string( "return " ) + payload : payload;
		status = luaL_loadbuffer( L, code.data(), code.length(), chunkName );
	}

	if ( 0 != status )
	{
		const char *message = lua_tostring( L, -1 );
		error = message ? message : "unable to load Lua code";
		lua_settop( L, originalTop );
		return false;
	}

	int functionIndex = originalTop + 1;
	lua_pushcfunction( L, SimulatorControlTraceback );
	lua_insert( L, functionIndex );
	status = lua_pcall( L, 0, LUA_MULTRET, functionIndex );
	lua_remove( L, functionIndex );
	if ( 0 != status )
	{
		const char *message = lua_tostring( L, -1 );
		error = message ? message : "Lua execution failed";
		lua_settop( L, originalTop );
		return false;
	}

	jsonResult.assign( "[" );
	int resultCount = lua_gettop( L ) - originalTop;
	for ( int index = 0; index < resultCount; index++ )
	{
		if ( index > 0 )
		{
			jsonResult.push_back( ',' );
		}
		AppendSimulatorControlValueSummary( L, originalTop + index + 1, jsonResult );
	}
	jsonResult.push_back( ']' );
	lua_settop( L, originalTop );
	return true;
}

static void
StripSimulatorControlCarriageReturn( std::string& value )
{
	if ( ! value.empty() && '\r' == value[value.length() - 1] )
	{
		value.erase( value.length() - 1 );
	}
}

static bool
ParseSimulatorControlRequest(
	const std::string& contents, std::string& identifier,
	std::string& sessionId, std::string& command,
	std::string& payload, std::string& error )
{
	size_t firstLineEnd = contents.find( '\n' );
	size_t secondLineEnd =
		std::string::npos == firstLineEnd ? std::string::npos : contents.find( '\n', firstLineEnd + 1 );
	size_t thirdLineEnd =
		std::string::npos == secondLineEnd ? std::string::npos : contents.find( '\n', secondLineEnd + 1 );
	if ( std::string::npos == firstLineEnd ||
		std::string::npos == secondLineEnd ||
		std::string::npos == thirdLineEnd )
	{
		error =
			"control request must contain identifier, session, and command lines";
		return false;
	}

	identifier.assign( contents, 0, firstLineEnd );
	sessionId.assign(
		contents, firstLineEnd + 1, secondLineEnd - firstLineEnd - 1 );
	command.assign(
		contents, secondLineEnd + 1, thirdLineEnd - secondLineEnd - 1 );
	payload.assign(
		contents, thirdLineEnd + 1, contents.length() - thirdLineEnd - 1 );
	StripSimulatorControlCarriageReturn( identifier );
	StripSimulatorControlCarriageReturn( sessionId );
	StripSimulatorControlCarriageReturn( command );

	if ( identifier.empty() || identifier.length() > 128 )
	{
		error = "control request identifier must contain 1 through 128 characters";
		identifier.clear();
		return false;
	}
	for ( size_t index = 0; index < identifier.length(); index++ )
	{
		char character = identifier[index];
		if ( '-' != character && '_' != character &&
			! isalnum( (unsigned char)character ) )
		{
			error = "control request identifier contains an unsupported character";
			identifier.clear();
			return false;
		}
	}
	if ( sessionId.empty() || sessionId.length() > 127 )
	{
		error = "control request session ID must contain 1 through 127 characters";
		return false;
	}
	for ( size_t index = 0; index < sessionId.length(); index++ )
	{
		char character = sessionId[index];
		if ( '-' != character && '_' != character &&
			! isalnum( (unsigned char)character ) )
		{
			error = "control request session ID contains an unsupported character";
			return false;
		}
	}
	if ( command.empty() || command.length() > 32 )
	{
		error = "control request command must contain 1 through 32 characters";
		return false;
	}
	return true;
}

static bool
ParseSimulatorControlInspectPayload(
	const std::string& payload, std::string& path, int& cursor, std::string& error )
{
	size_t lineEnd = payload.find( '\n' );
	path = std::string::npos == lineEnd ? payload : payload.substr( 0, lineEnd );
	StripSimulatorControlCarriageReturn( path );
	cursor = 0;

	if ( std::string::npos != lineEnd )
	{
		std::string options = payload.substr( lineEnd + 1 );
		StripSimulatorControlCarriageReturn( options );
		if ( ! options.empty() )
		{
			static const char kCursorPrefix[] = "cursor=";
			if ( 0 != options.compare( 0, sizeof( kCursorPrefix ) - 1, kCursorPrefix ) )
			{
				error = "inspect supports only a cursor option";
				return false;
			}
			std::string cursorString = options.substr( sizeof( kCursorPrefix ) - 1 );
			if ( cursorString.empty() )
			{
				error = "inspect cursor must be a non-negative integer";
				return false;
			}
			for ( size_t index = 0; index < cursorString.length(); index++ )
			{
				if ( ! isdigit( (unsigned char)cursorString[index] ) )
				{
					error = "inspect cursor must be a non-negative integer";
					return false;
				}
			}
			errno = 0;
			char *cursorEnd = NULL;
			unsigned long cursorValue =
				strtoul( cursorString.c_str(), & cursorEnd, 10 );
			if ( ERANGE == errno || ! cursorEnd || cursorEnd[0] ||
				cursorValue > INT_MAX )
			{
				error = "inspect cursor is too large";
				return false;
			}
			cursor = (int)cursorValue;
		}
	}
	return true;
}

static bool
ParseSimulatorControlTapPayload(
	const std::string& payload, MSimulatorHost::Input& input,
	std::string& error )
{
	std::istringstream stream( payload );
	std::string extra;
	if ( ! ( stream >> input.x >> input.y ) || stream >> extra ||
		! IsFiniteNumber( (lua_Number)input.x ) ||
		! IsFiniteNumber( (lua_Number)input.y ) )
	{
		error = "tap expects finite X and Y screen coordinates";
		return false;
	}
	input.type = MSimulatorHost::Input::kTouchInput;
	input.phase = MSimulatorHost::Input::kBeganPhase;
	input.xStart = input.x;
	input.yStart = input.y;
	return true;
}

static bool
ParseSimulatorControlKeyPayload(
	const std::string& payload, MSimulatorHost::Input& input,
	std::string& error )
{
	std::istringstream stream( payload );
	std::string phase;
	std::string extra;
	if ( ! ( stream >> input.keyName ) || input.keyName.length() > 128 )
	{
		error = "key expects a key name and optional phase";
		return false;
	}
	if ( stream >> phase && stream >> extra )
	{
		error = "key expects a key name and optional phase";
		return false;
	}

	input.type = MSimulatorHost::Input::kKeyInput;
	if ( phase.empty() || "pressed" == phase )
	{
		input.phase = MSimulatorHost::Input::kPressedPhase;
	}
	else if ( "down" == phase )
	{
		input.phase = MSimulatorHost::Input::kDownPhase;
	}
	else if ( "up" == phase )
	{
		input.phase = MSimulatorHost::Input::kUpPhase;
	}
	else
	{
		error = "key phase must be 'down', 'up', or 'pressed'";
		return false;
	}
	return true;
}

static bool
ParseSimulatorControlUnsignedLong(
	const std::string& value, const char *key, unsigned long& result )
{
	size_t offset = value.find( key );
	if ( std::string::npos == offset )
	{
		return false;
	}
	offset += strlen( key );
	while ( offset < value.length() && isspace( (unsigned char)value[offset] ) )
	{
		offset++;
	}
	if ( offset >= value.length() ||
		! isdigit( (unsigned char)value[offset] ) )
	{
		return false;
	}

	char *end = NULL;
	errno = 0;
	result = strtoul( value.c_str() + offset, & end, 10 );
	return ERANGE != errno && end && end != value.c_str() + offset;
}

static bool
ParseSimulatorControlJsonString(
	const std::string& value, const char *key, std::string& result )
{
	size_t offset = value.find( key );
	if ( std::string::npos == offset )
	{
		return false;
	}
	offset += strlen( key );
	while ( offset < value.length() && isspace( (unsigned char)value[offset] ) )
	{
		offset++;
	}
	if ( offset >= value.length() || '"' != value[offset++] )
	{
		return false;
	}

	size_t start = offset;
	while ( offset < value.length() && '"' != value[offset] )
	{
		unsigned char character = (unsigned char)value[offset];
		if ( '-' != character && '_' != character && ! isalnum( character ) )
		{
			return false;
		}
		offset++;
	}
	if ( offset >= value.length() || start == offset || offset - start > 127 )
	{
		return false;
	}
	result.assign( value, start, offset - start );
	return true;
}

static bool
ParseSimulatorControlSession(
	const std::string& value, SimulatorControlSession& result )
{
	unsigned long protocol = 0;
	return ParseSimulatorControlUnsignedLong(
			value, "\"protocol\":", protocol ) &&
		2 == protocol &&
		ParseSimulatorControlUnsignedLong(
			value, "\"generation\":", result.generation ) &&
		ParseSimulatorControlUnsignedLong(
			value, "\"pid\":", result.processId ) &&
		ParseSimulatorControlJsonString(
			value, "\"sessionId\":", result.sessionId );
}

static bool
ReadSimulatorControlSession(
	const std::string& directory, SimulatorControlSession& result )
{
	std::string contents;
	return ReadSimulatorControlFile(
			SimulatorControlPath( directory, "session.json" ), contents ) &&
		ParseSimulatorControlSession( contents, result );
}

static bool
IsSimulatorControlProtocolFilename( const char *filename )
{
	if ( ! filename || ! filename[0] )
	{
		return false;
	}
	if ( 0 == strcmp( filename, "session.json" ) ||
		0 == strcmp( filename, "session.json.tmp" ) ||
		0 == strcmp( filename, "diagnostics.json" ) ||
		0 == strcmp( filename, "diagnostics.json.tmp" ) ||
		0 == strcmp( filename, "screenshot.png" ) ||
		0 == strcmp( filename, "screenshot.png.tmp" ) ||
		0 == strcmp( filename, "request" ) ||
		0 == strcmp( filename, "request.tmp" ) ||
		0 == strcmp( filename, ".simulator-control.lock" ) )
	{
		return true;
	}
	if ( 0 == strncmp(
			filename, ".simulator-control.lock.stale.",
			strlen( ".simulator-control.lock.stale." ) ) ||
		0 == strncmp(
			filename, ".simulator-control.owner.stale.",
			strlen( ".simulator-control.owner.stale." ) ) )
	{
		return true;
	}
	if ( 0 == strncmp(
		filename, "request.processing.", strlen( "request.processing." ) ) )
	{
		const char *generation = filename + strlen( "request.processing." );
		if ( ! generation[0] )
		{
			return false;
		}
		while ( generation[0] )
		{
			if ( ! isdigit( (unsigned char)generation[0] ) )
			{
				return false;
			}
			generation++;
		}
		return true;
	}
	if ( 0 != strncmp( filename, "response.", strlen( "response." ) ) )
	{
		return false;
	}

	const char *identifier = filename + strlen( "response." );
	const char *suffix = strstr( identifier, ".json" );
	if ( ! suffix || suffix == identifier ||
		( suffix[5] && 0 != strcmp( suffix + 5, ".tmp" ) ) )
	{
		return false;
	}
	while ( identifier < suffix )
	{
		if ( '-' != identifier[0] && '_' != identifier[0] &&
			! isalnum( (unsigned char)identifier[0] ) )
		{
			return false;
		}
		identifier++;
	}
	return true;
}

static bool
CleanupSimulatorControlDirectory( const std::string& directory )
{
	bool succeeded = true;
#ifdef Rtt_WIN_ENV
	std::string pattern = SimulatorControlPath( directory, "*" );
	std::wstring widePattern;
	if ( ! ConvertSimulatorControlUtf8ToWide( pattern, widePattern ) )
	{
		return false;
	}
	WIN32_FIND_DATAW information;
	HANDLE findHandle =
		FindFirstFileW( widePattern.c_str(), & information );
	if ( INVALID_HANDLE_VALUE == findHandle )
	{
		return ERROR_FILE_NOT_FOUND == GetLastError();
	}
	do
	{
		std::string filename;
		if ( ConvertSimulatorControlWideToUtf8(
				information.cFileName, filename ) &&
			IsSimulatorControlProtocolFilename( filename.c_str() ) &&
			0 != DeleteSimulatorControlFile(
				SimulatorControlPath(
					directory, filename.c_str() ) ) )
		{
			succeeded = false;
		}
	}
	while ( FindNextFileW( findHandle, & information ) );
	FindClose( findHandle );
#else
	DIR *directoryHandle = opendir( directory.c_str() );
	if ( ! directoryHandle )
	{
		return false;
	}
	struct dirent *entry = NULL;
	while ( NULL != ( entry = readdir( directoryHandle ) ) )
	{
		if ( IsSimulatorControlProtocolFilename( entry->d_name ) &&
			0 != DeleteSimulatorControlFile(
				SimulatorControlPath(
					directory, entry->d_name ) ) )
		{
			succeeded = false;
		}
	}
	closedir( directoryHandle );
#endif
	return succeeded;
}

static bool
PrepareSimulatorControlDirectory(
	const std::string& directory, std::string& error )
{
	std::string ownerPath =
		SimulatorControlPath( directory, ".simulator-control.owner" );
	while ( ! CreateSimulatorControlExclusivePidFile( ownerPath ) )
	{
		if ( EEXIST != errno )
		{
			error = "unable to claim the Simulator control directory";
			return false;
		}

		unsigned long ownerProcessId = 0;
		if ( ReadSimulatorControlFileOwner( ownerPath, ownerProcessId ) )
		{
			if ( IsSimulatorControlProcessAlive( ownerProcessId ) )
			{
				char message[192];
				snprintf(
					message, sizeof( message ),
					"control directory is already owned by live process %lu",
					ownerProcessId );
				error.assign( message );
				return false;
			}
			if ( ! MoveSimulatorControlFileAside( ownerPath ) &&
				SimulatorControlFileExists( ownerPath ) )
			{
				error = "unable to remove stale Simulator control ownership";
				return false;
			}
			continue;
		}
		if ( IsSimulatorControlFileStale( ownerPath, 30 ) )
		{
			if ( ! MoveSimulatorControlFileAside( ownerPath ) &&
				SimulatorControlFileExists( ownerPath ) )
			{
				error = "unable to remove stale Simulator control ownership";
				return false;
			}
			continue;
		}

		error = "control directory has an unreadable active ownership file";
		return false;
	}

	std::string existingSession;
	if ( ReadSimulatorControlFile(
		SimulatorControlPath( directory, "session.json" ), existingSession ) )
	{
		unsigned long ownerProcessId = 0;
		if ( ParseSimulatorControlUnsignedLong(
				existingSession, "\"pid\":", ownerProcessId ) &&
			IsSimulatorControlProcessAlive( ownerProcessId ) )
		{
			char message[192];
			snprintf(
				message, sizeof( message ),
				"control directory is already owned by live process %lu",
				ownerProcessId );
			error.assign( message );
			DeleteSimulatorControlFile( ownerPath );
			return false;
		}
	}

	if ( ! CleanupSimulatorControlDirectory( directory ) )
	{
		error = "unable to clean stale Simulator control files";
		DeleteSimulatorControlFile( ownerPath );
		return false;
	}
	return true;
}

static bool
WaitForSimulatorControlSession(
	const std::string& directory, unsigned long timeoutMilliseconds,
	bool requireNewSession, const std::string& previousSessionId,
	SimulatorControlSession& session, std::string& error )
{
	std::string sessionPath = SimulatorControlPath( directory, "session.json" );
	unsigned long long deadline =
		GetSimulatorControlMilliseconds() + timeoutMilliseconds;
	bool foundExitedProcess = false;
	bool foundIncompatibleSession = false;
	while ( GetSimulatorControlMilliseconds() < deadline )
	{
		std::string contents;
		if ( ReadSimulatorControlFile( sessionPath, contents ) )
		{
			SimulatorControlSession candidate;
			if ( ParseSimulatorControlSession( contents, candidate ) )
			{
				if ( IsSimulatorControlProcessAlive( candidate.processId ) )
				{
					if ( ! requireNewSession ||
						candidate.sessionId != previousSessionId )
					{
						session = candidate;
						return true;
					}
				}
				else
				{
					foundExitedProcess = true;
				}
			}
			else
			{
				foundIncompatibleSession = true;
			}
		}
		SleepSimulatorControlMilliseconds( 20 );
	}

	if ( requireNewSession )
	{
		error =
			"the Simulator accepted relaunch but its replacement Lua runtime "
			"did not become ready";
	}
	else if ( foundExitedProcess )
	{
		error =
			"the Simulator control session belongs to a process that is no "
			"longer running";
	}
	else if ( foundIncompatibleSession )
	{
		error = "no compatible Simulator control session was found";
	}
	else
	{
		error = "no ready Simulator control session was found";
	}
	return false;
}

static bool
WaitForSimulatorControlDiagnostics(
	const std::string& directory,
	const SimulatorControlSession& session,
	unsigned long timeoutMilliseconds,
	std::string& response, std::string& error )
{
	std::string diagnosticsPath =
		SimulatorControlPath( directory, "diagnostics.json" );
	unsigned long long deadline =
		GetSimulatorControlMilliseconds() + timeoutMilliseconds;
	while ( GetSimulatorControlMilliseconds() < deadline )
	{
		std::string contents;
		std::string sessionId;
		if ( ReadSimulatorControlFile( diagnosticsPath, contents ) &&
			ParseSimulatorControlJsonString(
				contents, "\"sessionId\":", sessionId ) &&
			sessionId == session.sessionId )
		{
			response = contents;
			return true;
		}
		if ( ! IsSimulatorControlProcessAlive( session.processId ) )
		{
			error =
				"the Simulator exited before diagnostics could be read";
			return false;
		}
		SleepSimulatorControlMilliseconds( 20 );
	}

	error = "the Simulator did not publish diagnostics before the timeout";
	return false;
}

class SimulatorControlClientLock
{
	public:
		SimulatorControlClientLock()
		{
		}

		~SimulatorControlClientLock()
		{
			unsigned long ownerProcessId = 0;
			if ( ! fPath.empty() &&
				ReadSimulatorControlFileOwner( fPath, ownerProcessId ) &&
				ownerProcessId ==
					(unsigned long)GetSimulatorControlProcessId() )
			{
				DeleteSimulatorControlFile( fPath );
			}
		}

		bool Acquire(
			const std::string& directory, unsigned long timeoutMilliseconds,
			std::string& error )
		{
			std::string path;
			if ( ! AcquireSimulatorControlLock(
				directory, timeoutMilliseconds, path, error ) )
			{
				return false;
			}
			fPath = path;
			return true;
		}

	private:
		std::string fPath;
};

static bool
PerformSimulatorControlClientRequest(
	const std::string& directory, const std::string& command,
	const std::string& payload, unsigned long timeoutMilliseconds,
	std::string& response, std::string& error )
{
	SimulatorControlSession session;
	if ( ! WaitForSimulatorControlSession(
		directory, timeoutMilliseconds, false, std::string(), session, error ) )
	{
		return false;
	}

	if ( "diagnostics" == command )
	{
		return WaitForSimulatorControlDiagnostics(
			directory, session, timeoutMilliseconds,
			response, error );
	}

	SimulatorControlClientLock lock;
	if ( ! lock.Acquire( directory, timeoutMilliseconds, error ) )
	{
		return false;
	}

	SimulatorControlSession lockedSession;
	if ( ! ReadSimulatorControlSession( directory, lockedSession ) ||
		lockedSession.sessionId != session.sessionId ||
		lockedSession.processId != session.processId ||
		! IsSimulatorControlProcessAlive( lockedSession.processId ) )
	{
		error = "the Simulator control session changed before the request was sent";
		return false;
	}

	std::string requestPath = SimulatorControlPath( directory, "request" );
	if ( SimulatorControlFileExists( requestPath ) )
	{
		error = "the Simulator has not consumed an existing control request";
		return false;
	}

	static unsigned long requestCounter = 0;
	char identifier[128];
	snprintf(
		identifier, sizeof( identifier ), "%ld_%llu_%lu",
		GetSimulatorControlProcessId(), GetSimulatorControlMilliseconds(),
		++requestCounter );
	std::string responsePath = SimulatorControlPath(
		directory, ( std::string( "response." ) + identifier + ".json" ).c_str() );
	DeleteSimulatorControlFile( responsePath );

	std::string request( identifier );
	request.push_back( '\n' );
	request.append( session.sessionId );
	request.push_back( '\n' );
	request.append( command );
	request.push_back( '\n' );
	request.append( payload );
	if ( ! WriteSimulatorControlFileAtomically( requestPath, request ) )
	{
		error = "unable to write the Simulator control request";
		return false;
	}

	unsigned long long deadline =
		GetSimulatorControlMilliseconds() + timeoutMilliseconds;
	while ( GetSimulatorControlMilliseconds() < deadline )
	{
		if ( ReadSimulatorControlFile( responsePath, response ) )
		{
			DeleteSimulatorControlFile( responsePath );
			if ( "relaunch" == command &&
				0 == response.compare( 0, 10, "{\"ok\":true" ) )
			{
				SimulatorControlSession replacementSession;
				if ( ! WaitForSimulatorControlSession(
					directory, timeoutMilliseconds, true, session.sessionId,
					replacementSession, error ) )
				{
					return false;
				}
			}
			return true;
		}
		SleepSimulatorControlMilliseconds( 10 );
	}

	DeleteSimulatorControlFile( requestPath );
	error = "the Simulator did not answer the control request before the timeout";
	return false;
}

static bool
IsSimulatorControlArgument( const char *argument, const char *expected )
{
	return argument && 0 == strcmp( argument, expected );
}

static bool
ParseSimulatorControlClientTimeout(
	const char *value, unsigned long& timeoutMilliseconds )
{
	if ( ! value || ! value[0] )
	{
		return false;
	}
	char *end = NULL;
	double seconds = strtod( value, & end );
	if ( ! end || end == value || end[0] ||
		! IsFiniteNumber( (lua_Number)seconds ) ||
		seconds <= 0.0 || seconds > 3600.0 )
	{
		return false;
	}
	timeoutMilliseconds = (unsigned long)( seconds * 1000.0 );
	return timeoutMilliseconds > 0;
}

static std::string
JoinSimulatorControlClientArguments(
	int argc, const char * const argv[], int firstArgument )
{
	std::string result;
	for ( int index = firstArgument; index < argc; index++ )
	{
		if ( index > firstArgument )
		{
			result.push_back( ' ' );
		}
		result.append( argv[index] );
	}
	return result;
}

static void
PrintSimulatorControlClientHelp()
{
	fputs(
		"Usage:\n"
		"  [--timeout SECONDS] COMMAND [ARGUMENTS]\n"
		"\n"
		"Commands:\n"
		"  status\n"
		"  diagnostics\n"
		"  logs [--since SEQUENCE]\n"
		"  screenshot [PATH]\n"
		"  tap X Y\n"
		"  key NAME [down|up|pressed]\n"
		"  eval [LUA EXPRESSION]\n"
		"  exec [LUA STATEMENTS]\n"
		"  exec-file PATH\n"
		"  inspect PATH [--cursor OFFSET]\n"
		"  relaunch\n"
		"  quit [EXIT CODE]\n"
		"\n"
		"eval and exec read standard input when their code is omitted.\n",
		stdout );
}

static bool
RunSimulatorControlClientInternal(
	int argc, const char * const argv[], int& exitCode )
{
	int controlArgumentIndex = -1;
	for ( int index = 1; index < argc; index++ )
	{
		if ( IsSimulatorControlArgument( argv[index], "-simulator-control" ) )
		{
			controlArgumentIndex = index;
			break;
		}
	}
	if ( controlArgumentIndex < 0 )
	{
		return false;
	}

	exitCode = 2;
	const char *directoryArgument = NULL;
	for ( int index = 1; index < controlArgumentIndex; index++ )
	{
		if ( IsSimulatorControlArgument(
			argv[index], "-simulator-control-dir" ) )
		{
			if ( index + 1 >= controlArgumentIndex )
			{
				fprintf(
					stderr,
					"Simulator control: -simulator-control-dir expects a path\n" );
				return true;
			}
			directoryArgument = argv[++index];
		}
	}

	int argumentIndex = controlArgumentIndex + 1;
	unsigned long timeoutMilliseconds = 10000;
	if ( argumentIndex < argc &&
		( IsSimulatorControlArgument( argv[argumentIndex], "--timeout" ) ||
			IsSimulatorControlArgument( argv[argumentIndex], "-timeout" ) ) )
	{
		if ( argumentIndex + 1 >= argc ||
			! ParseSimulatorControlClientTimeout(
				argv[argumentIndex + 1], timeoutMilliseconds ) )
		{
			fprintf(
				stderr,
				"Simulator control: --timeout expects more than 0 and at most "
				"3600 seconds\n" );
			return true;
		}
		argumentIndex += 2;
	}

	if ( argumentIndex >= argc ||
		IsSimulatorControlArgument( argv[argumentIndex], "--help" ) ||
		IsSimulatorControlArgument( argv[argumentIndex], "-help" ) ||
		IsSimulatorControlArgument( argv[argumentIndex], "help" ) )
	{
		PrintSimulatorControlClientHelp();
		exitCode = 0;
		return true;
	}

	if ( ! directoryArgument || ! directoryArgument[0] )
	{
		fprintf(
			stderr,
			"Simulator control: -simulator-control-dir is required\n" );
		return true;
	}

	std::string directory;
	if ( ! MakeSimulatorControlAbsolutePath( directoryArgument, directory ) ||
		! SimulatorControlDirectoryExists( directory ) )
	{
		fprintf(
			stderr, "Simulator control: directory does not exist: %s\n",
			directoryArgument );
		return true;
	}

	std::string command( argv[argumentIndex++] );
	std::string payload;
	if ( "status" == command ||
		"diagnostics" == command ||
		"relaunch" == command )
	{
		if ( argumentIndex != argc )
		{
			fprintf(
				stderr, "Simulator control: %s does not accept arguments\n",
				command.c_str() );
			return true;
		}
	}
	else if ( "logs" == command )
	{
		payload.assign( "0" );
		if ( argumentIndex < argc )
		{
			if ( argumentIndex + 2 != argc ||
				( ! IsSimulatorControlArgument(
					argv[argumentIndex], "--since" ) &&
					! IsSimulatorControlArgument(
						argv[argumentIndex], "-since" ) ) )
			{
				fprintf(
					stderr,
					"Simulator control: logs accepts only --since SEQUENCE\n" );
				return true;
			}
			const char *sequence = argv[argumentIndex + 1];
			if ( ! sequence[0] )
			{
				fprintf(
					stderr,
					"Simulator control: logs sequence must be non-negative\n" );
				return true;
			}
			for ( const char *character = sequence; *character; character++ )
			{
				if ( ! isdigit( (unsigned char)*character ) )
				{
					fprintf(
						stderr,
						"Simulator control: logs sequence must be non-negative\n" );
					return true;
				}
			}
			errno = 0;
			char *sequenceEnd = NULL;
			strtoul( sequence, & sequenceEnd, 10 );
			if ( ERANGE == errno || ! sequenceEnd || sequenceEnd[0] )
			{
				fprintf(
					stderr, "Simulator control: logs sequence is too large\n" );
				return true;
			}
			payload.assign( sequence );
		}
	}
	else if ( "screenshot" == command )
	{
		if ( argumentIndex + 1 < argc )
		{
			fprintf(
				stderr,
				"Simulator control: screenshot accepts at most one path\n" );
			return true;
		}
		const char *path = argumentIndex < argc ?
			argv[argumentIndex] : "screenshot.png";
		std::string outputPath = argumentIndex < argc ?
			std::string( path ) :
			SimulatorControlPath( directory, path );
		if ( ! MakeSimulatorControlAbsoluteOutputPath(
			outputPath, payload ) )
		{
			fprintf(
				stderr,
				"Simulator control: screenshot expects a valid output path\n" );
			return true;
		}
	}
	else if ( "tap" == command )
	{
		payload = JoinSimulatorControlClientArguments(
			argc, argv, argumentIndex );
		MSimulatorHost::Input input;
		std::string error;
		if ( ! ParseSimulatorControlTapPayload(
			payload, input, error ) )
		{
			fprintf(
				stderr, "Simulator control: %s\n",
				error.c_str() );
			return true;
		}
	}
	else if ( "key" == command )
	{
		payload = JoinSimulatorControlClientArguments(
			argc, argv, argumentIndex );
		MSimulatorHost::Input input;
		std::string error;
		if ( ! ParseSimulatorControlKeyPayload(
			payload, input, error ) )
		{
			fprintf(
				stderr, "Simulator control: %s\n",
				error.c_str() );
			return true;
		}
	}
	else if ( "eval" == command || "exec" == command )
	{
		if ( argumentIndex < argc )
		{
			payload = JoinSimulatorControlClientArguments(
				argc, argv, argumentIndex );
		}
		else
		{
			payload.assign(
				std::istreambuf_iterator< char >( std::cin ),
				std::istreambuf_iterator< char >() );
		}
		if ( payload.empty() )
		{
			fprintf(
				stderr, "Simulator control: %s expects Lua code\n",
				command.c_str() );
			return true;
		}
	}
	else if ( "exec-file" == command )
	{
		if ( argumentIndex + 1 != argc ||
			! MakeSimulatorControlAbsolutePath(
				argv[argumentIndex], payload ) ||
			! SimulatorControlFileExists( payload ) )
		{
			fprintf(
				stderr,
				"Simulator control: exec-file expects one existing file path\n" );
			return true;
		}
	}
	else if ( "inspect" == command )
	{
		if ( argumentIndex >= argc )
		{
			fprintf(
				stderr, "Simulator control: inspect expects a path\n" );
			return true;
		}
		payload.assign( argv[argumentIndex++] );
		if ( argumentIndex < argc )
		{
			if ( argumentIndex + 2 != argc ||
				( ! IsSimulatorControlArgument(
					argv[argumentIndex], "--cursor" ) &&
					! IsSimulatorControlArgument(
						argv[argumentIndex], "-cursor" ) ) )
			{
				fprintf(
					stderr,
					"Simulator control: inspect accepts only --cursor OFFSET\n" );
				return true;
			}
			const char *cursor = argv[argumentIndex + 1];
			if ( ! cursor[0] )
			{
				fprintf(
					stderr,
					"Simulator control: inspect cursor must be non-negative\n" );
				return true;
			}
			for ( const char *character = cursor; *character; character++ )
			{
				if ( ! isdigit( (unsigned char)*character ) )
				{
					fprintf(
						stderr,
						"Simulator control: inspect cursor must be non-negative\n" );
					return true;
				}
			}
			errno = 0;
			char *cursorEnd = NULL;
			unsigned long cursorValue = strtoul( cursor, & cursorEnd, 10 );
			if ( ERANGE == errno || ! cursorEnd || cursorEnd[0] ||
				cursorValue > INT_MAX )
			{
				fprintf(
					stderr,
					"Simulator control: inspect cursor is too large\n" );
				return true;
			}
			payload.append( "\ncursor=" );
			payload.append( cursor );
		}
	}
	else if ( "quit" == command )
	{
		payload.assign( argumentIndex < argc ? argv[argumentIndex++] : "0" );
		if ( argumentIndex != argc )
		{
			fprintf(
				stderr, "Simulator control: quit accepts one exit code\n" );
			return true;
		}
		for ( size_t index = 0; index < payload.length(); index++ )
		{
			if ( ! isdigit( (unsigned char)payload[index] ) )
			{
				fprintf(
					stderr,
					"Simulator control: quit exit code must be 0 through 255\n" );
				return true;
			}
		}
		if ( payload.empty() )
		{
			fprintf(
				stderr,
				"Simulator control: quit exit code must be 0 through 255\n" );
			return true;
		}
		errno = 0;
		char *exitCodeEnd = NULL;
		unsigned long quitExitCode =
			strtoul( payload.c_str(), & exitCodeEnd, 10 );
		if ( ERANGE == errno || ! exitCodeEnd || exitCodeEnd[0] ||
			quitExitCode > 255 )
		{
			fprintf(
				stderr,
				"Simulator control: quit exit code must be 0 through 255\n" );
			return true;
		}
	}
	else
	{
		fprintf(
			stderr, "Simulator control: unknown command '%s'\n",
			command.c_str() );
		return true;
	}

	std::string response;
	std::string error;
	if ( ! PerformSimulatorControlClientRequest(
		directory, command, payload, timeoutMilliseconds, response, error ) )
	{
		fprintf( stderr, "Simulator control: %s\n", error.c_str() );
		return true;
	}

	fwrite( response.data(), 1, response.length(), stdout );
	fputc( '\n', stdout );
	exitCode =
		0 == response.compare( 0, 10, "{\"ok\":true" ) ? 0 : 1;
	return true;
}

static std::string
ProcessSimulatorControlRequest(
	Runtime& runtime, SimulatorControlRuntimeState& state,
	const std::string& command, const std::string& payload, int& pendingQuitExitCode )
{
	if ( "status" == command )
	{
		char result[512];
		snprintf(
			result, sizeof( result ),
			"{\"protocol\":2,\"sessionId\":\"%s\",\"generation\":%lu,"
			"\"pid\":%ld,\"frame\":%lu,"
			"\"applicationLoaded\":%s,\"applicationExecuting\":%s,\"suspended\":%s}",
			state.sessionId.c_str(),
			state.generation,
			GetSimulatorControlProcessId(),
			(unsigned long)runtime.GetFrame(),
			runtime.IsProperty( Runtime::kIsApplicationLoaded ) ? "true" : "false",
			runtime.IsProperty( Runtime::kIsApplicationExecuting ) ? "true" : "false",
			runtime.IsSuspended() ? "true" : "false" );
		return SimulatorControlSuccessResponse( result );
	}

	if ( "diagnostics" == command )
	{
		return SimulatorControlSuccessResponse(
			BuildSimulatorControlDiagnostics( state ) );
	}

	if ( "logs" == command )
	{
		if ( payload.empty() )
		{
			return SimulatorControlErrorResponse(
				"logs expects a non-negative sequence" );
		}
		for ( size_t index = 0; index < payload.length(); index++ )
		{
			if ( ! isdigit( (unsigned char)payload[index] ) )
			{
				return SimulatorControlErrorResponse(
					"logs expects a non-negative sequence" );
			}
		}
		errno = 0;
		char *sequenceEnd = NULL;
		unsigned long sinceSequence =
			strtoul( payload.c_str(), & sequenceEnd, 10 );
		if ( ERANGE == errno || ! sequenceEnd || sequenceEnd[0] )
		{
			return SimulatorControlErrorResponse(
				"logs sequence is too large" );
		}
		return SimulatorControlSuccessResponse(
			BuildSimulatorControlLogs( state, sinceSequence ) );
	}

	if ( "screenshot" == command )
	{
		std::string result;
		std::string error;
		if ( ! CaptureSimulatorControlScreenshot(
			runtime, payload, result, error ) )
		{
			return SimulatorControlErrorResponse( error );
		}
		return SimulatorControlSuccessResponse( result );
	}

	if ( "tap" == command || "key" == command )
	{
		const MSimulatorHost *host = runtime.Platform().GetSimulatorHost();
		if ( ! host )
		{
			return SimulatorControlErrorResponse(
				"the Simulator cannot send input to this runtime" );
		}

		MSimulatorHost::Input input;
		std::string error;
		if ( "tap" == command )
		{
			if ( ! ParseSimulatorControlTapPayload(
				payload, input, error ) )
			{
				return SimulatorControlErrorResponse( error );
			}
			if ( ! host->SendInput( input ) )
			{
				return SimulatorControlErrorResponse(
					"the Simulator could not queue the requested tap" );
			}
			input.phase = MSimulatorHost::Input::kEndedPhase;
		}
		else if ( "key" == command )
		{
			if ( ! ParseSimulatorControlKeyPayload(
				payload, input, error ) )
			{
				return SimulatorControlErrorResponse( error );
			}
			if ( MSimulatorHost::Input::kPressedPhase == input.phase )
			{
				input.phase = MSimulatorHost::Input::kDownPhase;
				if ( ! host->SendInput( input ) )
				{
					return SimulatorControlErrorResponse(
						"the Simulator could not queue the requested key" );
				}
				input.phase = MSimulatorHost::Input::kUpPhase;
			}
		}
		if ( ! host->SendInput( input ) )
		{
			return SimulatorControlErrorResponse(
				"the Simulator could not queue the requested input" );
		}
		return SimulatorControlSuccessResponse( "true" );
	}

	lua_State *L = runtime.VMContext().L();
	if ( ! L )
	{
		return SimulatorControlErrorResponse( "the Lua runtime is not available" );
	}

	if ( "eval" == command || "exec" == command || "exec-file" == command )
	{
		if ( payload.empty() )
		{
			return SimulatorControlErrorResponse( command + " expects a non-empty payload" );
		}

		std::string jsonResult;
		std::string error;
		const bool isExpression = "eval" == command;
		const bool isFile = "exec-file" == command;
		const char *chunkName =
			isExpression ? "=@simulator-control/eval" : "=@simulator-control/exec";
		if ( ! ExecuteSimulatorControlChunk(
			L, payload, chunkName, isExpression, isFile, jsonResult, error ) )
		{
			RecordSimulatorControlExecutionDiagnostic(
				runtime, state, error );
			WriteSimulatorControlDiagnostics( state );
			return SimulatorControlErrorResponse( error );
		}
		return SimulatorControlSuccessResponse( jsonResult );
	}

	if ( "inspect" == command )
	{
		std::string path;
		std::string error;
		int cursor = 0;
		if ( ! ParseSimulatorControlInspectPayload( payload, path, cursor, error ) )
		{
			return SimulatorControlErrorResponse( error );
		}

		int originalTop = lua_gettop( L );
		if ( ! PushSimulatorControlInspectedValue( L, path, error ) )
		{
			lua_settop( L, originalTop );
			return SimulatorControlErrorResponse( error );
		}
		std::string jsonResult = BuildSimulatorControlInspection( L, -1, cursor );
		lua_settop( L, originalTop );
		return SimulatorControlSuccessResponse( jsonResult );
	}

	if ( "relaunch" == command )
	{
		const MSimulatorHost *host = runtime.Platform().GetSimulatorHost();
		if ( ! host || ! host->Relaunch() )
		{
			return SimulatorControlErrorResponse( "the Simulator could not relaunch the project" );
		}
		return SimulatorControlSuccessResponse( "true" );
	}

	if ( "quit" == command )
	{
		int exitCode = 0;
		if ( ! payload.empty() )
		{
			for ( size_t index = 0; index < payload.length(); index++ )
			{
				if ( ! isdigit( (unsigned char)payload[index] ) )
				{
					return SimulatorControlErrorResponse(
						"quit exit code must be an integer between 0 and 255" );
				}
			}
			errno = 0;
			char *exitCodeEnd = NULL;
			unsigned long parsedExitCode =
				strtoul( payload.c_str(), & exitCodeEnd, 10 );
			if ( ERANGE == errno || ! exitCodeEnd || exitCodeEnd[0] ||
				parsedExitCode > 255 )
			{
				return SimulatorControlErrorResponse(
					"quit exit code must be an integer between 0 and 255" );
			}
			exitCode = (int)parsedExitCode;
		}

		const MSimulatorHost *host = runtime.Platform().GetSimulatorHost();
		if ( ! host )
		{
			return SimulatorControlErrorResponse( "the Simulator cannot quit this runtime" );
		}
		pendingQuitExitCode = exitCode;
		return SimulatorControlSuccessResponse( "true" );
	}

	return SimulatorControlErrorResponse( std::string( "unknown control command '" ) + command + "'" );
}

void
SimulatorControl::SetDirectory( const char *directory )
{
	Rtt_SetLogCallback( NULL, NULL );
	ResetSimulatorControlLogs();
	std::string& configuredDirectory = GetSimulatorControlDirectory();
	configuredDirectory.clear();
	if ( ! directory || ! directory[0] )
	{
		return;
	}

	std::string value( directory );
	std::string error;
	if ( ! SimulatorControlDirectoryExists( value ) )
	{
		error = "control directory does not exist";
	}
	else if ( PrepareSimulatorControlDirectory( value, error ) )
	{
		configuredDirectory = value;
		GetSimulatorControlOwnerPath() =
			SimulatorControlPath( value, ".simulator-control.owner" );
		static bool registeredExitHandler = false;
		if ( ! registeredExitHandler )
		{
			registeredExitHandler =
				0 == atexit( ReleaseSimulatorControlDirectory );
		}
		Rtt_SetLogCallback( RecordSimulatorControlLog, NULL );
		return;
	}

	fprintf(
		stderr, "Simulator control disabled: %s: %s\n",
		error.c_str(), directory );
}

void
SimulatorControl::Process( Runtime& sender )
{
	SimulatorControlRuntimeState *statePointer =
		GetOrCreateSimulatorControlRuntimeState( sender );
	if ( ! statePointer )
	{
		return;
	}
	SimulatorControlRuntimeState& state = *statePointer;

	if ( ! state.sessionWritten )
	{
		WriteSimulatorControlSession( state );
	}
	if ( state.sessionWritten && ! state.diagnosticsWritten )
	{
		WriteSimulatorControlDiagnostics( state );
	}

	std::string requestPath = SimulatorControlPath( state.directory, "request" );
	char processingFilename[96];
	snprintf(
		processingFilename, sizeof( processingFilename ),
		"request.processing.%lu",
		state.generation );
	std::string processingPath =
		SimulatorControlPath( state.directory, processingFilename );
	DeleteSimulatorControlFile( processingPath );
	if ( 0 != MoveSimulatorControlFile(
		requestPath, processingPath, true ) )
	{
		return;
	}

	std::string contents;
	bool didRead = ReadSimulatorControlFile( processingPath, contents );
	DeleteSimulatorControlFile( processingPath );

	std::string identifier;
	std::string requestSessionId;
	std::string command;
	std::string payload;
	std::string error;
	std::string response;
	int pendingQuitExitCode = -1;
	if ( ! didRead )
	{
		response = SimulatorControlErrorResponse(
			"unable to read control request or request exceeded one megabyte" );
	}
	else if ( ! ParseSimulatorControlRequest(
		contents, identifier, requestSessionId, command, payload, error ) )
	{
		response = SimulatorControlErrorResponse( error );
	}
	else if ( requestSessionId != state.sessionId )
	{
		response = SimulatorControlErrorResponse(
			"control request belongs to a different Simulator session" );
	}
	else
	{
		response = ProcessSimulatorControlRequest(
			sender, state, command, payload, pendingQuitExitCode );
	}

	if ( identifier.empty() )
	{
		identifier = "invalid";
	}
	std::string responsePath =
		SimulatorControlPath( state.directory, ( std::string( "response." ) + identifier + ".json" ).c_str() );
	WriteSimulatorControlFileAtomically( responsePath, response );

	if ( pendingQuitExitCode >= 0 )
	{
		const MSimulatorHost *host = sender.Platform().GetSimulatorHost();
		if ( host )
		{
			host->Quit( pendingQuitExitCode );
		}
	}
}

void
SimulatorControl::RecordRuntimeError(
	Runtime& sender, const char *errorType,
	const char *message, const char *stackTrace )
{
	SimulatorControlRuntimeState *state =
		GetOrCreateSimulatorControlRuntimeState( sender );
	if ( state )
	{
		RecordSimulatorControlDiagnostic(
			sender, *state, errorType, message, stackTrace );
		WriteSimulatorControlSession( *state );
		WriteSimulatorControlDiagnostics( *state );
	}
}

void
SimulatorControl::Shutdown( Runtime& sender )
{
	SimulatorControlRuntimeStateMap& states = GetSimulatorControlRuntimeStates();
	SimulatorControlRuntimeStateMap::iterator iterator = states.find( & sender );
	if ( states.end() == iterator )
	{
		return;
	}

	SimulatorControlSession currentSession;
	if ( ReadSimulatorControlSession(
			iterator->second.directory, currentSession ) &&
		currentSession.sessionId == iterator->second.sessionId &&
		currentSession.processId ==
			(unsigned long)GetSimulatorControlProcessId() )
	{
		DeleteSimulatorControlFile(
			SimulatorControlPath(
				iterator->second.directory, "session.json" ) );
		std::string diagnostics;
		std::string diagnosticsSessionId;
		std::string diagnosticsPath =
			SimulatorControlPath(
				iterator->second.directory, "diagnostics.json" );
		if ( ReadSimulatorControlFile( diagnosticsPath, diagnostics ) &&
			ParseSimulatorControlJsonString(
				diagnostics, "\"sessionId\":",
				diagnosticsSessionId ) &&
			diagnosticsSessionId == iterator->second.sessionId )
		{
			DeleteSimulatorControlFile( diagnosticsPath );
		}
	}
	states.erase( iterator );
}

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------

#endif // Rtt_AUTHORING_SIMULATOR

#ifdef Rtt_AUTHORING_SIMULATOR

extern "C" int
Rtt_RunSimulatorControlClient(
	int argc, const char * const argv[], int *outExitCode )
{
	if ( ! outExitCode )
	{
		return 0;
	}
	int exitCode = 0;
	if ( ! Rtt::RunSimulatorControlClientInternal( argc, argv, exitCode ) )
	{
		return 0;
	}
	*outExitCode = exitCode;
	return 1;
}

#else

extern "C" int
Rtt_RunSimulatorControlClient(
	int argc, const char * const argv[], int *outExitCode )
{
	return 0;
}

#endif
