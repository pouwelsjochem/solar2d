//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

// ============================================================================
// NOTE: Only functionality common to both iOS/tvOS should be added here.
// ============================================================================

#include "Core/Rtt_Build.h"

#include "Core/Rtt_String.h"

#import <UIKit/UIAlertView.h>
#import <UIKit/UIAlertController.h>
#import <GameController/GameController.h>

#include "Rtt_IPhonePlatformBase.h"
#include "Rtt_IPhoneTextBoxObject.h"
#include "Rtt_IPhoneTextFieldObject.h"
#include "Rtt_IPhoneTimer.h"

#include "Rtt_AppleFont.h"
#include "Rtt_IPhoneFont.h"
#include "Rtt_IPhoneScreenSurface.h"
#include "Rtt_Lua.h"
#include "Rtt_LuaLibNative.h"
#include "Rtt_LuaResource.h"
#include "Rtt_TouchInhibitor.h"

#import "CoronaViewPrivate.h"

// ----------------------------------------------------------------------------

@interface CustomAlertController : UIAlertController
{
	Rtt::LuaResource *fResource;
	AlertViewDelegate *fDelegate;
}
@property (nonatomic, readwrite, assign) Rtt::LuaResource *fResource;
@property (nonatomic, readwrite, assign) AlertViewDelegate *fDelegate;

@end

@implementation CustomAlertController

@synthesize fResource;
@synthesize fDelegate;

-(void)dismissViewControllerAnimated:(BOOL)flag completion:(void (^)())completion
{
	[fDelegate removeObjectForKey:self];
	return [super dismissViewControllerAnimated:flag completion:completion];
}

-(void)dealloc
{
	Rtt_DELETE( fResource );
	[super dealloc];
}

@end

// ----------------------------------------------------------------------------

@interface AlertViewDelegate : NSObject
{
	NSMutableDictionary *fWrappers;
}

@property (nonatomic, readonly, getter=wrappers) NSMutableDictionary *fWrappers;

-(id)objectForKey:(void*)key;
-(void)setObject:(id)value forKey:(void*)key;
-(void)removeObjectForKey:(void*)key;

@end

@implementation AlertViewDelegate

@synthesize fWrappers;

-(id)init
{
	self = [super init];
	if ( self )
	{
		fWrappers = [[NSMutableDictionary alloc] init];
	}
	return self;
}

-(void)dealloc
{
	[fWrappers release];

	[super dealloc];
}

-(id)objectForKey:(void*)key
{
	NSString *k = [[NSString alloc] initWithFormat:@"%lx", (unsigned long) key];
	
	id object = [fWrappers objectForKey:k];

	id result = Rtt_VERIFY( [object isKindOfClass:[CustomAlertController class]] ) ? object : nil;

	[k release];

	return result;
}

-(void)setObject:(id)value forKey:(void*)key
{
	NSString *k = [[NSString alloc] initWithFormat:@"%lx", (unsigned long) key];
	
	Rtt_ASSERT( nil == [fWrappers objectForKey:k] );
	[fWrappers setValue:value forKey:k];
	[k release];
}

-(void)removeObjectForKey:(void*)key
{
	NSString *k = [[NSString alloc] initWithFormat:@"%lx", (unsigned long) key];
	
	Rtt_ASSERT( [fWrappers objectForKey:k]
				&& [[fWrappers objectForKey:k] isKindOfClass:[CustomAlertController class]] );

	[fWrappers removeObjectForKey:k];
	[k release];
}

@end

// ----------------------------------------------------------------------------

