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

@class GLView;
@class NSDictionary;
@class NSError;
@class NSNotification;
@class NSPopUpButton;
@class NSString;
@class NSView;
@class NSWindow;

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

	IBOutlet NSView *fOpenAccessoryView;
	IBOutlet NSPopUpButton *fDeviceSkins;
	int fSkin;

	id fPreferencesControllerDelegate;
	IBOutlet NSWindow* fPreferencesWindow;

	BOOL fIsRemote;
	
	BOOL applicationIsTerminating; // used to try to avoid occasional shutdown crash (home screen related?)
	
	BOOL fRuntimeErrorNotified;
	
	BOOL applicationHasBeenInitialized;
	BOOL launchedWithFile;
	BOOL allowLuaExit;
	time_t fNextUpsellTime;

	Rtt::MacPlatformServices *fServices;

	// Used to synchronize the Open Project accessory view to pick the start skin
	IBOutlet NSPopUpButton* popupButtonOpenAccessorySkinSelection;

	long fRelaunchCount;
	BOOL fSimulatorRelaunchPending;
	BOOL fLaunchSimulatorConfigurationHandled;
	Rtt::Runtime *fBackgroundedRuntime;
	NSDictionary *fActiveSimulatorDeviceInfo;
	NSDictionary *fTemporarySimulatorDeviceInfo;
	BOOL fAgentMode;
}

@property (nonatomic, readonly, getter=simulator) Rtt::MacSimulator *fSimulator;
@property (nonatomic, readwrite, assign) int fSkin;
//@property (nonatomic, readwrite, assign) NSPopUpButton* signingIdentities;
@property (nonatomic, readonly, copy) NSString* fAppPath;
@property (nonatomic, readwrite, assign) BOOL applicationHasBeenInitialized;
@property (nonatomic, readwrite, assign) BOOL launchedWithFile;
@property (nonatomic, readwrite, assign) BOOL allowLuaExit;
@property (nonatomic, readonly, retain) GLView* layerHostView;
@property (nonatomic, readonly) BOOL agentMode;

-(BOOL)isRunning;
-(IBAction)showHelp:(id)sender;

-(IBAction)orderFrontStandardAboutPanel:(id)sender;

-(BOOL)isRelaunchable;

-(BOOL)isRunnable;
@property (assign) IBOutlet NSMenuItem *ccc;

-(IBAction)showPreferences:(id)sender;

-(void) showOpenPanel:(NSString*)title withAccessoryView:(NSView*)accessoryView startDirectory:(NSString*)start_directory completionHandler:(void(^)(NSString* path))completionhandler;

-(IBAction)open:(id)sender;

-(BOOL)runApp:(NSString*)appPath;
-(NSView*)openAccessoryView;

- (void) closeSimulator:(id)sender;
-(IBAction)close:(id)sender;
-(IBAction)showProjectSandbox:(id)sender;
- (IBAction) showProjectFiles:(id)sender;
- (IBAction) clearProjectSandbox:(id)sender;
- (BOOL) setClearProjectSandboxTitle;

-(BOOL)setSkinForTitle:(NSString*)title;
-(BOOL)configureSimulator:(NSDictionary*)configuration relaunchIfNeeded:(BOOL)relaunchIfNeeded
	didScheduleRelaunch:(BOOL*)didScheduleRelaunch;
-(BOOL)relaunchSimulator;
-(BOOL)setSimulatorSafeAreaGuidesVisible:(BOOL)visible;
-(BOOL)setSimulatorFullscreen:(BOOL)fullscreen;
-(NSDictionary*)simulatorDeviceInfo;
-(NSDictionary*)simulatorStateInfo;
-(NSArray*)simulatorDevices;
-(BOOL)dispatchSimulatorInput:(NSDictionary*)input;
-(BOOL)simulateSimulatorEvent:(NSDictionary*)event;

-(IBAction)launchSimulator:(id)sender;
-(IBAction)toggleSuspendResume:(id)sender;

-(NSWindow*)currentWindow;
-(void)notifyRuntimeError:(NSString *)message;

//-(void)setRuntimeWithView:(GLView*)view;
-(Rtt::Runtime*)runtime;
//-(Rtt::MacPlatform*)platform;

-(NSArray*)GetRecentDocuments;

//-(void)applicationDidBecomeActive:(NSNotification*)aNotification;
- (void) startDebugAndOpenPanel; // second half of applicationDidFinishLaunching

-(IBAction)changedPreference:(id)sender;

-(NSString *) getAppSpecificPreferenceKeyName:(NSString *)prefName withProjectPath:(NSString *)projectDirectoryPath;

- (void)didPresentError:(BOOL)didRecover contextInfo:(void*)contextInfo;

- (BOOL) alertShowHelp:(NSAlert *) alert;

-(void)notifyWithTitle:(NSString*)title description:(NSString*)description iconData:(NSImage*)iconData;

@end

@interface CoronaSimulatorApplication : NSApplication

@property (nonatomic, readwrite) BOOL suppressAttentionRequests;

- (NSInteger)requestUserAttention:(NSRequestUserAttentionType)requestType;

@end
