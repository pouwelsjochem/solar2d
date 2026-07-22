//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Solar2D game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@Solar2D.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Core/Rtt_Build.h"

#include "Rtt_AppleMobileAppPackager.h"

#include "Rtt_Lua.h"
#include "Rtt_LuaFrameworks.h"
#include "Rtt_MPlatform.h"
#include "Rtt_MPlatformServices.h"
#include "Rtt_FileSystem.h"
#include "Rtt_DeviceBuildData.h"
#include "Rtt_HTTPClient.h"
#include "NSString+Extensions.h"
#include "XcodeToolHelper.h"

#include <string.h>
#include <time.h>

// ----------------------------------------------------------------------------

namespace Rtt
{

int luaload_CoronaPListSupport(lua_State* L);
int luaload_CoronaOfflineiOSPackager(lua_State* L);
int luaload_IOSPackageSupport(lua_State* L);
int luaload_TVOSPackageSupport(lua_State* L);

// ----------------------------------------------------------------------------

void
AppleMobileAppPackagerParams::Print()
{
	Super::Print();
	fprintf( stderr,
		"\tSDK path: '%s'\n"
		"\tCertificate path: '%s'\n"
		"\tSigning identity: '%s'\n"
		"\tBundle id: '%s'\n"
		"\tBuildSettingsPath: '%s'\n",
		GetSdkRoot(),
		GetProvisionFile(),
		GetIdentity(),
		GetAppPackage(),
		GetBuildSettingsPath() );
}

// ----------------------------------------------------------------------------

#define kDefaultNumBytes 1024

AppleMobileAppPackager::AppleMobileAppPackager(
	const MPlatformServices& services,
	TargetDevice::Platform targetPlatform,
	LuaLoader packageLoader,
	const char* platformName)
:	PlatformAppPackager(services, targetPlatform),
	fTargetPlatform(targetPlatform),
	fPlatformName(platformName)
{
	lua_State *L = fVM;

	Lua::RegisterModuleLoader( L, "CoronaPListSupport", Lua::Open< luaload_CoronaPListSupport > );
	Lua::RegisterModuleLoader(L, "IOSPackageSupport", Lua::Open< luaload_IOSPackageSupport >);
	Lua::RegisterModuleLoader(L, "TVOSPackageSupport", Lua::Open< luaload_TVOSPackageSupport >);
	HTTPClient::registerFetcherModuleLoaders(L);
	Lua::DoBuffer(fVM, packageLoader, NULL);

	lua_getglobal(L, "configureAppleMobilePackager");
	Rtt_ASSERT(lua_isfunction(L, -1));
	lua_pushstring(L, platformName);
	lua_pushstring(L, targetPlatform == TargetDevice::kTVOSPlatform
		? Rtt_MACRO_TO_STRING(S2D_MIN_VER_TVOS)
		: Rtt_MACRO_TO_STRING(S2D_MIN_VER_IOS));
	(void)Rtt_VERIFY(0 == Lua::DoCall(L, 2, 0));
}

AppleMobileAppPackager::~AppleMobileAppPackager()
{
}

int
AppleMobileAppPackager::Build( AppPackagerParams * params, const char* tmpDirBase )
{
	int result = PlatformAppPackager::kBuildError;
	const AppleMobileAppPackagerParams* mobileParams = static_cast<const AppleMobileAppPackagerParams*>(params);
	time_t startTime = time(NULL);

	const char tmpTemplate[] = "CLtmpXXXXXX";
	char tmpDir[kDefaultNumBytes];
	int tmpDirLen = snprintf(tmpDir, sizeof(tmpDir), "%s%s", tmpDirBase, tmpTemplate);
	if (tmpDirLen < 0 || tmpDirLen >= sizeof(tmpDir))
	{
		params->SetBuildMessage("Temporary directory path is too long");
		return result;
	}

	if (Rtt_VERIFY(NULL != mkdtemp(tmpDir)))
	{
		char* inputFile = Prepackage( params, tmpDir );

		if ( inputFile )
		{
			const char kOutputName[] = "output.zip";
			size_t tmpDirLen = strlen( tmpDir );
			size_t outputFileLen = tmpDirLen + sizeof(kOutputName) + sizeof( LUA_DIRSEP );
			char* outputFile = (char*)malloc( outputFileLen );
			snprintf(outputFile, outputFileLen, "%s" LUA_DIRSEP "%s", tmpDir, kOutputName);

			{
				lua_State *L = fVM;
				
				Lua::DoBuffer( L, & luaload_CoronaOfflineiOSPackager, NULL);
				lua_getglobal(L, "CreateOfflinePackage");
				lua_newtable( L );
				
				lua_pushstring(L, tmpDir);
				lua_setfield(L, -2, "tmpDir");
				
				lua_pushstring(L, outputFile);
				lua_setfield(L, -2, "outputFile");
				
				lua_pushstring(L, inputFile);
				lua_setfield(L, -2, "inputFile");
				
				const char *platform, *modernPlatform, *pluginPlatform;
				bool isAppleTV = false;
				switch (params->GetTargetDevice()) {
					case TargetDevice::kIPhone:
					case TargetDevice::kIPad:
					case TargetDevice::kIOSUniversal:
						platform = "iphoneos";
						modernPlatform = "ios";
						pluginPlatform = "iphone";
						break;
					case TargetDevice::kAppleTV:
						platform = "appletvos";
						modernPlatform = "tvos";
						pluginPlatform = "appletvos";
						isAppleTV = true;
						break;
					case TargetDevice::kIPhoneXCodeSimulator:
					case TargetDevice::kIPadXCodeSimulator:
					case TargetDevice::kIOSUniversalXCodeSimulator:
						platform = "iphonesimulator";
						modernPlatform = "ios-sim";
						pluginPlatform = "iphone-sim";
						break;
					case TargetDevice::kTVOSXCodeSimulator:
						platform = "appletvsimulator";
						modernPlatform = "tvos-sim";
						pluginPlatform = "appletvsimulator";
						isAppleTV = true;
						break;
					default:
						Rtt_ASSERT(0);
						platform = "iphoneos";
						modernPlatform = "ios";
						pluginPlatform = "iphone";
						break;
				}
				
				lua_pushstring(L, platform);
				lua_setfield(L, -2, "platform");
				lua_pushstring(L, modernPlatform);
				lua_setfield(L, -2, "modernPlatform");
				lua_pushstring(L, pluginPlatform);
				lua_setfield(L, -2, "pluginPlatform");
				lua_pushboolean(L, isAppleTV);
				lua_setfield(L, -2, "isAppleTV");
				
				lua_pushstring(L, Rtt_MACRO_TO_STRING( Rtt_BUILD_REVISION ) );
				lua_setfield(L, -2, "build");
				
				char templateZip[255];
				setlocale(LC_NUMERIC, "en_US");
				snprintf(templateZip, 255, "%s_%.1f%s.tar.bz", platform, params->GetTargetVersion()/10000.0f, params->GetCustomTemplate());
				lua_pushstring(L, templateZip);
				lua_setfield(L, -2, "template");
				
				Rtt::String resourceDir;
				fServices.Platform().PathForFile(NULL, MPlatform::kSystemResourceDir, 0, resourceDir);
				lua_pushstring(L, resourceDir.GetString());
				lua_setfield(L, -2, "resourceDir");

				lua_pushstring(L, params->GetAppPackage());
				lua_setfield(L, -2, "appPackage");
				
				
				DeviceBuildData & buildData = params->GetDeviceBuildData(fServices.Platform(), fServices);
				Rtt::String json;
				buildData.GetJSON(json);
				lua_pushstring(L, json.GetString());
				lua_setfield(L, -2, "buildData");
				
				String escapedAppName;
                PlatformAppPackager::EscapeFileName( params->GetAppName(), escapedAppName );
                lua_pushstring(L, escapedAppName.GetString());
                lua_setfield(L, -2, "appName");
                
				lua_pushstring(L, params->GetCoronaUser());
				lua_setfield(L, -2, "user");
				
				if ( Rtt_VERIFY( 0 == Lua::DoCall( L, 1, 1 ) ) )
				{
					result = PlatformAppPackager::kNoError;
					if ( lua_isstring( L, -1 ) )
					{
						Rtt_TRACE_SIM( ( "BUILD ERROR: %s\n", lua_tostring( L, -1 ) ) );
						params->SetBuildMessage( lua_tostring( L, -1 ) );
						result = PlatformAppPackager::kBuildError;
					}
				}
				else
				{
					result = PlatformAppPackager::kLocalPackagingError;
				}
				lua_pop( L, 1 );
			}
			if ( PlatformAppPackager::kNoError == result )
			{
				lua_State *L = fVM;
				lua_getglobal( L, "AppleMobilePostPackage" ); Rtt_ASSERT( lua_isfunction( L, -1 ) );

				// params
				lua_newtable( L );
				{
					lua_pushstring( L, mobileParams->GetSrcDir() );
					lua_setfield( L, -2, "srcAssets" );
					
					lua_pushstring( L, tmpDir );
					lua_setfield( L, -2, "tmpDir" );

					lua_pushstring( L, mobileParams->GetDstDir() );
					lua_setfield( L, -2, "dstDir" );

                    String sanitizedName;
					PlatformAppPackager::EscapeFileName(mobileParams->GetAppName(), sanitizedName);
					lua_pushstring( L, sanitizedName.GetString() );
					lua_setfield( L, -2, "dstFile" );

					lua_pushstring( L, mobileParams->GetAppName() );
					lua_setfield( L, -2, "bundledisplayname" );

					lua_pushstring( L, mobileParams->GetVersion() );
					lua_setfield( L, -2, "bundleversion" );

					lua_pushstring( L, mobileParams->GetProvisionFile() );
					lua_setfield( L, -2, "provisionFile" );

					lua_pushstring( L, mobileParams->GetIdentity() );
					lua_setfield( L, -2, "signingIdentity" );

					lua_pushstring( L, mobileParams->GetSdkRoot() );
					lua_setfield( L, -2, "sdkRoot" );

					lua_pushinteger( L, mobileParams->GetTargetDevice() );
					lua_setfield( L, -2, "targetDevice" );

					lua_pushstring( L, TargetDevice::StringForPlatform( mobileParams->GetTargetPlatform() ) );
                    lua_setfield( L, -2, "targetPlatform" );

					// By default, assumes ARM architecture, so we need to override
					// when building for Xcode simulator
					bool isSimulator = ( params->GetTargetDevice() >= TargetDevice::kXCodeSimulator );
					if ( isSimulator )
					{
						lua_pushstring(L, fTargetPlatform == TargetDevice::kTVOSPlatform ? "appletvsimulator" : "iphonesimulator");
						lua_setfield( L, -2, "sdkType" );
					}

                    lua_newtable(L);
                    {
						NSString* codesign = [XcodeToolHelper pathForCodesign];
                        NSString* codesign_allocate = [XcodeToolHelper pathForCodesignAllocate];
						NSString *codesign_framework = [XcodeToolHelper pathForCodesignFramework];

						lua_pushstring( L, [codesign UTF8String] );
                        lua_setfield( L, -2, "codesign" );
                        
                        lua_pushstring( L, [codesign_allocate UTF8String] );
                        lua_setfield( L, -2, "codesign_allocate" );

						lua_pushstring( L, [codesign_framework UTF8String] );
						lua_setfield( L, -2, "codesign_framework" );
                    }
                    lua_setfield( L, -2, "xcodetoolhelper" );
				}

				// AppleMobilePostPackage(params)
				if ( ! Rtt_VERIFY( 0 == Lua::DoCall( L, 1, 1 ) ) )
				{
					// The packaging script failed to compile
					result = PlatformAppPackager::kLocalPackagingError;
				}
				else
				{
					if ( lua_isstring( L, -1 ) )
					{
						result = PlatformAppPackager::kLocalPackagingError;
						Rtt_TRACE_SIM( ( "BUILD ERROR: %s\n", lua_tostring( L, -1 ) ) );
                        params->SetBuildMessage( lua_tostring( L, -1 ) );
					}
					lua_pop( L, 1 );
				}

				/*
				// Obtain output.zip file from server 
				const char kCmdFormat[] = "/Volumes/rtt/bin/mac/app_sign/build_output.sh %s %s %s %s %s little";
				char cmd[kDefaultNumBytes + sizeof(kCmdFormat) + strlen( params.appName ) + sizeof(tmpDir) + sizeof() ];
				const char kTemplateFile[] = "/Volumes/rtt/bin/iphone/template.app";
				const char kCertificateFile[] = "/Volumes/rtt/bin/mac/app_sign/build/Debug/foo";
				sprintf(
					cmd,
					kCmdFormat,
					params.appName, kTemplateFile, tmpDir, kCertificateFile, );
				system(  );
				*/
			}
            else
            {
                if (params->GetBuildMessage() == NULL)
                {
                    // If we don't already have a more precise error, use the XMLRPC layer's error message
                    params->SetBuildMessage( "Unknown error" );
                }
            }

			free( outputFile );
			free( inputFile );
		}

		// Clean up intermediate files
		char cmd[kDefaultNumBytes];
		snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", tmpDir);
		(void)Rtt_VERIFY( 0 == system( cmd ) );
	}

	// Indicate status in the console
    if (PlatformAppPackager::kNoError == result)
    {
		Rtt_LogException("%s build succeeded in %ld seconds", fPlatformName, (time(NULL) - startTime));
    }
    else
    {
		Rtt_LogException("%s build failed (%d) after %ld seconds", fPlatformName, result, (time(NULL) - startTime));
    }

	return result;
}

const char *
AppleMobileAppPackager::GetBundleId( const char *provisionFile, const char *appName ) const
{
	const char *result = NULL;

	lua_State *L = fVM;
	lua_getglobal( L, "getBundleId" ); Rtt_ASSERT( lua_isfunction( L, -1 ) );
	lua_pushstring( L, provisionFile );
	lua_pushstring( L, PlatformAppPackager::EscapeStringForIOS( appName ) );
	lua_pushstring( L, appName );

	if ( Rtt_VERIFY( 0 == Lua::DoCall( L, 3, 1 ) ) )
	{
		const char *s = lua_tostring( L, -1 );
		if ( s )
		{
			// NOTE: This is a slimey trick to guarantee the C-string is valid for use by the caller
			// 'str' is auto-released so the result of [str UTF8String] is valid until the auto-released
			NSString *str = [NSString stringWithExternalString:s];
			result = [str UTF8String];
		}

		lua_pop( L, 1 );
	}

	return result;
}

bool
AppleMobileAppPackager::VerifyConfiguration() const
{
	// Add code to check existence of various utilities
	return true;
}

char* 
AppleMobileAppPackager::Prepackage( AppPackagerParams * params, const char* tmpDir )
{
	char* result = NULL;

	// Copy provision file (if we have one) into tmpDir
	if ( params->GetProvisionFile() )
	{
		bool retflag = CopyProvisionFile( params, tmpDir );
		if ( false == retflag )
		{
			return result;
		}
	}

	// Then Prepackage like before
	result = Super::Prepackage( params, tmpDir );
	
	return result;
}

bool
AppleMobileAppPackager::CopyProvisionFile( const AppPackagerParams * params, const char* tmpDir )
{
	const char kDstName[] = "embedded.mobileprovision";
	String dstFileStr;

	dstFileStr.Append(tmpDir);
	dstFileStr.Append(LUA_DIRSEP);
	dstFileStr.Append(kDstName);

	return Rtt_CopyFile( params->GetProvisionFile(), dstFileStr.GetString() );
}

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------
