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

#if (MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_5)
	#import <Foundation/NSAutoreleasePool.h>
	#import <Foundation/NSBundle.h>
	#import <Foundation/NSData.h>
	#import <Foundation/NSFileHandle.h>
	#import <Foundation/NSFileManager.h>
	#import <Foundation/NSPathUtilities.h>
	#import <Foundation/NSProcessInfo.h>
	#import <Foundation/NSString.h>
	#import <Foundation/NSTask.h>
	#import <Foundation/NSThread.h>
	#if __LP64__
		typedef long NSInteger;
	#else
		typedef int NSInteger;
	#endif
#endif

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
#include "Rtt_TargetAndroidAppStore.h"

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

#import "BuildSessionState.h" // holds the WebServicesSession object for build now.

#import "GLView.h"
#import "FoundationUtilities.h"
#import "XcodeToolHelper.h"
#import "NSAlert-OAExtensions.h"

#include <CommonCrypto/CommonCrypto.h>

#include <sys/sysctl.h>
#include <string.h>

#include "Rtt_IOSAppPackager.h"
#include "Rtt_AndroidAppPackager.h"

#import "IOSAppBuildController.h"
#import "AndroidAppBuildController.h"
#import "WebAppBuildController.h"
#import "LinuxAppBuildController.h"
#import "OSXAppBuildController.h"
#import "TVOSAppBuildController.h"

#include "Rtt_PlatformDictionaryWrapper.h"

#import "AppleSigningIdentityController.h"

#include "Rtt_TargetDevice.h"

#include "ListKeyStore.h"

#import "TextEditorSupport.h"
#import "ValidationToolOutputViewController.h"
#import "ValidationSupportMacUI.h"
#import "SDKList.h"

#include "Rtt_MacDialogController.h"

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

static void SigPIPEHandler(int signal)
{
	Rtt_VLogException_UseStdout = false;

	// If we get a SIGPIPE that means the CoronaConsole process has gone AWOL and we'll
	// hang if we continue writing to it so we reopen stdout and stderr so the Simulator keeps
	// running (what actually seems to happen is output gets directed to the system console)
	close(1);
	open("/dev/tty", O_WRONLY);
	close(2);
	open("/dev/tty", O_WRONLY);

	NSLog(@"CoronaConsole seems to have vanished ... switching to syslog");
}

// -----------------------------------------------------------------------------
// BEGIN: Validation functions
// -----------------------------------------------------------------------------

// Settings stored in user preferences.
//
// Some of these are set from the command line. For example:
static NSString* kDockIconBounceTime = @"dockIconBounceTime";
static NSString* kSuppressUnsupportedOSWarning = @"suppressUnsupportedOSWarning";

static NSString* kWindowMenuItemName = @"Window";
static NSString* kViewAsMenuItemName = @"View As";

// TODO: Remove once the Beta is over
static NSString* kEnableLinuxBuild = @"enableLinuxBuild";

static const int       kClearProjectSandboxMenuTag = 1001;

NSString *kosVersionMinimum = @"10.9";   // we refuse to run on OSes older than this
NSString *kosVersionPrevious = @"10.12";  // should be updated as Apple releases new OSes
NSString *kosVersionCurrent = @"11.4";  // should be updated as Apple releases new OSes; we will run on this one and the previous one

// These tags are defined on the various DeviceBuild dialogs in Interface Builder
enum {
	osxSigningIdentitiesTag = 1001,
};

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

@interface AppDelegate ()

@property (nonatomic, readwrite, copy) NSString* fAppPath;

-(void)setSkin:(Rtt::TargetDevice::Skin)skin;
- (void) updateMenuForSkinChange;
- (void) restoreUserSkinSetting;
- (void) saveUserSkinSetting;
@end


@implementation AppDelegate

@synthesize fSimulator;
@synthesize appName;
@synthesize appVersionCode;
@synthesize appVersion;
@synthesize dstPath;
@synthesize projectPath;
@synthesize fSkin;
@synthesize fAppPath;
@synthesize applicationHasBeenInitialized;
@synthesize launchedWithFile;
@synthesize allowLuaExit;

