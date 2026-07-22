//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Solar2D game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@solar2d.com
//
//////////////////////////////////////////////////////////////////////////////

#include <objc/message.h>

#include <vector>
#include <map>

#include "Core/Rtt_Build.h"

#import "AppDelegate.h"
#include <stdlib.h>

#import <AppKit/NSAlert.h>
#import <AppKit/NSApplication.h>
#import <AppKit/NSAttributedString.h>
#import <AppKit/NSControl.h>
#import <AppKit/NSFont.h>
#import <AppKit/NSNibLoading.h>
#import <AppKit/NSMenuItem.h>
#import <AppKit/NSOpenPanel.h>
#import <AppKit/NSPopUpButton.h>
#import <AppKit/NSScreen.h>
#import <AppKit/NSWindowController.h>
#import <AppKit/NSWorkspace.h>
#include <CoreServices/CoreServices.h>
#include <SystemConfiguration/SystemConfiguration.h>
//#include <CoreFoundation/CFBundle.h>

#include "Rtt_Event.h"

@class NSNotification;

// -------------------------
#include "Rtt_TargetDevice.h"

#include "Rtt_MacConsolePlatform.h"
#include "Rtt_MacPlatform.h"
#include "Rtt_MacSimulator.h"
#include "Rtt_PlatformPlayer.h"
#include "Rtt_PlatformSimulator.h"
#include "Rtt_Runtime.h"

#include "Rtt_Lua.h"
#include "Rtt_LuaFile.h"
#include "Rtt_LuaContext.h"

#include "Rtt_LuaConsole.h"
#include "CoronaLua.h"

#include "Rtt_VersionTimestamp.h"
#include "Rtt_String.h"

#import "GLView.h"
#import "NSAlert-OAExtensions.h"

#include <CommonCrypto/CommonCrypto.h>

#include <sys/sysctl.h>
#include <string.h>

#include "Rtt_PlatformDictionaryWrapper.h"

#include "Rtt_TargetDevice.h"

#import "TextEditorSupport.h"

// -------------------------

static void SigTERMHandler(int signal)
{
	NSLog(@"termination requested by 3rd party ... shutting down (signal %d)", signal);

	// Some IDEs will terminate us quite abruptly so make sure we're on disk
	[[NSUserDefaults standardUserDefaults] synchronize];

	// This can get called at any point so we need to just exit
	exit(0);
}

extern int Rtt_VLogException_UseStdout;

// -----------------------------------------------------------------------------
// BEGIN: Validation functions
// -----------------------------------------------------------------------------

// Settings stored in user preferences.
//
// Some of these are set from the command line. For example:
static NSString* kDockIconBounceTime = @"dockIconBounceTime";

static NSString* kWindowMenuItemName = @"Window";
static NSString* kViewAsMenuItemName = @"View As";
static NSString* kRoundedCornersMenuItemName = @"Rounded Corners";
static NSString* kShowSafeAreaGuidesMenuItemName = @"Show Safe Area Guides";
static NSString* kCustomDevicePreferenceValue = @"__custom__";
static NSString* kCustomDeviceWidthPreference = @"customDeviceWidth";
static NSString* kCustomDeviceHeightPreference = @"customDeviceHeight";
static NSString* kCustomDeviceSafeAreaInsetTopPreference = @"customDeviceSafeAreaInsetTop";
static NSString* kCustomDeviceSafeAreaInsetLeftPreference = @"customDeviceSafeAreaInsetLeft";
static NSString* kCustomDeviceSafeAreaInsetBottomPreference = @"customDeviceSafeAreaInsetBottom";
static NSString* kCustomDeviceSafeAreaInsetRightPreference = @"customDeviceSafeAreaInsetRight";
static NSString* kShowSafeAreaGuidesPreference = @"showSafeAreaGuides";
static NSString* kSimulatorDeviceArgument = @"simulator-device";
static NSString* kSimulatorWidthArgument = @"simulator-width";
static NSString* kSimulatorHeightArgument = @"simulator-height";
static NSString* kSimulatorSafeAreaTopArgument = @"simulator-safe-area-top";
static NSString* kSimulatorSafeAreaLeftArgument = @"simulator-safe-area-left";
static NSString* kSimulatorSafeAreaBottomArgument = @"simulator-safe-area-bottom";
static NSString* kSimulatorSafeAreaRightArgument = @"simulator-safe-area-right";
static NSString* kSimulatorRoundedCornersArgument = @"simulator-rounded-corners";

// TODO: Remove once the Beta is over
static const int       kClearProjectSandboxMenuTag = 1001;
static const int       kCustomDeviceMenuTag = -1000;
static const int       kEditCustomDeviceMenuTag = -1001;
static const NSInteger kDefaultCustomDeviceWidth = 800;
static const NSInteger kDefaultCustomDeviceHeight = 600;
static const NSInteger kMaximumCustomDeviceDimension = 16384;

static BOOL
ReadSimulatorIntegerArgument(id value, NSInteger *result)
{
	if (!value)
	{
		return NO;
	}
	if ([value isKindOfClass:[NSNumber class]])
	{
		*result = [value integerValue];
		return YES;
	}
	if (![value isKindOfClass:[NSString class]])
	{
		return NO;
	}

	NSScanner *scanner = [NSScanner scannerWithString:value];
	return [scanner scanInteger:result] && [scanner isAtEnd];
}

static BOOL
ReadSimulatorBooleanArgument(id value, BOOL *result)
{
	if ([value isKindOfClass:[NSNumber class]])
	{
		*result = [value boolValue];
		return YES;
	}
	if (![value isKindOfClass:[NSString class]])
	{
		return NO;
	}

	NSString *normalizedValue = [value lowercaseString];
	if ([normalizedValue isEqualToString:@"true"] ||
		[normalizedValue isEqualToString:@"yes"] ||
		[normalizedValue isEqualToString:@"1"])
	{
		*result = YES;
		return YES;
	}
	if ([normalizedValue isEqualToString:@"false"] ||
		[normalizedValue isEqualToString:@"no"] ||
		[normalizedValue isEqualToString:@"0"])
	{
		*result = NO;
		return YES;
	}
	return NO;
}

#ifdef Rtt_DEBUG

#if 0
static bool IsRunningUnderDebugger()
{
//! \TODO Move this function to a better Mac-specific library file.
// From:
// https://developer.apple.com/library/mac/qa/qa1361/_index.html
//		This returns true if the current process is being debugged (either
//		running under the debugger or has a debugger attached post facto).
#if( defined( Rtt_AUTHORING_SIMULATOR ) && defined( Rtt_DEBUG ) )

	int mib[ 4 ];
	memset( &mib, 0, sizeof( mib ) );
	mib[ 0 ] = CTL_KERN;
	mib[ 1 ] = KERN_PROC;
	mib[ 2 ] = KERN_PROC_PID;
	mib[ 3 ] = getpid();

	struct kinfo_proc info;
    // Initialize the flags so that, if sysctl fails for some bizarre
    // reason, we get a predictable result.
	memset( &info, 0, sizeof( info ) );

	size_t sizeof_info = sizeof( info );
	if( sysctl( mib,
				( sizeof( mib ) / sizeof( mib[ 0 ] ) ),
				&info,
				&sizeof_info,
				NULL,
				0 ) != 0 )
	{
		// sysctl() failed.
		// Assume NO debuggers are attached to this process.
		// See "strerror( errno )" for details.
		return false;
	}

	return ( !! ( info.kp_proc.p_flag & P_TRACED ) );

#else // Not ( defined( Rtt_AUTHORING_SIMULATOR ) && defined( Rtt_DEBUG ) )

	// Assume NO debuggers are attached to this process.
	return false;

#endif // ( defined( Rtt_AUTHORING_SIMULATOR ) && defined( Rtt_DEBUG ) )
}
#endif

#include <sys/types.h>
#include <unistd.h>

int getprocessname( pid_t inPID, char *outName, size_t inMaxLen)
{
	struct kinfo_proc info;
	size_t length = sizeof(struct kinfo_proc);
	int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, inPID };

    outName[0] = '\0';

	if (sysctl(mib, 4, &info, &length, NULL, 0) < 0)
    {
		return -1;
    }
	else
    {
		strncpy(outName, info.kp_proc.p_comm, inMaxLen);
    }

	return 0;
}
#endif // Rtt_DEBUG

// TODO: This is total crap.
static bool
IsValidAppPath( NSFileManager* fileMgr, NSString* appPath, NSString* mainObjectFile, NSString* mainScriptFile, bool *outIsDir = NULL )
{
	using namespace Rtt;
	bool result = false;

	BOOL isDir = NO;
	if ( [fileMgr fileExistsAtPath:appPath isDirectory:&isDir] && isDir )
	{
		result = ( (mainObjectFile && [fileMgr fileExistsAtPath:[appPath stringByAppendingPathComponent:mainObjectFile]] && LuaContext::IsBinaryLua([[appPath stringByAppendingPathComponent:mainObjectFile] UTF8String]) )
				 || ((mainScriptFile && [fileMgr fileExistsAtPath:[appPath stringByAppendingPathComponent:mainScriptFile]]) && !LuaContext::IsBinaryLua([[appPath stringByAppendingPathComponent:mainScriptFile] UTF8String])) );
	}
	else
	{
		result = [[appPath lastPathComponent] isEqualToString:mainScriptFile];
	}

	if ( outIsDir )
	{
		*outIsDir = isDir;
	}

	return result;
}

static bool
IsEmptyLuaObjectFile( NSString* filePath )
{
	const unsigned char kEmptyLuaFile[] =
	{
		0x1B,0x4C,0x75,0x61,0x51,0x00,0x01,0x04,0x04,0x04,0x08,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x02,0x01,0x00,0x00,0x00,
		0x1E,0x00,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
	};

	NSFileHandle* f = [NSFileHandle fileHandleForReadingAtPath:filePath];
	NSData* data = [f readDataOfLength:sizeof(kEmptyLuaFile)];
	const void* bytes = [data bytes];
	return 0 == memcmp( kEmptyLuaFile, bytes, sizeof(kEmptyLuaFile) );
}

// -----------------------------------------------------------------------------
// END: Validation functions
// -----------------------------------------------------------------------------


// ----------------------------------------------------------------------------

static void
MD5Hash( char *dst, const char *src )
{
	U8 hash[CC_MD5_DIGEST_LENGTH];
	CC_MD5( (const unsigned char*)src, (unsigned)strlen( src ), hash );

	char *p = dst;
	for ( int i = 0; i < CC_MD5_DIGEST_LENGTH; i++ )
	{
		p += sprintf( p, "%02x", hash[i] );
	}

	Rtt_ASSERT( strlen( dst ) == CC_MD5_DIGEST_LENGTH*2 );
}

// ----------------------------------------------------------------------------

#if !defined( Rtt_CUSTOM_CODE )
Rtt_EXPORT const luaL_Reg* Rtt_GetCustomModulesList()
{
	return NULL;
}
#endif


// -----------------------------------------------------------------------------
// BEGIN: Project Open
// -----------------------------------------------------------------------------

@interface OpenProjectDelegate : NSObject<NSOpenSavePanelDelegate>
{
	NSFileManager* fFileMgr;
}

-(id)initWithFileManager:(NSFileManager*)fileMgr;
- (BOOL) panel:(id)sender shouldEnableURL:(NSURL*)url;
- (BOOL) panel:(id)sender validateURL:(NSURL*)url error:(NSError**)error;

@end

@implementation OpenProjectDelegate

-(id)initWithFileManager:(NSFileManager*)fileMgr
{
	self = [super init];
	if ( self )
	{
		if ( !fileMgr ) { fileMgr = [NSFileManager defaultManager]; }
		fFileMgr = [fileMgr retain];
	}
	return self;
}

-(void)dealloc
{
	[fFileMgr release];
	[super dealloc];
}

// NOTE: This callback only works in 10.6 and later.
// WARNING: The URL APIs used for 2365 are 10.6+ APIs only.
// I don't check for existance because this block of code should only be called in 10.6 or later.
- (BOOL) panel:(id)sender shouldEnableURL:(NSURL*)url
{
	BOOL result = NO;
	BOOL isDir = NO;
	NSString* mainScriptFile = [NSString stringWithExternalString:Rtt_LUA_SCRIPT_FILE( "main" )];
	
	// casenum: 2365
	// Network mounts are being disabled by this code.
	// Currently [url path] changes something like smb://compy.local/strongbad to /strongbad
	// Then fileExistsAtPath will return NO which causes the bug.
	// This work around will skip evaluating if isFileURL returns NO and always return YES.
	if( ! [url isFileURL] )
	{
	
		// Drat, I was hoping getResourceValue with NSURLIsDirectoryKey or NSURLIsVolumeKey would work for us.
		// But it seems to always fail with network mount URLs. Maybe this will be fixed by Apple in the future.
/*
		NSNumber* value = nil;
		NSError* theError = nil;
		BOOL hitError = [url getResourceValue:&value forKey:NSURLIsVolumeKey error:&theError];
		if(YES == hitError)
		{
			NSLog(@"[url getResourceValue] returned error: %@", [theError localizedDescription]);
		}
*/		
		if( YES == [url isFileReferenceURL] )
		{
			result = [[url lastPathComponent] isEqualToString:mainScriptFile];
		}
		/*
		This depends on getResourceValue with NSURLIsDirectoryKey working.
		else if( YES == [value boolValue] )
		{
			result = YES;
		}
		*/
		// Since getResourceValue is failing, just returning YES here seems to work just well enough for our cases
		// because we still have the legacy/working code with NSFileManager below to handle those cases.
		// But in the future, it would be nice to unify the two code paths into one using the URL way which is Apple's
		// designated way to do things moving forward.
		else
		{
			result = YES;
		}
		return result;
	}
	
	NSString *path = [url path];

	result = [fFileMgr fileExistsAtPath:path isDirectory:&isDir];

	// If not a directory, make sure only main.lua is allowed
	if ( result && ! isDir )
	{
		NSString* mainScriptFile = [NSString stringWithExternalString:Rtt_LUA_SCRIPT_FILE( "main" )];
		result = [[path lastPathComponent] isEqualToString:mainScriptFile];
	}

	return result;
}

- (BOOL) panel:(id)sender validateURL:(NSURL*)url error:(NSError**)error
{
	NSString* filename = [url path];
	BOOL isdir = NO;
	BOOL result = [fFileMgr fileExistsAtPath:filename isDirectory:&isdir];
	
	NSString* mainScriptFile = [NSString stringWithExternalString:Rtt_LUA_SCRIPT_FILE( "main" )];
	
	// Allow the parent directory of the main.lua file to be chosen or any file in that directory
	if ( result )
	{
		if ( isdir )
		{
			NSString* mainPath = [filename stringByAppendingPathComponent:mainScriptFile];
			result = [fFileMgr fileExistsAtPath:mainPath];
		}
		else
		{
			result = [[filename lastPathComponent] isEqualToString:mainScriptFile];
		}
	}
	
	
	if ( ! result )
	{
		NSString* msg = [NSString stringWithFormat:@"Please select a %@ file or a directory that contains that file", mainScriptFile];
		NSDictionary* details = [[[NSDictionary alloc] initWithObjectsAndKeys:msg, NSLocalizedDescriptionKey, nil] autorelease];
		*error = [[[NSError alloc] initWithDomain:@"CoronaSimulator" code:102 userInfo:details] autorelease];
	}
	
	return result;
}


@end

// ----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// END: Project Open
// -----------------------------------------------------------------------------

// ----------------------------------------------------------------------------

@interface SimulatorSafeAreaGuideView : NSView
{
	CGFloat fInsetTop;
	CGFloat fInsetLeft;
	CGFloat fInsetBottom;
	CGFloat fInsetRight;
}

- (id)initWithFrame:(NSRect)frame
	insetTop:(CGFloat)top
	insetLeft:(CGFloat)left
	insetBottom:(CGFloat)bottom
	insetRight:(CGFloat)right;

@end

@implementation SimulatorSafeAreaGuideView

