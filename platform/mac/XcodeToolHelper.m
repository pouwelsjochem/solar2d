//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#import "XcodeToolHelper.h"

@implementation XcodeToolHelper

+ (void) printNotFoundWarningForTool:(NSString*)toolbasename
{
	NSLog(@"Warning: Could not find Xcode build tool: %@\n - perhaps Xcode isn't installed", toolbasename);
}

//
// Return the path for codesign_allocate
//
+ (NSString*) pathForCodesignAllocate
{
	NSString* toolpath = [XcodeToolHelper findXcodePathFor:@"codesign_allocate"];

	if (! [[NSFileManager defaultManager] fileExistsAtPath:toolpath])
	{
		toolpath = nil;

		[XcodeToolHelper printNotFoundWarningForTool:@"codesign_allocate"];
	}

	return toolpath;
}

//
// Return the path for productbuild
//
+ (NSString*) pathForProductBuild
{
	NSString* toolpath = [XcodeToolHelper findXcodePathFor:@"productbuild"];

	if (! [[NSFileManager defaultManager] fileExistsAtPath:toolpath])
	{
		toolpath = nil;

		[XcodeToolHelper printNotFoundWarningForTool:@"productbuild"];
	}

	return toolpath;
}
//
// Return the path for codesign
//
+ (NSString*) pathForCodesign
{
	NSString* toolpath = @"/usr/bin/codesign";

	if ( NO == [[NSFileManager defaultManager] fileExistsAtPath:toolpath] )
	{
		toolpath = nil;
		[XcodeToolHelper printNotFoundWarningForTool:@"codesign"];
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
