//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Solar2D game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@Solar2D.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Rtt_IOSAppPackager.h"

namespace Rtt
{

int luaload_AppleMobilePackageApp(lua_State* L);

IOSAppPackager::IOSAppPackager(const MPlatformServices& services)
:	AppleMobileAppPackager(
		services,
		TargetDevice::kIPhonePlatform,
		&luaload_AppleMobilePackageApp,
		"ios")
{
}

IOSAppPackager::~IOSAppPackager()
{
}

} // namespace Rtt
