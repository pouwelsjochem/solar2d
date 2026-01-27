//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#import <Foundation/Foundation.h>

#ifdef Rtt_MAC_ENV
#import <AppKit/AppKit.h>
// TIS* and UCKeyTranslate functions are dynamically loaded at runtime - no static imports needed
#endif
enum KeyCodes
{
    #if __MAC_OS_X_VERSION_MAX_ALLOWED < 101200
        kVK_RightCommand = 0x36,
    #endif
	kVK_Menu = 0x6E,
	kVK_Back = 0x7F,
};

@interface AppleKeyServices : NSObject

+ (NSString*)getNameForKey:(NSNumber*)keyCode;

+ (NSUInteger)getModifierMaskForKey:(unsigned short)keyCode;

#ifdef Rtt_MAC_ENV
// Returns a Corona key name string for the current keyboard layout using the given QWERTY key name.
// Returns nil if the key is unknown or cannot be mapped.
+ (NSString*)getLayoutNameForQwertyKeyName:(NSString*)keyName;
#endif

@end
