//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Core/Rtt_Build.h"

#include "Rtt_SimulatorControl.h"

#ifdef Rtt_AUTHORING_SIMULATOR

#include "Rtt_LuaContext.h"
#include "Rtt_MPlatform.h"
#include "Rtt_MSimulatorHost.h"
#include "Rtt_Runtime.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <float.h>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits.h>
#include <map>
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
static const size_t kSimulatorControlMaximumStringSize = 4096;
static const size_t kSimulatorControlMaximumResponseSize = 256 * 1024;
static const int kSimulatorControlMaximumEntries = 100;
static char kSimulatorControlRegistryKey;

static std::string&
GetSimulatorControlDirectory()
{
	static std::string directory;
	return directory;
}

struct SimulatorControlRuntimeState
{
	SimulatorControlRuntimeState()
	:	generation( 0 ),
		sessionWritten( false )
	{
	}

	unsigned long generation;
	std::string directory;
	bool sessionWritten;
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

static bool
ReadSimulatorControlFile( const std::string& path, std::string& result )
{
	std::ifstream stream( path.c_str(), std::ios::in | std::ios::binary );
	if ( ! stream )
	{
		return false;
	}

	stream.seekg( 0, std::ios::end );
	std::streamoff length = stream.tellg();
	if ( length < 0 || (size_t)length > kSimulatorControlMaximumRequestSize )
	{
		return false;
	}
	stream.seekg( 0, std::ios::beg );

	result.assign( (std::istreambuf_iterator< char >( stream )), std::istreambuf_iterator< char >() );
	return stream.good() || stream.eof();
}

static bool
WriteSimulatorControlFileAtomically( const std::string& path, const std::string& contents )
{
	std::string temporaryPath = path + ".tmp";
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
			remove( temporaryPath.c_str() );
			return false;
		}
	}

	// Windows does not replace an existing destination with rename(). Session
	// descriptors are rewritten only during lifecycle changes, so the small gap
	// between remove() and rename() is acceptable there.
	remove( path.c_str() );
	if ( 0 != rename( temporaryPath.c_str(), path.c_str() ) )
	{
		remove( temporaryPath.c_str() );
		return false;
	}
	return true;
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
SimulatorControlFileExists( const std::string& path )
{
#ifdef Rtt_WIN_ENV
	struct _stat information;
	return 0 == _stat( path.c_str(), & information );
#else
	struct stat information;
	return 0 == stat( path.c_str(), & information );
#endif
}