- (id)initWithFrame:(NSRect)frame
	insetTop:(CGFloat)top
	insetLeft:(CGFloat)left
	insetBottom:(CGFloat)bottom
	insetRight:(CGFloat)right
{
	self = [super initWithFrame:frame];
	if (self)
	{
		fInsetTop = top;
		fInsetLeft = left;
		fInsetBottom = bottom;
		fInsetRight = right;
		[self setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
	}
	return self;
}

- (BOOL)isOpaque
{
	return NO;
}

- (NSView*)hitTest:(NSPoint)point
{
	return nil;
}

- (void)drawRect:(NSRect)dirtyRect
{
	NSRect bounds = [self bounds];
	CGFloat top = MIN(MAX(fInsetTop, 0), NSHeight(bounds));
	CGFloat left = MIN(MAX(fInsetLeft, 0), NSWidth(bounds));
	CGFloat bottom = MIN(MAX(fInsetBottom, 0), NSHeight(bounds) - top);
	CGFloat right = MIN(MAX(fInsetRight, 0), NSWidth(bounds) - left);
	NSRect safeRect = NSMakeRect(
		NSMinX(bounds) + left,
		NSMinY(bounds) + bottom,
		MAX(0, NSWidth(bounds) - left - right),
		MAX(0, NSHeight(bounds) - top - bottom));

	[[NSColor colorWithCalibratedRed:1.0 green:0.45 blue:0.0 alpha:0.18] setFill];
	NSRectFillUsingOperation(
		NSMakeRect(NSMinX(bounds), NSMinY(bounds), left, NSHeight(bounds)), NSCompositingOperationSourceOver );
	NSRectFillUsingOperation(
		NSMakeRect(NSMaxX(bounds) - right, NSMinY(bounds), right, NSHeight(bounds)), NSCompositingOperationSourceOver );
	NSRectFillUsingOperation(
		NSMakeRect(NSMinX(safeRect), NSMinY(bounds), NSWidth(safeRect), bottom), NSCompositingOperationSourceOver );
	NSRectFillUsingOperation(
		NSMakeRect(NSMinX(safeRect), NSMaxY(bounds) - top, NSWidth(safeRect), top), NSCompositingOperationSourceOver );

	if (NSWidth(safeRect) > 1.0 && NSHeight(safeRect) > 1.0)
	{
		NSBezierPath *outline = [NSBezierPath bezierPathWithRect:NSInsetRect(safeRect, 0.5, 0.5)];
		[outline setLineWidth:1.0];
		[[NSColor colorWithCalibratedRed:1.0 green:0.55 blue:0.0 alpha:0.95] setStroke];
		[outline stroke];
	}
}

@end

@interface AppDelegate ()

@property (nonatomic, readwrite, copy) NSString* fAppPath;

-(void)setSkin:(Rtt::TargetDevice::Skin)skin;
- (void) updateMenuForSkinChange;
- (void) restoreUserSkinSetting;
- (void) saveUserSkinSetting;
- (void) reloadDeviceSkinsForProject:(NSString*)appPath;
- (void) invalidateViewAsMenu;
- (NSInteger) customDeviceWidth;
- (NSInteger) customDeviceHeight;
- (NSInteger) customDeviceSafeAreaInsetForPreference:(NSString*)preference;
- (NSDictionary*) customDeviceSafeAreaInsets;
- (Rtt::TargetDevice::Skin) skinForSimulatorDeviceIdentifier:(NSString*)identifier;
- (NSDictionary*) simulatorDeviceInfoForSkin:(Rtt::TargetDevice::Skin)skin roundedCorners:(BOOL)roundedCorners;
- (NSDictionary*) simulatorCustomDeviceInfoWithWidth:(NSInteger)width
	height:(NSInteger)height
	safeAreaInsetTop:(NSInteger)top
	safeAreaInsetLeft:(NSInteger)left
	safeAreaInsetBottom:(NSInteger)bottom
	safeAreaInsetRight:(NSInteger)right
	roundedCorners:(BOOL)roundedCorners;
- (NSDictionary*) persistentSimulatorDeviceInfo;
- (NSDictionary*) configuredSimulatorDeviceInfo;
- (BOOL) configuredSimulatorRoundedCorners;
- (void) clearTemporarySimulatorConfiguration;
- (BOOL) refreshTemporarySimulatorConfiguration;
- (void) persistSimulatorDeviceInfo:(NSDictionary*)deviceInfo;
- (BOOL) applySimulatorConfiguration:(NSDictionary*)configuration
	relaunchIfNeeded:(BOOL)relaunchIfNeeded
	scheduleRelaunch:(BOOL)scheduleRelaunch
	didScheduleRelaunch:(BOOL*)didScheduleRelaunch;
- (BOOL) applyLaunchSimulatorConfiguration;
- (void) scheduleSimulatorRelaunch;
- (void) resumeSimulatorAfterBackground;
-(BOOL)setSimulatorCustomWidth:(NSInteger)width
	height:(NSInteger)height
	safeAreaInsetTop:(NSInteger)top
	safeAreaInsetLeft:(NSInteger)left
	safeAreaInsetBottom:(NSInteger)bottom
	safeAreaInsetRight:(NSInteger)right;
- (void) updateSimulatorDisplayMenuItems:(NSMenu*)windowMenu;
- (void) updateSafeAreaGuideOverlay;
- (IBAction) selectCustomDeviceAction:(id)sender;
- (IBAction) editCustomDeviceAction:(id)sender;
- (IBAction) toggleRoundedCornersAction:(id)sender;
- (IBAction) toggleSafeAreaGuidesAction:(id)sender;
@end


@implementation AppDelegate

@synthesize fSimulator;
@synthesize fSkin;
@synthesize fAppPath;
@synthesize applicationHasBeenInitialized;
@synthesize launchedWithFile;
@synthesize allowLuaExit;
@synthesize agentMode = fAgentMode;

-(id)init
{
	self = [super init];
	if ( self )
	{
		applicationIsTerminating = NO;
		fSimulator = NULL;
		fAppPath = nil;
		memset( & fOptions, 0, sizeof( fOptions ) );
		fAgentMode = [[NSUserDefaults standardUserDefaults] boolForKey:@"agent-mode"];

		if (fAgentMode)
		{
			Rtt_VLogException_UseStdout = true;
		}

		fConsolePlatform = new Rtt::MacConsolePlatform;
		fRelaunchCount = 0;

		fOpenAccessoryView = nil;
		fDeviceSkins = nil;
		fSkin = Rtt::TargetDevice::kUnknownSkin;
		
		fPreferencesWindow = nil;
        
		fIsRemote = NO;

		if (!fAgentMode)
		{
			[[NSUserNotificationCenter defaultUserNotificationCenter] setDelegate:self];
		}
		fRuntimeErrorNotified = FALSE;

		fServices = NULL;
		fSimulatorRelaunchPending = NO;
		fLaunchSimulatorConfigurationHandled = NO;
		fBackgroundedRuntime = NULL;
		fActiveSimulatorDeviceInfo = nil;
		fTemporarySimulatorDeviceInfo = nil;
		// Register corona:// URL scheme handler
		[[NSAppleEventManager sharedAppleEventManager]
		 setEventHandler:self
		 andSelector:@selector(handleOpenURL:replyEvent:)
		 forEventClass:kInternetEventClass
		 andEventID:kAEGetURL];
	}
	return self;
}

// -----------------------------------------------------------------------------
// BEGIN: Simulator Startup
// -----------------------------------------------------------------------------

-(void)coronaInit:(NSNotification*)aNotification
{
	[self reloadDeviceSkinsForProject:nil];
}

- (void) reloadDeviceSkinsForProject:(NSString*)appPath
{
	NSFileManager *fileManager = [NSFileManager defaultManager];
	NSMutableArray *devicePaths = [NSMutableArray arrayWithCapacity:32];
	NSArray *deviceDirectories = @[
		[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:@"Skins"],
		appPath ? [appPath stringByAppendingPathComponent:@"simulator/devices"] : @""
	];

	for (NSString *directory in deviceDirectories)
	{
		if ([directory length] == 0)
		{
			continue;
		}

		BOOL isDirectory = NO;
		if (![fileManager fileExistsAtPath:directory isDirectory:&isDirectory] || !isDirectory)
		{
			continue;
		}

		NSDirectoryEnumerator *enumerator = [fileManager enumeratorAtPath:directory];
		NSString *relativePath = nil;
		while ((relativePath = [enumerator nextObject]))
		{
			NSString *extension = [[relativePath pathExtension] lowercaseString];
			if ([extension isEqualToString:@"lua"] || [extension isEqualToString:@"lu"])
			{
				[devicePaths addObject:[directory stringByAppendingPathComponent:relativePath]];
			}
		}
	}

	char **paths = (char **)calloc(sizeof(char *), [devicePaths count]);
	if (!paths && [devicePaths count] > 0)
	{
		Rtt_LogException("ERROR: Could not allocate the Simulator device list");
		return;
	}

	int count = 0;
	for (NSString *path in devicePaths)
	{
		paths[count++] = strdup([path UTF8String]);
	}

	Rtt::TargetDevice::Initialize(paths, count);

	for (int index = 0; index < count; index++)
	{
		free(paths[index]);
	}
	free(paths);

	// Skin identifiers are array indices and are invalid after rebuilding the
	// list. restoreUserSkinSetting will choose the project-specific selection.
	fSkin = Rtt::TargetDevice::kDefaultSkin;
	[self invalidateViewAsMenu];
}

- (void) invalidateViewAsMenu
{
	NSMenu *appMenu = [[NSApplication sharedApplication] mainMenu];
	NSMenuItem *windowMenuItem = [appMenu itemWithTitle:kWindowMenuItemName];
	NSMenuItem *viewAsItem = [[windowMenuItem submenu] itemWithTitle:kViewAsMenuItemName];
	[[viewAsItem submenu] removeAllItems];
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// END: Simulator Startup
// -----------------------------------------------------------------------------


/*
- (void) awakeFromNib
{
}
*/

+ (void)initialize
{
    // Note: http://www.cocoabuilder.com/archive/cocoa/232525-double-initialize-is-that-how-it-should-be.html
    
    if ( self == [AppDelegate class] )
    {
        
#ifdef Rtt_DEBUG
        // This is useful for debugging IDE issues
        NSArray *argv = [[NSProcessInfo processInfo] arguments];
        
        if ([argv count] > 2)  // Finder adds a "-psn..." parameter
        {
            NSLog(@"[AppDelegate initialize]: argv: %@", argv);
        }

        // Display the name of the parent process
        // Unfortunately, this doesn't identify IDEs well enough to be useful (e.g. Xerobrane shows up as "lua")
        //char parentProcessName[BUFSIZ];
        //getprocessname( getppid(), parentProcessName, BUFSIZ);
        //NSLog(@"Parent process: %s", parentProcessName);
#endif
        
        // We get SIGTERM from IDEs who want to terminate us and need to make sure the user's preferences are saved
        struct sigaction termAction = { 0 };
		termAction.sa_handler = SigTERMHandler;
		sigaction(SIGTERM, &termAction, NULL);

        [super initialize];
    }
}


- (void)deduplicateRunningInstances
{
    NSArray *otherSims = [NSRunningApplication runningApplicationsWithBundleIdentifier:[[NSBundle mainBundle] bundleIdentifier]];
    
    for (NSRunningApplication *app in otherSims)
    {
        if ([app processIdentifier] != [[NSRunningApplication currentApplication] processIdentifier])
        {
            [app terminate];
        }
    }
}

- (void) viewAsAction:(id)sender
{
    NSMenuItem *menuItem = (NSMenuItem *) sender;
    Rtt::TargetDevice::Skin skinID = (Rtt::TargetDevice::Skin) [menuItem tag];

    // NSLog(@"viewAsAction: %d: %s", skinID, Rtt::TargetDevice::LuaObjectFileFromSkin(skinID));
	[self clearTemporarySimulatorConfiguration];
    [self setSkin:skinID];
    [self launchSimulator:sender];
}

- (NSInteger) customDeviceWidth
{
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	NSString *projectKey = [self getAppSpecificPreferenceKeyName:kCustomDeviceWidthPreference withProjectPath:fAppPath];
	NSNumber *value = projectKey ? [defaults objectForKey:projectKey] : nil;
	if (!value)
	{
		value = [defaults objectForKey:kCustomDeviceWidthPreference];
	}
	NSInteger width = value ? [value integerValue] : kDefaultCustomDeviceWidth;
	return width > 0 && width <= kMaximumCustomDeviceDimension ? width : kDefaultCustomDeviceWidth;
}

- (NSInteger) customDeviceHeight
{
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	NSString *projectKey = [self getAppSpecificPreferenceKeyName:kCustomDeviceHeightPreference withProjectPath:fAppPath];
	NSNumber *value = projectKey ? [defaults objectForKey:projectKey] : nil;
	if (!value)
	{
		value = [defaults objectForKey:kCustomDeviceHeightPreference];
	}
	NSInteger height = value ? [value integerValue] : kDefaultCustomDeviceHeight;
	return height > 0 && height <= kMaximumCustomDeviceDimension ? height : kDefaultCustomDeviceHeight;
}

- (NSInteger) customDeviceSafeAreaInsetForPreference:(NSString*)preference
{
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	NSString *projectKey = [self getAppSpecificPreferenceKeyName:preference withProjectPath:fAppPath];
	NSNumber *value = projectKey ? [defaults objectForKey:projectKey] : nil;
	if (!value)
	{
		value = [defaults objectForKey:preference];
	}

	NSInteger inset = value ? [value integerValue] : 0;
	return (inset >= 0 && inset <= kMaximumCustomDeviceDimension) ? inset : 0;
}

- (NSDictionary*) customDeviceSafeAreaInsets
{
	return [NSDictionary dictionaryWithObjectsAndKeys:
		[NSNumber numberWithInteger:[self customDeviceSafeAreaInsetForPreference:kCustomDeviceSafeAreaInsetTopPreference]], @"top",
		[NSNumber numberWithInteger:[self customDeviceSafeAreaInsetForPreference:kCustomDeviceSafeAreaInsetLeftPreference]], @"left",
		[NSNumber numberWithInteger:[self customDeviceSafeAreaInsetForPreference:kCustomDeviceSafeAreaInsetBottomPreference]], @"bottom",
		[NSNumber numberWithInteger:[self customDeviceSafeAreaInsetForPreference:kCustomDeviceSafeAreaInsetRightPreference]], @"right",
		nil];
}

- (void) scheduleSimulatorRelaunch
{
	if ([self isRelaunchable] && !fSimulatorRelaunchPending)
	{
		fSimulatorRelaunchPending = YES;
		[self performSelectorOnMainThread:@selector(launchSimulator:) withObject:nil waitUntilDone:NO];
	}
}

-(BOOL)relaunchSimulator
{
	if (![self isRelaunchable])
	{
		return NO;
	}

	[self scheduleSimulatorRelaunch];
	return YES;
}

-(BOOL)setSimulatorSafeAreaGuidesVisible:(BOOL)visible
{
	if (!fSimulator)
	{
		return NO;
	}

	[[NSUserDefaults standardUserDefaults] setBool:visible forKey:kShowSafeAreaGuidesPreference];
	[self updateSafeAreaGuideOverlay];
	return YES;
}

-(BOOL)setSimulatorFullscreen:(BOOL)fullscreen
{
	NSWindow *window = [self currentWindow];
	if (!window)
	{
		return NO;
	}

	BOOL isFullscreen =
		([window styleMask] & NSWindowStyleMaskFullScreen) == NSWindowStyleMaskFullScreen;
	if (isFullscreen != fullscreen)
	{
		[window toggleFullScreen:nil];
	}
	return YES;
}

-(BOOL)setSimulatorCustomWidth:(NSInteger)width
	height:(NSInteger)height
	safeAreaInsetTop:(NSInteger)top
	safeAreaInsetLeft:(NSInteger)left
	safeAreaInsetBottom:(NSInteger)bottom
	safeAreaInsetRight:(NSInteger)right
{
	if (width <= 0 || height <= 0 ||
		width > kMaximumCustomDeviceDimension || height > kMaximumCustomDeviceDimension ||
		top < 0 || left < 0 || bottom < 0 || right < 0 ||
		top + bottom > height || left + right > width)
	{
		Rtt_LogException(
			"WARNING: Custom Simulator dimensions and safe area insets are invalid (maximum dimension %ld)",
			(long)kMaximumCustomDeviceDimension);
		return NO;
	}

	NSDictionary *currentSafeAreaInsets = [self customDeviceSafeAreaInsets];
	BOOL didChange = (fSkin != kCustomDeviceMenuTag ||
		[self customDeviceWidth] != width ||
		[self customDeviceHeight] != height ||
		[[currentSafeAreaInsets objectForKey:@"top"] integerValue] != top ||
		[[currentSafeAreaInsets objectForKey:@"left"] integerValue] != left ||
		[[currentSafeAreaInsets objectForKey:@"bottom"] integerValue] != bottom ||
		[[currentSafeAreaInsets objectForKey:@"right"] integerValue] != right);

	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	[defaults setInteger:width forKey:kCustomDeviceWidthPreference];
	[defaults setInteger:height forKey:kCustomDeviceHeightPreference];
	[defaults setInteger:top forKey:kCustomDeviceSafeAreaInsetTopPreference];
	[defaults setInteger:left forKey:kCustomDeviceSafeAreaInsetLeftPreference];
	[defaults setInteger:bottom forKey:kCustomDeviceSafeAreaInsetBottomPreference];
	[defaults setInteger:right forKey:kCustomDeviceSafeAreaInsetRightPreference];

	NSString *widthProjectKey = [self getAppSpecificPreferenceKeyName:kCustomDeviceWidthPreference withProjectPath:fAppPath];
	NSString *heightProjectKey = [self getAppSpecificPreferenceKeyName:kCustomDeviceHeightPreference withProjectPath:fAppPath];
	NSString *topProjectKey = [self getAppSpecificPreferenceKeyName:kCustomDeviceSafeAreaInsetTopPreference withProjectPath:fAppPath];
	NSString *leftProjectKey = [self getAppSpecificPreferenceKeyName:kCustomDeviceSafeAreaInsetLeftPreference withProjectPath:fAppPath];
	NSString *bottomProjectKey = [self getAppSpecificPreferenceKeyName:kCustomDeviceSafeAreaInsetBottomPreference withProjectPath:fAppPath];
	NSString *rightProjectKey = [self getAppSpecificPreferenceKeyName:kCustomDeviceSafeAreaInsetRightPreference withProjectPath:fAppPath];
	if (widthProjectKey && heightProjectKey && topProjectKey && leftProjectKey &&
		bottomProjectKey && rightProjectKey)
	{
		[defaults setInteger:width forKey:widthProjectKey];
		[defaults setInteger:height forKey:heightProjectKey];
		[defaults setInteger:top forKey:topProjectKey];
		[defaults setInteger:left forKey:leftProjectKey];
		[defaults setInteger:bottom forKey:bottomProjectKey];
		[defaults setInteger:right forKey:rightProjectKey];
	}

	if (fSkin != kCustomDeviceMenuTag)
	{
		[self setSkin:(Rtt::TargetDevice::Skin)kCustomDeviceMenuTag];
	}
	else
	{
		[self saveUserSkinSetting];
		[self updateMenuForSkinChange];
	}

	[self invalidateViewAsMenu];

	if (didChange)
	{
		[self scheduleSimulatorRelaunch];
	}
	return didChange;
}

-(BOOL)configureSimulator:(NSDictionary*)configuration relaunchIfNeeded:(BOOL)relaunchIfNeeded
		didScheduleRelaunch:(BOOL*)didScheduleRelaunch
{
	return [self applySimulatorConfiguration:configuration
		relaunchIfNeeded:relaunchIfNeeded
		scheduleRelaunch:YES
		didScheduleRelaunch:didScheduleRelaunch];
}

-(BOOL)applySimulatorConfiguration:(NSDictionary*)configuration
		relaunchIfNeeded:(BOOL)relaunchIfNeeded
		scheduleRelaunch:(BOOL)scheduleRelaunch
		didScheduleRelaunch:(BOOL*)didScheduleRelaunch
{
	if (didScheduleRelaunch)
	{
		*didScheduleRelaunch = NO;
	}
	if (![self isRelaunchable] || ![configuration count])
	{
		return NO;
	}

	NSString *deviceIdentifier = [configuration objectForKey:@"deviceId"];
	NSNumber *deviceWidth = [configuration objectForKey:@"deviceWidth"];
	NSNumber *deviceHeight = [configuration objectForKey:@"deviceHeight"];
	NSNumber *safeAreaInsetTop = [configuration objectForKey:@"safeAreaInsetTop"];
	NSNumber *safeAreaInsetLeft = [configuration objectForKey:@"safeAreaInsetLeft"];
	NSNumber *safeAreaInsetBottom = [configuration objectForKey:@"safeAreaInsetBottom"];
	NSNumber *safeAreaInsetRight = [configuration objectForKey:@"safeAreaInsetRight"];
	NSNumber *roundedCorners = [configuration objectForKey:@"roundedCorners"];
	NSNumber *temporary = [configuration objectForKey:@"temporary"];
	if ((deviceIdentifier && ![deviceIdentifier isKindOfClass:[NSString class]]) ||
		(deviceWidth && ![deviceWidth isKindOfClass:[NSNumber class]]) ||
		(deviceHeight && ![deviceHeight isKindOfClass:[NSNumber class]]) ||
		(safeAreaInsetTop && ![safeAreaInsetTop isKindOfClass:[NSNumber class]]) ||
		(safeAreaInsetLeft && ![safeAreaInsetLeft isKindOfClass:[NSNumber class]]) ||
		(safeAreaInsetBottom && ![safeAreaInsetBottom isKindOfClass:[NSNumber class]]) ||
		(safeAreaInsetRight && ![safeAreaInsetRight isKindOfClass:[NSNumber class]]) ||
		(roundedCorners && ![roundedCorners isKindOfClass:[NSNumber class]]) ||
		(temporary && ![temporary isKindOfClass:[NSNumber class]]))
	{
		return NO;
	}

	BOOL hasCustomDimensions = (deviceWidth != nil || deviceHeight != nil);
	BOOL hasDeviceIdentifier = [deviceIdentifier length] > 0;
	BOOL hasAnySafeAreaInset = safeAreaInsetTop || safeAreaInsetLeft || safeAreaInsetBottom || safeAreaInsetRight;
	BOOL hasAllSafeAreaInsets = safeAreaInsetTop && safeAreaInsetLeft && safeAreaInsetBottom && safeAreaInsetRight;

	if ((hasCustomDimensions && hasDeviceIdentifier) ||
		(hasCustomDimensions && (!deviceWidth || !deviceHeight)) ||
		(hasAnySafeAreaInset && (!hasCustomDimensions || !hasAllSafeAreaInsets)) ||
		(!hasCustomDimensions && !hasDeviceIdentifier && !roundedCorners))
	{
		return NO;
	}

	NSDictionary *currentDeviceInfo = [self configuredSimulatorDeviceInfo];
	BOOL requestedRoundedCorners = roundedCorners ? [roundedCorners boolValue] :
		[[currentDeviceInfo objectForKey:@"roundedCorners"] boolValue];
	NSDictionary *requestedDeviceInfo = nil;
	if (hasCustomDimensions)
	{
		NSInteger requestedWidth = [deviceWidth integerValue];
		NSInteger requestedHeight = [deviceHeight integerValue];
		if (requestedWidth <= 0 || requestedHeight <= 0 ||
			requestedWidth > kMaximumCustomDeviceDimension ||
			requestedHeight > kMaximumCustomDeviceDimension)
		{
			return NO;
		}

		NSInteger requestedSafeAreaInsetTop = safeAreaInsetTop ? [safeAreaInsetTop integerValue] : 0;
		NSInteger requestedSafeAreaInsetLeft = safeAreaInsetLeft ? [safeAreaInsetLeft integerValue] : 0;
		NSInteger requestedSafeAreaInsetBottom = safeAreaInsetBottom ? [safeAreaInsetBottom integerValue] : 0;
		NSInteger requestedSafeAreaInsetRight = safeAreaInsetRight ? [safeAreaInsetRight integerValue] : 0;
		if (requestedSafeAreaInsetTop < 0 || requestedSafeAreaInsetLeft < 0 ||
			requestedSafeAreaInsetBottom < 0 || requestedSafeAreaInsetRight < 0 ||
			requestedSafeAreaInsetTop + requestedSafeAreaInsetBottom > requestedHeight ||
			requestedSafeAreaInsetLeft + requestedSafeAreaInsetRight > requestedWidth)
		{
			return NO;
		}
		requestedDeviceInfo = [self simulatorCustomDeviceInfoWithWidth:requestedWidth
			height:requestedHeight
			safeAreaInsetTop:requestedSafeAreaInsetTop
			safeAreaInsetLeft:requestedSafeAreaInsetLeft
			safeAreaInsetBottom:requestedSafeAreaInsetBottom
			safeAreaInsetRight:requestedSafeAreaInsetRight
			roundedCorners:requestedRoundedCorners];
	}
	else if (hasDeviceIdentifier)
	{
		Rtt::TargetDevice::Skin requestedSkin =
			[self skinForSimulatorDeviceIdentifier:deviceIdentifier];
		if (requestedSkin == Rtt::TargetDevice::kUnknownSkin)
		{
			return NO;
		}
		requestedDeviceInfo = [self simulatorDeviceInfoForSkin:requestedSkin
			roundedCorners:requestedRoundedCorners];
	}
	else
	{
		NSMutableDictionary *deviceInfo = [[currentDeviceInfo mutableCopy] autorelease];
		[deviceInfo setObject:[NSNumber numberWithBool:requestedRoundedCorners]
			forKey:@"roundedCorners"];
		requestedDeviceInfo = deviceInfo;
	}

	BOOL configurationChanged = ![requestedDeviceInfo isEqualToDictionary:currentDeviceInfo];
	if ([temporary boolValue])
	{
		[fTemporarySimulatorDeviceInfo release];
		fTemporarySimulatorDeviceInfo = [requestedDeviceInfo copy];
		if (![self refreshTemporarySimulatorConfiguration])
		{
			[self clearTemporarySimulatorConfiguration];
			return NO;
		}
	}
	else
	{
		[self clearTemporarySimulatorConfiguration];
		[self persistSimulatorDeviceInfo:requestedDeviceInfo];
	}

	if (relaunchIfNeeded && !configurationChanged)
	{
		if (didScheduleRelaunch)
		{
			*didScheduleRelaunch = fSimulatorRelaunchPending;
		}
		return YES;
	}

	if (scheduleRelaunch)
	{
		[self scheduleSimulatorRelaunch];
	}
	if (didScheduleRelaunch)
	{
		*didScheduleRelaunch = scheduleRelaunch && fSimulatorRelaunchPending;
	}
	return YES;
}

-(Rtt::TargetDevice::Skin)skinForSimulatorDeviceIdentifier:(NSString*)identifier
{
	if (!identifier)
	{
		return Rtt::TargetDevice::kUnknownSkin;
	}
	if ([identifier isEqualToString:kCustomDevicePreferenceValue] ||
		[identifier caseInsensitiveCompare:@"custom"] == NSOrderedSame)
	{
		return (Rtt::TargetDevice::Skin)kCustomDeviceMenuTag;
	}

	Rtt::TargetDevice::Skin skin =
		Rtt::TargetDevice::FindSkinForLabel([identifier UTF8String]);
	if (skin == Rtt::TargetDevice::kUnknownSkin &&
		![identifier hasPrefix:@"project:"])
	{
		NSString *projectIdentifier = [@"project:" stringByAppendingString:identifier];
		skin = Rtt::TargetDevice::FindSkinForLabel([projectIdentifier UTF8String]);
	}
	return skin;
}

-(NSDictionary*)simulatorCustomDeviceInfoWithWidth:(NSInteger)width
		height:(NSInteger)height
		safeAreaInsetTop:(NSInteger)top
		safeAreaInsetLeft:(NSInteger)left
		safeAreaInsetBottom:(NSInteger)bottom
		safeAreaInsetRight:(NSInteger)right
		roundedCorners:(BOOL)roundedCorners
{
	NSDictionary *safeAreaInsets = [NSDictionary dictionaryWithObjectsAndKeys:
		[NSNumber numberWithInteger:top], @"top",
		[NSNumber numberWithInteger:left], @"left",
		[NSNumber numberWithInteger:bottom], @"bottom",
		[NSNumber numberWithInteger:right], @"right",
		nil];
	return [NSDictionary dictionaryWithObjectsAndKeys:
		@"custom", @"id",
		@"Custom", @"name",
		@"Custom", @"category",
		[NSNumber numberWithInteger:width], @"width",
		[NSNumber numberWithInteger:height], @"height",
		[NSNumber numberWithBool:YES], @"isCustom",
		[NSNumber numberWithBool:NO], @"isProject",
		[NSNumber numberWithBool:roundedCorners], @"roundedCorners",
		safeAreaInsets, @"safeAreaInsets",
		nil];
}

-(NSDictionary*)simulatorDeviceInfoForSkin:(Rtt::TargetDevice::Skin)skin
		roundedCorners:(BOOL)roundedCorners
{
	if (skin == kCustomDeviceMenuTag)
	{
		NSDictionary *safeAreaInsets = [self customDeviceSafeAreaInsets];
		return [self simulatorCustomDeviceInfoWithWidth:[self customDeviceWidth]
			height:[self customDeviceHeight]
			safeAreaInsetTop:[[safeAreaInsets objectForKey:@"top"] integerValue]
			safeAreaInsetLeft:[[safeAreaInsets objectForKey:@"left"] integerValue]
			safeAreaInsetBottom:[[safeAreaInsets objectForKey:@"bottom"] integerValue]
			safeAreaInsetRight:[[safeAreaInsets objectForKey:@"right"] integerValue]
			roundedCorners:roundedCorners];
	}

	const char *skinIdentifier = Rtt::TargetDevice::LabelForSkin(skin);
	const char *skinName = Rtt::TargetDevice::NameForSkin(skin);
	const char *skinCategory = Rtt::TargetDevice::CategoryForSkin(skin);
	BOOL isProject = skinIdentifier && 0 == strncmp(skinIdentifier, "project:", 8);
	NSDictionary *safeAreaInsets = [NSDictionary dictionaryWithObjectsAndKeys:
		[NSNumber numberWithInt:Rtt::TargetDevice::SafeAreaInsetTopForSkin(skin)], @"top",
		[NSNumber numberWithInt:Rtt::TargetDevice::SafeAreaInsetLeftForSkin(skin)], @"left",
		[NSNumber numberWithInt:Rtt::TargetDevice::SafeAreaInsetBottomForSkin(skin)], @"bottom",
		[NSNumber numberWithInt:Rtt::TargetDevice::SafeAreaInsetRightForSkin(skin)], @"right",
		nil];
	return [NSDictionary dictionaryWithObjectsAndKeys:
		[NSString stringWithExternalString:skinIdentifier ? skinIdentifier : ""], @"id",
		[NSString stringWithExternalString:skinName ? skinName : ""], @"name",
		[NSString stringWithExternalString:skinCategory ? skinCategory : ""], @"category",
		[NSNumber numberWithInt:Rtt::TargetDevice::WidthForSkin(skin)], @"width",
		[NSNumber numberWithInt:Rtt::TargetDevice::HeightForSkin(skin)], @"height",
		[NSNumber numberWithBool:NO], @"isCustom",
		[NSNumber numberWithBool:isProject], @"isProject",
		[NSNumber numberWithBool:roundedCorners], @"roundedCorners",
		safeAreaInsets, @"safeAreaInsets",
		nil];
}

-(NSDictionary*)persistentSimulatorDeviceInfo
{
	return [self simulatorDeviceInfoForSkin:(Rtt::TargetDevice::Skin)fSkin
		roundedCorners:fAgentMode ? NO : ![[NSUserDefaults standardUserDefaults] boolForKey:@"disableRoundedCorners"]];
}

-(NSDictionary*)configuredSimulatorDeviceInfo
{
	return fTemporarySimulatorDeviceInfo ? fTemporarySimulatorDeviceInfo : [self persistentSimulatorDeviceInfo];
}

-(BOOL)configuredSimulatorRoundedCorners
{
	return [[[self configuredSimulatorDeviceInfo] objectForKey:@"roundedCorners"] boolValue];
}

-(void)clearTemporarySimulatorConfiguration
{
	[fTemporarySimulatorDeviceInfo release];
	fTemporarySimulatorDeviceInfo = nil;
}

-(BOOL)refreshTemporarySimulatorConfiguration
{
	if (!fTemporarySimulatorDeviceInfo)
	{
		return YES;
	}

	NSString *identifier = [fTemporarySimulatorDeviceInfo objectForKey:@"id"];
	if ([[fTemporarySimulatorDeviceInfo objectForKey:@"isCustom"] boolValue])
	{
		fSkin = kCustomDeviceMenuTag;
	}
	else
	{
		Rtt::TargetDevice::Skin skin = [self skinForSimulatorDeviceIdentifier:identifier];
		if (skin == Rtt::TargetDevice::kUnknownSkin)
		{
			Rtt_LogException(
				"ERROR: Temporary Simulator device '%s' no longer exists",
				[identifier UTF8String]);
			return NO;
		}

		BOOL roundedCorners = [[fTemporarySimulatorDeviceInfo objectForKey:@"roundedCorners"] boolValue];
		NSDictionary *refreshedDeviceInfo = [self simulatorDeviceInfoForSkin:skin roundedCorners:roundedCorners];
		[fTemporarySimulatorDeviceInfo release];
		fTemporarySimulatorDeviceInfo = [refreshedDeviceInfo copy];
		fSkin = skin;
	}

	[self invalidateViewAsMenu];
	[self updateMenuForSkinChange];
	return YES;
}

-(void)persistSimulatorDeviceInfo:(NSDictionary*)deviceInfo
{
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	[defaults setBool:![[deviceInfo objectForKey:@"roundedCorners"] boolValue]
		forKey:@"disableRoundedCorners"];

	Rtt::TargetDevice::Skin skin = [self skinForSimulatorDeviceIdentifier:
		[deviceInfo objectForKey:@"id"]];
	if ([[deviceInfo objectForKey:@"isCustom"] boolValue])
	{
		NSDictionary *safeAreaInsets = [deviceInfo objectForKey:@"safeAreaInsets"];
		NSDictionary *values = [NSDictionary dictionaryWithObjectsAndKeys:
			[deviceInfo objectForKey:@"width"], kCustomDeviceWidthPreference,
			[deviceInfo objectForKey:@"height"], kCustomDeviceHeightPreference,
			[safeAreaInsets objectForKey:@"top"], kCustomDeviceSafeAreaInsetTopPreference,
			[safeAreaInsets objectForKey:@"left"], kCustomDeviceSafeAreaInsetLeftPreference,
			[safeAreaInsets objectForKey:@"bottom"], kCustomDeviceSafeAreaInsetBottomPreference,
			[safeAreaInsets objectForKey:@"right"], kCustomDeviceSafeAreaInsetRightPreference,
			nil];
		for (NSString *preference in values)
		{
			[defaults setObject:[values objectForKey:preference] forKey:preference];
			NSString *projectKey =
				[self getAppSpecificPreferenceKeyName:preference withProjectPath:fAppPath];
			if (projectKey)
			{
				[defaults setObject:[values objectForKey:preference] forKey:projectKey];
			}
		}
		skin = (Rtt::TargetDevice::Skin)kCustomDeviceMenuTag;
	}

	if (skin != Rtt::TargetDevice::kUnknownSkin)
	{
		if (fSkin != skin)
		{
			[self setSkin:skin];
		}
		else
		{
			[self saveUserSkinSetting];
			[self updateMenuForSkinChange];
		}
	}
	[self invalidateViewAsMenu];
}

-(BOOL)applyLaunchSimulatorConfiguration
{
	if (fLaunchSimulatorConfigurationHandled)
	{
		return YES;
	}

	NSDictionary *arguments = [[NSUserDefaults standardUserDefaults]
		volatileDomainForName:NSArgumentDomain];
	id device = [arguments objectForKey:kSimulatorDeviceArgument];
	id width = [arguments objectForKey:kSimulatorWidthArgument];
	id height = [arguments objectForKey:kSimulatorHeightArgument];
	id safeAreaTop = [arguments objectForKey:kSimulatorSafeAreaTopArgument];
	id safeAreaLeft = [arguments objectForKey:kSimulatorSafeAreaLeftArgument];
	id safeAreaBottom = [arguments objectForKey:kSimulatorSafeAreaBottomArgument];
	id safeAreaRight = [arguments objectForKey:kSimulatorSafeAreaRightArgument];
	id roundedCorners = [arguments objectForKey:kSimulatorRoundedCornersArgument];
	BOOL hasCustomDimensions = width || height;
	BOOL hasSafeAreaInsets =
		safeAreaTop || safeAreaLeft || safeAreaBottom || safeAreaRight;

	if (!device && !hasCustomDimensions && !hasSafeAreaInsets && !roundedCorners)
	{
		fLaunchSimulatorConfigurationHandled = YES;
		return YES;
	}
	if ((device && hasCustomDimensions) ||
		(hasCustomDimensions && (!width || !height)) ||
		(hasSafeAreaInsets && !hasCustomDimensions))
	{
		Rtt_LogException(
			"ERROR: Launch configuration must specify either -%s or both -%s and -%s; "
			"safe area arguments require custom dimensions",
			[kSimulatorDeviceArgument UTF8String],
			[kSimulatorWidthArgument UTF8String],
			[kSimulatorHeightArgument UTF8String]);
		return NO;
	}

	NSMutableDictionary *configuration = [NSMutableDictionary dictionaryWithCapacity:9];
	if (device)
	{
		if (![device isKindOfClass:[NSString class]] || ![device length])
		{
			Rtt_LogException(
				"ERROR: -%s expects a non-empty device identifier",
				[kSimulatorDeviceArgument UTF8String]);
			return NO;
		}
		[configuration setObject:device forKey:@"deviceId"];
	}
	else if (hasCustomDimensions)
	{
		NSInteger parsedWidth = 0;
		NSInteger parsedHeight = 0;
		NSInteger parsedSafeAreaTop = 0;
		NSInteger parsedSafeAreaLeft = 0;
		NSInteger parsedSafeAreaBottom = 0;
		NSInteger parsedSafeAreaRight = 0;
		if (!ReadSimulatorIntegerArgument(width, &parsedWidth) ||
			!ReadSimulatorIntegerArgument(height, &parsedHeight) ||
			(safeAreaTop && !ReadSimulatorIntegerArgument(safeAreaTop, &parsedSafeAreaTop)) ||
			(safeAreaLeft && !ReadSimulatorIntegerArgument(safeAreaLeft, &parsedSafeAreaLeft)) ||
			(safeAreaBottom && !ReadSimulatorIntegerArgument(safeAreaBottom, &parsedSafeAreaBottom)) ||
			(safeAreaRight && !ReadSimulatorIntegerArgument(safeAreaRight, &parsedSafeAreaRight)))
		{
			Rtt_LogException("ERROR: Simulator launch dimensions and safe area insets must be integers");
			return NO;
		}

		[configuration setObject:[NSNumber numberWithInteger:parsedWidth] forKey:@"deviceWidth"];
		[configuration setObject:[NSNumber numberWithInteger:parsedHeight] forKey:@"deviceHeight"];
		[configuration setObject:[NSNumber numberWithInteger:parsedSafeAreaTop] forKey:@"safeAreaInsetTop"];
		[configuration setObject:[NSNumber numberWithInteger:parsedSafeAreaLeft] forKey:@"safeAreaInsetLeft"];
		[configuration setObject:[NSNumber numberWithInteger:parsedSafeAreaBottom] forKey:@"safeAreaInsetBottom"];
		[configuration setObject:[NSNumber numberWithInteger:parsedSafeAreaRight] forKey:@"safeAreaInsetRight"];
	}

	if (roundedCorners)
	{
		BOOL parsedRoundedCorners = NO;
		if (!ReadSimulatorBooleanArgument(roundedCorners, &parsedRoundedCorners))
		{
			Rtt_LogException(
				"ERROR: -%s expects true or false",
				[kSimulatorRoundedCornersArgument UTF8String]);
			return NO;
		}
		[configuration setObject:[NSNumber numberWithBool:parsedRoundedCorners]
			forKey:@"roundedCorners"];
	}

	[configuration setObject:[NSNumber numberWithBool:YES] forKey:@"temporary"];
	if (![self applySimulatorConfiguration:configuration
		relaunchIfNeeded:NO
		scheduleRelaunch:NO
		didScheduleRelaunch:NULL])
	{
		Rtt_LogException("ERROR: The Simulator could not apply its launch configuration");
		return NO;
	}
	fLaunchSimulatorConfigurationHandled = YES;
	return YES;
}

-(NSDictionary*)simulatorDeviceInfo
{
	return fSimulator && fActiveSimulatorDeviceInfo ? fActiveSimulatorDeviceInfo : [self configuredSimulatorDeviceInfo];
}

-(NSDictionary*)simulatorStateInfo
{
	BOOL isSuspended = fSimulator && fSimulator->GetPlayer() &&
		fSimulator->GetPlayer()->GetRuntime().IsSuspended();
	NSWindow *window = [self currentWindow];
	NSRect frame = window ? [window frame] : NSZeroRect;
	NSDictionary *windowInfo = [NSDictionary dictionaryWithObjectsAndKeys:
		[NSNumber numberWithDouble:frame.origin.x], @"x",
		[NSNumber numberWithDouble:frame.origin.y], @"y",
		[NSNumber numberWithDouble:frame.size.width], @"width",
		[NSNumber numberWithDouble:frame.size.height], @"height",
		[NSNumber numberWithDouble:window ? [window backingScaleFactor] : 1.0], @"backingScale",
		[NSNumber numberWithBool:window &&
			(([window styleMask] & NSWindowStyleMaskFullScreen) == NSWindowStyleMaskFullScreen)], @"isFullscreen",
		nil];

	return [NSDictionary dictionaryWithObjectsAndKeys:
		[self simulatorDeviceInfo], @"device",
		[NSNumber numberWithBool:isSuspended], @"isSuspended",
		[NSNumber numberWithBool:[[NSUserDefaults standardUserDefaults]
			boolForKey:kShowSafeAreaGuidesPreference]], @"safeAreaGuidesVisible",
		[NSNumber numberWithBool:fSimulatorRelaunchPending], @"isRelaunchPending",
		[NSNumber numberWithLong:fRelaunchCount], @"relaunchCount",
		windowInfo, @"window",
		nil];
}

-(NSArray*)simulatorDevices
{
	NSMutableArray *devices = [NSMutableArray arrayWithCapacity:Rtt::TargetDevice::fSkinCount + 1];
	NSDictionary *currentDevice = [self simulatorDeviceInfo];
	NSString *currentDeviceIdentifier = [currentDevice objectForKey:@"id"];
	for (int skin = 0; skin < Rtt::TargetDevice::fSkinCount; skin++)
	{
		const char *identifier = Rtt::TargetDevice::LabelForSkin(skin);
		NSString *identifierString = [NSString stringWithExternalString:identifier ? identifier : ""];
		const char *name = Rtt::TargetDevice::NameForSkin(skin);
		const char *category = Rtt::TargetDevice::CategoryForSkin(skin);
		BOOL isProject = identifier && 0 == strncmp(identifier, "project:", 8);
		NSDictionary *safeAreaInsets = [NSDictionary dictionaryWithObjectsAndKeys:
			[NSNumber numberWithInt:Rtt::TargetDevice::SafeAreaInsetTopForSkin(skin)], @"top",
			[NSNumber numberWithInt:Rtt::TargetDevice::SafeAreaInsetLeftForSkin(skin)], @"left",
			[NSNumber numberWithInt:Rtt::TargetDevice::SafeAreaInsetBottomForSkin(skin)], @"bottom",
			[NSNumber numberWithInt:Rtt::TargetDevice::SafeAreaInsetRightForSkin(skin)], @"right",
			nil];
		[devices addObject:[NSDictionary dictionaryWithObjectsAndKeys:
			identifierString, @"id",
			[NSString stringWithExternalString:name ? name : ""], @"name",
			[NSString stringWithExternalString:category ? category : ""], @"category",
			[NSNumber numberWithInt:Rtt::TargetDevice::WidthForSkin(skin)], @"width",
			[NSNumber numberWithInt:Rtt::TargetDevice::HeightForSkin(skin)], @"height",
			[NSNumber numberWithBool:NO], @"isCustom",
			[NSNumber numberWithBool:isProject], @"isProject",
			[NSNumber numberWithBool:[currentDeviceIdentifier isEqualToString:identifierString]], @"isCurrent",
			safeAreaInsets, @"safeAreaInsets",
			nil]];
	}

	BOOL isCustomCurrent = [currentDeviceIdentifier isEqualToString:@"custom"];
	NSDictionary *customSafeAreaInsets = isCustomCurrent
		? [currentDevice objectForKey:@"safeAreaInsets"]
		: [self customDeviceSafeAreaInsets];
	NSNumber *customWidth = isCustomCurrent
		? [currentDevice objectForKey:@"width"]
		: [NSNumber numberWithInteger:[self customDeviceWidth]];
	NSNumber *customHeight = isCustomCurrent
		? [currentDevice objectForKey:@"height"]
		: [NSNumber numberWithInteger:[self customDeviceHeight]];
	[devices addObject:[NSDictionary dictionaryWithObjectsAndKeys:
		@"custom", @"id",
		@"Custom", @"name",
		@"Custom", @"category",
		customWidth, @"width",
		customHeight, @"height",
		[NSNumber numberWithBool:YES], @"isCustom",
		[NSNumber numberWithBool:NO], @"isProject",
		[NSNumber numberWithBool:isCustomCurrent], @"isCurrent",
		customSafeAreaInsets, @"safeAreaInsets",
		nil]];

	return devices;
}

- (IBAction) selectCustomDeviceAction:(id)sender
{
	[self clearTemporarySimulatorConfiguration];
	NSDictionary *safeAreaInsets = [self customDeviceSafeAreaInsets];
	[self setSimulatorCustomWidth:[self customDeviceWidth]
		height:[self customDeviceHeight]
		safeAreaInsetTop:[[safeAreaInsets objectForKey:@"top"] integerValue]
		safeAreaInsetLeft:[[safeAreaInsets objectForKey:@"left"] integerValue]
		safeAreaInsetBottom:[[safeAreaInsets objectForKey:@"bottom"] integerValue]
		safeAreaInsetRight:[[safeAreaInsets objectForKey:@"right"] integerValue]];
}

- (IBAction) editCustomDeviceAction:(id)sender
{
	NSAlert *alert = [[NSAlert alloc] init];
	[alert setMessageText:@"Custom Device"];
	[alert setInformativeText:@"Enter the simulated dimensions and safe area insets in pixels. The project will relaunch using this device."];
	[alert addButtonWithTitle:@"Apply"];
	[alert addButtonWithTitle:@"Cancel"];

	NSView *accessory = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 360, 150)];
	NSTextField *dimensionsLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 126, 160, 20)];
	NSTextField *safeAreaLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 70, 180, 20)];
	NSTextField *widthLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 98, 60, 20)];
	NSTextField *heightLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(190, 98, 60, 20)];
	NSTextField *topLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 42, 60, 20)];
	NSTextField *bottomLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(190, 42, 60, 20)];
	NSTextField *leftLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 10, 60, 20)];
	NSTextField *rightLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(190, 10, 60, 20)];
	NSTextField *widthField = [[NSTextField alloc] initWithFrame:NSMakeRect(65, 96, 105, 24)];
	NSTextField *heightField = [[NSTextField alloc] initWithFrame:NSMakeRect(255, 96, 105, 24)];
	NSTextField *topField = [[NSTextField alloc] initWithFrame:NSMakeRect(65, 40, 105, 24)];
	NSTextField *bottomField = [[NSTextField alloc] initWithFrame:NSMakeRect(255, 40, 105, 24)];
	NSTextField *leftField = [[NSTextField alloc] initWithFrame:NSMakeRect(65, 8, 105, 24)];
	NSTextField *rightField = [[NSTextField alloc] initWithFrame:NSMakeRect(255, 8, 105, 24)];

	NSArray *labels = @[
		dimensionsLabel, safeAreaLabel,
		widthLabel, heightLabel, topLabel, bottomLabel, leftLabel, rightLabel
	];
	for (NSTextField *label in labels)
	{
		[label setBezeled:NO];
		[label setDrawsBackground:NO];
		[label setEditable:NO];
		[label setSelectable:NO];
	}
	[dimensionsLabel setFont:[NSFont boldSystemFontOfSize:[NSFont systemFontSize]]];
	[safeAreaLabel setFont:[NSFont boldSystemFontOfSize:[NSFont systemFontSize]]];
	[dimensionsLabel setStringValue:@"Dimensions"];
	[safeAreaLabel setStringValue:@"Safe Area Insets"];
	[widthLabel setStringValue:@"Width:"];
	[heightLabel setStringValue:@"Height:"];
	[topLabel setStringValue:@"Top:"];
	[bottomLabel setStringValue:@"Bottom:"];
	[leftLabel setStringValue:@"Left:"];
	[rightLabel setStringValue:@"Right:"];

	NSDictionary *safeAreaInsets = [self customDeviceSafeAreaInsets];
	[widthField setIntegerValue:[self customDeviceWidth]];
	[heightField setIntegerValue:[self customDeviceHeight]];
	[topField setIntegerValue:[[safeAreaInsets objectForKey:@"top"] integerValue]];
	[leftField setIntegerValue:[[safeAreaInsets objectForKey:@"left"] integerValue]];
	[bottomField setIntegerValue:[[safeAreaInsets objectForKey:@"bottom"] integerValue]];
	[rightField setIntegerValue:[[safeAreaInsets objectForKey:@"right"] integerValue]];

	for (NSView *view in @[
		dimensionsLabel, safeAreaLabel,
		widthLabel, heightLabel, topLabel, bottomLabel, leftLabel, rightLabel,
		widthField, heightField, topField, bottomField, leftField, rightField
	])
	{
		[accessory addSubview:view];
	}
	[alert setAccessoryView:accessory];
	[alert.window setInitialFirstResponder:widthField];

	if ([alert runModal] == NSAlertFirstButtonReturn)
	{
		NSInteger width = [widthField integerValue];
		NSInteger height = [heightField integerValue];
		NSInteger top = [topField integerValue];
		NSInteger left = [leftField integerValue];
		NSInteger bottom = [bottomField integerValue];
		NSInteger right = [rightField integerValue];
		if (width <= 0 || height <= 0 ||
			width > kMaximumCustomDeviceDimension || height > kMaximumCustomDeviceDimension ||
			top < 0 || left < 0 || bottom < 0 || right < 0 ||
			top + bottom > height || left + right > width)
		{
			NSBeep();
			NSRunAlertPanel(
				@"Invalid Custom Device",
				[NSString stringWithFormat:
					@"Width and height must be between 1 and %ld pixels. Insets must be non-negative and fit within those dimensions.",
					(long)kMaximumCustomDeviceDimension],
				@"OK", nil, nil);
		}
		else
		{
			[self clearTemporarySimulatorConfiguration];
			[self setSimulatorCustomWidth:width
				height:height
				safeAreaInsetTop:top
				safeAreaInsetLeft:left
				safeAreaInsetBottom:bottom
				safeAreaInsetRight:right];
		}
	}

	for (NSView *view in @[
		dimensionsLabel, safeAreaLabel,
		widthLabel, heightLabel, topLabel, bottomLabel, leftLabel, rightLabel,
		widthField, heightField, topField, bottomField, leftField, rightField
	])
	{
		[view release];
	}
	[accessory release];
	[alert release];
}

