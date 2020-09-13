//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Core/Rtt_Build.h"

#include "Core/Rtt_String.h"

#include "Rtt_IPhonePlatform.h"
#include "Rtt_IPhoneTimer.h"

#include "Rtt_IPhoneAudioSessionManager.h"
#include "Rtt_IPhoneFont.h"
#include "Rtt_AppleInAppStore.h"
#include "Rtt_IPhoneScreenSurface.h"

#include "Rtt_LuaLibNative.h"
#include "Rtt_LuaLibSystem.h"
#include "Rtt_LuaResource.h"

#import "AppDelegate.h"
#import "CoronaViewPrivate.h"

#import <UIKit/UIApplication.h>
#import <UIKit/UIDevice.h>
#import <UIKit/UIGestureRecognizerSubclass.h>
#import <MessageUI/MFMailComposeViewController.h>
#import <MessageUI/MFMessageComposeViewController.h>

#include "CoronaLua.h"

#include "Rtt_TouchInhibitor.h"

// ----------------------------------------------------------------------------

@interface PopupControllerDelegate : NSObject< MFMailComposeViewControllerDelegate, MFMessageComposeViewControllerDelegate >
{
}

@end

// ----------------------------------------------------------------------------

@implementation PopupControllerDelegate

- (void)mailComposeController:(MFMailComposeViewController *)controller didFinishWithResult:(MFMailComposeResult)result error:(NSError *)error
{
	[controller dismissViewControllerAnimated:YES completion:nil];
}

- (void)messageComposeViewController:(MFMessageComposeViewController *)controller didFinishWithResult:(MessageComposeResult)result
{
	[controller dismissViewControllerAnimated:YES completion:nil];
}

@end

// ----------------------------------------------------------------------------

// Consume all touches, preventing their propagation
@interface CoronaNullGestureRecognizer : UIGestureRecognizer
@end

@implementation CoronaNullGestureRecognizer

- (instancetype)init
{
	self = [super initWithTarget:self action:@selector(handleGesture)];
	return self;
}

- (void)handleGesture
{
	// no-op
}

- (void)reset
{
	self.state = UIGestureRecognizerStatePossible;
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
	self.state = UIGestureRecognizerStateRecognized;
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
	self.state = UIGestureRecognizerStateRecognized;
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
	self.state = UIGestureRecognizerStateRecognized;
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
	self.state = UIGestureRecognizerStateRecognized;
}

@end

// ----------------------------------------------------------------------------

