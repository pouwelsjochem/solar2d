//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

//#import <AppKit/AppKit.h>
//#import <Foundation/NSObject.h>
//#import <Foundation/NSGeometry.h>
//#import <AppKit/NSNibDeclarations.h>
#import "GLView.h"
#import <AppKit/AppKit.h>
#import "NSString+Extensions.h"

#include "Rtt_PlatformSimulator.h"
#include "Rtt_TargetDevice.h"

//#import <Foundation/Foundation.h>

// ----------------------------------------------------------------------------

@class DialogController;
@class GLView;
@class CoronaWindowController;
@class AndroidAppBuildController;
@class IOSAppBuildController;
@class OSXAppBuildController;
@class TVOSAppBuildController;
@class WebAppBuildController;
@class LinuxAppBuildController;
@class NSError;
@class NSMatrix;
@class NSNotification;
@class NSPopUpButton;
@class NSProgressIndicator;
@class NSString;
@class NSTextField;
@class NSView;
@class NSWindow;
@class BuildSessionState;
@class SDKItem;

namespace Rtt
{
	class MacConsolePlatform;
	class MacPlatformServices;
	class MacSimulator;
}

@interface AppDelegate : NSObject <NSMenuDelegate,NSAlertDelegate,GLViewDelegate
#if !defined( Rtt_WEB_PLUGIN )
	,NSUserNotificationCenterDelegate
#endif
	>
{
	Rtt::MacSimulator* fSimulator;
	NSString* fAppPath;
	Rtt::SimulatorOptions fOptions;

	Rtt::MacConsolePlatform *fConsolePlatform;

	NSString *fSdkRoot;

	IBOutlet NSView *fOpenAccessoryView;
	IBOutlet NSPopUpButton *fDeviceSkins;
	int fSkin;

	IBOutlet NSView *fBuildAccessoryView;
	IBOutlet NSMatrix *fBuildPlatform;

	NSString* appName;
	NSNumber* appVersionCode;
	NSString* appVersion;
	NSString* dstPath;
	NSString* projectPath;

	// TODO: Move to separate controller
	// BuildProgress.xib
	IBOutlet NSWindow* progressSheet;
	IBOutlet NSProgressIndicator* progressBar;

	// TODO: Move to separate controller
	// BuildProgressBanner.xib
	IBOutlet NSWindow *progressSheetBanner;
	IBOutlet NSProgressIndicator* progressBarBanner;
	int progressBarLabelWaitTime;
	IBOutlet NSTextField *progressBarLabel;
	IBOutlet NSImageView *buildProgressBanner;
	IBOutlet NSButton *buildProgressLearnMore;
	BOOL inCountDown;
	// This is a temporary variable to hold the webservicessession object to survive the weird build: code flow.
	// This ulimately should go away, but the object wrapper can be reused for Blocks.
	BuildSessionState* temporaryBuildSessionState;

	id fPreferencesControllerDelegate;
	IBOutlet NSWindow* fPreferencesWindow;

	BOOL fIsRemote;
	
	BOOL applicationIsTerminating; // used to try to avoid occasional shutdown crash (home screen related?)
	
	// Android
	BOOL fBuildProblemNotified;
	
	IBOutlet NSButton* rememberMyPreferenceAccessoryCheckboxView;
	BOOL applicationHasBeenInitialized;
	BOOL launchedWithFile;
	BOOL allowLuaExit;
	BOOL fSimulatorWasSuspended;
	time_t fNextUpsellTime;


    AndroidAppBuildController *fAndroidAppBuildController;
    IOSAppBuildController *fIOSAppBuildController;
	OSXAppBuildController *fOSXAppBuildController;
	TVOSAppBuildController *fTVOSAppBuildController;
	WebAppBuildController *fWebAppBuildController;
	LinuxAppBuildController *fLinuxAppBuildController;

	Rtt::MacPlatformServices *fServices;

	// Used to synchronize the Open Project accessory view to pick the start skin
	IBOutlet NSPopUpButton* popupButtonOpenAccessorySkinSelection;

	NSTask *consoleTask;
	long fRelaunchCount;
}

