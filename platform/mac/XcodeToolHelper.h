//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#import <Foundation/Foundation.h>

// Helper functions to help deal with Xcode command lines tools.
@interface XcodeToolHelper : NSObject

+ (void) printNotFoundWarningForTool:(NSString*)toolbasename;

+ (NSString*) pathForCodesignAllocate;
+ (NSString*) pathForCodesign;
+ (NSString*) pathForProductBuild;

+ (NSString*) pathForCodesignFramework;
+ (NSString*) pathForResources;

+ (NSString*) getXcodePath;

+ (NSString *) findXcodePathFor:(NSString *)cmd;

+ (NSString *) launchTaskAndReturnOutput:(NSString *)cmd arguments:(NSArray *)args printWarning:(BOOL)printWarning;

@end
