//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#import <Cocoa/Cocoa.h>

// ----------------------------------------------------------------------------

namespace Rtt
{
	class MSimulatorServices;
}

// ----------------------------------------------------------------------------

@interface NewProjectController : NSObject< NSOpenSavePanelDelegate >
{
	@private
		IBOutlet NSWindow *fWindow;
		NSString *fProjectName;
		NSInteger fTemplateIndex;
		IBOutlet NSPopUpButton *fDeviceSize;
		NSInteger fDeviceSizeIndex;
		NSNumber *fDeviceWidth;
		NSNumber *fDeviceHeight;

		NSString *fResourcePath;

		NSWindow *fParent;
		NSString *fProjectPath;

		const Rtt::MSimulatorServices *fServices;
}

@property (nonatomic, readonly, assign, getter=window) NSWindow *fWindow;
@property (nonatomic, readwrite, assign, getter=projectName, setter=setProjectName:) NSString *fProjectName;
@property (nonatomic, readwrite, assign, getter=templateIndex, setter=setTemplateIndex:) NSInteger fTemplateIndex;
@property (nonatomic, readwrite, assign, getter=deviceSizeIndex, setter=setDeviceSizeIndex:) NSInteger fDeviceSizeIndex;
@property (nonatomic, readwrite, assign, getter=deviceWidth, setter=setDeviceWidth:) NSNumber *fDeviceWidth;
@property (nonatomic, readwrite, assign, getter=deviceHeight, setter=setDeviceHeight:) NSNumber *fDeviceHeight;
@property (nonatomic, readwrite, retain, getter=projectPath, setter=setProjectPath:) NSString *fProjectPath;

@property (nonatomic, readwrite, assign, getter=services, setter=setServices:) const Rtt::MSimulatorServices *fServices;


- (id)initWithWindowNibName:(NSString*)windowNibName resourcePath:(NSString*)path;

- (void)beginSheetModalForWindow:(NSWindow*)parent;

- (void)sheetDidEnd:(NSWindow *)sheet returnCode:(NSInteger)returnCode contextInfo:(void *)contextInfo;

- (BOOL)formComplete;

@end

// ----------------------------------------------------------------------------
