//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Solar2D game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@Solar2D.com
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _Rtt_IOSAppPackager_H__
#define _Rtt_IOSAppPackager_H__

#include "Rtt_AppleMobileAppPackager.h"

namespace Rtt
{

typedef AppleMobileAppPackagerParams IOSAppPackagerParams;

class IOSAppPackager : public AppleMobileAppPackager
{
	public:
		IOSAppPackager(const MPlatformServices& services);
		virtual ~IOSAppPackager();
};

} // namespace Rtt

#endif // _Rtt_IOSAppPackager_H__
