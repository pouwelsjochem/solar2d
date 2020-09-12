//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#import <AppKit/AppKit.h>
#import "SimulatorDeviceWindow.h"


@class NSImageView;
@class GLView;

@interface SkinlessSimulatorWindow : SimulatorDeviceWindow
{
}

@property(nonatomic, readwrite, copy) NSString *windowTitle;

- (id)initWithScreenView:(GLView*)screenView
				viewRect:(NSRect)screenRect
				   title:(NSString*)title;

@end
