//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Core/Rtt_Build.h"

#include "Rtt_AndroidAppPackager.h"

#include "Rtt_Lua.h"
#include "Rtt_LuaFrameworks.h"
#include "Rtt_MPlatform.h"
#include "Rtt_MPlatformServices.h"
#include "Rtt_FileSystem.h"
#include "Rtt_DeviceBuildData.h"

#include "ListKeyStore.h"

#include <string>

extern "C"
{
#	include "lfs.h"
}


// ----------------------------------------------------------------------------

namespace Rtt
{

// ----------------------------------------------------------------------------

#define kDefaultNumBytes 1024

AndroidAppPackagerParams::AndroidAppPackagerParams(
	const char* appName,
	const char* versionName,
	const char* identity,
	const char* provisionFile,
	const char* srcDir,
	const char* dstDir,
	const char* sdkRoot,
	TargetDevice::Platform targetPlatform,
	S32 targetVersion,
	const char * customBuildId,
	const char * productId,
	const char * appPackage,
	bool isDistributionBuild,
	const char * keyStore,
	const char * storePassword,
	const char * keyAlias,
	const char * aliasPassword,
    U32 versionCode
)
 :	AppPackagerParams(
		appName, versionName, identity, provisionFile, srcDir, dstDir, sdkRoot,
		targetPlatform, targetVersion, TargetDevice::kAndroidGenericDevice,
		customBuildId, productId, appPackage, isDistributionBuild ),
	fVersionCode(versionCode)
	, fWindowsNonAscii(false)
{
	fKeyStore.Set( keyStore );
	fKeyStorePassword.Set( storePassword );
	fKeyAlias.Set( keyAlias );
	fKeyAliasPassword.Set( aliasPassword );
}

void
AndroidAppPackagerParams::Print()
{
	Super::Print();
	fprintf( stderr,
		"\tKeystore path: '%s'\n"
		"\tKeystore alias: '%s'\n"
		"\tAndroid version code: '%x'\n"
		"\tAndroid app package: '%s'\n",
		GetAndroidKeyStore(),
		GetAndroidKeyAlias(),
		GetVersionCode(),
		GetAppPackage() );
}

	
// ----------------------------------------------------------------------------

// TODO: Move create_build_properties.lua out of librtt and into rtt_player in XCode
// Current issue with doing that is this lua file needs to be precompiled into C
// via custom build step --- all .lua files in librtt already do that, so we're
// taking a shortcut for now by putting it under librtt.

// create_build_properties.lua is pre-compiled into bytecodes and then placed in a byte array
// constant in a generated .cpp file. The file also contains the definition of the
// following function which loads the bytecodes via luaL_loadbuffer.
int luaload_create_build_properties(lua_State* L);

AndroidAppPackager::AndroidAppPackager( const MPlatformServices & services, const char * resourcesDir )
:	PlatformAppPackager( services, TargetDevice::kAndroidPlatform ),
	fResourcesDir( & services.Platform().GetAllocator(), resourcesDir )
{
    Lua::RegisterModuleLoader( fVM, "lfs", luaopen_lfs );
    Lua::RegisterModuleLoader( fVM, "lpeg", luaopen_lpeg ); // json depends on lpeg
	Lua::RegisterModuleLoader( fVM, "dkjson", Lua::Open< luaload_dkjson > );
	Lua::RegisterModuleLoader( fVM, "json", Lua::Open< luaload_json > );

	Lua::DoBuffer( fVM, & luaload_create_build_properties, NULL );
}

AndroidAppPackager::~AndroidAppPackager()
{
}

std::string
AndroidAppPackager::EscapeArgument(std::string arg)
{
	std::string result = arg;

	// On macOS escape shell special characters in the strings by replacing single quotes with "'\''" and
	// then enclosing in single quotes
	ReplaceString(result, "'", "'\\''");	// escape single quotes
	result = "'" + result + "'";
	
	return result;
}

int
AndroidAppPackager::Build( AppPackagerParams * params, const char * tmpDirBase )
{
	int result = PlatformAppPackager::kBuildError;
	time_t startTime = time(NULL);

    const char tmpTemplate[] = "CLtmpXXXXXX";
	char tmpDir[kDefaultNumBytes]; Rtt_ASSERT( kDefaultNumBytes > ( strlen( tmpDirBase ) + strlen( tmpTemplate ) ) );
	snprintf(tmpDir, kDefaultNumBytes, "%s" LUA_DIRSEP "%s", tmpDirBase, tmpTemplate);

    // This is not as foolproof as mkdtemp() but has the advantage of working on Win32
    if ( mkdir( mktemp(tmpDir) ) )
	{
		char* inputFile = Prepackage( params, tmpDir );

		if (inputFile) //offline build
		{
			const AndroidAppPackagerParams * androidParams = (const AndroidAppPackagerParams *)params;

			std::string gradleGo = "cd ";
			gradleGo.append(EscapeArgument(tmpDir));
			gradleGo.append(" && cd template &&");
			
			bool java11Installed = 0 == system("JAVA_VERSION=11 /usr/bin/java -version > /dev/null 2>/dev/null");
			if(java11Installed) {
				gradleGo.append(" ./setup.sh && JAVA_VERSION=11 ./gradlew");
			} else {
				Rtt_TRACE_SIM(("WARNING: Java 11 does not seems to be available. If build fails, install Java 1.11."));
				gradleGo.append(" ./setup.sh && ./gradlew");
			}

			if (androidParams->IsWindowsNonAsciiUser()) {
				std::string gradleDir(tmpDirBase);
				gradleDir += LUA_DIRSEP ".gradle";
				gradleDir = EscapeArgument(gradleDir);

				gradleGo.append(" -g ");
				gradleGo.append(gradleDir);

				gradleGo.append(" -Dgradle.user.home=");
				gradleGo.append(gradleDir);
			}

			gradleGo.append(" buildCoronaApp");
			gradleGo.append(" --no-daemon");
			
			gradleGo.append(" -PconfigureCoronaPlugins=YES");
			gradleGo.append(" -PcoronaBuild=" Rtt_STRING_BUILD);

			if (androidParams->IsWindowsNonAsciiUser()) {
				gradleGo.append(" -PcoronaCustomHome=");
				gradleGo.append(EscapeArgument(tmpDirBase));
			}

			gradleGo.append(" -PcoronaResourcesDir=");
			gradleGo.append(EscapeArgument(fResourcesDir.GetString()));
			
			gradleGo.append(" -PcoronaDstDir=");
			gradleGo.append(EscapeArgument(params->GetDstDir()));
			
			gradleGo.append(" -PcoronaTmpDir=");
			gradleGo.append(EscapeArgument(tmpDir));
			
			gradleGo.append(" -PcoronaSrcDir=");
			gradleGo.append(EscapeArgument(params->GetSrcDir()));
			
			String appFileName;
			PlatformAppPackager::EscapeFileName( params->GetAppName(), appFileName );
			gradleGo.append(" -PcoronaAppFileName=");
			gradleGo.append(EscapeArgument(appFileName.GetString()));
			
			gradleGo.append(" -PcoronaAppPackage=");
			gradleGo.append(EscapeArgument(params->GetAppPackage()));

			gradleGo.append(" -PcoronaVersionCode=");
			gradleGo.append(std::to_string(androidParams->GetVersionCode()));
			
			gradleGo.append(" -PcoronaVersionName=");
			gradleGo.append(EscapeArgument(params->GetVersion()));
			
			gradleGo.append(" -PcoronaKeystore=");
			gradleGo.append(EscapeArgument(androidParams->GetAndroidKeyStore()));
			
			gradleGo.append(" -PcoronaKeystorePassword=");
			gradleGo.append(EscapeArgument(androidParams->GetAndroidKeyStorePassword()));
			
			gradleGo.append(" -PcoronaKeyAlias=");
			gradleGo.append(EscapeArgument(androidParams->GetAndroidKeyAlias()));

			gradleGo.append(" -PcoronaKeyAliasPassword=");
			if(androidParams->GetAndroidKeyAliasPassword()!=NULL)
			{
				gradleGo.append(EscapeArgument(androidParams->GetAndroidKeyAliasPassword()));
			}
			else
			{
				gradleGo.append(EscapeArgument(androidParams->GetAndroidKeyStorePassword()));
			}
			
			DeviceBuildData& deviceBuildData = params->GetDeviceBuildData( fServices.Platform(), fServices );
			String json( & fServices.Platform().GetAllocator() );
			deviceBuildData.GetJSON( json );
			const size_t maxPath = 600;
			char buildDataFileOutput[maxPath+1];
			snprintf(buildDataFileOutput, maxPath, "%s" LUA_DIRSEP "build.data", tmpDir);
			Rtt::Data<const unsigned char> jsonData(& fServices.Platform().GetAllocator());
			jsonData.Set((unsigned char*)json.GetString(), json.GetLength());
			Rtt_WriteDataToFile(buildDataFileOutput, jsonData);

			gradleGo.append(" -PcoronaBuildData=");
			gradleGo.append(EscapeArgument(buildDataFileOutput));
			
			gradleGo.append(" --console=plain -q");
			
			// Obfuscate passwords
			std::string placeHolder = EscapeArgument("XXXXXX");
			std::string sanitizedCmdBuf = gradleGo;
			
			std::string keystorePasswordStr = EscapeArgument(androidParams->GetAndroidKeyStorePassword());
			if (keystorePasswordStr.length() > 0)
			{
				ReplaceString(sanitizedCmdBuf, keystorePasswordStr, placeHolder);
			}
			
			std::string keyaliasPasswordStr;
			if (androidParams->GetAndroidKeyAliasPassword() != NULL)
			{
				keyaliasPasswordStr = EscapeArgument(androidParams->GetAndroidKeyAliasPassword());
			}
			if (keyaliasPasswordStr.length() > 0)
			{
				ReplaceString(sanitizedCmdBuf, keyaliasPasswordStr, placeHolder);
			}
			
			Rtt_Log("Build: running: %s\n", sanitizedCmdBuf.c_str());

			gradleGo.append(" < /dev/null ");

#if !defined(Rtt_NO_GUI)
			gradleGo.insert(0, "(");
			gradleGo.append(") > ");
			std::string gradleLogFile(tmpDir);
			gradleLogFile.append(LUA_DIRSEP);
			gradleLogFile.append("gradleLogFile.log");
			gradleGo.append(EscapeArgument(gradleLogFile.c_str()));
			gradleGo.append(" 2>&1 ");
#endif

			result = system(gradleGo.c_str());

#if !defined(Rtt_NO_GUI)
			FILE *log = Rtt_FileOpen(gradleLogFile.c_str(), "rb");
			if (log) {
				Rtt_FileSeek(log, 0, SEEK_END);
				long sz = Rtt_FileTell(log);
				Rtt_FileSeek(log, 0, SEEK_SET);
				char* buf = new char[sz + 1];
				long read = Rtt_FileRead(buf, sizeof(char), sz, log);
				buf[read] = 0;
				Rtt_Log("%s", buf);
				delete[] buf;
				Rtt_FileClose(log);
			}
			else 
			{
				Rtt_Log("%s", "Unable to open build log file");
			}
#endif
		}
		
		// Clean up intermediate files
		String retainTmpDirStr;
		fServices.GetPreference( "retainBuildTmpDir", &retainTmpDirStr );
		if(Rtt_StringCompare(retainTmpDirStr, "1") != 0) {
        	rmdir( tmpDir );
		}
	}
	else
	{
		// Note that the failing mkdir() that brought us here is a member of the AndroidAppPackager class
		String tmpString;

		tmpString.Set("AndroidAppPackager::Build: failed to create temporary directory\n\n");
		tmpString.Append(tmpDir);
		tmpString.Append("\n");

		Rtt_TRACE_SIM( ( "%s", tmpString.GetString() ) );
		params->SetBuildMessage(tmpString.GetString());
	}

    // Indicate status in the console
    if (PlatformAppPackager::kNoError == result)
    {
		// This isn't an exception but Rtt_Log() is only defined for debug builds
        Rtt_LogException("Android build succeeded in %ld seconds", (time(NULL) - startTime));
    }
    else
    {
		Rtt_LogException("Android build failed (%d) after %ld seconds", result, (time(NULL) - startTime));
    }

	return result;
}

bool
AndroidAppPackager::VerifyConfiguration() const
{
	// TODO: Add code to check existence of various utilities
	return true;
}

/// Called when the "build.settings" file is being read.
/// @param L Pointer to the Lua state that has loaded the build settings table.
/// @param index The index to the "settings" table in the Lua stack.
void
AndroidAppPackager::OnReadingBuildSettings( lua_State *L, int index )
{

}

bool
AndroidAppPackager::CreateBuildProperties( const AppPackagerParams& params, const char *tmpDir )
{
	lua_State *L = fVM;
	lua_getglobal( L, "androidCreateProperties" ); Rtt_ASSERT( lua_isfunction( L, -1 ) );
	lua_pushstring( L, tmpDir );
	lua_pushstring( L, params.GetAppPackage() );
	lua_pushstring( L, params.GetSrcDir() );
	lua_pushinteger( L, ((AndroidAppPackagerParams&)params).GetVersionCode() );
	lua_pushstring( L, params.GetVersion() );
	lua_pushstring( L, params.GetAppName() );

	bool result = Rtt_VERIFY( 0 == Lua::DoCall( L, 7, 1 ) );
	if ( ! lua_isnil( L, -1 ) )
	{
		Rtt_TRACE_SIM( ( "ERROR: Could not create build.properties:\n\t%s\n", lua_tostring( L, -1 ) ) );
		result = false;
	}
	return result;
}

char *
AndroidAppPackager::Prepackage( AppPackagerParams * params, const char * tmpDir )
{
	char* result = NULL;
	int iresult = -1;

	Rtt_ASSERT( params );

	// Convert build.settings into build.properties
	// And run Android specific pre package script
	Rtt_Log("Prepackage: Compiling Lua ...");
	
	if ( CompileScripts( params, tmpDir ) && CreateBuildProperties( * params, tmpDir ) )
	{
		std::string javaCmd = "/usr/bin/java";

		const char kCmdFormat[] = "\"%s\" -Djava.class.path=%s org.apache.tools.ant.launch.Launcher %s -DTEMP_DIR=%s -DSRC_DIR=%s -DBUNDLE_DIR=%s -f %s/build.xml build-input-zip";

		char cmdBuf[20480];

		std::string jarPathStr;
		std::string tmpDirStr = EscapeArgument(tmpDir);
		std::string srcDirStr = EscapeArgument(params->GetSrcDir());
		std::string resourcesDirStr = EscapeArgument(fResourcesDir.GetString());

		jarPathStr.append(fResourcesDir.GetString());
		jarPathStr.append("/");
		jarPathStr.append("ant.jar");

		jarPathStr.append(":");
		jarPathStr.append(fResourcesDir.GetString());
		jarPathStr.append("/");
		jarPathStr.append("ant-launcher.jar" );

		jarPathStr.append(":");
		jarPathStr.append(fResourcesDir.GetString());
		jarPathStr.append("/");
		jarPathStr.append("AntLiveManifest.jar");
		jarPathStr = EscapeArgument(jarPathStr);

		const char *antDebugFlag = "-d";

		snprintf(cmdBuf, sizeof(cmdBuf), kCmdFormat,
				 javaCmd.c_str(),
				 jarPathStr.c_str(),
				 antDebugFlag,
				 tmpDirStr.c_str(),
				 srcDirStr.c_str(),
				 resourcesDirStr.c_str(),
				 resourcesDirStr.c_str() );
		Rtt_Log("Prepackage: running: %s\n", cmdBuf);

		iresult = system( cmdBuf );
	
		if ( iresult == 0 )
		{
			const char kDstName[] = "input.zip";
			size_t resultLen = strlen( tmpDir ) + strlen( kDstName ) + sizeof( LUA_DIRSEP );
			result = (char *) malloc( resultLen + 1 );
			snprintf( result, resultLen, "%s" LUA_DIRSEP "%s", tmpDir, kDstName );
		}
	}

	return result;
}


// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------

