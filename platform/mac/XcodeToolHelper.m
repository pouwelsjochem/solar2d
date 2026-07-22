//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Core/Rtt_Build.h"
#include "Core/Rtt_Assert.h"

#import "XcodeToolHelper.h"

#import <AppKit/AppKit.h>


@implementation XcodeToolHelper

static const NSString* kXcodeToolHelperUserDefaultsPrefix = @"XcodeOverrideTool_";

+ (NSString*) toolLocationFromPreferences:(NSString*)toolbasename printWarning:(BOOL)should_print_warning
{
	// toolpath is the full path including the executable itself
	// Preference convention is to make all keys have the prefix: XcodeOverrideTool_ followed by the tool name
	NSString* key = [kXcodeToolHelperUserDefaultsPrefix stringByAppendingString:toolbasename];
	NSString* toolpath = [[NSUserDefaults standardUserDefaults] objectForKey:key];

	if ( nil != toolpath && should_print_warning )
	{
		NSLog(@"Note: '%@' location has been overridden in Preferences:\n\t%@ = %@", toolbasename, key, toolpath);
	}
	
	return toolpath;
}

+ (void) printNotFoundWarningForTool:(NSString*)toolbasename
{
	NSLog(@"Warning: Could not find Xcode build tool: %@\n - perhaps Xcode isn't installed", toolbasename);
}

//
// Return the path for codesign_allocate
//
+ (NSString*) pathForCodesignAllocateUsingDeveloperBase:(NSString*)developerbase printWarning:(BOOL)should_print_warning
{
	// User preferences always overrides.
	NSString* toolpath = [XcodeToolHelper toolLocationFromPreferences:@"codesign_allocate" printWarning:should_print_warning];

	//  Don't do any validation for overrides because the only reason overrides are in effect is to force work around problems.
	if ( nil != toolpath )
	{
		return toolpath;
	}

	// No override in effect. Do the normal thing.
	toolpath = [XcodeToolHelper findXcodePathFor:@"codesign_allocate"];

	if ([toolpath length] == 0)
	{
		// If xcrun can't find "codesign_allocate", this is the best bet
		toolpath = [developerbase stringByAppendingPathComponent:@"Platforms/iPhoneOS.platform/Developer/usr/bin/codesign_allocate"];
	}

	if (should_print_warning && ! [[NSFileManager defaultManager] fileExistsAtPath:toolpath])
	{
		toolpath = nil;

		[XcodeToolHelper printNotFoundWarningForTool:@"codesign_allocate"];
	}

	return toolpath;
}

//
// Return the path for productbuild
//
+ (NSString*) pathForProductBuildUsingDeveloperBase:(NSString*)developerbase printWarning:(BOOL)should_print_warning
{
	// User preferences always overrides.
	NSString* toolpath = [XcodeToolHelper toolLocationFromPreferences:@"productbuild" printWarning:should_print_warning];

	//  Don't do any validation for overrides because the only reason overrides are in effect is to force work around problems.
	if ( nil != toolpath )
	{
		return toolpath;
	}

	// No override in effect. Do the normal thing.
	toolpath = [XcodeToolHelper findXcodePathFor:@"productbuild"];

	if (should_print_warning && ! [[NSFileManager defaultManager] fileExistsAtPath:toolpath])
	{
		toolpath = nil;

		[XcodeToolHelper printNotFoundWarningForTool:@"productbuild"];
	}

	return toolpath;
}

//
// Return the path for copypng
//
+ (NSString*) pathForCopyPngUsingDeveloperBase:(NSString*)developerbase printWarning:(BOOL)should_print_warning
{
	// User preferences always overrides.
	NSString* toolpath = [XcodeToolHelper toolLocationFromPreferences:@"copypng" printWarning:should_print_warning];
	
	//  Don't do any validation for overrides because the only reason overrides are in effect is to force work around problems.
	if ( nil != toolpath )
	{
		return toolpath;
	}
	
	// No override in effect. Don't use [XcodeToolHelper findXcodePathFor:] because we want to suppress the warning
	toolpath = [self launchTaskAndReturnOutput:@"/usr/bin/xcrun" arguments:@[@"--find", @"copypng"] printWarning:NO];
	
	if ([toolpath length] == 0)
	{
		// Xcode prior to 7.0 doesn't configure "copypng" as an xcrun tool but all the versions we care about have it here
		toolpath = [developerbase stringByAppendingPathComponent:@"Platforms/iPhoneOS.platform/Developer/usr/bin/copypng"];
	}
	
	if (should_print_warning && ! [[NSFileManager defaultManager] fileExistsAtPath:toolpath])
	{
		toolpath = nil;
		
		[XcodeToolHelper printNotFoundWarningForTool:@"copypng"];
	}
	
	return toolpath;
}