- (void) updateSimulatorDisplayMenuItems:(NSMenu*)windowMenu
{
	NSMenuItem *viewAsItem = [windowMenu itemWithTitle:kViewAsMenuItemName];
	NSInteger insertionIndex = [windowMenu indexOfItem:viewAsItem] + 1;

	NSMenuItem *roundedCornersItem = [windowMenu itemWithTitle:kRoundedCornersMenuItemName];
	if (!roundedCornersItem)
	{
		roundedCornersItem = [windowMenu insertItemWithTitle:kRoundedCornersMenuItemName
			action:@selector(toggleRoundedCornersAction:)
			keyEquivalent:@""
			atIndex:insertionIndex];
		[roundedCornersItem setTarget:self];
	}

	NSMenuItem *safeAreaGuidesItem = [windowMenu itemWithTitle:kShowSafeAreaGuidesMenuItemName];
	if (!safeAreaGuidesItem)
	{
		safeAreaGuidesItem = [windowMenu insertItemWithTitle:kShowSafeAreaGuidesMenuItemName
			action:@selector(toggleSafeAreaGuidesAction:)
			keyEquivalent:@""
			atIndex:[windowMenu indexOfItem:roundedCornersItem] + 1];
		[safeAreaGuidesItem setTarget:self];
	}

	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	[roundedCornersItem setState:[self configuredSimulatorRoundedCorners] ? NSOnState : NSOffState];
	[roundedCornersItem setEnabled:[self isRelaunchable]];
	[safeAreaGuidesItem setState:[defaults boolForKey:kShowSafeAreaGuidesPreference] ? NSOnState : NSOffState];
	[safeAreaGuidesItem setEnabled:[self isRunning]];
}