static bool
SimulatorControlDirectoryExists( const std::string& path )
{
#ifdef Rtt_WIN_ENV
	struct _stat information;
	return 0 == _stat( path.c_str(), & information ) &&
		0 != ( information.st_mode & _S_IFDIR );
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
	struct _stat information;
	if ( 0 != _stat( path.c_str(), & information ) )
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
CreateSimulatorControlLockFile( const std::string& path )
{
#ifdef Rtt_WIN_ENV
	int descriptor = _open(
		path.c_str(), _O_CREAT | _O_EXCL | _O_WRONLY | _O_BINARY,
		_S_IREAD | _S_IWRITE );
#else
	int descriptor = open( path.c_str(), O_CREAT | O_EXCL | O_WRONLY, 0600 );
#endif
	if ( descriptor < 0 )
	{
		return false;
	}

	char contents[64];
	int length = snprintf(
		contents, sizeof( contents ), "%ld\n%lu\n",
		GetSimulatorControlProcessId(), (unsigned long)time( NULL ) );
#ifdef Rtt_WIN_ENV
	_write( descriptor, contents, length );
	_close( descriptor );
#else
	write( descriptor, contents, (size_t)length );
	close( descriptor );
#endif
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
		if ( CreateSimulatorControlLockFile( lockPath ) )
		{
			return true;
		}
		if ( EEXIST != errno )
		{
			error = "unable to create the Simulator control lock";
			return false;
		}
		if ( IsSimulatorControlFileStale( lockPath, 30 ) )
		{
			remove( lockPath.c_str() );
			continue;
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
	char *absolutePath = _fullpath( NULL, path.c_str(), 0 );
#else
	char *absolutePath = realpath( path.c_str(), NULL );
#endif
	if ( ! absolutePath )
	{
		return false;
	}
	result.assign( absolutePath );
	free( absolutePath );
	return true;
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
		status = luaL_loadfile( L, payload.c_str() );
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
	std::string& command, std::string& payload, std::string& error )
{
	size_t firstLineEnd = contents.find( '\n' );
	size_t secondLineEnd =
		std::string::npos == firstLineEnd ? std::string::npos : contents.find( '\n', firstLineEnd + 1 );
	if ( std::string::npos == firstLineEnd || std::string::npos == secondLineEnd )
	{
		error = "control request must contain an identifier line and command line";
		return false;
	}

	identifier.assign( contents, 0, firstLineEnd );
	command.assign( contents, firstLineEnd + 1, secondLineEnd - firstLineEnd - 1 );
	payload.assign( contents, secondLineEnd + 1, contents.length() - secondLineEnd - 1 );
	StripSimulatorControlCarriageReturn( identifier );
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

	char *end = NULL;
	result = strtoul( value.c_str() + offset, & end, 10 );
	return end && end != value.c_str() + offset;
}

static bool
WaitForSimulatorControlSession(
	const std::string& directory, unsigned long timeoutMilliseconds,
	bool requireNewGeneration, unsigned long previousGeneration,
	unsigned long& generation, std::string& error )
{
	std::string sessionPath = SimulatorControlPath( directory, "session.json" );
	unsigned long long deadline =
		GetSimulatorControlMilliseconds() + timeoutMilliseconds;
	while ( GetSimulatorControlMilliseconds() < deadline )
	{
		std::string session;
		unsigned long candidateGeneration = 0;
		if ( ReadSimulatorControlFile( sessionPath, session ) &&
			std::string::npos != session.find( "\"protocol\":1" ) &&
			ParseSimulatorControlUnsignedLong(
				session, "\"generation\":", candidateGeneration ) &&
			( ! requireNewGeneration || candidateGeneration != previousGeneration ) )
		{
			generation = candidateGeneration;
			return true;
		}
		SleepSimulatorControlMilliseconds( 20 );
	}

	error = requireNewGeneration ?
		"the Simulator accepted relaunch but its replacement Lua runtime did not become ready" :
		"no ready Simulator control session was found";
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
			if ( ! fPath.empty() )
			{
				remove( fPath.c_str() );
			}
		}

		bool Acquire(
			const std::string& directory, unsigned long timeoutMilliseconds,
			std::string& error )
		{
			return AcquireSimulatorControlLock(
				directory, timeoutMilliseconds, fPath, error );
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
	unsigned long generation = 0;
	if ( ! WaitForSimulatorControlSession(
		directory, timeoutMilliseconds, false, 0, generation, error ) )
	{
		return false;
	}

	SimulatorControlClientLock lock;
	if ( ! lock.Acquire( directory, timeoutMilliseconds, error ) )
	{
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
	remove( responsePath.c_str() );

	std::string request( identifier );
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
			remove( responsePath.c_str() );
			if ( "relaunch" == command &&
				0 == response.compare( 0, 10, "{\"ok\":true" ) )
			{
				unsigned long newGeneration = 0;
				if ( ! WaitForSimulatorControlSession(
					directory, timeoutMilliseconds, true, generation,
					newGeneration, error ) )
				{
					return false;
				}
			}
			return true;
		}
		SleepSimulatorControlMilliseconds( 10 );
	}

	remove( requestPath.c_str() );
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
		"  Corona Simulator -simulator-control-dir DIR "
			"-simulator-control [--timeout SECONDS] COMMAND [ARGUMENTS]\n"
		"\n"
		"Commands:\n"
		"  status\n"
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
	if ( "status" == command || "relaunch" == command )
	{
		if ( argumentIndex != argc )
		{
			fprintf(
				stderr, "Simulator control: %s does not accept arguments\n",
				command.c_str() );
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
	Runtime& runtime, const SimulatorControlRuntimeState& state,
	const std::string& command, const std::string& payload, int& pendingQuitExitCode )
{
	lua_State *L = runtime.VMContext().L();
	if ( ! L )
	{
		return SimulatorControlErrorResponse( "the Lua runtime is not available" );
	}

	if ( "status" == command )
	{
		char result[320];
		snprintf(
			result, sizeof( result ),
			"{\"protocol\":1,\"generation\":%lu,\"pid\":%ld,\"frame\":%lu,"
			"\"applicationLoaded\":%s,\"applicationExecuting\":%s,\"suspended\":%s}",
			state.generation,
			GetSimulatorControlProcessId(),
			(unsigned long)runtime.GetFrame(),
			runtime.IsProperty( Runtime::kIsApplicationLoaded ) ? "true" : "false",
			runtime.IsProperty( Runtime::kIsApplicationExecuting ) ? "true" : "false",
			runtime.IsSuspended() ? "true" : "false" );
		return SimulatorControlSuccessResponse( result );
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
	GetSimulatorControlDirectory() = directory ? directory : "";
}

void
SimulatorControl::Process( Runtime& sender )
{
	const std::string& directory = GetSimulatorControlDirectory();
	if ( directory.empty() )
	{
		return;
	}

	SimulatorControlRuntimeStateMap& states = GetSimulatorControlRuntimeStates();
	SimulatorControlRuntimeStateMap::iterator iterator = states.find( & sender );
	if ( states.end() == iterator )
	{
		SimulatorControlRuntimeState state;
		state.directory = directory;
		state.generation = NextSimulatorControlGeneration();
		iterator = states.insert(
			std::make_pair( (const Runtime*)& sender, state ) ).first;
	}
	SimulatorControlRuntimeState& state = iterator->second;

	if ( ! state.sessionWritten )
	{
		char session[512];
		snprintf(
			session, sizeof( session ),
			"{\"protocol\":1,\"transport\":\"filesystem\",\"generation\":%lu,"
			"\"pid\":%ld,\"requestFile\":\"request\"}",
			state.generation, GetSimulatorControlProcessId() );
		state.sessionWritten = WriteSimulatorControlFileAtomically(
			SimulatorControlPath( state.directory, "session.json" ), session );
	}

	std::string requestPath = SimulatorControlPath( state.directory, "request" );
	char processingFilename[96];
	snprintf(
		processingFilename, sizeof( processingFilename ),
		"request.processing.%lu",
		state.generation );
	std::string processingPath =
		SimulatorControlPath( state.directory, processingFilename );
	remove( processingPath.c_str() );
	if ( 0 != rename( requestPath.c_str(), processingPath.c_str() ) )
	{
		return;
	}

	std::string contents;
	bool didRead = ReadSimulatorControlFile( processingPath, contents );
	remove( processingPath.c_str() );

	std::string identifier;
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
		contents, identifier, command, payload, error ) )
	{
		response = SimulatorControlErrorResponse( error );
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
SimulatorControl::Shutdown( Runtime& sender )
{
	SimulatorControlRuntimeStateMap& states = GetSimulatorControlRuntimeStates();
	SimulatorControlRuntimeStateMap::iterator iterator = states.find( & sender );
	if ( states.end() == iterator )
	{
		return;
	}

	remove( SimulatorControlPath( iterator->second.directory, "session.json" ).c_str() );
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
