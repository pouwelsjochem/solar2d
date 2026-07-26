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

namespace Rtt
{

class Runtime;

namespace SimulatorControl
{
	void SetDirectory( const char *directory );
	void Process( Runtime& runtime );
	void RecordRuntimeError(
		Runtime& runtime, const char *errorType,
		const char *message, const char *stackTrace );
	void Shutdown( Runtime& runtime );
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