namespace Rtt
{

// ----------------------------------------------------------------------------

IPhonePlatform::IPhonePlatform( CoronaView *view )
:	Super( view ),
	fInAppStoreProvider( NULL ),
	fPopupControllerDelegate( [[PopupControllerDelegate alloc] init] )
{
}

IPhonePlatform::~IPhonePlatform()
{
	[fPopupControllerDelegate release];
	[fActivityView release];
	Rtt_DELETE( fInAppStoreProvider );
}

// =====================================================================
	
PlatformStoreProvider*
IPhonePlatform::GetStoreProvider( const ResourceHandle<lua_State>& handle ) const
{
	if (!fInAppStoreProvider)
	{
		fInAppStoreProvider = Rtt_NEW( fAllocator, AppleStoreProvider( handle ) );
	}
	return fInAppStoreProvider;
}

// =====================================================================

#if 0 

void
IPhonePlatform::SetIdleTimer( bool enabled ) const
{
	[UIApplication sharedApplication].idleTimerDisabled = ! enabled;
}
	
bool
IPhonePlatform::GetIdleTimer() const
{
	return (bool)( ! [UIApplication sharedApplication].idleTimerDisabled);
}

#endif // 0

// =====================================================================

//NativeAlertRef
//IPhonePlatform::ShowNativeAlert(
//	const char *title,
//	const char *msg,
//	const char **buttonLabels,
//	U32 numButtons,
//	LuaResource* resource ) const
//{
//	AppDelegate *appDelegate = (AppDelegate*)[UIApplication sharedApplication].delegate;
//	Rtt::TouchInhibitor inhibitor( (CoronaView*)appDelegate.view );
//
//	NSString *t = [[NSString alloc] initWithUTF8String:title];
//	NSString *m = [[NSString alloc] initWithUTF8String:msg];
//	CustomAlertView *alertView = [[CustomAlertView alloc]
//								initWithTitle:t
//								message:m
//								delegate:fDelegate
//								cancelButtonTitle:nil
//								otherButtonTitles:nil];
//	alertView.fResource = resource;
//	[fDelegate setObject:alertView forKey:alertView];
//
//	for ( U32 i = 0; i < numButtons; i++ )
//	{
//		const char *label = buttonLabels[i]; Rtt_ASSERT( label );
//		NSString *l = [[NSString alloc] initWithUTF8String:label];
//		(void)Rtt_VERIFY( (U32)[alertView addButtonWithTitle:l] == i );
//		[l release];
//	}
//
//	[alertView show];
//
//	[m release];
//	[t release];
//
//	return alertView;
//}
//
//void
//IPhonePlatform::CancelNativeAlert( NativeAlertRef alert, S32 index ) const
//{
//	CustomAlertView *alertView = [fDelegate objectForKey:alert];
//	[alertView dismissWithClickedButtonIndex:index animated:true];
//}

// Returns an array of NSStrings. The object at index t is either a string
// or a (table) array of strings.
static NSArray*
ArrayWithStrings( lua_State *L, int t )
{
	Rtt_ASSERT( t > 0 );

	NSArray *result = nil;

	if ( LUA_TTABLE == lua_type( L, t ) )
	{
		int iMax = (int)lua_objlen( L, t );
		if ( iMax > 0 )
		{
			NSMutableArray *array = [NSMutableArray arrayWithCapacity:iMax];

			for ( int i = 1; i <= iMax; i++ )
			{
				lua_rawgeti( L, t, i );
				if ( LUA_TSTRING == lua_type( L, -1 ) )
				{
					const char *str = lua_tostring( L, -1 );
					[array addObject:[NSString stringWithUTF8String:str]];
				}
				lua_pop( L, 1 );
			}

			result = array;
		}
	}
	else if ( LUA_TSTRING == lua_type( L, t ) )
	{
		const char *str = lua_tostring( L, t );
		result = [NSArray arrayWithObject:[NSString stringWithUTF8String:str]];
	}

	return result;
}

static void
AddAttachment( MFMailComposeViewController *controller, const char *path, LuaLibSystem::FileType fileType )
{
	if ( path )
	{
		NSString *p = [[NSString alloc] initWithUTF8String:path];
		NSString *filename = [p lastPathComponent];
		NSString *extension = [filename pathExtension];

		NSString *mimeType = ( LuaLibSystem::kImageFileType == fileType
			? [NSString stringWithFormat:@"image/%@", extension]
			: @"content/unknown");

		NSData *data = [NSData dataWithContentsOfFile:p];
		[controller addAttachmentData:data mimeType:mimeType fileName:filename];
	}
}

static bool
InitializeController( lua_State *L, int index, MFMailComposeViewController *controller )
{
	bool result = true;

	if ( index > 0 )
	{
		lua_getfield( L, index, "to" );
		NSArray *to = ArrayWithStrings( L, lua_gettop( L ) );
		if ( to )
		{
			[controller setToRecipients:to];
		}	
		lua_pop( L, 1 );

		lua_getfield( L, index, "cc" );
		NSArray *cc = ArrayWithStrings( L, lua_gettop( L ) );
		if ( cc )
		{
			[controller setCcRecipients:cc];
		}	
		lua_pop( L, 1 );

		lua_getfield( L, index, "bcc" );
		NSArray *bcc = ArrayWithStrings( L, lua_gettop( L ) );
		if ( bcc )
		{
			[controller setBccRecipients:bcc];
		}	
		lua_pop( L, 1 );

		lua_getfield( L, index, "body" );
		if ( lua_type( L, -1 ) == LUA_TSTRING )
		{
			const char *body = lua_tostring( L, -1 );

			lua_getfield( L, index, "isBodyHtml" );
			bool isBodyHtml = lua_toboolean( L, -1 );
			lua_pop( L, 1 );
			
			[controller setMessageBody:[NSString stringWithUTF8String:body] isHTML:isBodyHtml];
		}	
		lua_pop( L, 1 );

		lua_getfield( L, index, "subject" );
		if ( lua_type( L, -1 ) == LUA_TSTRING )
		{
			const char *body = lua_tostring( L, -1 );

			[controller setSubject:[NSString stringWithUTF8String:body]];
		}	
		lua_pop( L, 1 );

		// TODO: suppport multiple attachments (i.e. array of these 'path' tables)
		LuaLibSystem::FileType fileType;
		lua_getfield( L, index, "attachment" );
		int numAttachments = (int)lua_objlen( L, -1 );
		if ( numAttachments > 0 )
		{
			// table is an array of 'path' tables
			for ( int i = 1; i <= numAttachments; i++ )
			{
				lua_rawgeti( L, -1, i );
				int numResults = LuaLibSystem::PathForTable( L, -1, fileType );
				if ( numResults > 0 )
				{
					const char *path = lua_tostring( L, -1 );
					AddAttachment( controller, path, fileType );
					lua_pop( L, numResults );
				}
				lua_pop( L, 1 );
			}
		}
		else
		{
			// table itself is a 'path' table
			int numResults = LuaLibSystem::PathForTable( L, -1, fileType );
			if ( numResults > 0 )
			{
				const char *path = lua_tostring( L, -1 );
				AddAttachment( controller, path, fileType );
				lua_pop( L, numResults );
			}
		}
		lua_pop( L, 1 );
	}

	return result;
}

static bool
InitializeController( lua_State *L, int index, MFMessageComposeViewController *controller )
{
	bool result = true;

	if ( index > 0 )
	{
		lua_getfield( L, index, "to" );
		NSArray *to = ArrayWithStrings( L, lua_gettop( L ) );
		if ( to )
		{
			controller.recipients = to;
		}	
		lua_pop( L, 1 );

		lua_getfield( L, index, "body" );
		if ( lua_type( L, -1 ) == LUA_TSTRING )
		{
			const char *body = lua_tostring( L, -1 );

			controller.body = [NSString stringWithUTF8String:body];
		}
		lua_pop( L, 1 );
	}

	return result;
}

bool
IPhonePlatform::CanShowPopup( const char *name ) const
{
	bool result =
		( Rtt_StringCompareNoCase( name, "mail" ) == 0 && [MFMailComposeViewController canSendMail] )
		|| ( Rtt_StringCompareNoCase( name, "sms" ) == 0 && [MFMessageComposeViewController canSendText] )
		|| ( Rtt_StringCompareNoCase( name, "rateApp" ) == 0 )
		|| ( Rtt_StringCompareNoCase( name, "appStore" ) == 0 );

	return result;
}

bool
IPhonePlatform::ShowPopup( lua_State *L, const char *name, int optionsIndex ) const
{
	bool result = false;

	AppDelegate* delegate = (AppDelegate*)[[UIApplication sharedApplication] delegate];
	UIViewController* viewController = delegate.viewController;
	if ( viewController.presentedViewController )
	{
		Rtt_ERROR( ( "ERROR: There is already a native modal interface being displayed. The '%s' popup will not be shown.\n", name ? name : "" ) );
	}
	else if ( Rtt_StringCompareNoCase( name, "mail" ) == 0 )
	{
		if ( [MFMailComposeViewController canSendMail] )
		{
			MFMailComposeViewController *controller = [[MFMailComposeViewController alloc] init];
			controller.mailComposeDelegate = fPopupControllerDelegate;

			result = InitializeController( L, optionsIndex, controller );
			if ( result )
			{
				[viewController presentViewController:controller animated:YES completion:nil];
			}

			[controller release];
		}
		else
		{
			Rtt_ERROR( ( "ERROR: This device cannot send mail. The 'mail' popup will not be shown.\n" ) );
		}
	}
	else if ( Rtt_StringCompareNoCase( name, "sms" ) == 0 )
	{
		if ( [MFMessageComposeViewController canSendText] )
		{
			MFMessageComposeViewController *controller = [[MFMessageComposeViewController alloc] init];
			controller.messageComposeDelegate = fPopupControllerDelegate;

			result = InitializeController( L, optionsIndex, controller );
			if ( result ) { // casenum:12085 The controller is messing up hidden status bars. This must be fetched before we present the controller.
				[viewController presentViewController:controller animated:YES completion:nil];
				[[UIApplication sharedApplication] setStatusBarHidden:YES withAnimation:UIStatusBarAnimationNone];
			}

			[controller release];
		}
		else
		{
			Rtt_ERROR( ( "ERROR: This device cannot send SMS. The 'sms' popup will not be shown.\n" ) );
		}
	}
	else if ( !Rtt_StringCompareNoCase( name, "rateApp" ) || !Rtt_StringCompareNoCase( name, "appStore" ) )
	{
		const char *appStringId = NULL;
		if ( lua_istable( L, optionsIndex ) )
		{
			lua_getfield( L, optionsIndex, "iOSAppId" );
			if ( lua_type( L, -1 ) == LUA_TSTRING )
			{
				appStringId = lua_tostring( L, -1 );
			}
			lua_pop( L, 1 );
		}
		if ( appStringId )
		{
			char url[256];
			snprintf( url, sizeof(url), "itms-apps://itunes.apple.com/%s/app/id%s",
					[[[NSLocale currentLocale] objectForKey: NSLocaleCountryCode] UTF8String], appStringId );
			result = OpenURL( url );
		}
		else
		{
			Rtt_ERROR( ( "ERROR: native.showPopup('rateApp') requires the iOS app ID.\n" ) );
		}
	}

	return result;
}

bool
IPhonePlatform::HidePopup( const char *name ) const
{
	bool result = false;

	Rtt_ASSERT_NOT_IMPLEMENTED();

	return result;
}

void*
IPhonePlatform::CreateAndScheduleNotification( lua_State *L, int index ) const
{
	/*
	 local variable = require("plugin.notifications")
	 variable.scheduleNotification(...);
	 
	This function has been deprecated improvements should go into the plugin.
	 */
	Rtt_LUA_STACK_GUARD( L );
	
	void *notificationId = NULL;
	int absoluateIndex = Lua::Normalize( L, index );
	int numArgs = lua_gettop( L ) - (absoluateIndex - 1);

	// local variable = require("plugin.notifications")
	lua_getglobal( L, "require" );
	lua_pushstring( L, "plugin.notifications" );
	int result = lua_pcall( L, 1, 1, 0 );

	// requiring the plugin suceeded, first item on the stack should be
	if ( 0 == result )
	{
		lua_getfield( L, -1, "scheduleNotification" );
		
		// setting up arguments for pcall
		for ( int i = 0; i < numArgs; i++ )
		{
			lua_pushvalue( L, absoluateIndex + i );
		}
		
		result = lua_pcall( L, numArgs, 1, 0 );
		if ( 0 == result )
		{
			notificationId = Lua::CheckUserdata( L, -1, "notification" );
		}
		
		// pop the notificationId object off the stack
		// or the error code from the pcall
		lua_pop( L, 1 );
	}
	else
	{
		CoronaLuaError(L, "system.scheduleNotification: Notifications plugin not found, you need to include the plugin in your build.settings file.  This function has been deprecated.");
	}
	
	// pop the library object put on the stack by require("plugin.notificatoins")
	// or the error code from the pcall
	lua_pop( L, 1 );
	
	return notificationId;
}

void
IPhonePlatform::ReleaseNotification( void *notificationId ) const
{
	UILocalNotification *notification = (UILocalNotification*)notificationId;
	[notification release];
}

void
IPhonePlatform::CancelNotification( void *notificationId ) const
{
	/*
	 This function has been deprecated improvements should go into the plugin.
	 */
	// Duplicate from the notifications plugin
	UIApplication *application = [UIApplication sharedApplication];
	
	if ( notificationId )
	{
		UILocalNotification *notification = (UILocalNotification*)notificationId;
		
		[application cancelLocalNotification:notification];
	}
	else
	{
		[application cancelAllLocalNotifications];
		application.applicationIconBadgeNumber = 0;
	}
}

void
IPhonePlatform::RuntimeErrorNotification( const char *errorType, const char *message, const char *stacktrace ) const
{
	// In the delegate we catch the press of the button on the non-modal dialog and take the app down
	AppDelegate *delegate = (AppDelegate*)[[UIApplication sharedApplication] delegate];

	// we need to suspend the app so Lua doesn't keep churning out errors
	[delegate suspend];
	
	// This alert is designed to be similar to the Android version (we remove empty file and linenumber info if it's there)
	UIAlertView *alert = [[UIAlertView alloc] initWithTitle:[NSString stringWithUTF8String:errorType]
												message:[[NSString stringWithUTF8String:message] stringByReplacingOccurrencesOfString:@"?:0: " withString:@""]
												   delegate:delegate
										  cancelButtonTitle:@"OK"
										  otherButtonTitles:nil];

	[alert show];
	[alert release];
}


void
IPhonePlatform::SetNativeProperty( lua_State *L, const char *key, int valueIndex ) const
{
	AppDelegate *delegate = (AppDelegate*)[[UIApplication sharedApplication] delegate];
	AppViewController* viewController = (AppViewController*)delegate.viewController;

	NSString *k = [NSString stringWithUTF8String:key];

	if ( [k isEqualToString:@"prefersHomeIndicatorAutoHidden"] )
	{
		viewController.prefersHomeIndicatorAutoHidden = lua_toboolean( L, valueIndex );
		if (@available(iOS 11.0, *))
		{
			[viewController setNeedsUpdateOfHomeIndicatorAutoHidden];
		}
	}
	else if ([k isEqualToString:@"preferredScreenEdgesDeferringSystemGestures"])
	{
		viewController.preferredScreenEdgesDeferringSystemGestures = lua_toboolean( L, valueIndex )?UIRectEdgeAll:UIRectEdgeNone;
		if (@available(iOS 11.0, *))
		{
			[viewController setNeedsUpdateOfScreenEdgesDeferringSystemGestures];
		}
	}
	else
	{
		Super::SetNativeProperty(L, key, valueIndex);
	}
}

int
IPhonePlatform::PushNativeProperty( lua_State *L, const char *key ) const
{
	int result = 1;

	AppDelegate *delegate = (AppDelegate*)[[UIApplication sharedApplication] delegate];
	AppViewController* viewController = (AppViewController*)delegate.viewController;

	NSString *k = [NSString stringWithUTF8String:key];

	if ( [k isEqualToString:@"prefersHomeIndicatorAutoHidden"] )
	{
		lua_pushboolean(L, viewController.prefersHomeIndicatorAutoHidden);
	}
	else if ([k isEqualToString:@"preferredScreenEdgesDeferringSystemGestures"])
	{
		lua_pushboolean(L, viewController.preferredScreenEdgesDeferringSystemGestures != UIRectEdgeNone);
	}
	else
	{
		result = Super::PushNativeProperty(L, key);
	}

	return result;
}

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------

