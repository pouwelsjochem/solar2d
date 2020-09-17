//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Rtt_MPlatform.h"


#pragma region Forward Declarations
namespace Rtt
{
	class LuaResource;
}

#pragma endregion


namespace Interop {

class MDeviceSimulatorServices
{
	public:
		virtual ~MDeviceSimulatorServices() {}

		virtual bool IsLuaExitAllowed() const = 0;
		virtual void* ShowNativeAlert(
						const char *title, const char *message, const char **buttonLabels,
						int buttonCount, Rtt::LuaResource* resource) = 0;
		virtual void CancelNativeAlert(void* alertReference) = 0;
		virtual void RequestRestart() = 0;
		virtual void RequestTerminate() = 0;

};

}	// namespace Interop
