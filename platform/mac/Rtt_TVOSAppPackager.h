//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Solar2D game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@Solar2D.com
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _Rtt_TVOSAppPackager_H__
#define _Rtt_TVOSAppPackager_H__

#include "Rtt_AppleMobileAppPackager.h"

namespace Rtt
{

typedef AppleMobileAppPackagerParams TVOSAppPackagerParams;

class TVOSAppPackager : public AppleMobileAppPackager
{
	public:
		TVOSAppPackager(const MPlatformServices& services);
		virtual ~TVOSAppPackager();
};

} // namespace Rtt

#endif // _Rtt_TVOSAppPackager_H__
