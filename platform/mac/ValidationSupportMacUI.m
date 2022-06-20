//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#import "ValidationSupportMacUI.h"
#import "ValidationToolOutputViewController.h"
#include "Rtt_AndroidSupportTools.h"

static NSString* kAndroidValidationLuaFileName = @"AndroidValidation.lu";


bool ValidationSupportMacUI_ValidateAndroidPackageName( const char* packagename, const char* filepath )
{
	lua_State* L = Rtt_AndroidSupportTools_NewLuaState( filepath );
	bool isvalid = Rtt_AndroidSupportTools_IsAndroidPackageName( L, packagename );
	Rtt_AndroidSupportTools_CloseLuaState(L);
	return isvalid;
}


bool ValidationSupportMacUI_ValidateIOSAppName( const char* name, const char* filepath )
{
#ifdef RTT_VALIDATE_NAMES
	lua_State* L = Rtt_AndroidSupportTools_NewLuaState( filepath );
	bool isvalid = Rtt_CommonSupportTools_ValidateNameForRestrictedASCIICharacters( L, name );
	Rtt_AndroidSupportTools_CloseLuaState(L);
	return isvalid;
#else
    return true;
#endif // RTT_VALIDATE_NAMES
}

@interface ValidationSupportMacUI ()
- (void) displayAlertForFileValidationFailure:(NSString*)alertmessage messageComment:(NSString*)messagecomment informativeText:(NSString*)informativetext informativeComment:(NSString*)informativecomment fileList:(NSArray*)filelist;
@end

@implementation ValidationSupportMacUI

@synthesize parentWindow;

- (id) init
{
	self = [super init];
	if(nil != self)
	{
		NSString* script = [[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:kAndroidValidationLuaFileName];
		luaState = Rtt_AndroidSupportTools_NewLuaState( [script fileSystemRepresentation] );
	}
	return self;
}

- (id) initWithParentWindow:(NSWindow*)window
{
	self = [super init];
	if(nil != self)
	{
		NSString* script = [[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:kAndroidValidationLuaFileName];
		luaState = Rtt_AndroidSupportTools_NewLuaState( [script fileSystemRepresentation] );
		parentWindow = [window retain];
	}
	return self;
}

- (void) dealloc
{
	[parentWindow release];
	Rtt_AndroidSupportTools_CloseLuaState( luaState );
	[super dealloc];
}

- (void) finalize
{
	Rtt_AndroidSupportTools_CloseLuaState( luaState );
	[super finalize];
}

-(void) alertDidEndForAndroidFileValidation:(NSAlert*)alert returnCode:(int)returncode contextInfo:(void*)contextinofo
{
	NSArray* filelist = (NSArray*)contextinofo;
	
	if ( NSAlertSecondButtonReturn == returncode )
	{
		NSMutableArray* urllist = [NSMutableArray array];
		for(NSString* file in filelist)
		{
			NSURL* url = [NSURL fileURLWithPath:file isDirectory:NO];
			[urllist addObject:url];
		}
		[[NSWorkspace sharedWorkspace] activateFileViewerSelectingURLs:urllist];
	}
	else if ( NSAlertFirstButtonReturn == returncode )
	{
	}
	
	[filelist release];
}

- (void) displayAlertForFileValidationFailure:(NSString*)alertmessage messageComment:(NSString*)messagecomment informativeText:(NSString*)informativetext informativeComment:(NSString*)informativecomment fileList:(NSArray*)filelist
{
	// flatten the array into a string to be presented
	NSString* messagelist = [[filelist valueForKey:@"description"] componentsJoinedByString:@"\n"];
	NSAlert* alert = [[[NSAlert alloc] init] autorelease];
	[alert addButtonWithTitle:NSLocalizedString(@"Dismiss", @"Dismiss")];
	[alert addButtonWithTitle:NSLocalizedString(@"Show in Finder...", @"Show in Finder...")];
	
	[alert setMessageText:NSLocalizedString(alertmessage, messagecomment)];
	
	[alert setInformativeText:NSLocalizedString(informativetext, informativecomment)];
	
	// It so happens that the ValidationToolOutputViewController has the same UI interface we want.
	ValidationToolOutputViewController* validationToolViewController = [[ValidationToolOutputViewController alloc] initWithNibName:@"ValidationToolOutput" bundle:nil];
	[validationToolViewController autorelease];
	
	[validationToolViewController setValidationMessage:messagelist];
	[alert setAccessoryView:validationToolViewController.view];
	
	
	[alert setAlertStyle:NSCriticalAlertStyle];
	[alert setDelegate:self];
	
	[alert beginSheetModalForWindow:[self parentWindow] 
		modalDelegate:self
		didEndSelector:@selector(alertDidEndForAndroidFileValidation:returnCode:contextInfo:)
		contextInfo:[filelist retain]
	];
}

- (bool) runAndroidFileValidationTestsInProjectPath:(NSString *)projectpath
{
	// Currently, we don't have any more validation tests to run on the post-press-build-button step.
	return true;
}

- (bool) runIOSAppNameValidation:(NSString*)appname
{
	// Currently, we don't have any more validation tests to run on the post-press-build-button step.
	return true;
}

- (bool) runOSXAppNameValidation:(NSString*)appname
{
	// Currently, we don't have any more validation tests to run on the post-press-build-button step.
	return true;
}

@end
