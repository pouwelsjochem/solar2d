//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#import <Cocoa/Cocoa.h>

#include "Rtt_SimulatorControl.h"

int main(int argc, char *argv[])
{
    int exitCode = 0;
    if (Rtt_RunSimulatorControlClient(argc, (const char * const *)argv, &exitCode))
    {
        return exitCode;
    }
    return NSApplicationMain(argc,  (const char **) argv);
}
