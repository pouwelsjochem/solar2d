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

// ----------------------------------------------------------------------------

@interface CoronaView : NSView

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