@property (nonatomic, readonly, getter=simulator) Rtt::MacSimulator *fSimulator;
@property (nonatomic, readwrite, copy) NSString *appName;
@property (nonatomic, readwrite, copy) NSNumber *appVersionCode;
@property (nonatomic, readwrite, copy) NSString *appVersion;
@property (nonatomic, readwrite, copy) NSString *dstPath;
@property (nonatomic, readonly) NSString *projectPath;
@property (nonatomic, readwrite, copy) NSString *androidAppPackage;
@property (nonatomic, readwrite, copy) NSString *androidKeyStore;
@property (nonatomic, readwrite, assign) int fSkin;
//@property (nonatomic, readwrite, assign) NSPopUpButton* signingIdentities;
@property (nonatomic, readonly, copy) NSString* fAppPath;
@property (nonatomic, readwrite, assign) BOOL applicationHasBeenInitialized;
@property (nonatomic, readwrite, assign) BOOL launchedWithFile;
@property (nonatomic, readwrite, assign) BOOL allowLuaExit;
@property (nonatomic, readonly, retain) GLView* layerHostView;
@property (nonatomic, readwrite) BOOL stopRequested;
@property (nonatomic, readwrite) float buildDownloadProgess;

+(BOOL)offlineModeAllowed;

-(BOOL)isRunning;
-(IBAction)showHelp:(id)sender;

-(IBAction)orderFrontStandardAboutPanel:(id)sender;

-(BOOL)isRelaunchable;

#if !defined( Rtt_PROJECTOR )
-(BOOL)isRunnable;
-(BOOL)isBuildAvailable;
-(BOOL)isAndroidBuildAvailable;
-(BOOL)isHTML5BuildHidden;
@property (assign) IBOutlet NSMenuItem *ccc;
-(BOOL)isLinuxBuildHidden;
-(BOOL)isTVOSBuildHidden;

-(IBAction)showPreferences:(id)sender;

-(void) showOpenPanel:(NSString*)title withAccessoryView:(NSView*)accessoryView startDirectory:(NSString*)start_directory completionHandler:(void(^)(NSString* path))completionhandler;

-(void)openWithPath:(NSString*)path;
-(IBAction)open:(id)sender;
#endif // Rtt_PROJECTOR

-(BOOL)runApp:(NSString*)appPath;
-(NSView*)openAccessoryView;

- (void) closeSimulator:(id)sender;
-(IBAction)close:(id)sender;
-(IBAction)showProjectSandbox:(id)sender;
- (IBAction) showProjectFiles:(id)sender;
- (IBAction) clearProjectSandbox:(id)sender;
- (BOOL) setClearProjectSandboxTitle;

-(BOOL)setSkinForTitle:(NSString*)title;

-(IBAction)launchSimulator:(id)sender;
-(IBAction)toggleSuspendResume:(id)sender;

-(NSWindow*)currentWindow;

-(void)beginProgressSheet:(NSWindow*)parent;
-(void)endProgressSheet;

-(IBAction)openForBuildiOS:(id)sender;
-(IBAction)openForBuildAndroid:(id)sender;
-(IBAction)openForBuildHTML5:(id)sender;
-(IBAction)openForBuildLinux:(id)sender;
-(IBAction)openForBuildOSX:(id)sender;
-(IBAction)openForBuildTVOS:(id)sender;
-(void)notifyRuntimeError:(NSString *)message;
- (NSString *) getOSVersion;

//-(void)setRuntimeWithView:(GLView*)view;
-(Rtt::Runtime*)runtime;
//-(Rtt::MacPlatform*)platform;

-(NSArray*)GetRecentDocuments;

//-(void)applicationDidBecomeActive:(NSNotification*)aNotification;
- (void) startDebugAndOpenPanel; // second half of applicationDidFinishLaunching

-(IBAction)changedPreference:(id)sender;

-(void) saveAppSpecificPreference:(NSString *)prefName value:(NSString *)prefValue;
-(NSString *) restoreAppSpecificPreference:(NSString *)prefName defaultValue:(NSString *)defaultValue;
-(NSString *) getAppSpecificPreferenceKeyName:(NSString *)prefName;
-(NSString *) getAppSpecificPreferenceKeyName:(NSString *)prefName withProjectPath:(NSString *)projectDirectoryPath;

- (void)didPresentError:(BOOL)didRecover contextInfo:(void*)contextInfo;

- (BOOL) alertShowHelp:(NSAlert *) alert;

-(void)notifyWithTitle:(NSString*)title description:(NSString*)description iconData:(NSImage*)iconData;
- (void) clearConsole;

@end

@interface CoronaSimulatorApplication : NSApplication

@property (nonatomic, readwrite) BOOL suppressAttentionRequests;

- (NSInteger)requestUserAttention:(NSRequestUserAttentionType)requestType;

@end
