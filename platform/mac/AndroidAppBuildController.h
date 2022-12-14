//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#import "AppBuildController.h"

@class DialogController;

// ----------------------------------------------------------------------------

@interface AndroidAppBuildController : AppBuildController
{
	@protected
		NSString *androidAppPackage;
		NSNumber *appVersionCode;
        NSString *androidKeyAlias;
        NSString *androidKeyAliasPassword;

	@private
		IBOutlet NSPopUpButton *fAndroidKeystore;
		IBOutlet NSPopUpButton *fSigningIdentitiesAndroid;
		DialogController *fAndroidPasswordController;
		DialogController *fAndroidAliasPasswordController;
}

@property (nonatomic, readwrite, copy) NSString* androidAppPackage;
@property (nonatomic, readwrite, copy) NSNumber* appVersionCode;
@property (nonatomic, readwrite, copy) NSString* androidKeyAlias;
@property (nonatomic, readwrite, copy) NSString* androidKeyAliasPassword;

- (IBAction)browseKeystorePath:(id)sender;

@end

// ----------------------------------------------------------------------------
