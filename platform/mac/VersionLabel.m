//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#import "VersionLabel.h"
#include "Core/Rtt_Version.h"

@implementation VersionLabel

- (void) awakeFromNib
{
	[self setStringValue:[NSString stringWithExternalString:Rtt_STRING_HUMAN_FRIENDLY_VERSION]];
}

@end
