//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#ifndef __Rtt_SimulatorControl__
#define __Rtt_SimulatorControl__

#ifdef __cplusplus

#include "Rtt_MSimulatorHost.h"

struct lua_State;

namespace Rtt
{

class Runtime;

namespace SimulatorControl
{
	class InputDispatchGuard
	{
		public:
			explicit InputDispatchGuard( Runtime& runtime );
			~InputDispatchGuard();

		private:
			InputDispatchGuard( const InputDispatchGuard& );
			InputDispatchGuard& operator=( const InputDispatchGuard& );

			const Runtime *fRuntime;
	};

	void SetDirectory( const char *directory );
	void Process( Runtime& runtime );
	bool ShouldRunRuntimeFrame( Runtime& runtime );
	bool CanDispatchApplicationEvent( Runtime& runtime );
	void RecordRuntimeError(
		Runtime& runtime, lua_State *L, const char *errorType,
		const char *message, const char *stackTrace );
	void HaltOnRuntimeError( Runtime& runtime );
	bool IsRuntimeErrorHalted( Runtime& runtime );
	void Shutdown( Runtime& runtime );
	bool DispatchControllerInput(
		Runtime& runtime, const MSimulatorHost::Input& input );
}

} // namespace Rtt

#endif

#ifdef __cplusplus
extern "C" {
#endif

// Runs the Simulator executable in control-client mode when the command line
// contains "-simulator-control". Returns non-zero when the command was handled
// and writes the process exit code to outExitCode.
int Rtt_RunSimulatorControlClient(
	int argc, const char * const argv[], int *outExitCode );

#ifdef __cplusplus
}
#endif

#endif // __Rtt_SimulatorControl__
