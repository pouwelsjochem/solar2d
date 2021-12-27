//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Rtt_TargetDevice.h"

namespace Interop
{
	class SimulatorRuntimeEnvironment;
}
namespace Rtt
{
	class MouseEvent;
	class WebServicesSession;
	class WinSimulator;
	struct SimulatorOptions;
};

void appNxSBuild(Interop::SimulatorRuntimeEnvironment* pSim,
						const char* srcDir,
						const char* nmetaPath,
						const char* applicationName, const char* versionName,
						const char* dstDir,
						const Rtt::TargetDevice::Platform targetPlatform,
						const char* targetos, bool isDistribution, int versionCode);

void appEndNativeAlert(void *pLuaResource, int nButtonIndex, bool bCanceled);