//
// Return the path for codesign
//
+ (NSString*) pathForCodesignUsingDeveloperBase:(NSString*)developerbase printWarning:(BOOL)should_print_warning
{
	// User preferences always overrides.
	NSString* toolpath = [XcodeToolHelper toolLocationFromPreferences:@"codesign" printWarning:should_print_warning];

	//  Don't do any validation for overrides because the only reason overrides are in effect is to force work around problems.
	if ( nil != toolpath )
	{
		return toolpath;
	}

	// No override in effect. Do the normal thing.
	toolpath = @"/usr/bin/codesign";

	if ( NO == [[NSFileManager defaultManager] fileExistsAtPath:toolpath] )
	{
		toolpath = nil;
		if ( should_print_warning )
		{
			[XcodeToolHelper printNotFoundWarningForTool:@"codesign"];
		}
	}

	return toolpath;
}

//
// Return the path for Application Loader
//
+ (NSString*) pathForApplicationLoaderUsingDeveloperBase:(NSString*)developerbase printWarning:(BOOL)should_print_warning
{
	// User preferences always overrides.
	NSString* toolpath = [XcodeToolHelper toolLocationFromPreferences:@"applicationloader" printWarning:should_print_warning];

	//  Don't do any validation for overrides because the only reason overrides are in effect is to force work around problems.
	if ( nil != toolpath )
	{
		return toolpath;
	}

	// No override in effect. Do the normal thing.
	toolpath = [self getXcodePath];
	toolpath = [toolpath stringByAppendingPathComponent:@"../Applications/Application Loader.app"];

	if ( NO == [[NSFileManager defaultManager] fileExistsAtPath:toolpath] )
	{
		toolpath = nil;
		if ( should_print_warning )
		{
			[XcodeToolHelper printNotFoundWarningForTool:@"Application Loader"];
		}
	}

	return toolpath;
}

//
// Find the Xcode "developer root" using xcode-select
//
+ (NSString*) getXcodePath
{
	return [self launchTaskAndReturnOutput:@"/usr/bin/xcode-select" arguments:@[@"-print-path"] printWarning:YES];
}

//
// Find the Xcode path (if any) for a utility using xcrun
//
+ (NSString *) findXcodePathFor:(NSString *)cmd
{
	return [self launchTaskAndReturnOutput:@"/usr/bin/xcrun" arguments:@[@"--find", cmd] printWarning:YES];
}

+ (NSString*) pathForResources
{
	return [[NSBundle mainBundle] resourcePath];
}

+ (NSString*) pathForCodesignFramework
{
	return [[XcodeToolHelper pathForResources] stringByAppendingPathComponent:@"codesign-framework.sh"];
}


//
// Launch a command and capture its output and return it as a string
//
+ (NSString *) launchTaskAndReturnOutput:(NSString *)cmd arguments:(NSArray *)args printWarning:(BOOL)printWarning
{
	NSString *result = nil;
	NSMutableData *resultData = [[[NSMutableData alloc] init] autorelease];
	NSTask *task = [[NSTask alloc] init];
	NSPipe *stdoutPipe = [NSPipe pipe];
	NSPipe *stderrPipe = [NSPipe pipe];

	[task setLaunchPath:cmd];
	[task setArguments:args];

	[task setStandardOutput:stdoutPipe];
	[task setStandardError:stderrPipe];

	NSFileHandle *stderrFileHandle = [stderrPipe fileHandleForReading];

	// Using a readability handler allows us to get more than 4096 bytes without blocking the pipe
	[[task.standardOutput fileHandleForReading] setReadabilityHandler:^(NSFileHandle *file) {
		NSData *data = [file availableData]; // read to current EOF

		[resultData appendData:data];
	}];

	@try
	{
		[task launch];
		[task waitUntilExit];

		if (! [task isRunning] && [task terminationStatus] != 0 && printWarning)
		{
			// Command failed, emit any stderr to the log
			NSData *stderrData = [stderrFileHandle readDataToEndOfFile];
			NSLog(@"Error running %@ %@: %s", cmd, args, (const char *)[stderrData bytes]);
		}

		result = [[[NSMutableString alloc] initWithData:resultData encoding:NSUTF8StringEncoding] autorelease];
	}
	@catch( NSException* exception )
	{
		NSLog( @"launchTaskAndReturnOutput: exception %@ (%@ %@)", exception, cmd, args );
	}
	@finally
	{
		[[task.standardOutput fileHandleForReading] setReadabilityHandler:nil];
		[task release];
	}

	result = [result stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];

	return result;
}

@end