-(id)init
{
	self = [super init];
	if ( self )
	{
		applicationIsTerminating = NO;
		fSimulator = NULL;
		fAppPath = nil;
		memset( & fOptions, 0, sizeof( fOptions ) );

		// Be sure to call this early enough to catch calls to external libraries
		[self checkOSVersionAndWarn];

		fConsolePlatform = new Rtt::MacConsolePlatform;
		fRelaunchCount = 0;

		fSdkRoot = nil;

		fOpenAccessoryView = nil;
		fDeviceSkins = nil;
		fSkin = Rtt::TargetDevice::kUnknownSkin;

		appName = nil;
		appVersionCode = nil;
		appVersion = nil;
		dstPath = nil;
		projectPath = nil;
		
		fPreferencesWindow = nil;
        
		fIsRemote = NO;

		[[NSUserNotificationCenter defaultUserNotificationCenter] setDelegate:self];
		fBuildProblemNotified = FALSE;

        fAndroidAppBuildController = nil;
        fIOSAppBuildController = nil;
		fOSXAppBuildController = nil;
		fWebAppBuildController = nil;
		fLinuxAppBuildController = nil;

		fServices = NULL;

        fSimulatorWasSuspended = FALSE;
		_stopRequested = NO;

		// Register corona:// URL scheme handler
		[[NSAppleEventManager sharedAppleEventManager]
		 setEventHandler:self
		 andSelector:@selector(handleOpenURL:replyEvent:)
		 forEventClass:kInternetEventClass
		 andEventID:kAEGetURL];
	}
	return self;
}

- (void) checkOpenGLRequirements
{
	// Force the OpenGL context to be created now and make current and query OpenGL for extensions.
	BOOL could_init = YES;//[CoronaMacGLLayer checkOpenGLRequirements];
	if ( NO == could_init )
	{
		NSString* appname = [FoundationUtilities bundleApplicationName];
		NSRunAlertPanel( 
			[NSString stringWithFormat:NSLocalizedString( @"%@ cannot run", @"<AppName> cannot run"), appname],
			NSLocalizedString( @"Your computer does not meet the minimum OpenGL requirements. Check the console for more details.", @"Your computer does not meet the minimum OpenGL requirements. Check the console for more details." ),
			nil, nil, nil );

		// Don't allow the program to continue.
		[[NSApplication sharedApplication] terminate:self];
	}
}

// -----------------------------------------------------------------------------
// BEGIN: Simulator Startup
// -----------------------------------------------------------------------------