- (IBAction) toggleRoundedCornersAction:(id)sender
{
	NSDictionary *configuration = [NSDictionary dictionaryWithObjectsAndKeys:
		[NSNumber numberWithBool:![self configuredSimulatorRoundedCorners]], @"roundedCorners",
		[NSNumber numberWithBool:fTemporarySimulatorDeviceInfo != nil], @"temporary",
		nil];
	[self applySimulatorConfiguration:configuration
		relaunchIfNeeded:NO
		scheduleRelaunch:YES
		didScheduleRelaunch:NULL];
}

- (IBAction) toggleSafeAreaGuidesAction:(id)sender
{
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	[self setSimulatorSafeAreaGuidesVisible:![defaults boolForKey:kShowSafeAreaGuidesPreference]];
}

- (void) updateSafeAreaGuideOverlay
{
	GLView *screenView = [self layerHostView];
	NSView *container = [screenView superview];
	NSArray *subviews = [[container subviews] copy];
	for (NSView *subview in subviews)
	{
		if ([subview isKindOfClass:[SimulatorSafeAreaGuideView class]])
		{
			[subview removeFromSuperview];
		}
	}
	[subviews release];

	if (!screenView || fAgentMode || ![[NSUserDefaults standardUserDefaults] boolForKey:kShowSafeAreaGuidesPreference])
	{
		return;
	}

	NSDictionary *device = [self simulatorDeviceInfo];
	NSDictionary *insets = [device objectForKey:@"safeAreaInsets"];
	CGFloat deviceWidth = [[device objectForKey:@"width"] doubleValue];
	CGFloat deviceHeight = [[device objectForKey:@"height"] doubleValue];
	CGFloat scaleX = deviceWidth > 0 ? NSWidth([screenView bounds]) / deviceWidth : 1.0;
	CGFloat scaleY = deviceHeight > 0 ? NSHeight([screenView bounds]) / deviceHeight : 1.0;

	SimulatorSafeAreaGuideView *guide = [[SimulatorSafeAreaGuideView alloc]
		initWithFrame:[screenView frame]
		insetTop:[[insets objectForKey:@"top"] doubleValue] * scaleY
		insetLeft:[[insets objectForKey:@"left"] doubleValue] * scaleX
		insetBottom:[[insets objectForKey:@"bottom"] doubleValue] * scaleY
		insetRight:[[insets objectForKey:@"right"] doubleValue] * scaleX];
	[container addSubview:guide positioned:NSWindowAbove relativeTo:screenView];
	[guide release];
}