namespace Rtt
{

// ----------------------------------------------------------------------------

IPhonePlatformBase::IPhonePlatformBase( CoronaView *view )
:	Super(),
	fView( view ), // NOTE: Weak ref b/c fView owns this.
	fDelegate( [[AlertViewDelegate alloc] init] ),
	fCachedControllerUserInteractionStatus( NO )
{
}

IPhonePlatformBase::~IPhonePlatformBase()
{
 	[fDelegate release];
}

PlatformSurface*
IPhonePlatformBase::CreateScreenSurface() const
{
	return Rtt_NEW( fAllocator, IPhoneScreenSurface( fView ) );
}

PlatformTimer*
IPhonePlatformBase::CreateTimerWithCallback( MCallback& callback ) const
{
	return Rtt_NEW( fAllocator, IPhoneTimer( callback, fView.viewController ) );
}

bool
IPhonePlatformBase::OpenURL( const char* url ) const
{
	bool result = false;

	if ( url )
	{
		NSURL* urlPlatform = [NSURL URLWithString:[NSString stringWithUTF8String:url]];
		if ( nil != urlPlatform)
		{
			result = [[UIApplication sharedApplication] openURL:urlPlatform];
		}
	}

	return result;
}

int
IPhonePlatformBase::CanOpenURL( const char* url ) const
{
	int result = -1;
	
	if ( url )
	{
		NSURL* urlPlatform = [NSURL URLWithString:[NSString stringWithUTF8String:url]];
		if ( nil != urlPlatform)
		{
			result = [[UIApplication sharedApplication] canOpenURL:urlPlatform];
		}
	}
	
	return result;
}
	
// ----------------------------------------------------------------------------

void
IPhonePlatformBase::SetIdleTimer( bool enabled ) const
{
	[UIApplication sharedApplication].idleTimerDisabled = ! enabled;
}
	
bool
IPhonePlatformBase::GetIdleTimer() const
{
	return (bool)( ! [UIApplication sharedApplication].idleTimerDisabled);
}

// ----------------------------------------------------------------------------

NativeAlertRef
IPhonePlatformBase::ShowNativeAlert(
	const char *title,
	const char *msg,
	const char **buttonLabels,
	U32 numButtons,
	LuaResource* resource ) const
{
	Rtt::TouchInhibitor inhibitor( GetView() );
	NativeAlertRef ret = NULL;
	
	Rtt_ASSERT( [GetView().delegate isKindOfClass:[UIViewController class]] );
	UIViewController *viewController = (UIViewController *)GetView().delegate;
	
	// If we are already showing an alert, abort and warn the user.
	if ( fDelegate.wrappers.count != 0 )
	{
		Rtt_TRACE_SIM( ( "WARNING: This device does not support simultaneous alerts via native.showAlert(). If an alert is shown, subsequent calls will be ignored.\n" ) );
		return ret;
	}

	NSString *t = [[NSString alloc] initWithUTF8String:title];
	NSString *m = [[NSString alloc] initWithUTF8String:msg];
	
	CustomAlertController *alertController = [CustomAlertController
												  alertControllerWithTitle:t
												  message:m
												  preferredStyle:UIAlertControllerStyleAlert];
	
	alertController.fResource = resource;
	alertController.fDelegate = fDelegate;
	[fDelegate setObject:alertController forKey:alertController];
	
	for ( U32 i = 0; i < numButtons; i++ )
	{
		const char *label = buttonLabels[i]; Rtt_ASSERT( label );
		NSString *l = [[NSString alloc] initWithUTF8String:label];
		
		U32 buttonIndex = i;
		void (^handler)(UIAlertAction*) = ^(UIAlertAction *action) {
#ifdef Rtt_TVOS_ENV
			// Restore the cached controllerUserInteractionEnabled state set when the alert was displayed.
			GCEventViewController* controller = (GCEventViewController*)(GetView().viewController.parentViewController);
			controller.controllerUserInteractionEnabled = fCachedControllerUserInteractionStatus;
#endif
				
			[alertController dismissViewControllerAnimated:YES completion:nil];
			
			if ( resource )
			{
				Rtt::LuaLibNative::AlertComplete( * resource, buttonIndex, false );
			}
		};
		UIAlertAction *action = [UIAlertAction
								 actionWithTitle:l
								 style:UIAlertActionStyleDefault
								 handler:handler];
		[alertController addAction:action];
		[l release];
	}
		
#ifdef Rtt_TVOS_ENV
	// On tvOS, native alerts need to respond to game controllers. We try to protect the user and set controllerUserInteractionEnabled
	// for the relevent view controller. As we do not know if this is being called on a main menu or not, we store/restore the state
	// before/after the alert is displayed.
	GCEventViewController* controller = (GCEventViewController*)(GetView().viewController.parentViewController);
	fCachedControllerUserInteractionStatus = controller.controllerUserInteractionEnabled;
	controller.controllerUserInteractionEnabled = YES;
#endif
		
	[viewController presentViewController:alertController animated:YES completion:nil];
	ret = alertController;
	
	[m release];
	[t release];

	return ret;
}

void
IPhonePlatformBase::CancelNativeAlert( NativeAlertRef alert, S32 index ) const
{
	id object = [fDelegate objectForKey:alert];
	
	if ( object != nil )
	{
#ifdef Rtt_TVOS_ENV
		// Restore the cached controllerUserInteractionEnabled state set when the alert was displayed.
		GCEventViewController* controller = (GCEventViewController*)(GetView().viewController.parentViewController);
		controller.controllerUserInteractionEnabled = fCachedControllerUserInteractionStatus;
#endif
		
		CustomAlertController* alertController = (CustomAlertController*)object;
		LuaResource* resource = alertController.fResource;
		if ( resource )
		{
			[alertController dismissViewControllerAnimated:YES completion:^{
				Rtt::LuaLibNative::AlertComplete( * resource, index, true );
			}];
		}
		else
		{
			[alertController dismissViewControllerAnimated:YES completion:nil];
		}
	}
}

PlatformDisplayObject*
IPhonePlatformBase::CreateNativeTextBox( const Rect& bounds ) const
{
	return Rtt_NEW( & GetAllocator(), IPhoneTextBoxObject( bounds ) );
}

PlatformDisplayObject*
IPhonePlatformBase::CreateNativeTextField( const Rect& bounds ) const
{
	return Rtt_NEW( & GetAllocator(), IPhoneTextFieldObject( bounds ) );
}

// ----------------------------------------------------------------------------

Real
IPhonePlatformBase::GetStandardFontSize() const
{
	Real pointSize = 17.f;

	if ( [UIFont respondsToSelector:@selector(preferredFontForTextStyle:)] )
	{
		pointSize = [UIFont preferredFontForTextStyle:UIFontTextStyleBody].pointSize;
	}
	
	return pointSize * fView.contentScaleFactor;
}

S32
IPhonePlatformBase::GetFontNames( lua_State *L, int index ) const
{
	S32 numFonts = 0;

	NSArray *fontFamilyNames = [UIFont familyNames];
	for ( NSString *familyName in fontFamilyNames )
	{
		NSArray *names = [UIFont fontNamesForFamilyName:familyName];
		for ( NSString *fontName in names )
		{
			lua_pushstring( L, [fontName UTF8String] );
			lua_rawseti( L, index, ++numFonts );
		}
	}

	return numFonts;
}

// ----------------------------------------------------------------------------

// NOTE: If you have iOS-specific (i.e. not available on tvOS) code,
// then add it to IPhonePlatformCore::PushSystemInfo() instead.
// 
// This is a place where we can add system.getInfo() categories that return arbitrary types
// (it also allows us to add categories on a per platform basis)
int
IPhonePlatformBase::PushSystemInfo( lua_State *L, const char *key ) const
{
	// Validate.
	if (!L)
	{
		return 0;
	}

	// Fetch the app bundle's information.
	NSBundle *bundle = [NSBundle mainBundle];
	NSDictionary *info = [bundle infoDictionary];

	// Push the requested system information to Lua.
	int pushedValues = 0;
	if (Rtt_StringCompare(key, "appName") == 0)
	{
		// Fetch the application's name.
		const char *applicationName = "";
		NSString *value = [info objectForKey:@"CFBundleDisplayName"];
		if (value)
		{
			applicationName = [value UTF8String];
		}
		lua_pushstring(L, applicationName);
		pushedValues = 1;
	}
	else if (Rtt_StringCompare(key, "appVersionString") == 0)
	{
		// Fetch the application's version string.
		const char *versionName = "";
		NSString *value = [info objectForKey:@"CFBundleVersion"];
		if (value)
		{
			versionName = [value UTF8String];
		}
		lua_pushstring(L, versionName);
		pushedValues = 1;
	}
	else if ( Rtt_StringCompare( key, "darkMode" ) == 0 )
	{
		BOOL res = NO;
		if (@available(iOS 13.0, tvOS 13.0, *)) {
			res = fView.viewController.traitCollection.userInterfaceStyle == UIUserInterfaceStyleDark;
		}
		lua_pushboolean(L, res);
		pushedValues = 1;
	}
	else
	{
		// Attempt to fetch the requested system info from the base class.
		pushedValues = Super::PushSystemInfo(L, key);
	}

	// Return the number of values pushed into Lua.
	return pushedValues;
}

NSString *
IPhonePlatformBase::PathForPluginsFile( const char *filename ) const
{
	NSBundle *bundle = [NSBundle mainBundle];
	NSString *pluginsDir = [[bundle resourcePath] stringByAppendingPathComponent:@"corona-plugins"];

	if (filename != NULL)
	{
		return [pluginsDir stringByAppendingPathComponent:[NSString stringWithUTF8String:filename]];
	}
	else
	{
		return pluginsDir;
	}
}

void IPhonePlatformBase::GetSafeAreaInsetsPixels(Rtt_Real &top, Rtt_Real &left, Rtt_Real &bottom, Rtt_Real &right) const
{
#if __IPHONE_11_0
	if (@available(iOS 11.0, tvOS 11.0, *))
	{
		UIEdgeInsets insets = [fView safeAreaInsets];
		const CGFloat scaleFactor = fView.contentScaleFactor;
		top = Rtt_FloatToReal(insets.top*scaleFactor);
		left = Rtt_FloatToReal(insets.left*scaleFactor);
		bottom = Rtt_FloatToReal(insets.bottom*scaleFactor);
		right = Rtt_FloatToReal(insets.right*scaleFactor);
	}
	else
#endif
	{
		top = left = bottom = right = 0;
	}
}

// ============================================================================

#if Rtt_IPHONE_PLATFORM_STUB

bool
IPhonePlatformBase::SaveBitmap( PlatformBitmap* bitmap, const char* filePath, float jpegQuality ) const
{
	Rtt_ASSERT_NOT_REACHED();
	return false;
}

PlatformStoreProvider*
IPhonePlatformBase::GetStoreProvider( const ResourceHandle<lua_State>& handle ) const
{
	Rtt_ASSERT_NOT_REACHED();
	return NULL;
}

void
IPhonePlatformBase::SetActivityIndicator( bool visible ) const
{
	Rtt_ASSERT_NOT_REACHED();
}

void
IPhonePlatformBase::SetKeyboardFocus( PlatformDisplayObject *object ) const
{
	if ( object )
	{
		// Verify that this is actually a text field or text box
		const LuaProxyVTable *vtable = & object->ProxyVTable();
		
		if ( & PlatformDisplayObject::GetTextFieldObjectProxyVTable() == vtable
			 || & PlatformDisplayObject::GetTextBoxObjectProxyVTable() == vtable )
		{
			((IPhoneDisplayObject*)object)->SetFocus();
		}
	}
	else
	{
		// Dismiss keyboard
		[GetView() dismissKeyboard];
	}
}

#endif // Rtt_IPHONE_PLATFORM_STUB

#if Rtt_IPHONE_PLATFORM_STUB

PlatformFBConnect*
IPhonePlatformBase::GetFBConnect() const
{
	return NULL;
}

void*
IPhonePlatformBase::CreateAndScheduleNotification( lua_State *L, int index ) const
{
	Rtt_ASSERT_NOT_REACHED();
	return NULL;
}

void
IPhonePlatformBase::ReleaseNotification( void *notificationId ) const
{
	Rtt_ASSERT_NOT_REACHED();
}

void
IPhonePlatformBase::CancelNotification( void *notificationId ) const
{
	Rtt_ASSERT_NOT_REACHED();
}

void
IPhonePlatformBase::RuntimeErrorNotification( const char *errorType, const char *message, const char *stacktrace ) const
{
	Rtt_ASSERT_NOT_REACHED();
}

#endif // Rtt_IPHONE_PLATFORM_STUB

// ============================================================================

Rtt_EXPORT CGSize Rtt_GetDeviceSize();
	
Rtt_EXPORT CGSize
Rtt_GetDeviceSize()
{
	UIScreen *mainScreen = [UIScreen mainScreen];
	CGSize result = mainScreen.bounds.size;
	CGFloat scale = mainScreen.scale;
	result.width *= scale;
	result.height *= scale;
	return result;
}

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------

