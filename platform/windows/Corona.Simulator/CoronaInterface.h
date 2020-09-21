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

void appEndNativeAlert(void *pLuaResource, int nButtonIndex, bool bCanceled);