- (void)menuNeedsUpdate:(NSMenu *)menu
{
	// Remove the "Start dictation" and "Emoji" items from the Edit menu
	[[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"NSDisabledDictationMenuItem"];
	[[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"NSDisabledCharacterPaletteMenuItem"];

    // NSLog(@"menuNeedsUpdate: %@", menu);
    
    Rtt_ASSERT([[menu title] isEqualToString:kWindowMenuItemName]);
    
    NSMenuItem *viewAsItem = [menu itemWithTitle:kViewAsMenuItemName];
    NSMenu *viewAsMenu = [viewAsItem submenu];
    
    Rtt_ASSERT(viewAsMenu != nil);

	[self updateSimulatorDisplayMenuItems:menu];
    
    // If we haven't added any menu items yet
    if ([viewAsMenu numberOfItems] > 0)
    {
        return;
    }

    const char *itemTitle = NULL;
    NSString *lastSkinCategory = nil;
    int skinCount = 0;
    long itemCount = 0;
    NSFont *font = [NSFont systemFontOfSize:[NSFont systemFontSizeForControlSize:NSRegularControlSize]];
    NSMenu *parentMenu = viewAsMenu;

    while ((itemTitle = Rtt::TargetDevice::NameForSkin(skinCount)) != NULL)
    {
        int skinWidth = Rtt::TargetDevice::WidthForSkin(skinCount);
        int skinHeight = Rtt::TargetDevice::HeightForSkin(skinCount);
        NSString *skinCategory = [NSString stringWithExternalString:Rtt::TargetDevice::CategoryForSkin(skinCount)];
        NSMenuItem *newItem = [parentMenu insertItemWithTitle:[NSString stringWithExternalString:itemTitle]
                                                       action:@selector(viewAsAction:)
                                                keyEquivalent:@""
                                                      atIndex:itemCount];
        [newItem setTag:skinCount];

        NSMutableParagraphStyle* paragraphStyle = [[NSMutableParagraphStyle alloc] init];
        NSMutableArray *tabs = [NSMutableArray array];
        [tabs addObject:[[NSTextTab alloc] initWithTextAlignment:NSRightTextAlignment location:220 options:[NSDictionary dictionary]]];
        paragraphStyle.tabStops = tabs;
        NSMutableDictionary* attr = [[NSMutableDictionary alloc] initWithObjectsAndKeys:font, NSFontAttributeName, paragraphStyle, NSParagraphStyleAttributeName, nil ];
        NSString *widthHeight = [NSString stringWithFormat:@"\t%dx%d%@", skinWidth, skinHeight, (skinHeight < 1000 ? @"\u2007" : @"")]; // Unicode numeric space
        NSMutableAttributedString* formattedTitle = [[NSMutableAttributedString alloc] initWithString:[[newItem title] stringByAppendingString:widthHeight] attributes:attr];

        NSRange range = { ([formattedTitle length] - [widthHeight length]), [widthHeight length] };
        [attr setObject:[NSFont boldSystemFontOfSize:[NSFont labelFontSize]] forKey:NSFontAttributeName];
        [formattedTitle setAttributes:attr range:range];

        [newItem setAttributedTitle:formattedTitle];

        [attr release];
        [paragraphStyle release];
        [formattedTitle release];
       
        if (lastSkinCategory != nil && ! [skinCategory isEqualToString:lastSkinCategory])
        {
            [parentMenu insertItem:[NSMenuItem separatorItem] atIndex:itemCount];
            ++itemCount;
        }

        lastSkinCategory = skinCategory;

        ++skinCount;
        ++itemCount;
    }
    
	[viewAsMenu addItem:[NSMenuItem separatorItem]];

	NSString *customTitle = [NSString stringWithFormat:@"Custom (%ld \u00d7 %ld)",
		(long)[self customDeviceWidth], (long)[self customDeviceHeight]];
	NSMenuItem *customItem = [viewAsMenu addItemWithTitle:customTitle
		action:@selector(selectCustomDeviceAction:)
		keyEquivalent:@""];
	[customItem setTag:kCustomDeviceMenuTag];

	NSMenuItem *editCustomItem = [viewAsMenu addItemWithTitle:@"Edit Custom Device\u2026"
		action:@selector(editCustomDeviceAction:)
		keyEquivalent:@""];
	[editCustomItem setTag:kEditCustomDeviceMenuTag];

    // Make sure the current skin is checked
    [self updateMenuForSkinChange];
}

-(void)applicationDidFinishLaunching:(NSNotification*)aNotification
{
    // If requested, make this the only running Simulator.  Useful in IDE-like environments where you
    // want to "relaunch" the Simulator but do it by rerunning the executable (e.g. Sublime Text)
    if ([[NSUserDefaults standardUserDefaults] boolForKey:@"singleton"])
    {
        [self deduplicateRunningInstances];
    }
    
	// Use Key-Value-Observing (KVO) to listen for changes to properties
	[self addObserver:self forKeyPath:@"fSkin" options:NSKeyValueObservingOptionNew context:NULL];

    NSMenu *appMenu = [[NSApplication sharedApplication] mainMenu];
    NSMenuItem *windowMenuItem = [appMenu itemWithTitle:kWindowMenuItemName];
    NSMenu *windowMenu = [windowMenuItem submenu];
    [windowMenu setDelegate:self];

	if( NO == self.launchedWithFile )
	{
		[self startDebugAndOpenPanel];
	}
	self.launchedWithFile = NO;
	self.applicationHasBeenInitialized = YES;
	
	// Initialize random number generator
	srand((unsigned int) time(NULL));
	
	NSDictionary *userDefaultsDefaults = [NSDictionary dictionaryWithObjectsAndKeys: [NSNumber numberWithBool:YES], nil];
	[[NSUserDefaults standardUserDefaults] registerDefaults:userDefaultsDefaults];
	
	// Arrange for "Relaunch" (Cmd-R) to work on startup if we have any Recent documents (this matches the behavior on Windows)
    NSArray *recentDocuments = fAgentMode ? nil : [[NSDocumentController sharedDocumentController] recentDocumentURLs];

    if ( [recentDocuments count] > 0 )
    {
		NSString *appPath = [[recentDocuments objectAtIndex:0] path];

		// This will make fixing the recent documents menu in the Dock (and, to a lesser extent, in the File menu) easier later
		// by making "older" Simulators handle paths without "main.lua" at the end correctly (Simulators before 2565 remove the
		// last path component without checking to see what it is)
		if ([[appPath lastPathComponent] isEqualToString:@"main.lua"])
		{
			appPath = [appPath stringByDeletingLastPathComponent];
		}

		if ([[NSFileManager defaultManager] isReadableFileAtPath:[appPath stringByAppendingPathComponent:@"main.lua"]])
		{
			self.fAppPath = [appPath stringByStandardizingPath];
		}
	}
    
    //[[NSProcessInfo processInfo] setAutomaticTerminationSupportEnabled:YES];
    [[NSProcessInfo processInfo] disableSuddenTermination];
}

-(NSArray*)GetRecentDocuments
{
	NSArray *recentDocuments = [[NSDocumentController sharedDocumentController] recentDocumentURLs];
    return recentDocuments;
}

// Note: Formerly, we had most of this code (particularly coronaInit in applicationDidFinishLaunching.
// I moved to applicationWillFinishLaunching so the initialization could be done before application:openFile: is invoked.
// I believe this is generally safe enough because awakeFromNib gets called before applicationWillFinishLaunching.
-(void)applicationWillFinishLaunching:(NSNotification*)aNotification
{
	fServices = new Rtt::MacPlatformServices( *fConsolePlatform );
	fNextUpsellTime = 0;

	[self coronaInit:aNotification];
	
#ifdef Rtt_AUTHORING_SIMULATOR
	NSString *jhome = [[[NSBundle mainBundle] bundlePath] stringByAppendingPathComponent:@"Contents/jre/jdk/Contents/Home"];
	if([[NSFileManager defaultManager] fileExistsAtPath:jhome]) {
		setenv("JAVA_HOME", [jhome UTF8String], YES);
	}
#endif
}

- (void) startDebugAndOpenPanel
{
	using namespace Rtt;

	NSBundle* appBundle = [NSBundle mainBundle];
	NSFileManager* fileMgr = [NSFileManager defaultManager];

	NSString* appPath = [appBundle resourcePath];

	NSString* mainObjectFile = [NSString stringWithExternalString:Rtt_LUA_OBJECT_FILE( "main" )];

	NSString* mainScriptFile = [NSString stringWithExternalString:Rtt_LUA_SCRIPT_FILE( "main" )];

	bool runScriptOnly = false;
	NSString* scriptPath = nil;


	// Invoke as a projector. Therefore, we only allow pre-compiled scripts.
	// 
	// See if a main.lu file exists (i.e. if the bundle contains a *compiled*
	// rtt-based app). If no file exists, then set appPath to nil. That's 
	// the signal to look externally for a main app file (cmd-line args or
	// prompt user). If a file does exist, it must not be the empty dummy
	// object file or we consider it non-existent.
	if ( ! IsValidAppPath( fileMgr, appPath, mainObjectFile, nil )
		 || IsEmptyLuaObjectFile( [appPath stringByAppendingPathComponent:mainObjectFile] ) )
	{
		appPath = nil;
	}


	// User NSUserDefaults (NSArgumentDomain to capture command line arguements.
	// This ensures Cocoa doesn't get confused with application:openFile:
	NSUserDefaults* userdefaults = [NSUserDefaults standardUserDefaults];
	
	fOptions.connectToDebugger = [userdefaults boolForKey:@"debug"];
	
	NSString* runscriptpath = [userdefaults stringForKey:@"run"];
	if ( nil != runscriptpath )
	{
		runscriptpath = [runscriptpath stringByStandardizingPath];
		if ( [fileMgr fileExistsAtPath:runscriptpath]
				 && LuaContext::IsBinaryLua( [runscriptpath UTF8String] ) )
		{
			runScriptOnly = true;
			scriptPath = runscriptpath;
		}
	}
	
	[self restoreUserSkinSetting];
	
	fIsRemote  = [userdefaults boolForKey:@"remote"];

	NSString* projectpath = [userdefaults stringForKey:@"project"];
	if ( nil != projectpath )
	{
		BOOL isDirectory = YES;
		projectpath = [projectpath stringByStandardizingPath];
		if( [[NSFileManager defaultManager] fileExistsAtPath:projectpath isDirectory:&isDirectory] )
		{
			if(NO == isDirectory)
			{
				projectpath = [projectpath stringByDeletingLastPathComponent];
				// TODO: Maybe we should verify that main.lua is being passed in.
/*
				if( [[projectpath lastPathComponent] isEqualToString:[NSString stringWithExternalString:Rtt_LUA_SCRIPT_FILE( "main" )]] )
				{
				}
*/
			}
			if ( IsValidAppPath( fileMgr, projectpath, nil, mainScriptFile ) )
			{
				// Invoke as simulator, so only raw lua files are allowed here
				// No pre-compiled scripts are allowed
				appPath = projectpath;
			}
			else
			{
				Rtt_TRACE_SIM( ( "Error: Requested -project file (%s) is not a valid Solar2D path/file\n", [projectpath UTF8String] ) );
			}
		}
	}
	
	allowLuaExit = [userdefaults boolForKey:@"allowLuaExit"];
	// End of NSArgumentDomain argument parsing
	
		
	if ( runScriptOnly )
	{
		MacConsolePlatform platform;
		Rtt_Allocator& allocator = platform.GetAllocator();
		LuaContext* vm = LuaContext::New( & allocator, platform ); // vm cannot outlive platform
		vm->Initialize( platform, NULL );
		vm->DoFile( [scriptPath UTF8String], fOptions.connectToDebugger );
		LuaContext::Delete( vm );
		[[NSApplication sharedApplication] terminate:self];
	}
	else
	{
		// If no path, then prompt user to specify a valid path
		if ( ! appPath )
		{
			Rtt_ASSERT( ! fSimulator );
			if (fAgentMode)
			{
				NSString *message = @"Agent mode requires -project to reference a Solar2D project containing main.lua";
				fprintf(stderr, "ERROR: %s\n", [message UTF8String]);
				exit(EXIT_FAILURE);
			}
			// Reset the preference in case the Simulator crashes, it gets set again on normal exit
			[[NSUserDefaults standardUserDefaults] synchronize];
			
			[self openLastProject];
		}
		else if (![self runApp:appPath])
		{
			exit(EXIT_FAILURE);
		}
	}
}

-(NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender
{
	return NSTerminateNow;
}

-(void)applicationWillTerminate:(NSNotification*)aNotification
{
	using namespace Rtt;
	applicationIsTerminating = YES;

    // This is used by the Simulator Console to know when the session is over
    NSLog(@"Corona Simulator: Goodbye");

    // Restore the preference since the Simulator didn't crash
    [[NSUserDefaults standardUserDefaults] synchronize];

	delete fServices;
	[fPreferencesWindow release];

	delete fConsolePlatform;

	[fAppPath release];
	[self clearTemporarySimulatorConfiguration];
	[fActiveSimulatorDeviceInfo release];
	delete fSimulator;
	
	[self removeObserver:self forKeyPath:@"fSkin"];

}

- (void)applicationWillResignActive:(NSNotification *)aNotification
{
	if(self.simulator && self.simulator->GetPlayer() ) {
		Rtt::Runtime& runtime = self.simulator->GetPlayer()->GetRuntime();
		Rtt::WindowStateEvent e( false );
		runtime.DispatchEvent( e );
	}
}

- (void) applicationDidBecomeActive:(NSNotification *)notification
{
	if(self.simulator && self.simulator->GetPlayer() ) {
		Rtt::Runtime& runtime = self.simulator->GetPlayer()->GetRuntime();
		Rtt::WindowStateEvent e( true );
		runtime.DispatchEvent( e );
	}
}

-(BOOL)isRelaunchable
{
    return self.fAppPath != nil;
}

// This is used by the Main Menu to control enabling of things
-(BOOL) isRunning
{
    NSWindow *mainWindow = [NSApp mainWindow];
    if (nil != mainWindow)
    {
        return YES;
    }
    else
    {
        return NO;
    }
}

-(IBAction)showHelp:(id)sender
{
	fConsolePlatform->OpenURL( "https://coronalabs.com/links/simulator/documentation" );
}

-(NSString*)suspendResumeLabel
{
	using namespace Rtt;

	PlatformPlayer* player = fSimulator ? fSimulator->GetPlayer() : NULL;
	return ( player && player->GetRuntime().IsSuspended() ? @"Resume" : @"Suspend" );
}

-(IBAction)orderFrontStandardAboutPanel:(id)sender
{
	NSString *version = [[NSString alloc] initWithUTF8String:Rtt_STRING_BUILD_DATE];
	NSString *applicationVersion = [[NSString alloc] initWithUTF8String:Rtt_STRING_BUILD];
    // The file "Resources/Credits.rtfd" is also added to the About box
    NSDictionary *options = [[NSDictionary alloc] initWithObjectsAndKeys:
                             version, @"Version",
                             applicationVersion, @"ApplicationVersion",
                             @"", @"Copyright",
                             nil];

	[NSApp orderFrontStandardAboutPanelWithOptions:options];
	[options release];
	[applicationVersion release];
	[version release];
}

// -----------------------------------------------------------------------------
// BEGIN: Simulator UI (Preferences, Deauth, Open project)
// -----------------------------------------------------------------------------

-(IBAction)showPreferences:(id)sender
{
	if ( ! fPreferencesWindow )
	{
		[NSBundle loadNibNamed:@"Preferences" owner:self];
	}
    
	[fPreferencesWindow center];
	[fPreferencesWindow makeKeyAndOrderFront:self];
}

-(void) showOpenPanel:(NSString*)title withAccessoryView:(NSView*)accessoryView startDirectory:(NSString*)start_directory completionHandler:(void(^)(NSString* path))completionhandler
{
	// Run a NSOpenPanel until a valid path is found
	OpenProjectDelegate* delegate = [[OpenProjectDelegate alloc] initWithFileManager:[NSFileManager defaultManager]];

	NSOpenPanel* panel = [NSOpenPanel openPanel];
	[panel setAllowsMultipleSelection:NO];
	[panel setCanChooseDirectories:YES];
	[panel setCanChooseFiles:YES];
	[panel setDelegate:delegate];
	[panel setAccessoryView:accessoryView];
	// directory is deprecated. directoryURL is only available on 10.6.
	if(nil != start_directory)
	{
		Rtt_ASSERT( [start_directory isAbsolutePath] );
		NSURL* url = [NSURL fileURLWithPath:start_directory isDirectory:YES];
		[panel setDirectoryURL:url];
	}
	
	if ( title ) { [panel setTitle:title]; }

	// TODO: Should we allow .lu files?

	
	// Stackoverflow says retain the panel and release it in the callback.
	[panel retain];

	// Old code used runModal with a while loop on IsValidAppPath. But this was causing bad problems with the open panel
	// not updating/displaying files. runModal is bad anyway and this is much better.
	void (^handlePanelCompletion)(NSInteger) = ^(NSInteger result)
	{
		if (NSFileHandlingPanelCancelButton==result)
		{
			completionhandler(nil);
		}
		else
		{
			
			NSArray* filenames = [panel URLs]; Rtt_ASSERT( [filenames count] <= 1 );
			NSString* apppath = [[filenames lastObject] path];
			BOOL isdir = NO;
			
			// Used to test if a directory. 
			[[NSFileManager defaultManager] fileExistsAtPath:apppath isDirectory:&isdir];
			if ( ! isdir )
			{
				apppath = [apppath stringByDeletingLastPathComponent];
			}
			
			completionhandler(apppath);
			
		}
		[delegate release];
		[panel release];

	};

	[panel beginWithCompletionHandler:handlePanelCompletion];
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// END: Simulator UI (Preferences, Deauth, Open project)
// -----------------------------------------------------------------------------

-(BOOL)runApp:(NSString*)appPath
{
	BOOL result = NO; // did launch?

	fRuntimeErrorNotified = FALSE;

	// Only launch simulator if valid appPath exists
	if ( appPath )
	{
		NSBundle* appBundle = [NSBundle mainBundle];
		NSFileManager* fileMgr = [NSFileManager defaultManager];

		NSString* mainObjectFile = [NSString stringWithExternalString:Rtt_LUA_OBJECT_FILE( "main" )];

		// TODO: This is kind of hacky, but it works...
		// If the object/script file lies outside the bundle, then we specify
		// a resource path (appPath). However, for projectors, the resource
		// path must be the bundle's resourcePath; in addition, only object 
		// files (.lu) are allowed. This situation occurs when appPath is the 
		// bundle's resourcePath and a .lu file exists in the resource dir. 
		// In this case, we set appPath to nil which means look in the bundle's
		// resource directory.
		if ( [appPath isEqualToString:[appBundle resourcePath]]
			 && [fileMgr fileExistsAtPath:[appPath stringByAppendingPathComponent:mainObjectFile]] )
		{
			appPath = nil;
		}
		else
		{
			// Update Recent Items only if appPath lies outside the bundle
			if (!fAgentMode)
			{
				NSString* mainScriptFile = [NSString stringWithExternalString:Rtt_LUA_SCRIPT_FILE( "main" )];
				[[NSDocumentController sharedDocumentController] noteNewRecentDocumentURL:[NSURL fileURLWithPath:[appPath stringByAppendingPathComponent:mainScriptFile]]];
			}
		}

		self.fAppPath = [appPath stringByStandardizingPath];
		[self clearTemporarySimulatorConfiguration];
		[self reloadDeviceSkinsForProject:self.fAppPath];
		[self restoreUserSkinSetting];
		if (![self applyLaunchSimulatorConfiguration])
		{
			return NO;
		}

		// There is an inital state condition where we need to make sure the skin checkmarks have been checked.
		// This is mostly hit the very first time Solar2D is run since there is no previous skin and
		// the default skin was setup before KVO was setup (in init) so we need to force a menu update.
		[self updateMenuForSkinChange];

		[self launchSimulator:nil];
		result = YES;
	}

	return result;
}

-(NSView*)openAccessoryView
{
    return nil;
}


-(void)setSkin:(Rtt::TargetDevice::Skin)skin
{
	self.fSkin = skin;
}

-(void)openLastProject
{
    NSArray *recentDocuments = [[NSDocumentController sharedDocumentController] recentDocumentURLs];
    if ( [recentDocuments count] > 0 )
    {
        NSURL *fileURL = [recentDocuments objectAtIndex:0];
        NSString *path = [fileURL path];
        
        if( [fileURL isFileURL] && [[NSFileManager defaultManager] fileExistsAtPath:path] )
        {            
            [self application:nil openFile:path];            
        }
    }
}

-(IBAction)open:(id)sender
{
	void (^runProject)(NSString*) = ^(NSString* path)
	{
		if ( path )
		{
			// Save the chosen path into user defaults so we can use it as the starting path next time
			[[NSUserDefaults standardUserDefaults] setObject:path forKey:kOpenFolderPath];
		}

        [self closeSimulator:sender];
        [self runApp:path];
	};
	
	// Pull the user's last folder from user defaults for their convenience
	NSString* start_directory = [[NSUserDefaults standardUserDefaults] stringForKey:kOpenFolderPath];

	// We save the directory of the last opened project but the UX works better if we open the
	// directory that _contains_ the last project directory so remove the last path component
	start_directory = [start_directory stringByDeletingLastPathComponent];

	[self showOpenPanel:nil withAccessoryView:[self openAccessoryView] startDirectory:start_directory completionHandler:runProject];	
}

// Delegate callback that is triggered when the user selects an "Open Recent"
- (BOOL) application:(NSApplication*)theApplication openFile:(NSString*)filepath
{
	// This block will be run before applicationDidFinishLaunching if a file is passed in via command line
	// or if a file is double clicked.
	// This messes up initialization assumptions and the command line processing.
	// The applicationHasBeenInitialized flag is used to know if this is an application launch or continuing an already open application.
	// The launchedWithFile is used to prevent command line switches from interfering with the launch behavior.
	if(NO == self.applicationHasBeenInitialized)
	{
		self.launchedWithFile = YES;
	}
	
	BOOL isDirectory = YES;
	if( ! [[NSFileManager defaultManager] fileExistsAtPath:filepath isDirectory:&isDirectory] )
	{
		return NO;
	}

	NSAlert* alert = [[[NSAlert alloc] init] autorelease];
	[alert addButtonWithTitle:@"OK"];
	NSString* this_app_name = [[NSFileManager defaultManager] displayNameAtPath:[[NSBundle mainBundle] bundlePath]];
	[alert setAlertStyle:NSCriticalAlertStyle];

	if (YES == isDirectory)
	{
		// In theory, we should just pass to runApp, but passing an empty directory seems to lead to Solar2D quiting.
		NSString* mainScriptFile = [NSString stringWithExternalString:Rtt_LUA_SCRIPT_FILE( "main" )];
		NSString* fullpath = [filepath stringByAppendingPathComponent:mainScriptFile]; 
		if( ! [[NSFileManager defaultManager] fileExistsAtPath:fullpath] )
		{
			NSString* message = [NSString stringWithFormat:@"The folder \"%@\" could not be opened. %@ only accepts folders containing a main.lua file.", [filepath lastPathComponent], this_app_name];
			[alert setMessageText:message];
			[alert runModal];

			return YES;
		}

		// Postponing the actual opening of the app allows the Simulator to initialize itself
		[self performSelector:@selector(runApp:) withObject:filepath afterDelay:0.01];

		return YES;
	}
	else
	{
		// We should verify that main.lua is being passed in.
		if( [[filepath lastPathComponent] isEqualToString:[NSString stringWithExternalString:Rtt_LUA_SCRIPT_FILE( "main" )]] )
		{
			// chop off file name to pass just the directory because runApp expects just the path

			// Postponing the actual opening of the app allows the Simulator to initialize itself
			[self performSelector:@selector(runApp:) withObject:[filepath stringByDeletingLastPathComponent] afterDelay:0.01];

			return YES;
		}
		else
		{
			NSString* message = [NSString stringWithFormat:@"The document \"%@\" could not be opened. %@ only accepts main.lua files.", [filepath lastPathComponent], this_app_name];
			[alert setMessageText:message];
			[alert runModal];

			return YES;
		}
	}
}


- (void) handleOpenURL:(NSAppleEventDescriptor*)event replyEvent:(NSAppleEventDescriptor*)reply
{
	BOOL isbadurl = NO; 
	// URL Scheme/Syntax inspired by TextMate and MacVim
    // Example:
	// corona://open?url=file:///Applications/CoronaSDK/
	NSString* urlstring = [[event paramDescriptorForKeyword:keyDirectObject] stringValue];
    NSURL* url = [NSURL URLWithString:urlstring];
	
	if ( [[url host] isEqualToString:@"open"] )
	{
		NSMutableDictionary* dict = [NSMutableDictionary dictionary];		
        // Parse query ("url=file://...&line=14") into a dictionary
		for ( NSString* param in [[url query] componentsSeparatedByString:@"&"] )
		{
			NSArray* elts = [param componentsSeparatedByString:@"="];
			[dict setObject:[[elts objectAtIndex:1] stringByReplacingPercentEscapesUsingEncoding:NSUTF8StringEncoding] forKey:[[elts objectAtIndex:0] stringByReplacingPercentEscapesUsingEncoding:NSUTF8StringEncoding]];
		}

        // Actually open the file.
        NSString* file = [dict objectForKey:@"url"];
		NSURL* fileUrl = nil;
		if( nil != file )
		{
			// Only handle file:// right now.
			if( ([file length] > 7) && [@"file://" isEqualToString:[file substringToIndex:7]] )
			{
				// chop off file:// because it confuses the standardizing path stuff
				NSString* non_resource_specifier = [file substringFromIndex:7];
				NSString* sanitized_path = nil;
				// run stringByStandardizingPath (which calls stringByExpandingTildeInPath)
				sanitized_path = [non_resource_specifier stringByStandardizingPath];

				NSURL* fileUrl = [NSURL fileURLWithPath:sanitized_path];
				BOOL isfileurl = [fileUrl isFileURL];
				if( YES == isfileurl )
				{
					BOOL fileexists = [[NSFileManager defaultManager] fileExistsAtPath:sanitized_path];
				
					if( YES == fileexists )
					{
						// Now check for the skin option
						// corona://open?url=file:///Applications/Corona.268/SampleCode/GettingStarted/HelloWorld/main.lua&skin=iPhone4
						NSString* skin = [dict objectForKey:@"skin"];
						// Maybe we want to explicitly set a default case? Though this would be better left for the simulator to decide.
						if( nil != skin )
						{
							// We could check for the return value and throw an error, but maybe that is more annoying than useful.
							[self clearTemporarySimulatorConfiguration];
							[self setSkinForTitle:skin];
						}
						// Reuse the drag-launch code to launch the app
						[self application:nil openFile:sanitized_path];				
					}
					else
					{
						isbadurl = YES;
					}
				}
				else
				{
					isbadurl = YES;
				}

			}
			else
			{
				isbadurl = YES;
			}

		}
		else
		{
			isbadurl = YES;
		}
		
		if( NO == isbadurl )
		{
			[self application:nil openFile:[fileUrl path]];            
		}
		else
		{
			NSAlert *alert = [[NSAlert alloc] init];
			[alert addButtonWithTitle:NSLocalizedString(@"OK", @"Dialog button")];
			
			[alert setMessageText:NSLocalizedString(@"File Does Not Exist", @"File Does Not Exist")];
			[alert setInformativeText:[NSString stringWithFormat:NSLocalizedString(
				@"Could not find the requested file \"%@\"",
				@"File Does Not Exist"),
				[url query]]];
			
			[alert setAlertStyle:NSWarningAlertStyle];
			[alert runModal];
			[alert release];
		}
    }
	else if ( [[url host] isEqualToString:@"relaunch"] )
	{
		if ( [self isRelaunchable] )
		{
			NSMutableDictionary* dict = [NSMutableDictionary dictionary];		
			// Parse query ("url=file://...&line=14") into a dictionary
			for ( NSString* param in [[url query] componentsSeparatedByString:@"&"] )
			{
				NSArray* elts = [param componentsSeparatedByString:@"="];
				[dict setObject:[[elts objectAtIndex:1] stringByReplacingPercentEscapesUsingEncoding:NSUTF8StringEncoding] forKey:[[elts objectAtIndex:0] stringByReplacingPercentEscapesUsingEncoding:NSUTF8StringEncoding]];
			}
			
			// Now check for the skin option
			// corona://open?url=file:///Applications/Corona.268/SampleCode/GettingStarted/HelloWorld/main.lua&skin=iPhone4
			NSString* skin = [dict objectForKey:@"skin"];
			// Maybe we want to explicitly set a default case? Though this would be better left for the simulator to decide.
			if( nil != skin )
			{
				// We could check for the return value and throw an error, but maybe that is more annoying than useful.
				[self clearTemporarySimulatorConfiguration];
				[self setSkinForTitle:skin];
			}
			[self launchSimulator:nil];
		}
		else
		{
			NSAlert *alert = [[NSAlert alloc] init];
			[alert addButtonWithTitle:NSLocalizedString(@"OK", @"Dialog button")];
			
			[alert setMessageText:NSLocalizedString(@"Cannot Relaunch Simulator", @"Cannot Relaunch Simulator")];
			[alert setInformativeText:NSLocalizedString(@"No project is loaded that can be relaunched.", @"No project is loaded that can be relaunched.")];
			[alert setAlertStyle:NSWarningAlertStyle];
			[alert runModal];
			[alert release];
		}
	}
	else
	{
        NSAlert *alert = [[NSAlert alloc] init];
        [alert addButtonWithTitle:NSLocalizedString(@"OK", @"Dialog button")];
		
        [alert setMessageText:NSLocalizedString(@"Unknown URL Scheme", @"Unknown URL Scheme")];
        [alert setInformativeText:[NSString stringWithFormat:NSLocalizedString(
			@"This version of Solar2D does not support \"%@\""
			@" in its URL scheme.",
			@"Unknown URL Scheme"),
			[url host]]];
		
        [alert setAlertStyle:NSWarningAlertStyle];
        [alert runModal];
        [alert release];
    }	
}

- (void) closeSimulator:(id)sender
{
	using namespace Rtt;

	[NSObject cancelPreviousPerformRequestsWithTarget:self
		selector:@selector(resumeSimulatorAfterBackground)
		object:nil];
	fBackgroundedRuntime = NULL;
	[fActiveSimulatorDeviceInfo release];
	fActiveSimulatorDeviceInfo = nil;

	if ( fSimulator )
	{
		delete fSimulator;
		fSimulator = NULL;

		// Prevent subsequent launches from connecting to debugger --- must relaunch
		// process to connect to debugger
		fOptions.connectToDebugger = false;
	}
}

-(IBAction)close:(id)sender
{
	[self closeSimulator:sender];
}

-(IBAction)showProjectSandbox:(id)sender
{
	if (fSimulator != NULL)
	{
		const Rtt::MacPlatform& platform = static_cast< const Rtt::MacPlatform& >( fSimulator->GetPlayer()->GetPlatform() );
		NSString *sandboxPath = platform.GetSandboxPath(); Rtt_ASSERT( sandboxPath );

		[[NSWorkspace sharedWorkspace] selectFile:nil inFileViewerRootedAtPath:sandboxPath];
	}
}

- (IBAction) showProjectFiles:(id)sender
{
	if (! [fAppPath isEqualToString:@""])
	{
		[[NSWorkspace sharedWorkspace] selectFile:nil inFileViewerRootedAtPath:fAppPath];
	}
}

// The "Clear Project Sandbox" menuitem needs an ellipsis if the user has not not chosen to suppress
// the confirmation dialog so we do that here (binding the menuitem title doesn't work here)
- (BOOL) setClearProjectSandboxTitle
{
	NSString *menuitemTitle = @"Clear Project Sandbox";
	NSMenu *appMenu = [[NSApplication sharedApplication] mainMenu];
	NSMenuItem *fileMenuItem = [appMenu itemWithTitle:@"File"];
	NSMenu *fileMenu = [fileMenuItem submenu];
	NSMenuItem *clearProjectSandboxItem = [fileMenu itemWithTag:kClearProjectSandboxMenuTag];

	if (fAppPath != nil && [self isRunning])
	{
		char keyId[CC_MD5_DIGEST_LENGTH*2 + 1];
		MD5Hash( keyId, [fAppPath UTF8String] );
		NSString *suppressionPrefName = [NSString stringWithFormat:@"%@/%s", @"shouldSuppressClearProjectAlert", keyId];

		if (! [[NSUserDefaults standardUserDefaults] boolForKey:suppressionPrefName])
		{
			menuitemTitle = [menuitemTitle stringByAppendingString:@"…"];
		}
	}

	[clearProjectSandboxItem setTitle:menuitemTitle];
	[clearProjectSandboxItem setTag:kClearProjectSandboxMenuTag]; // this seems to be forgotten unless reset

	return [self isRunning];
}

// Handle the "Clear Project Sandbox" menu item which will confirm the action unless the user has previously
// checked the "Do not show this message again" checkbox
- (IBAction) clearProjectSandbox:(id)sender
{
	if (fSimulator != NULL)
	{
		NSString *suppressionPrefName = [self getAppSpecificPreferenceKeyName:@"shouldSuppressClearProjectAlert" withProjectPath:fAppPath];
		BOOL shouldSuppressAlert = [[NSUserDefaults standardUserDefaults] boolForKey:suppressionPrefName];
		NSAlert *alert = nil;

		if (! shouldSuppressAlert)
		{
			alert = [[NSAlert new] autorelease];

			[alert setMessageText:@"Clear Project Sandbox"];
			[alert setInformativeText:[NSString stringWithFormat:@"Are you sure you want to delete the contents of the sandbox for '%@'?\n\nThis will also clear any app preferences and restart the project", [fAppPath lastPathComponent]]];
			[alert setAlertStyle:NSAlertStyleWarning];
			[alert addButtonWithTitle:@"Yes"];
			[alert addButtonWithTitle:@"No"];
			[alert setShowsSuppressionButton:YES];

			[alert beginSheetModalForWindow:self.currentWindow completionHandler:^(NSModalResponse returnCode) {
				if (returnCode == NSAlertFirstButtonReturn)
				{
					[self doClearProjectSandbox];

					BOOL shouldSuppressAlert = ([[alert suppressionButton] state] == NSOnState);

					if (shouldSuppressAlert)
					{
						[[NSUserDefaults standardUserDefaults] setObject:@(shouldSuppressAlert) forKey:suppressionPrefName];
					}
				}
			}];
		}
		else
		{
			[self doClearProjectSandbox];
		}
	}
}

// Clear the contents of the current project's sandbox and remove any app preferences stored for the project
// from the user defaults.  Note the project is stopped before the removals and started after them
- (void) doClearProjectSandbox
{
	const Rtt::MacPlatform& platform = static_cast< const Rtt::MacPlatform& >( fSimulator->GetPlayer()->GetPlatform() );
	NSURL *sandboxURL = [NSURL fileURLWithPath:platform.GetSandboxPath()];

	// Enumerate app preferences
	NSArray *prefKeys = [[[NSUserDefaults standardUserDefaults] dictionaryRepresentation] allKeys];
	NSString *appPrefKeyPrefix = [self getAppSpecificPreferenceKeyName:@"appPreferences" withProjectPath:fAppPath];

	// Close the project while we're deleting things
	[self closeSimulator:self];

	// Trash sandbox files
	[[NSFileManager defaultManager] trashItemAtURL:sandboxURL resultingItemURL:nil error:nil];

	// Delete app preferences
	for (NSString* prefKey in prefKeys)
	{
		// Remove only the preferences for this project
		if ([prefKey hasPrefix:appPrefKeyPrefix])
		{
			// NSLog(@"doClearProjectSandbox: value: %@ forKey: %@", [[NSUserDefaults standardUserDefaults] valueForKey:prefKey], prefKey);
			[[NSUserDefaults standardUserDefaults] removeObjectForKey:prefKey];
		}
	}

	Rtt_Log("Project sandbox and preferences cleared");

	// Relaunch the project
	[self launchSimulator:self];
}

-(BOOL)setSkinForTitle:(NSString*)title
{
	using namespace Rtt;

	if ([title isEqualToString:kCustomDevicePreferenceValue] ||
		[title caseInsensitiveCompare:@"custom"] == NSOrderedSame)
	{
		[self setSkin:(TargetDevice::Skin)kCustomDeviceMenuTag];
		return YES;
	}

	if (!title)
	{
		return NO;
	}

	TargetDevice::Skin skin = Rtt::TargetDevice::FindSkinForLabel([title UTF8String]);
	if (skin == TargetDevice::kUnknownSkin)
	{
		return NO;
	}

	[self setSkin:skin];
	return YES;
}

- (void) restoreUserSkinSetting
{
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	NSString *skinname = [defaults stringForKey:kUserPreferenceUsersCurrentSelectedSkin];
	NSString *projectKey = [self getAppSpecificPreferenceKeyName:kUserPreferenceUsersCurrentSelectedSkin withProjectPath:fAppPath];
	if (projectKey && [defaults stringForKey:projectKey])
	{
		skinname = [defaults stringForKey:projectKey];
	}

	if ( nil != skinname )
	{
        if ( ! [self setSkinForTitle:skinname] )
        {
            Rtt_TRACE_SIM( ( "WARNING: Skin '%s' does not exist\n", [skinname UTF8String] ) );
			[self setSkin:Rtt::TargetDevice::kDefaultSkin];
        }
	}
}

- (void) saveUserSkinSetting
{
	NSString *skinName = nil;
	if (self.fSkin == kCustomDeviceMenuTag)
	{
		skinName = kCustomDevicePreferenceValue;
	}
	else
	{
		const char *skinString = Rtt::TargetDevice::LabelForSkin((Rtt::TargetDevice::Skin)self.fSkin);
		if (skinString)
		{
			skinName = [NSString stringWithExternalString:skinString];
		}
	}

	if (!skinName)
	{
		return;
	}

	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	NSString *projectKey = [self getAppSpecificPreferenceKeyName:kUserPreferenceUsersCurrentSelectedSkin withProjectPath:fAppPath];
	if (projectKey)
	{
		[defaults setObject:skinName forKey:projectKey];
	}

	// A project device does not exist until that project is loaded. Keeping it
	// as the global fallback would cause a spurious "unknown skin" warning the
	// next time the Simulator starts.
	if (![skinName hasPrefix:@"project:"])
	{
		[defaults setObject:skinName forKey:kUserPreferenceUsersCurrentSelectedSkin];
	}
}

- (void) updateMenuForSkinChange
{
    NSMenu *appMenu = [[NSApplication sharedApplication] mainMenu];
    NSMenuItem *windowMenuItem = [appMenu itemWithTitle:kWindowMenuItemName];
    NSMenu *windowMenu = [windowMenuItem submenu];
    NSMenuItem *viewAsItem = [windowMenu itemWithTitle:kViewAsMenuItemName];
    NSMenu *viewAsMenu = [viewAsItem submenu];

    Rtt_ASSERT(viewAsItem != nil);

    NSMutableArray *skinMenuItems = [NSMutableArray arrayWithArray:[viewAsMenu itemArray]];
    for (NSMenuItem *item in skinMenuItems)
    {
        if ([item tag] == self.fSkin)
        {
            [item setState:NSOnState];
        }
        else
        {
            [item setState:NSOffState];
        }
    }
}

- (void) observeValueForKeyPath:(NSString*)key_path
					  ofObject:(id)the_object 
						change:(NSDictionary*)the_change
					   context:(void*)the_context
{
	if([key_path isEqualToString:@"fSkin"])
	{
		[self saveUserSkinSetting];
		[self updateMenuForSkinChange];		
	}
}

-(IBAction)launchSimulator:(id)sender
{
	using namespace Rtt;
	fSimulatorRelaunchPending = NO;
	BOOL isRelaunch = fSimulator != NULL;

	// Detect relaunch
	if ( isRelaunch )
	{
		// Always close
		[self closeSimulator:sender];

		++fRelaunchCount;
	}

	const char *resourcePath = [self.fAppPath UTF8String];

	if (resourcePath == NULL)
	{
		if (fAgentMode)
		{
			NSString *message = @"Agent mode cannot launch without a project";
			fprintf(stderr, "ERROR: %s\n", [message UTF8String]);
			return;
		}
		[self open:sender];

		return;
	}

	if (isRelaunch)
	{
		[self reloadDeviceSkinsForProject:self.fAppPath];
		if (![self refreshTemporarySimulatorConfiguration])
		{
			[self clearTemporarySimulatorConfiguration];
			[self restoreUserSkinSetting];
		}
		else if (!fTemporarySimulatorDeviceInfo)
		{
			[self restoreUserSkinSetting];
		}
	}

	[self willChangeValueForKey:@"suspendResumeLabel"];

	fSimulator = new Rtt::MacSimulator;

	fRuntimeErrorNotified = false;
	
	// [1] Somewhere in Initialize (or its sub-calls), GLView's prepareOpenGL is invoked, and Runtime is instantiated
	NSDictionary *deviceInfo = [[self configuredSimulatorDeviceInfo] retain];
	[fActiveSimulatorDeviceInfo release];
	fActiveSimulatorDeviceInfo = deviceInfo;
	BOOL roundedCorners = [[deviceInfo objectForKey:@"roundedCorners"] boolValue];
	if ([[deviceInfo objectForKey:@"isCustom"] boolValue])
	{
		NSDictionary *safeAreaInsets = [deviceInfo objectForKey:@"safeAreaInsets"];
		fSimulator->Initialize(
			"Custom",
			(float)[[deviceInfo objectForKey:@"width"] doubleValue],
			(float)[[deviceInfo objectForKey:@"height"] doubleValue],
			(float)[[safeAreaInsets objectForKey:@"top"] doubleValue],
			(float)[[safeAreaInsets objectForKey:@"left"] doubleValue],
			(float)[[safeAreaInsets objectForKey:@"bottom"] doubleValue],
			(float)[[safeAreaInsets objectForKey:@"right"] doubleValue],
			roundedCorners,
			resourcePath );
	}
	else
	{
		Rtt::TargetDevice::Skin skin = [self skinForSimulatorDeviceIdentifier:
			[deviceInfo objectForKey:@"id"]];
		const char *skinFile = Rtt::TargetDevice::LuaObjectFileFromSkin(skin);

		// If all else fails, default to the default skin file
		if (skinFile == NULL)
		{
			skinFile = Rtt::TargetDevice::LuaObjectFileFromSkin( Rtt::TargetDevice::kDefaultSkin );
		}

		fSimulator->Initialize( skinFile, roundedCorners, resourcePath );
	}

	[self updateSafeAreaGuideOverlay];
	
	[self didChangeValueForKey:@"suspendResumeLabel"];
}

// [2] GLView's prepareOpenGL calls this method, so we know the OpenGL context is valid
-(void)didPrepareOpenGLContext:(id)sender
{
	// [3] This triggers Runtime::LoadApplication() and Runtime::BeginRunLoop()
	fSimulator->Start( fOptions );
}

- (GLView*) layerHostView
{
	if( NULL == fSimulator )
	{
		return nil;
	}
	else
	{
		return [[fSimulator->GetScreenView() retain] autorelease];
	}
}

- (Rtt::Runtime*) runtime
{
	if( fSimulator && fSimulator->GetPlayer() )
	{
		return &(fSimulator->GetPlayer()->GetRuntime());
	}
	return NULL;
}

-(IBAction)back:(id)sender
{
	using namespace Rtt;

	if (fSimulator != NULL)
	{
		// If the Lua handler for the back key returns false, exit the app
		if (! fSimulator->Back())
		{
			[self close:sender];
		}
	}
}

-(BOOL)dispatchSimulatorInput:(NSDictionary*)input
{
	using namespace Rtt;

	GLView *screenView = [self layerHostView];
	if (![screenView canDispatchEvents])
	{
		return NO;
	}

	NSString *type = [input objectForKey:@"type"];
	if ([type isEqualToString:@"back"])
	{
		[self back:nil];
		return YES;
	}
	else if ([type isEqualToString:@"key"])
	{
		NSString *phase = [input objectForKey:@"phase"];
		NSString *keyName = [input objectForKey:@"keyName"];
		NSString *qwertyKeyName = [input objectForKey:@"qwertyKeyName"];
		S32 nativeKeyCode = (S32)[[input objectForKey:@"nativeKeyCode"] intValue];
		bool isShiftDown = [[input objectForKey:@"isShiftDown"] boolValue];
		bool isAltDown = [[input objectForKey:@"isAltDown"] boolValue];
		bool isCtrlDown = [[input objectForKey:@"isCtrlDown"] boolValue];
		bool isCommandDown = [[input objectForKey:@"isCommandDown"] boolValue];

		BOOL wasDispatched = YES;
		if ([phase isEqualToString:@"down"] || [phase isEqualToString:@"pressed"])
		{
			KeyEvent event(
				NULL,
				KeyEvent::kDown,
				[keyName UTF8String],
				nativeKeyCode,
				isShiftDown,
				isAltDown,
				isCtrlDown,
				isCommandDown,
				[qwertyKeyName UTF8String] );
			wasDispatched = [screenView dispatchEvent:&event];
		}
		if (wasDispatched && ([phase isEqualToString:@"up"] || [phase isEqualToString:@"pressed"]))
		{
			KeyEvent event(
				NULL,
				KeyEvent::kUp,
				[keyName UTF8String],
				nativeKeyCode,
				isShiftDown,
				isAltDown,
				isCtrlDown,
				isCommandDown,
				[qwertyKeyName UTF8String] );
			wasDispatched = [screenView dispatchEvent:&event];
		}
		return wasDispatched;
	}
	else if ([type isEqualToString:@"touch"])
	{
		NSString *phaseName = [input objectForKey:@"phase"];
		TouchEvent::Phase phase = TouchEvent::kBegan;
		if ([phaseName isEqualToString:@"moved"])
		{
			phase = TouchEvent::kMoved;
		}
		else if ([phaseName isEqualToString:@"ended"])
		{
			phase = TouchEvent::kEnded;
		}
		else if ([phaseName isEqualToString:@"cancelled"])
		{
			phase = TouchEvent::kCancelled;
		}

		TouchEvent event(
			Rtt_FloatToReal([[input objectForKey:@"x"] doubleValue]),
			Rtt_FloatToReal([[input objectForKey:@"y"] doubleValue]),
			Rtt_FloatToReal([[input objectForKey:@"xStart"] doubleValue]),
			Rtt_FloatToReal([[input objectForKey:@"yStart"] doubleValue]),
			phase );
		static U32 sSimulatorTouchId;
		event.SetId(&sSimulatorTouchId);
		return [screenView dispatchTouchEvent:&event];
	}
	else if ([type isEqualToString:@"mouse"])
	{
		NSString *phaseName = [input objectForKey:@"phase"];
		MouseEvent::MouseEventType eventType = MouseEvent::kGeneric;
		if ([phaseName isEqualToString:@"up"])
		{
			eventType = MouseEvent::kUp;
		}
		else if ([phaseName isEqualToString:@"down"])
		{
			eventType = MouseEvent::kDown;
		}
		else if ([phaseName isEqualToString:@"drag"])
		{
			eventType = MouseEvent::kDrag;
		}
		else if ([phaseName isEqualToString:@"move"])
		{
			eventType = MouseEvent::kMove;
		}
		else if ([phaseName isEqualToString:@"exit"])
		{
			eventType = MouseEvent::kExit;
		}
		else if ([phaseName isEqualToString:@"scroll"])
		{
			eventType = MouseEvent::kScroll;
		}

		MouseEvent event(
			eventType,
			Rtt_FloatToReal([[input objectForKey:@"x"] doubleValue]),
			Rtt_FloatToReal([[input objectForKey:@"y"] doubleValue]),
			Rtt_FloatToReal([[input objectForKey:@"scrollX"] doubleValue]),
			Rtt_FloatToReal([[input objectForKey:@"scrollY"] doubleValue]),
			[[input objectForKey:@"clickCount"] intValue],
			[[input objectForKey:@"isPrimaryButtonDown"] boolValue],
			[[input objectForKey:@"isSecondaryButtonDown"] boolValue],
			[[input objectForKey:@"isMiddleButtonDown"] boolValue],
			[[input objectForKey:@"isShiftDown"] boolValue],
			[[input objectForKey:@"isAltDown"] boolValue],
			[[input objectForKey:@"isCtrlDown"] boolValue],
			[[input objectForKey:@"isCommandDown"] boolValue] );
		return [screenView dispatchEvent:&event];
	}
	return NO;
}

-(void)resumeSimulatorAfterBackground
{
	Rtt::Runtime *runtime = [self runtime];
	if (runtime && runtime == fBackgroundedRuntime && runtime->IsSuspended())
	{
		fSimulator->ToggleSuspendResume(true);
	}
	fBackgroundedRuntime = NULL;
}

-(BOOL)simulateSimulatorEvent:(NSDictionary*)event
{
	using namespace Rtt;

	Runtime *runtime = [self runtime];
	GLView *screenView = [self layerHostView];
	if (![screenView canDispatchEvents])
	{
		return NO;
	}

	NSString *type = [event objectForKey:@"type"];
	if ([type isEqualToString:@"memoryWarning"])
	{
		MemoryWarningEvent memoryWarning;
		return [screenView dispatchEvent:&memoryWarning];
	}
	else if ([type isEqualToString:@"background"])
	{
		[screenView setAllowOverlay:NO];
		fSimulator->ToggleSuspendResume(true);
		[screenView setAllowOverlay:YES];

		[NSObject cancelPreviousPerformRequestsWithTarget:self
			selector:@selector(resumeSimulatorAfterBackground)
			object:nil];
		fBackgroundedRuntime = runtime;
		NSTimeInterval duration = [[event objectForKey:@"duration"] doubleValue] / 1000.0;
		[self performSelector:@selector(resumeSimulatorAfterBackground) withObject:nil afterDelay:duration];
		return YES;
	}
	else if ([type isEqualToString:@"accelerometer"])
	{
		double gravity[] = {
			[[event objectForKey:@"xGravity"] doubleValue],
			[[event objectForKey:@"yGravity"] doubleValue],
			[[event objectForKey:@"zGravity"] doubleValue]
		};
		double instant[] = {
			[[event objectForKey:@"xInstant"] doubleValue],
			[[event objectForKey:@"yInstant"] doubleValue],
			[[event objectForKey:@"zInstant"] doubleValue]
		};
		double raw[] = {
			[[event objectForKey:@"xRaw"] doubleValue],
			[[event objectForKey:@"yRaw"] doubleValue],
			[[event objectForKey:@"zRaw"] doubleValue]
		};
		AccelerometerEvent accelerometer(
			gravity,
			instant,
			raw,
			[[event objectForKey:@"isShake"] boolValue],
			[[event objectForKey:@"deltaTime"] doubleValue] );
		return [screenView dispatchEvent:&accelerometer];
	}
	else if ([type isEqualToString:@"gyroscope"])
	{
		GyroscopeEvent gyroscope(
			[[event objectForKey:@"xRotation"] doubleValue],
			[[event objectForKey:@"yRotation"] doubleValue],
			[[event objectForKey:@"zRotation"] doubleValue],
			[[event objectForKey:@"deltaTime"] doubleValue] );
		return [screenView dispatchEvent:&gyroscope];
	}
	return NO;
}

-(IBAction)toggleSuspendResume:(id)sender
{
	if ( fSimulator )
	{
        // If the Shift key is down, tell the GLView not to display the graphical suspended state
        [[self layerHostView] setAllowOverlay:(([[NSApp currentEvent] modifierFlags] & NSShiftKeyMask) != NSShiftKeyMask)];
        
		[self willChangeValueForKey:@"suspendResumeLabel"];
		fSimulator->ToggleSuspendResume(true);
		[self didChangeValueForKey:@"suspendResumeLabel"];
        
        [[self layerHostView] setAllowOverlay:YES];
	}
}

-(NSWindow*)currentWindow
{
	return fSimulator ? fSimulator->GetWindow() : nil;
}

/*
static const int kCounterCycle = 30;

static void
RunLoopObserverCallback( CFRunLoopObserverRef observer, CFRunLoopActivity activity, void* info )
{
	if ( kCFRunLoopBeforeWaiting == activity )
	{
		int& counter = * (int*)info;
		counter--;
		if ( counter == 0 )
		{
			Rtt_TRACE( ( "COUNTER HIT 0!!!!!!!!!!!!!!!!!!\n" ) );
			counter = kCounterCycle;
		}
	}
}
*/

-(BOOL)isRunnable
{
	return YES;
}

-(void)alertDidEnd:(NSAlert *)alert returnCode:(int)returnCode contextInfo:(void  *)contextInfo
{
	[[NSApplication sharedApplication] stopModal];

	if ( contextInfo != nil && contextInfo == fPreferencesWindow )
	{
		if ( NSAlertFirstButtonReturn == returnCode )
		{
			[[alert window] close];
		}
	}
}

//This mimics growl send notification selector.
-(void)notifyWithTitle:(NSString*)title
		   description:(NSString*)description
			  iconData:(NSImage*)iconData
{
	if (fAgentMode)
	{
		fprintf(stderr, "%s%s%s\n",
			[title length] ? [title UTF8String] : "",
			[title length] && [description length] ? ": " : "",
			[description length] ? [description UTF8String] : "");
		return;
	}
	NSUserNotification *notification = [[[NSUserNotification alloc] init] autorelease];
	notification.title = title;
	notification.informativeText = description;
	notification.contentImage = iconData;
	notification.soundName = NSUserNotificationDefaultSoundName;
	[[NSUserNotificationCenter defaultUserNotificationCenter] deliverNotification:notification];
}

-(NSString *) getAppSpecificPreferenceKeyName:(NSString *)prefName withProjectPath:(NSString *)projectDirectoryPath
{
	if (!projectDirectoryPath)
	{
		return nil;
	}
	if (!prefName)
	{
		prefName = @"";
	}
	char keyId[CC_MD5_DIGEST_LENGTH*2 + 1];
	MD5Hash( keyId, [projectDirectoryPath UTF8String] );
	return [NSString stringWithFormat:@"%@/%s", prefName, keyId];
}

// Used by Rtt_MacAuthorizationDelegate.mm
- (void)didPresentError:(BOOL)didRecover contextInfo:(void*)contextInfo
{
	using namespace Rtt;

	ptrdiff_t code = (ptrdiff_t)contextInfo;
	const char *url = NULL;
	switch( code )
	{
	}

	if ( url )
	{
		fConsolePlatform->OpenURL( url );
	}
}

-(void)notifyRuntimeError:(NSString *)message
{
	if (fAgentMode)
	{
		fprintf(stderr, "Runtime error: %s\n", message ? [message UTF8String] : "Unknown runtime error");
		fRuntimeErrorNotified = true;
		return;
	}
	if ( !fRuntimeErrorNotified )
	{
		[self notifyWithTitle:@"Solar2D Simulator" description:message iconData:nil];
		fRuntimeErrorNotified = true;
	}
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// END: Device Build
// -----------------------------------------------------------------------------

#if !defined( Rtt_WEB_PLUGIN )

- (BOOL)userNotificationCenter:(NSUserNotificationCenter *)center shouldPresentNotification:(NSUserNotification *)notification{
	return YES;
}

#endif

// -----------------------------------------------------------------------------
// BEGIN: Simulator UI
// -----------------------------------------------------------------------------

- (void) setFAppPath:(NSString*)appPath
{
    if(fAppPath != appPath)
	{
		[fAppPath release];

		// This might be bad, but due to a bug caught by Sean doing commandline runs,
		// using relative paths broke the code.
		// So I am going to convert to an absolute path.
		if(nil == appPath)
		{
			fAppPath = nil;
		}
		else if( ! [appPath isAbsolutePath] )
		{
			fAppPath = [[[NSFileManager defaultManager] currentDirectoryPath] stringByAppendingPathComponent:appPath];
			[fAppPath retain];
		}
		else
		{
			fAppPath = [appPath stringByStandardizingPath];
			[fAppPath retain];
		}
	}
}

-(IBAction)changedPreference:(id)sender
{
	using namespace Rtt;
	
	// NSLog(@"changedPreference: %@", [sender description]);
    [[NSUserDefaults standardUserDefaults] synchronize];
}

// This subverts the standard Cocoa alert help system a little in that it uses the "helpAnchor" to store
// a normal URL that will be opened in the user's browser if they press the "?" on the dialog
- (BOOL) alertShowHelp:(NSAlert *) alert
{
    NSString *helpURL = [alert helpAnchor];

    if (helpURL != nil)
    {
        [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:helpURL]];
    }

    return YES;
}

// -----------------------------------------------------------------------------

- (NSString *) launchTaskAndReturnOutput:(NSString *)cmd arguments:(NSArray *)args
{
	NSString *result = nil;
	NSTask *task = [[NSTask alloc] init];
	NSPipe *stdoutPipe = [NSPipe pipe];
	NSPipe *stderrPipe = [NSPipe pipe];

	[task setLaunchPath:cmd];
	[task setArguments:args];

	[task setStandardOutput:stdoutPipe];
	[task setStandardError:stderrPipe];

	NSFileHandle *stdoutFileHandle = [stdoutPipe fileHandleForReading];
	NSFileHandle *stderrFileHandle = [stderrPipe fileHandleForReading];

	@try
	{
		[task launch];
		[task waitUntilExit];

		if (! [task isRunning] && [task terminationStatus] != 0)
		{
			// Command failed, emit any stderr to the log
			NSData *stderrData = [stderrFileHandle readDataToEndOfFile];
			NSLog(@"Error running %@ %@: %s", cmd, args, (const char *)[stderrData bytes]);
		}

		NSData *data = [stdoutFileHandle readDataToEndOfFile];
		result = [[[NSMutableString alloc] initWithData:data encoding:NSUTF8StringEncoding] autorelease];
	}
	@catch( NSException* exception )
	{
		NSLog( @"launchTaskAndReturnOutput: exception %@ (%@ %@)", exception, cmd, args );
	}
	@finally
	{
		[task release];
	}

	result = [result stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];

	return result;
}

- (IBAction)bringAllToFront:(id)sender
{
	[NSApp arrangeInFront:sender];
}

// -----------------------------------------------------------------------------
// END: Simulator UI
// -----------------------------------------------------------------------------

@end

//
// This subclass of NSApplication exists solely to limit the time the dock icon bounces when
// a modal sheet is displayed by the Simulator when it's in the background, typically after a
// build has completed (without this code the Dock icon bounces forever or until the Simulator
// is brought to the foreground)
//
// The time the bouncing continues can be set using the user preference "dockIconBounceTime"
// The default is 5 seconds which is enough time for 3 bounces (OSX 10.8.5)
//
// TODO: if CoronaSimulatorApplication gets any more complex, move it to a separate file
//

@implementation CoronaSimulatorApplication

@synthesize suppressAttentionRequests;

- (NSInteger)requestUserAttention:(NSRequestUserAttentionType)requestType
{
	if (suppressAttentionRequests)
	{
		return 0;
	}

    float dockIconBounceTime = 5.0;

	id dockIconBounceTimeSetting = [[NSUserDefaults standardUserDefaults] stringForKey:kDockIconBounceTime];
    if ([dockIconBounceTimeSetting respondsToSelector:@selector(integerValue)])
    {
        dockIconBounceTime = [dockIconBounceTimeSetting integerValue];
    }

	if (dockIconBounceTime == 0)
	{
		return 0;
	}
	
	NSInteger attentionId = [super requestUserAttention:requestType];
	
	if (dockIconBounceTime > 0)
	{
		dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(dockIconBounceTime * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
			[self cancelUserAttentionRequest:attentionId];
		});
    }
	return attentionId;
}



@end