-(void)coronaInit:(NSNotification*)aNotification
{
    // The builtin Skins directory is in the Resource directory in the bundle
    NSString *builtinSkinsDir = [[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:@"Skins"];
    NSFileManager *localFileManager = [NSFileManager defaultManager];
    NSDirectoryEnumerator *dirEnum = [localFileManager enumeratorAtPath:builtinSkinsDir];
    NSMutableArray *skinPathStrs = [[[NSMutableArray alloc] initWithCapacity:20] autorelease];
    NSString *file;
    while ((file = [dirEnum nextObject]))
    {
        if ([[file pathExtension] isEqualToString: @"lu"] || [[file pathExtension] isEqualToString: @"lua"])
        {
            NSString *luaPath = [builtinSkinsDir stringByAppendingPathComponent:file];
            
            [skinPathStrs addObject:luaPath];
        }
    }
    
    // Put the skins into a data structure we can share with core code
    char **skinPaths;
    
    skinPaths = (char **) calloc(sizeof(char *), [skinPathStrs count]);
    
    if (skinPaths == NULL && [skinPathStrs count] > 0)
    {
        NSLog(@"coronaInit: Problem processing skin files.  Please restart.");
    }
    else
    {
        int count = 0;
        
        for (file in skinPathStrs)
        {
            skinPaths[count] = (char *) strdup([file UTF8String]);
            ++count;
        }
        
        // Tell the core about the skins
        Rtt::TargetDevice::Initialize(skinPaths, count);
        
        for (int i = 0; i < count; i++)
        {
            free(skinPaths[i]);
        }
        
        free(skinPaths);
    }
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
		// We get SIGPIPE if the CoronaConsole subprocess goes away
		struct sigaction pipeAction = { 0 };
		pipeAction.sa_handler = SigPIPEHandler;
		sigaction(SIGPIPE, &pipeAction, NULL);

        // Migrate old prefs to new prefs, do it only once because we don't want to remove the old prefs in case
        // they subsequently downgrade
        NSDictionary *oldPrefs = [[NSUserDefaults standardUserDefaults]
                                  persistentDomainForName:@"com.anscamobile.Corona_Simulator"];
        NSDictionary *newPrefs = [[NSUserDefaults standardUserDefaults]
                                  persistentDomainForName:[[NSBundle mainBundle] bundleIdentifier]];
        
        if ( oldPrefs && ! [newPrefs valueForKey:@"prefsMigrated"])
        {
            NSMutableDictionary *convertedPrefs = [[[NSMutableDictionary alloc] initWithDictionary:oldPrefs] autorelease];
            
            // If we wanted to remove the old prefs we'd do this, not doing so allows people to go back to older versions
            // [[NSUserDefaults standardUserDefaults] removePersistentDomainForName:@"old.bundle.identifier"];
            
            // Now the old legacy build server stuff
            NSDictionary *oldBuildPrefs = [[NSUserDefaults standardUserDefaults]
                                           persistentDomainForName:@"com.anscamobile.ratatouille"];
            
            if (oldBuildPrefs)
            {
                [convertedPrefs addEntriesFromDictionary:oldBuildPrefs];
            }
            
            [[NSUserDefaults standardUserDefaults] setPersistentDomain:convertedPrefs forName:[[NSBundle mainBundle] bundleIdentifier]];
            [[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"prefsMigrated"];
            [[NSUserDefaults standardUserDefaults] synchronize];
            
            // Now the recent files
            NSDictionary *oldRecentFiles = [[NSUserDefaults standardUserDefaults]
                                            persistentDomainForName:@"com.anscamobile.Corona_Simulator.LSSharedFileList"];
            if ( oldRecentFiles )
            {
                [[NSUserDefaults standardUserDefaults] setPersistentDomain:oldRecentFiles forName:@"com.coronalabs.Corona_Simulator.LSSharedFileList"];
                
                [[NSUserDefaults standardUserDefaults] synchronize];
            }
        }
        
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
    [self setSkin:skinID];
    [self launchSimulator:sender];
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
    
    // If we haven't added any menu items yet
    if ([viewAsMenu numberOfItems] > 0)
    {
        return;
    }

    const char *itemTitle = NULL;
    NSString *lastSkinCategory = nil;
    int skinCount = 0;
    long itemCount = 0;
    long viewAsItemCount = 0;
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
    
	[viewAsMenu insertItem:[NSMenuItem separatorItem] atIndex:viewAsItemCount];
	++viewAsItemCount;
    
    // Make sure the current skin is checked
    [self updateMenuForSkinChange];
}

// TODO: Clean this up.  Total mess
-(void)applicationDidFinishLaunching:(NSNotification*)aNotification
{
    // If requested, don't display the window that shows the Simulator log.  Useful in IDE environments.
    // (the double access is needed because one specifies the command line parameter to specify a negative)
    // Also don't run the console if we are running as a debugger.
	// If we don't run the console, debug output appears on stdout
    if (([[NSUserDefaults standardUserDefaults] stringForKey:@"no-console"] == nil || ! [[NSUserDefaults standardUserDefaults] boolForKey:@"no-console"])
        && ! [[NSUserDefaults standardUserDefaults] boolForKey:@"debug"])
    {
		// Start the CoronaConsole task and attach out stdout to its stdin

		consoleTask = [[NSTask alloc] init];

		// If we don't nil out consoleTask here, any attempt to access its attributes will crash after
		// the CoronaConsole has exited/crashed
		consoleTask.terminationHandler =  ^(NSTask *aTask){
			consoleTask = nil;
			Rtt_VLogException_UseStdout = false;
			NSLog(@"CoronaConsole terminated");
		};

		[consoleTask setLaunchPath:[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:@"CoronaConsole.app/Contents/MacOS/CoronaConsole"]];

		NSPipe *stdoutPipe = [[NSPipe pipe] retain];

		dup2([[stdoutPipe fileHandleForWriting] fileDescriptor], fileno(stdout));
		dup2([[stdoutPipe fileHandleForWriting] fileDescriptor], fileno(stderr)); // capture stderr

		[consoleTask setStandardInput:stdoutPipe];

		@try
		{
			[consoleTask launch];
		}
		@catch( NSException* exception )
		{
			NSLog( @"consoleTask: exception %@", exception );
		}
    }

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
    NSArray *recentDocuments = [[NSDocumentController sharedDocumentController] recentDocumentURLs];

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
	NSString* v = [[NSUserDefaults standardUserDefaults] stringForKey:@"debugBuildProcess"];
	if(v) {
		setenv("DEBUG_BUILD_PROCESS", [v UTF8String], 0);
	}

	[self checkOpenGLRequirements];
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
			// Reset the preference in case the Simulator crashes, it gets set again on normal exit
			[[NSUserDefaults standardUserDefaults] synchronize];
			
            [self openLastProject];
		}
		else
		{
			[self runApp:appPath];
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

	[fSdkRoot release];
	delete fConsolePlatform;

	[dstPath release];
	[appVersionCode release];
	[appVersion release];
	[appName release];
	
	[fAppPath release];
	delete fSimulator;
	
	[self removeObserver:self forKeyPath:@"fSkin"];
}

- (void)applicationWillResignActive:(NSNotification *)aNotification
{
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

-(void)beginProgressSheet:(NSWindow*)parent
{
    if ( ! progressSheet )
    {
        [NSBundle loadNibNamed:@"BuildProgress" owner:self];
        [progressSheet setReleasedWhenClosed:NO];
        Rtt_ASSERT( progressSheet );
        Rtt_ASSERT( progressBar );
        [progressBar setUsesThreadedAnimation:YES];
    }

    [NSApp beginSheet:progressSheet modalForWindow:parent modalDelegate:nil didEndSelector:nil contextInfo:nil];
    [progressBar startAnimation:nil];
}

-(void)endProgressSheet
{
    [progressBar stopAnimation:nil];
    [NSApp endSheet:progressSheet];
    [progressSheet orderOut:self];
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

	fBuildProblemNotified = FALSE;

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
			NSString* mainScriptFile = [NSString stringWithExternalString:Rtt_LUA_SCRIPT_FILE( "main" )];
			[[NSDocumentController sharedDocumentController] noteNewRecentDocumentURL:[NSURL fileURLWithPath:[appPath stringByAppendingPathComponent:mainScriptFile]]];

            [self restoreUserSkinSetting];
		}

		self.fAppPath = [appPath stringByStandardizingPath];
				
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

	BOOL result = YES;

	TargetDevice::Skin skin = (TargetDevice::Skin)fSkin;

	if ( nil != title )
	{
        skin = Rtt::TargetDevice::SkinForLabel( [title UTF8String] );
	}
	
	if ( Rtt::TargetDevice::kUnknownSkin == skin )
	{
		result = NO;
	}

	if ( result )
	{
		[self setSkin:skin];
	}

	return result;
}

- (void) restoreUserSkinSetting
{
	NSString* skinname = [[NSUserDefaults standardUserDefaults] stringForKey:kUserPreferenceUsersCurrentSelectedSkin];
	if ( nil != skinname )
	{
        if ( ! [self setSkinForTitle:skinname] )
        {
            Rtt_TRACE_SIM( ( "WARNING: Skin '%s' does not exist\n", [skinname UTF8String] ) );
        }
	}
}

- (void) saveUserSkinSetting
{
    const char* skinstring = Rtt::TargetDevice::LabelForSkin((Rtt::TargetDevice::Skin)self.fSkin);
    if ( nil != skinstring )
    {
        [[[NSUserDefaultsController sharedUserDefaultsController] values] setValue:[NSString stringWithExternalString:skinstring] forKey:kUserPreferenceUsersCurrentSelectedSkin];
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
	if([[NSUserDefaults standardUserDefaults] boolForKey:@"clearConsoleOnRelaunch"])
	{
		[self clearConsole];
	}

	// Detect relaunch
	if ( fSimulator )
	{
		// Always close
		[self closeSimulator:sender];

		++fRelaunchCount;
	}

	const char *resourcePath = [self.fAppPath UTF8String];

	if (resourcePath == NULL)
	{
		[self open:sender];

		return;
	}

	[self willChangeValueForKey:@"suspendResumeLabel"];

	fSimulator = new Rtt::MacSimulator;

	fBuildProblemNotified = false;
	
	const char *skinFile = skinFile = Rtt::TargetDevice::LuaObjectFileFromSkin( (Rtt::TargetDevice::Skin)fSkin );

    // If all else fails, default to the default skin file
    if (skinFile == NULL)
    {
        skinFile = Rtt::TargetDevice::LuaObjectFileFromSkin( Rtt::TargetDevice::kDefaultSkin );
    }

	// [1] Somewhere in Initialize (or its sub-calls), GLView's prepareOpenGL is invoked, and Runtime is instantiated
	fSimulator->Initialize( skinFile, resourcePath );
	
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

// -----------------------------------------------------------------------------
// BEGIN: Device Build
// -----------------------------------------------------------------------------

-(BOOL)isRunnable
{
	return YES;
}

-(BOOL)isBuildAvailable
{
	bool result = (fAppPath != nil) && (fSimulator != nil);
	return result;
}

// TODO: Is this duplicate of isBuildAvailable?  If so remove...
-(BOOL)isAndroidBuildAvailable
{
	return [self isBuildAvailable];
}

-(BOOL)isAllowedToBuild:(Rtt::TargetDevice::Platform)platform
{
	return [self isBuildAvailable];
}

-(BOOL)isHTML5BuildHidden
{
	return false;
}

-(BOOL)isLinuxBuildHidden
{
	return ! [[NSUserDefaults standardUserDefaults] boolForKey:kEnableLinuxBuild];
}

-(BOOL)isOSXBuildHidden
{
	// return ! [[NSUserDefaults standardUserDefaults] boolForKey:kEnableOSXBuild];
	return false;
}

-(BOOL)isTVOSBuildHidden
{
	// return ! [[NSUserDefaults standardUserDefaults] boolForKey:kEnableTVOSBuild];
	return false;
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

-(void)willOpenForBuild:(id)sender
{
	fBuildProblemNotified = false;
}

-(void)didOpenForBuild:(id)sender
{
}

-(void)prepareController:(AppBuildController*)controller
{
	controller.projectPath = fAppPath;
	controller.appName = [fAppPath lastPathComponent];
	controller.appVersion = @"1.0";
	controller.dstPath = [AppBuildController defaultDstDir];
}

-(IBAction)openForBuildiOS:(id)sender
{
    [self willOpenForBuild:sender];

    if ( fIOSAppBuildController == nil )
    {
        fIOSAppBuildController = [[IOSAppBuildController alloc] initWithWindowNibName:@"IOSAppBuild"
                                                                          projectPath:fAppPath];
    }

    Rtt_ASSERT(fIOSAppBuildController != nil);

    if ( ! [fIOSAppBuildController verifyBuildTools:sender] )
    {
        return;
    }

    // If the simulator was not suspended prior to building, suspend it.
    fSimulatorWasSuspended = fSimulator->GetPlayer()->GetRuntime().IsSuspended();
    if( ! fSimulatorWasSuspended )
    {
        [self willChangeValueForKey:@"suspendResumeLabel"];
        fSimulator->ToggleSuspendResume(false);
        [self didChangeValueForKey:@"suspendResumeLabel"];
    }

    [self willChangeValueForKey:@"projectPath"];
    projectPath = fAppPath;
    [self didChangeValueForKey:@"projectPath"];

    [self prepareController:fIOSAppBuildController];

    [fIOSAppBuildController showWindow:self];
}

-(IBAction)openForBuildAndroid:(id)sender
{
    [self willOpenForBuild:sender];

    if ( fAndroidAppBuildController == nil )
    {
        fAndroidAppBuildController = [[AndroidAppBuildController alloc] initWithWindowNibName:@"AndroidAppBuild"
                                                                                  projectPath:fAppPath];
    }

    Rtt_ASSERT(fAndroidAppBuildController);

    // If the simulator was not suspended prior to building, suspend it.
    fSimulatorWasSuspended = fSimulator->GetPlayer()->GetRuntime().IsSuspended();
    if( ! fSimulatorWasSuspended )
    {
        [self willChangeValueForKey:@"suspendResumeLabel"];
        fSimulator->ToggleSuspendResume(false);
        [self didChangeValueForKey:@"suspendResumeLabel"];
    }

    [self willChangeValueForKey:@"projectPath"];
    projectPath = fAppPath;
    [self didChangeValueForKey:@"projectPath"];

    [self prepareController:fAndroidAppBuildController];

	if (! [fAndroidAppBuildController verifyBuildTools:sender])
	{
		return;
	}

    [fAndroidAppBuildController showWindow:self];
}

-(IBAction)openForBuildHTML5:(id)sender
{
	[self willOpenForBuild:sender];
	
	if ( ! fWebAppBuildController )
	{
		fWebAppBuildController = [[WebAppBuildController alloc] initWithWindowNibName:@"WebAppBuild" projectPath:fAppPath];
	}
	
	Rtt_ASSERT(fWebAppBuildController);
	
	// If the simulator was not suspended prior to building, suspend it.
	fSimulatorWasSuspended = fSimulator->GetPlayer()->GetRuntime().IsSuspended();
	if( ! fSimulatorWasSuspended )
	{
		[self willChangeValueForKey:@"suspendResumeLabel"];
		fSimulator->ToggleSuspendResume(false);
		[self didChangeValueForKey:@"suspendResumeLabel"];
	}
	
	[self willChangeValueForKey:@"projectPath"];
	projectPath = fAppPath;
	[self didChangeValueForKey:@"projectPath"];
	
	[self prepareController:fWebAppBuildController];
	if (! [fWebAppBuildController verifyBuildTools:sender])
	{
		return;
	}

	[fWebAppBuildController showWindow:self];
}

-(IBAction)openForBuildLinux:(id)sender
{
	[self willOpenForBuild:sender];
	
	if ( ! fLinuxAppBuildController )
	{
		fLinuxAppBuildController = [[LinuxAppBuildController alloc] initWithWindowNibName:@"LinuxAppBuild"
																				projectPath:fAppPath];
	}
	
	Rtt_ASSERT(fLinuxAppBuildController);
	
	// If the simulator was not suspended prior to building, suspend it.
	fSimulatorWasSuspended = fSimulator->GetPlayer()->GetRuntime().IsSuspended();
	if( ! fSimulatorWasSuspended )
	{
		[self willChangeValueForKey:@"suspendResumeLabel"];
		fSimulator->ToggleSuspendResume(false);
		[self didChangeValueForKey:@"suspendResumeLabel"];
	}
	
	[self willChangeValueForKey:@"projectPath"];
	projectPath = fAppPath;
	[self didChangeValueForKey:@"projectPath"];
	
	[self prepareController:fLinuxAppBuildController];
	if (! [fLinuxAppBuildController verifyBuildTools:sender])
	{
		return;
	}
	
	[fLinuxAppBuildController showWindow:self];
}

-(IBAction)openForBuildOSX:(id)sender
{
	[self willOpenForBuild:sender];

	if ( fOSXAppBuildController == nil )
	{
		fOSXAppBuildController = [[OSXAppBuildController alloc] initWithWindowNibName:@"OSXAppBuild"
                                                                          projectPath:fAppPath];
	}
    
    Rtt_ASSERT(fOSXAppBuildController);

    if (! [fOSXAppBuildController verifyBuildTools:sender])
    {
        return;
    }
    
    // If the simulator was not suspended prior to building, suspend it.
    fSimulatorWasSuspended = fSimulator->GetPlayer()->GetRuntime().IsSuspended();
    if( ! fSimulatorWasSuspended )
    {
        [self willChangeValueForKey:@"suspendResumeLabel"];
        fSimulator->ToggleSuspendResume(false);
        [self didChangeValueForKey:@"suspendResumeLabel"];
    }
    
	[self willChangeValueForKey:@"projectPath"];
	projectPath = fAppPath;
	[self didChangeValueForKey:@"projectPath"];

    // Set defaults
	[self prepareController:fOSXAppBuildController];

	[fOSXAppBuildController showWindow:self];
}

-(IBAction)openForBuildTVOS:(id)sender
{
	[self willOpenForBuild:sender];

	if ( fTVOSAppBuildController == nil )
	{
		fTVOSAppBuildController = [[TVOSAppBuildController alloc] initWithWindowNibName:@"TVOSAppBuild"
                                                                          projectPath:fAppPath];
	}
    
    Rtt_ASSERT(fTVOSAppBuildController);

    if (! [fTVOSAppBuildController verifyBuildTools:sender])
    {
        return;
    }
    
    // If the simulator was not suspended prior to building, suspend it.
    fSimulatorWasSuspended = fSimulator->GetPlayer()->GetRuntime().IsSuspended();
    if( ! fSimulatorWasSuspended )
    {
        [self willChangeValueForKey:@"suspendResumeLabel"];
        fSimulator->ToggleSuspendResume(false);
        [self didChangeValueForKey:@"suspendResumeLabel"];
    }
    
	[self willChangeValueForKey:@"projectPath"];
	projectPath = fAppPath;
	[self didChangeValueForKey:@"projectPath"];

    // Set defaults
	[self prepareController:fTVOSAppBuildController];

	[fTVOSAppBuildController showWindow:self];
}

-(void)sheetDidEnd:(NSWindow*)sheet returnCode:(NSInteger)returnCode contextInfo:(void *)contextInfo
{

}

//This mimics growl send notification selector.
-(void)notifyWithTitle:(NSString*)title
		   description:(NSString*)description
			  iconData:(NSImage*)iconData
{
	NSUserNotification *notification = [[[NSUserNotification alloc] init] autorelease];
	notification.title = title;
	notification.informativeText = description;
	notification.contentImage = iconData;
	notification.soundName = NSUserNotificationDefaultSoundName;
	[[NSUserNotificationCenter defaultUserNotificationCenter] deliverNotification:notification];
}


//
// Save and restore preferences in an application specific way so apps remember their individual settings
//
-(void) saveAppSpecificPreference:(NSString *)prefName value:(NSString *)prefValue
{
    char keyId[CC_MD5_DIGEST_LENGTH*2 + 1];
    MD5Hash( keyId, [projectPath UTF8String] );
    
    [[NSUserDefaults standardUserDefaults] setObject:prefValue forKey:[NSString stringWithFormat:@"%@/%s", prefName, keyId]];
}

-(NSString *) restoreAppSpecificPreference:(NSString *)prefName defaultValue:(NSString *)defaultValue
{
	if (projectPath == nil)
	{
		return defaultValue;
	}
    char keyId[CC_MD5_DIGEST_LENGTH*2 + 1];
    MD5Hash( keyId, [projectPath UTF8String] );
    
	NSString *value =  [[NSUserDefaults standardUserDefaults] stringForKey:[NSString stringWithFormat:@"%@/%s", prefName, keyId]];
    
    if ([value length] == 0)
    {
        return defaultValue;
    }
    else
    {
        return value;
    }
}

-(NSString *) getAppSpecificPreferenceKeyName:(NSString *)prefName
{
	return [self getAppSpecificPreferenceKeyName: prefName withProjectPath: self.projectPath];
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
	if ( !fBuildProblemNotified )
	{
		[self notifyWithTitle:@"Solar2D Simulator" description:message iconData:nil];
		fBuildProblemNotified = true;
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

- (NSComparisonResult) compareOSVersion:(NSString *) version with:(NSString *) otherVersion
{
    // typedef NS_ENUM(NSInteger, NSComparisonResult) {NSOrderedAscending = -1L, NSOrderedSame, NSOrderedDescending};
    NSScanner *scanner = [NSScanner scannerWithString:version];
    NSScanner *otherScanner = [NSScanner scannerWithString:otherVersion];
    NSInteger versionMajor, versionMinor;
    NSInteger otherVersionMajor, otherVersionMinor;

    @try
    {
        [scanner scanInteger:&versionMajor];
        [scanner setScanLocation:[scanner scanLocation] + 1]; // skip the period
        [scanner scanInteger:&versionMinor];

        [otherScanner scanInteger:&otherVersionMajor];
        [otherScanner setScanLocation:[otherScanner scanLocation] + 1]; // skip the period
        [otherScanner scanInteger:&otherVersionMinor];
    }

    @catch ( NSException *e )
    {
        Rtt_TRACE_SIM( ( "compareOSVersion: error scanning versions: version '%s', otherVersion '%s' (%s)", [version UTF8String], [otherVersion UTF8String], [[e reason] UTF8String] ) );

        return NSOrderedSame; // seems the most benign
    }

    @finally
    {
        if (versionMajor > otherVersionMajor)
            return NSOrderedDescending;
        else if (versionMajor < otherVersionMajor)
            return NSOrderedAscending;
        else
        {
            // major versions are equal
            if (versionMinor > otherVersionMinor)
                return NSOrderedDescending;
            else if (versionMinor < otherVersionMinor)
                return NSOrderedAscending;
            else
                return NSOrderedSame;
        }
    }
}

- (NSString *) getOSVersion
{
    // OS version determination (ironically this is very OS version dependent)
    typedef struct {
        NSInteger majorVersion;
        NSInteger minorVersion;
        NSInteger patchVersion;
    } OperatingSystemVersion;
    OperatingSystemVersion osVersion = {0};
    SEL operatingSystemVersionSelector = NSSelectorFromString(@"operatingSystemVersion");

    if ([[NSProcessInfo processInfo] respondsToSelector:operatingSystemVersionSelector])
    {
        // this works on 10.10 and above (and, apparently, 10.9)
        NSMethodSignature *signature = [NSProcessInfo instanceMethodSignatureForSelector:operatingSystemVersionSelector];
        if(signature)
        {
            NSInvocation *invocation = [NSInvocation invocationWithMethodSignature:signature];
            [invocation setTarget:[NSProcessInfo processInfo]];
            [invocation setSelector:operatingSystemVersionSelector];
            [invocation invoke];
            [invocation getReturnValue:&osVersion];
        }
    }
    else
    {
        // works on 10.8 and below but is now deprecated (actually it doesn't work correctly with "minorVersion" > 9)
        SInt32 versMaj, versMin, versPatch;

        Gestalt(gestaltSystemVersionMajor, &versMaj);
        Gestalt(gestaltSystemVersionMinor, &versMin);
        Gestalt(gestaltSystemVersionBugFix, &versPatch);

        osVersion.majorVersion = versMaj;
        osVersion.minorVersion = versMin;
        osVersion.patchVersion = versPatch;
    }

    return [NSString stringWithFormat:@"%d.%d", (int) osVersion.majorVersion, (int) osVersion.minorVersion];
}

- (void) checkOSVersionAndWarn
{
    NSAlert* alert = [[[NSAlert alloc] init] autorelease];
    NSString *suppressAlertOSVersion = [[NSUserDefaults standardUserDefaults] stringForKey:kSuppressUnsupportedOSWarning];
    BOOL shouldSuppressAlert = NO;

    NSString *currentOSVersion = [self getOSVersion];

    if ([currentOSVersion isEqualToString:suppressAlertOSVersion])
    {
        // they've already seen a warning and elected not to see it again so we're done
        // (if they install a new OS and still don't meet the criteria we'll show them
        // another warning)

        return;
    }

    if ([self compareOSVersion:currentOSVersion with:kosVersionMinimum] == NSOrderedAscending)
    {
        NSString *msg = [NSString stringWithFormat:@"This version of macOS (%@) is too old to run Solar2D.\n\nMinimum supported macOS version is %@", currentOSVersion, kosVersionPrevious];

        [alert setMessageText:@"macOS Version Error"];
        [alert setInformativeText:msg];
        [alert setAlertStyle:NSCriticalAlertStyle];
        [alert addButtonWithTitle:@"Exit"];

        NSLog( @"Solar2D Simulator: %@", msg );

        [alert runModal];  // we exit when they hit the button so run modally here
        
        [[NSApplication sharedApplication] terminate:self];
    }
    else if ([self compareOSVersion:currentOSVersion with:kosVersionCurrent] == NSOrderedDescending)
    {
        NSString *msg = [NSString stringWithFormat:@"This version of macOS (%@) is not supported. It is newer than the one this version of Solar2D was designed for (%@).\n\nProceed with caution, as some things might not work correctly.\n\nPlease report any issues you find with Solar2D and the new version of macOS to http://github.com/coronalabs/corona/issues", currentOSVersion, kosVersionCurrent];

        [alert setMessageText:@"macOS Version Warning"];
        [alert setInformativeText:msg];
        [alert setAlertStyle:NSWarningAlertStyle];
        [alert addButtonWithTitle:@"Continue"];
        [alert setShowsSuppressionButton:YES];

        NSLog( @"Solar2D Simulator: %@", msg );
    }
    else if ([self compareOSVersion:currentOSVersion with:kosVersionPrevious] == NSOrderedAscending)
    {
        NSString *msg = [NSString stringWithFormat:@"This version of macOS (%@) is not supported. It is older than the one this version of Solar2D was designed for (%@).\n\nProceed with caution, as some things might not work correctly.", currentOSVersion, kosVersionCurrent];

        [alert setMessageText:@"macOS Version Warning"];
        [alert setInformativeText:msg];
        [alert setAlertStyle:NSWarningAlertStyle];
        [alert addButtonWithTitle:@"Continue"];
        [alert setShowsSuppressionButton:YES];

        NSLog( @"Solar2D Simulator: %@", msg );
    }
    else
    {
        // we're good, just return and let the Simulator start

        return;
    }

    [alert runModal];

    shouldSuppressAlert = ([[alert suppressionButton] state] == NSOnState);

    if (shouldSuppressAlert)
    {
        [[NSUserDefaults standardUserDefaults] setObject:currentOSVersion forKey:kSuppressUnsupportedOSWarning];
    }
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

-(void) clearConsole
{
	if ([consoleTask isRunning])
	{
		CFNotificationCenterPostNotification( CFNotificationCenterGetDistributedCenter(), CFSTR("CoronaConsole.clearConsole"), NULL, NULL, YES);
	}
}

- (IBAction)consoleMenuitem:(id)sender
{
	if ([consoleTask isRunning])
	{
		CFNotificationCenterPostNotification( CFNotificationCenterGetDistributedCenter(), CFSTR("CoronaConsole.bringToFront"), NULL, NULL, YES);
	}
}

static void BringToFrontCallback(CFNotificationCenterRef center, void *observer, CFStringRef name, const void *object, CFDictionaryRef userInfo)
{
	AppDelegate *appDelegate = (__bridge AppDelegate*)observer;
	[appDelegate bringAllToFront:nil];
	CFNotificationCenterRemoveObserver(center, observer, name, object);
}

- (void)applicationWillBecomeActive:(NSNotification *)aNotification
{
	CFNotificationCenterAddObserver(CFNotificationCenterGetDistributedCenter(), (__bridge const void *)(self), BringToFrontCallback, CFSTR("CoronaSimulator.bringToFront"), NULL, CFNotificationSuspensionBehaviorDeliverImmediately);
	[self consoleMenuitem:nil];
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


