//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Solar2D game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@Solar2D.com
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _Rtt_AppleMobileAppPackager_H__
#define _Rtt_AppleMobileAppPackager_H__

#include "Rtt_PlatformAppPackager.h"

struct lua_State;

namespace Rtt
{

class MPlatformServices;

class AppleMobileAppPackagerParams : public AppPackagerParams
{
	public:
		typedef AppPackagerParams Super;

	public:
		AppleMobileAppPackagerParams(
			const char* appName,
			const char* version,
			const char* identity,
			const char* provisionFile,
			const char* srcDir,
			const char* dstDir,
			const char* sdkRoot,
			TargetDevice::Platform targetPlatform,
			S32 targetVersion,
			S32 targetDevice,
			const char* customBuildId,
			const char* productId,
			const char* appPackage,
			bool isDistributionBuild)
		: AppPackagerParams(
			appName, version, identity, provisionFile, srcDir, dstDir, sdkRoot,
			targetPlatform, targetVersion, targetDevice, customBuildId, productId,
			appPackage, isDistributionBuild)
		{
		}

	public:
		virtual void Print();
};

class AppleMobileAppPackager : public PlatformAppPackager
{
	public:
		typedef PlatformAppPackager Super;
		typedef int (*LuaLoader)(lua_State* L);

	protected:
		AppleMobileAppPackager(
			const MPlatformServices& services,
			TargetDevice::Platform targetPlatform,
			LuaLoader packageLoader,
			const char* platformName);

	public:
		virtual ~AppleMobileAppPackager();

	public:
		virtual int Build(AppPackagerParams* params, const char* tmpDirBase);
		virtual bool VerifyConfiguration() const;
		const char* GetBundleId(const char* provisionFile, const char* appName) const;

	protected:
		virtual char* Prepackage(AppPackagerParams* params, const char* tmpDir);
		bool CopyProvisionFile(const AppPackagerParams* params, const char* tmpDir);

	private:
		TargetDevice::Platform fTargetPlatform;
		const char* fPlatformName;
};

} // namespace Rtt

#endif // _Rtt_AppleMobileAppPackager_H__
