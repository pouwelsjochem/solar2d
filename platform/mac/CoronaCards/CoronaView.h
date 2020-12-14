//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#import <GLKit/GLKit.h>

typedef enum {
    kNormal,
    kMinimized,
    kFullscreen,
} CoronaViewWindowMode;

@protocol CoronaViewDelegate;

// ----------------------------------------------------------------------------

@interface CoronaView : NSView

@property (nonatomic, assign) id <CoronaViewDelegate> coronaViewDelegate;

- (NSInteger)run;
- (NSInteger)runWithPath:(NSString*)path parameters:(NSDictionary *)params;
- (void)suspend;
- (void)resume;
- (void)terminate;
- (void) handleOpenURL:(NSString *)urlStr;
- (void) restoreWindowProperties;

- (BOOL) settingsIsWindowCloseButtonEnabled;
- (BOOL) settingsIsWindowMinimizeButtonEnabled;
- (int) settingsMinContentWidth;
- (int) settingsMaxContentWidth;
- (int) settingsMinContentHeight;
- (int) settingsMaxContentHeight;
- (NSString *) settingsWindowTitle;
- (CoronaViewWindowMode) settingsDefaultWindowMode;
- (int) settingsDefaultWindowViewWidth;
- (int) settingsDefaultWindowViewHeight;
- (BOOL) settingsSuspendWhenMinimized;

- (id)sendEvent:(NSDictionary *)event;

@end

// ----------------------------------------------------------------------------

@protocol CoronaViewDelegate <NSObject>

@optional
- (id)coronaView:(CoronaView *)view receiveEvent:(NSDictionary *)event;

@optional
- (void)coronaViewWillSuspend:(CoronaView *)view;
- (void)coronaViewDidSuspend:(CoronaView *)view;
- (void)coronaViewWillResume:(CoronaView *)view;
- (void)coronaViewDidResume:(CoronaView *)view;
- (void)notifyRuntimeError:(NSString *)message;
- (void)didPrepareOpenGLContext:(id)sender;
@end

// ----------------------------------------------------------------------------

