//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Solar2D game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@Solar2D.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Rtt_TVOSAppPackager.h"

namespace Rtt
{

int luaload_AppleMobilePackageApp(lua_State* L);

TVOSAppPackager::TVOSAppPackager(const MPlatformServices& services)
:	AppleMobileAppPackager(
		services,
		TargetDevice::kTVOSPlatform,
		&luaload_AppleMobilePackageApp,
		"tvos")
{
}

TVOSAppPackager::~TVOSAppPackager()
{
}

} // namespace Rtt
